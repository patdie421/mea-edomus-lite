/*
 *  interface_type_001.c
 *
 *  Created by Patrice Dietsch on 25/08/12.
 *  Copyright 2012 -. All rights reserved.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <termios.h>

#include "globals.h"
#include "consts.h"
#include "macros.h"
#include "tokens.h"
#include "tokens_da.h"

#include "mea_string_utils.h"
#include "mea_verbose.h"

#include "serial.h"
#include "comio2.h"
#include "arduino_pins.h"

#include "tokens.h"
#include "parameters_utils.h"

#include "xPLServer.h"
#include "processManager.h"

#include "interfacesServer.h"
#include "interface_type_001.h"
#include "interface_type_001_sensors.h"
#include "interface_type_001_actuators.h"
#include "interface_type_001_counters.h"


#define TEMPO    5 // 5 secondes 2 read


// structure contenant les parametres du thread de gestion des capteurs
struct interface_type_001_thread_params_s
{
   interface_type_001_t *it001;
};


void set_interface_type_001_isnt_running(void *data)
{
   interface_type_001_t *i001 = (interface_type_001_t *)data;
   i001->thread_is_running=0;
}


//
// xPLSend -c control.basic -m cmnd device=RELAY1 type=output current=pulse data1=125
// xPLSend -c sensor.request -m cmnd request=current device=CONSO type=POWER => dernière puissance instantannée
// xPLSend -c sensor.request -m cmnd request=current device=CONSO type=ENERGY => valeur du compteur ERDF
//
int16_t interface_type_001_xPL_callback2(cJSON *xplMsgJson, struct device_info_s *device_info, void *userValue)
{
   char *schema = NULL, *device = NULL, *type = NULL;
   cJSON *j=NULL;

   interface_type_001_t *i001=(interface_type_001_t *)userValue;
   (i001->indicators.nbxplin)++;

   j = cJSON_GetObjectItem(xplMsgJson, XPLSCHEMA_STR_C); 
   if(j)
      schema = j->valuestring;
   else {
      VERBOSE(5) mea_log_printf("%s (%s) : xPL message no schema\n", INFO_STR, __func__);
      return 0;
   }

   j = cJSON_GetObjectItem(xplMsgJson, get_token_string_by_id(XPL_DEVICE_ID)); 
   if(j)
      device=j->valuestring;

   j = cJSON_GetObjectItem(xplMsgJson, get_token_string_by_id(XPL_TYPE_ID)); 
   if(j)
      type=j->valuestring;

   VERBOSE(9) mea_log_printf("%s (%s) : xPL Message to process : %s\n", INFO_STR, __func__, schema);

   if(mea_strcmplower(schema, XPL_CONTROLBASIC_STR_C) == 0) {
      if(!device) {
         VERBOSE(5) mea_log_printf("%s (%s) : xPL message no device\n", INFO_STR, __func__);
         return 0;
      }
      if(!type) {
         VERBOSE(5) mea_log_printf("%s (%s) : xPL message no type\n", INFO_STR, __func__);
         return 0;
      }
      return xpl_actuator2(i001, xplMsgJson, device, type);
   }
   else if(mea_strcmplower(schema, XPL_SENSORREQUEST_STR_C) == 0) {
      char *request = NULL;
      j = cJSON_GetObjectItem(xplMsgJson, get_token_string_by_id(XPL_REQUEST_ID)); 
      if(j)
         request = j->valuestring;
      if(!request) {
         VERBOSE(5) mea_log_printf("%s (%s) : xPL message no request\n", INFO_STR, __func__);
         return 0;
      }
      if(mea_strcmplower(request, get_token_string_by_id(XPL_CURRENT_ID))!=0) {
         VERBOSE(5) mea_log_printf("%s (%s) : xPL message request!=current\n", INFO_STR, __func__);
         return 0;
      }
      
//      interface_type_001_counters_process_xpl_msg2(i001, xplMsgJson, device, type);
//      interface_type_001_sensors_process_xpl_msg2(i001, xplMsgJson, device, type);
   }
   
   return 0;
}


int load_interface_type_001(interface_type_001_t *i001, cJSON *jsonInterface)
{
   int16_t nb_sensors_actuators=0;
   int ret;
  
   int interface_id=(int)cJSON_GetObjectItem(jsonInterface, "id_interface")->valuedouble;
 
   // préparation des éléments de contexte de l'interface
// i001->interface_id=interface_id;
   i001->xPL_callback2=NULL;
   i001->counters_list=NULL;
   i001->sensors_list=NULL;
   i001->actuators_list=NULL;
   i001->loaded=0;

   // initialisation de la liste des capteurs de type compteur
   i001->counters_list=(mea_queue_t *)malloc(sizeof(mea_queue_t));
   if(!i001->counters_list) {
      VERBOSE(2) {
         mea_log_printf("%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
         perror("");
      }
      goto load_interface_type_001_clean_exit;
   }
   mea_queue_init(i001->counters_list);
   
   
   // initialisation de la liste des actionneurs (sorties logiques de l'arduino)
   i001->actuators_list=(mea_queue_t *)malloc(sizeof(mea_queue_t));
   if(!i001->actuators_list) {
      VERBOSE(2) {
         mea_log_printf("%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
         perror(""); }
      goto load_interface_type_001_clean_exit;
   }
   mea_queue_init(i001->actuators_list);


   // initialisation de la liste des autres capteurs (entrees logiques et analogiques)
   i001->sensors_list=(mea_queue_t *)malloc(sizeof(mea_queue_t));
   if(!i001->sensors_list) {
      VERBOSE(2) {
         mea_log_printf("%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
         perror(""); }
      goto load_interface_type_001_clean_exit;
   }
   mea_queue_init(i001->sensors_list);

   cJSON *jsonDevices = cJSON_GetObjectItem(jsonInterface,"devices");
   if(jsonDevices) {
      cJSON *jsonDevice=jsonDevices->child;
      while(jsonDevice) {
         int id_sensor_actuator=(int)cJSON_GetObjectItem(jsonDevice,"id_sensor_actuator")->valuedouble;
         int id_type=(int)cJSON_GetObjectItem(jsonDevice,"id_type")->valuedouble;
         char *name=jsonDevice->string;
         char *parameters=cJSON_GetObjectItem(jsonDevice,"parameters")->valuestring;
         int todbflag = 0;

         switch (id_type) {
            case 1000: // capteur de type compteur
            {
               struct electricity_counter_s *counter;
               counter=interface_type_001_sensors_valid_and_malloc_counter(id_sensor_actuator, (char *)name, (char *)parameters);
               if(counter) {
                  counter->power=0.0;
                  counter->counter=0;
                  counter->last_power=0.0;
                  counter->last_counter=0;
                  counter->todbflag = todbflag;
                  mea_queue_in_elem(i001->counters_list, counter);
                  nb_sensors_actuators++;
               }
               break;
            }
               
            case 1002: // relais
            case 501: // type OUTPUT standard
            {
               struct actuator_s *actuator;
               actuator=interface_type_001_valid_and_malloc_actuator(id_sensor_actuator, (char *)name, (char *)parameters);
               if(actuator) {
                  actuator->todbflag = todbflag;
                  mea_queue_in_elem(i001->actuators_list, actuator);
                  nb_sensors_actuators++;
               }
               break;
            }
            
            case 1001: // entrées
            case 500: // type INPUT standard
            {
               struct sensor_s *sensor;
               sensor=interface_type_001_sensors_valid_and_malloc_sensor(id_sensor_actuator, (char *)name, (char *)parameters);
               if(sensor) {
                  sensor->todbflag = todbflag;
                  mea_queue_in_elem(i001->sensors_list, sensor);
                  nb_sensors_actuators++;
               }
               break;
            }

            default:
               break;
         }

         jsonDevice=jsonDevice->next;
      }
      i001->loaded=1;
      return nb_sensors_actuators;
   }

load_interface_type_001_clean_exit:
   clean_interface_type_001(i001);

   return -1;
}


int set_xPLCallback_interface_type_001(void *ixxx, xpl2_f cb)
{
   interface_type_001_t *i001 = (interface_type_001_t *)ixxx;

   if(i001 == NULL)
      return -1;
   else {
      i001->xPL_callback2 = cb;
      return 0;
   }
}


xpl2_f get_xPLCallback_interface_type_001(void *ixxx)
{
   interface_type_001_t *i001 = (interface_type_001_t *)ixxx;

   if(i001 == NULL)
      return NULL;
   else
      return i001->xPL_callback2;
}


int set_monitoring_id_interface_type_001(void *ixxx, int id)
{
   interface_type_001_t *i001 = (interface_type_001_t *)ixxx;

   if(i001 == NULL)
      return -1;
   else {
      i001->monitoring_id = id;
      return 0;
   }
}


int get_monitoring_id_interface_type_001(void *ixxx)
{
   interface_type_001_t *i001 = (interface_type_001_t *)ixxx;

   if(i001 == NULL)
      return -1;
   else
      return i001->monitoring_id;
}


int get_type_interface_type_001()
{
   return INTERFACE_TYPE_001;
}


interface_type_001_t *malloc_and_init2_interface_type_001(int id_driver, cJSON *jsonInterface)
{
   interface_type_001_t *i001;
                 
   // allocation du contexte de l'inteface
   i001=(interface_type_001_t *)malloc(sizeof(interface_type_001_t));
   if(!i001) {
      VERBOSE(2) {
         mea_log_printf("%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
         perror("");
      }
      return NULL;
   }

   // initialisation contexte de l'interface
   i001->thread_is_running=0;
   struct interface_type_001_start_stop_params_s *i001_start_stop_params=(struct interface_type_001_start_stop_params_s *)malloc(sizeof(struct interface_type_001_start_stop_params_s));
   if(!i001_start_stop_params) {
      free(i001);
      i001=NULL;
      VERBOSE(2) {
         mea_log_printf("%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
         perror("");
      }
      return NULL;
   }

   int id_interface=(int)cJSON_GetObjectItem(jsonInterface,"id_interface")->valuedouble;
   char *name=jsonInterface->string;
   char *dev=cJSON_GetObjectItem(jsonInterface,"dev")->valuestring;
   char *parameters=cJSON_GetObjectItem(jsonInterface,"parameters")->valuestring;
   char *description=cJSON_GetObjectItem(jsonInterface,"description")->valuestring;
 
   // initialisation contexte de l'interface
   i001->thread_is_running=0;

   strncpy(i001->name, (char *)name, sizeof(i001->name)-1);
   i001->monitoring_id=0;
   i001->loaded=0;
   i001->indicators.nbactuatorsout = 0;
   i001->indicators.nbactuatorsxplrecv = 0;
   i001->indicators.nbactuatorsouterr = 0;
   i001->indicators.nbsensorstraps = 0;
   i001->indicators.nbsensorsread = 0;
   i001->indicators.nbsensorsreaderr = 0;
   i001->indicators.nbsensorsxplsent = 0;
   i001->indicators.nbsensorsxplrecv = 0;
   i001->indicators.nbcounterstraps = 0;
   i001->indicators.nbcountersread = 0;
   i001->indicators.nbcountersreaderr = 0;
   i001->indicators.nbcountersxplsent = 0;
   i001->indicators.nbcountersxplrecv = 0;
   i001->indicators.nbxplin = 0;
   i001->interface_id=id_interface;
   strncpy(i001->name, (char *)name, sizeof(i001->name)-1);
   
   i001->thread_id=NULL;
   i001->ad=NULL;
   i001->counters_list=NULL;
   i001->actuators_list=NULL;
   i001->sensors_list=NULL;
   i001->xPL_callback2=NULL;

   // initialisation du process
   i001_start_stop_params->i001=i001;
   strncpy(i001_start_stop_params->dev, (char *)dev, sizeof(i001_start_stop_params->dev)-1);
   i001->monitoring_id=process_register((char *)name);

   process_set_group(i001->monitoring_id, 1);
   process_set_start_stop(i001->monitoring_id, start_interface_type_001, stop_interface_type_001, (void *)i001_start_stop_params, 1);
   process_set_watchdog_recovery(i001->monitoring_id, start_interface_type_001, (void *)i001_start_stop_params);
   process_set_description(i001->monitoring_id, (char *)description);
   process_set_heartbeat_interval(i001->monitoring_id, 60); // chien de garde au bout de 60 secondes sans heartbeat

   process_add_indicator(i001->monitoring_id, I001_XPLINNB, 0);
                 
   process_add_indicator(i001->monitoring_id, I001_SNBTRAPS, 0);
   process_add_indicator(i001->monitoring_id, I001_SNBREADS, 0);
   process_add_indicator(i001->monitoring_id, I001_SNBREADSERR, 0);
   process_add_indicator(i001->monitoring_id, I001_SNBXPLIN, 0);
   process_add_indicator(i001->monitoring_id, I001_SNBXPLOUT, 0);
                 
   process_add_indicator(i001->monitoring_id, I001_ANBOUTERR, 0);
   process_add_indicator(i001->monitoring_id, I001_ANBOUT, 0);
   process_add_indicator(i001->monitoring_id, I001_ANBXPLIN, 0);
                 
   process_add_indicator(i001->monitoring_id, I001_CNBTRAPS, 0);
   process_add_indicator(i001->monitoring_id, I001_CNBREADS, 0);
   process_add_indicator(i001->monitoring_id, I001_CNBREADSERR, 0);
   process_add_indicator(i001->monitoring_id, I001_CNBXPLOUT, 0);
   process_add_indicator(i001->monitoring_id, I001_CNBXPLIN, 0);

   return i001;
}


int clean_interface_type_001(interface_type_001_t *i001)
{
   if(i001==NULL)
      return 0;

   if(i001->counters_list) {
      struct counter_s *counter;
      mea_queue_first(i001->counters_list);
      while(i001->counters_list->nb_elem) {
         mea_queue_out_elem(i001->counters_list, (void **)&counter);
         free(counter);
         counter=NULL;
      }
      free(i001->counters_list);
      i001->counters_list=NULL;
   }

   if(i001->sensors_list) {
      struct sensor_s *sensor;
      mea_queue_first(i001->sensors_list);
      while(i001->sensors_list->nb_elem) {
         mea_queue_out_elem(i001->sensors_list, (void **)&sensor);
         free(sensor);
         sensor=NULL;
      } 
      free(i001->sensors_list);
      i001->sensors_list=NULL;
   }

   if(i001->actuators_list) {
      struct actuator_s *actuator;
      mea_queue_first(i001->actuators_list);
      while(i001->actuators_list->nb_elem) {
         mea_queue_out_elem(i001->actuators_list, (void **)&actuator);
         free(actuator);
         actuator=NULL;
      }
      free(i001->actuators_list);
      i001->actuators_list=NULL;
   }
   i001->loaded=0;
   
   return 0;
}


void *_thread_interface_type_001(void *args)
{
   struct interface_type_001_thread_params_s *interface_type_001_thread_params=(struct interface_type_001_thread_params_s *)args;
   
   interface_type_001_t *i001=interface_type_001_thread_params->it001;
   
   pthread_cleanup_push( (void *)set_interface_type_001_isnt_running, (void *)i001 );
   i001->thread_is_running=1;
   process_heartbeat(i001->monitoring_id);
   
   free(interface_type_001_thread_params);
   interface_type_001_thread_params=NULL;
   
   interface_type_001_counters_init(i001);
   interface_type_001_sensors_init(i001);

   uint32_t cntr=0;
   while(1) {
      // maj indicateurs process
      process_heartbeat(i001->monitoring_id);

      process_update_indicator(i001->monitoring_id, I001_XPLINNB,    i001->indicators.nbxplin);
      
      process_update_indicator(i001->monitoring_id, I001_SNBTRAPS,   i001->indicators.nbsensorstraps);
      process_update_indicator(i001->monitoring_id, I001_SNBREADS,   i001->indicators.nbsensorsread);
      process_update_indicator(i001->monitoring_id, I001_SNBREADSERR,i001->indicators.nbsensorsreaderr);
      process_update_indicator(i001->monitoring_id, I001_SNBXPLOUT,  i001->indicators.nbsensorsxplsent);
      process_update_indicator(i001->monitoring_id, I001_SNBXPLIN,   i001->indicators.nbsensorsxplrecv);

      process_update_indicator(i001->monitoring_id, I001_ANBXPLIN,   i001->indicators.nbactuatorsxplrecv);
      process_update_indicator(i001->monitoring_id, I001_ANBOUTERR,  i001->indicators.nbactuatorsouterr);
      process_update_indicator(i001->monitoring_id, I001_ANBOUT,     i001->indicators.nbactuatorsout);

      process_update_indicator(i001->monitoring_id, I001_CNBTRAPS,   i001->indicators.nbcounterstraps);
      process_update_indicator(i001->monitoring_id, I001_CNBREADS,   i001->indicators.nbcountersread);
      process_update_indicator(i001->monitoring_id, I001_CNBREADSERR,i001->indicators.nbcountersreaderr);
      process_update_indicator(i001->monitoring_id, I001_CNBXPLOUT,  i001->indicators.nbcountersxplsent);
      process_update_indicator(i001->monitoring_id, I001_CNBXPLIN,   i001->indicators.nbcountersxplrecv);

      if(interface_type_001_counters_poll_inputs2(i001)<0) {
         VERBOSE(2) mea_log_printf("%s (%s) : %s - thread goes down.\n", ERROR_STR, __func__, i001->name);
         pthread_exit(NULL);
      }
      
      if(interface_type_001_sensors_poll_inputs2(i001)<0) {
         VERBOSE(2) mea_log_printf("%s (%s) : %s - thread goes down.\n", ERROR_STR, __func__, i001->name);
         pthread_exit(NULL);
      }
      
      cntr++;
      pthread_testcancel();
      sleep(5);
   }
   
   pthread_cleanup_pop(1);
}


int update_devices_type_001(void *data)
{
   printf("Update device 001\n");

   return 0;
}


int restart_interface_type_001(int my_id, void *data, char *errmsg, int l_errmsg)
{
   process_stop(my_id, data, 0);
   sleep(5);
   return process_start(my_id, data, 0);
}


int stop_interface_type_001(int my_id, void *data, char *errmsg, int l_errmsg)
{
   if(!data)
      return -1;

   struct interface_type_001_start_stop_params_s *start_stop_params=(struct interface_type_001_start_stop_params_s *)data;

   if(start_stop_params->i001->thread_id) {
      pthread_cancel(*start_stop_params->i001->thread_id);
      int counter=100;
      while(counter--) {
         if(start_stop_params->i001->thread_is_running) {
            // pour éviter une attente "trop" active
            usleep(100); // will sleep for 10 ms
         }
         else {
            break;
         }
      }
      DEBUG_SECTION mea_log_printf("%s (%s) : %s, fin après %d itération(s)\n",DEBUG_STR, __func__,start_stop_params->i001->name,100-counter);

      free(start_stop_params->i001->thread_id);
      start_stop_params->i001->thread_id=NULL;
   }
   
   if(start_stop_params->i001->ad) {
      comio2_close(start_stop_params->i001->ad);
      comio2_free_ad(start_stop_params->i001->ad);
      start_stop_params->i001->ad=NULL;
   }
   VERBOSE(2) mea_log_printf("%s (%s) : %s %s.\n", INFO_STR, __func__, start_stop_params->i001->name, stopped_successfully_str);

   return 0;
}


int start_interface_type_001(int my_id, void *data, char *errmsg, int l_errmsg)
{
   int16_t ret;
   
   char dev[81];
   char buff[81];
   speed_t speed;
   int fd = 0;
   comio2_ad_t *ad=NULL;
   char err_str[81];

   pthread_t *interface_type_001_thread_id=NULL; // descripteur du thread
   
   struct interface_type_001_thread_params_s *interface_type_001_thread_params=NULL; // parametre à transmettre au thread
   struct interface_type_001_start_stop_params_s *start_stop_params=(struct interface_type_001_start_stop_params_s *)data;

   if(start_stop_params->i001->loaded!=1) {
      cJSON *jsonInterface = getInterfaceById_alloc(start_stop_params->i001->interface_id);
      if(jsonInterface) {
         ret=load_interface_type_001(start_stop_params->i001, jsonInterface);
         if(ret<0) {
            VERBOSE(2) mea_log_printf("%s (%s) : can not load sensors/actuators.\n", ERROR_STR,__func__);
            return -1;
         }
      }
      else {
         VERBOSE(2) mea_log_printf("%s (%s) : can not find interface.\n", ERROR_STR,__func__);
         return -1;
      }
   }

   // si on a trouvé une config
   if(start_stop_params->i001->loaded==1) {
      ret=get_dev_and_speed((char *)start_stop_params->dev, buff, sizeof(buff), &speed);
      if(!ret) {
         int n=snprintf(dev,sizeof(buff)-1,"/dev/%s",buff);
         if( n<0 || n==(sizeof(buff)-1) ) {
            strerror_r(errno, err_str, sizeof(err_str));
            VERBOSE(2) {
               mea_log_printf("%s (%s) : snprintf - %s\n", ERROR_STR, __func__, err_str);
               perror("");
            }
            goto start_interface_type_001_clean_exit;
         }
      }
      else {
         VERBOSE(2) mea_log_printf("%s (%s) : unknow interface device - %s\n", ERROR_STR,__func__, start_stop_params->dev);
         goto start_interface_type_001_clean_exit;
      }

      ad=(comio2_ad_t *)malloc(sizeof(comio2_ad_t));
      if(!ad) {
         strerror_r(errno, err_str, sizeof(err_str));
         VERBOSE(2) {
            mea_log_printf("%s (%s) : %s - %s\n", ERROR_STR, __func__, MALLOC_ERROR_STR, err_str);
            perror("");
         }
         goto start_interface_type_001_clean_exit;
      }
      
      fd = comio2_init(ad, dev, speed);
      if (fd == -1) {
         VERBOSE(2) {
            mea_log_printf("%s (%s) : init_arduino - Unable to open serial port (%s) - ",ERROR_STR,__func__,start_stop_params->dev);
            perror("");
         }
         fd=0;
         goto start_interface_type_001_clean_exit;
      }
   }
   else {
      VERBOSE(5) mea_log_printf("%s (%s) : no sensor/actuator active for this interface (%d) - ",ERROR_STR,__func__,start_stop_params->i001->interface_id);

      goto start_interface_type_001_clean_exit;
   }
   
   start_stop_params->i001->ad=ad;
   
   interface_type_001_thread_params=malloc(sizeof(struct interface_type_001_thread_params_s));
   if(!interface_type_001_thread_params)
      goto start_interface_type_001_clean_exit;
   
   interface_type_001_thread_params->it001=start_stop_params->i001;
   
   interface_type_001_thread_id=(pthread_t *)malloc(sizeof(pthread_t));
   if(!interface_type_001_thread_id)
      goto start_interface_type_001_clean_exit;
   
   start_stop_params->i001->xPL_callback2=interface_type_001_xPL_callback2;

   if(pthread_create (interface_type_001_thread_id, NULL, _thread_interface_type_001, (void *)interface_type_001_thread_params)) {
      VERBOSE(2) mea_log_printf("%s (%s) : pthread_create - can't start thread\n",ERROR_STR,__func__);
      goto start_interface_type_001_clean_exit;
   }
   fprintf(stderr,"INTERFACE_TYPE_001 : %x\n", (unsigned int)interface_type_001_thread_id);

   start_stop_params->i001->thread_id=interface_type_001_thread_id;
   
   pthread_detach(*interface_type_001_thread_id);
   
   VERBOSE(2) mea_log_printf("%s (%s) : %s %s.\n", INFO_STR, __func__, start_stop_params->i001->name, launched_successfully_str);
   
   return 0;
   
start_interface_type_001_clean_exit:
   if(interface_type_001_thread_params) {
      free(interface_type_001_thread_params);
      interface_type_001_thread_params=NULL;
   }
   if(interface_type_001_thread_id) {
      free(interface_type_001_thread_id);
      interface_type_001_thread_id=NULL;
   }
   if(fd)
      comio2_close(ad);
   if(ad) {
      free(ad);
      ad=NULL;
   }
   
   return -1;
}


int get_fns_interface_type_001(struct interfacesServer_interfaceFns_s *interfacesFns)
{
   interfacesFns->malloc_and_init2 = (malloc_and_init2_f)&malloc_and_init2_interface_type_001;
   interfacesFns->get_monitoring_id = (get_monitoring_id_f)&get_monitoring_id_interface_type_001;
   interfacesFns->get_xPLCallback = (get_xPLCallback_f)&get_xPLCallback_interface_type_001;
   interfacesFns->clean = (clean_f)&clean_interface_type_001;
   interfacesFns->set_monitoring_id = (set_monitoring_id_f)&set_monitoring_id_interface_type_001;
   interfacesFns->set_xPLCallback = (set_xPLCallback_f)&set_xPLCallback_interface_type_001;
   interfacesFns->update_devices = (update_devices_f)&update_devices_type_001;
   interfacesFns->get_type = (get_type_f)&get_type_interface_type_001;

   interfacesFns->lib = NULL;
   interfacesFns->type = interfacesFns->get_type();
   interfacesFns->plugin_flag = 0;

   return 0;
}
