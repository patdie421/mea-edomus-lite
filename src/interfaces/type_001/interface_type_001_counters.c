//
//  interface_type_001_counters.c
//  mea-eDomus
//
//  Created by Patrice DIETSCH on 29/11/12.
//
//
#include <Python.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

//#include "debug.h"
#include "mea_verbose.h"
#include "globals.h"
#include "comio2.h"
#include "mea_queue.h"
#include "arduino_pins.h"
#include "mea_string_utils.h"
#include "parameters_utils.h"
#include "tokens.h"
#include "tokens_da.h"
#include "mea_timer.h"
//DBSERVER #include "dbServer.h"
#include "xPLServer.h"

#include "interface_type_001.h"
#include "interface_type_001_counters.h"

#include "processManager.h"

uint16_t counters_mem[2][4]={
   {0,1,2,3},
   {10,11,12,13}
};

uint16_t counters_trap[2]={1,2};

/*
char *valid_counter_params[]={"I:M1","I:M2","I:M3","I:M4","I:TRAP",NULL};
#define COUNTER_PARAMS_M1       0
#define COUNTER_PARAMS_M2       1
#define COUNTER_PARAMS_M3       2
#define COUNTER_PARAMS_M4       3
#define COUNTER_PARAMS_TRAP     4
*/

char *valid_counter_params[]={"I:COUNTER", "I:POLLING_PERIODE", NULL};
#define COUNTER_PARAMS_COUNTER   0
#define COUNTER_PARAMS_POLLING_PERIODE 1


void interface_type_001_free_counters_queue_elem(void *d)
{
   struct electricity_counter_s *e=(struct electricity_counter_s *)d;
   
   free(e);
   e=NULL;
}


int16_t interface_type_001_counters_process_traps2(int16_t numTrap, char *buff, int16_t l_buff, void * args)
 {
   double t_old;
   struct timeval tv;
   struct electricity_counter_s *counter;
   counter=(struct electricity_counter_s *)args;

   *(counter->nbtrap)=*(counter->nbtrap)+1;
   
   // prise du chrono
   gettimeofday(&tv, NULL); 
   pthread_cleanup_push((void *)pthread_mutex_unlock, (void *)&(counter->lock));
   pthread_mutex_lock(&(counter->lock));
   {
      if(counter->t<0)
         counter->t=(double)tv.tv_sec+(double)tv.tv_usec/1000000.0;
      else
      {
         t_old=counter->t;
         counter->t=(double)tv.tv_sec+(double)tv.tv_usec/1000000.0;
         // calcul de la conso instantannée (enfin une estimation)
         counter->last_power=counter->power;
         counter->power=3600/(counter->t-t_old);
         mea_start_timer(&(counter->trap_timer)); // réinitialisation du timer à chaque trap

         char value[20];
         char str[256];

         cJSON *xplMsgJson = cJSON_CreateObject();
         cJSON_AddItemToObject(xplMsgJson, XPLMSGTYPE_STR_C, cJSON_CreateString(XPL_TRIG_STR_C));
         sprintf(str,"%s.%s", get_token_string_by_id(XPL_SENSOR_ID), get_token_string_by_id(XPL_BASIC_ID));
         cJSON_AddItemToObject(xplMsgJson, XPLSCHEMA_STR_C, cJSON_CreateString(str));
         cJSON_AddItemToObject(xplMsgJson, get_token_string_by_id(XPL_DEVICE_ID), cJSON_CreateString(counter->name));
         cJSON_AddItemToObject(xplMsgJson, get_token_string_by_id(XPL_TYPE_ID), cJSON_CreateString(get_token_string_by_id(XPL_POWER_ID)));
         sprintf(value,"%f",counter->power);
         cJSON_AddItemToObject(xplMsgJson, get_token_string_by_id(XPL_CURRENT_ID), cJSON_CreateString(value));

         // Broadcast the message
         mea_sendXPLMessage2(xplMsgJson);
 
         *(counter->nbxplout)=*(counter->nbxplout)+1;

         cJSON_Delete(xplMsgJson);

         VERBOSE(9) {
            char now[30];
            strftime(now,30,"%d/%m/%y;%H:%M:%S",localtime(&tv.tv_sec));
            mea_log_printf("%s (%s) : %s;%s;%f;%f;%f\n", INFO_STR, __func__, counter->name, now, counter->t, counter->t-t_old, counter->power);
         }
      }
   }
   // fin section critique
   pthread_mutex_unlock(&(counter->lock));
   pthread_cleanup_pop(0);
   return NOERROR;
}  


struct electricity_counter_s *interface_type_001_sensors_valid_and_malloc_counter(int id_sensor_actuator, char *name, char *parameters)
{
   parsed_parameters_t *counter_params=NULL;
   
   struct electricity_counter_s *counter=(struct electricity_counter_s *)malloc(sizeof(struct electricity_counter_s));
   if(!counter)
   {
      VERBOSE(1) {
         mea_log_printf("%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
         perror("");
      }
      goto valid_and_malloc_counter_clean_exit;
   }
   
   // récupération des paramètres et contrôle de validité
   int nb_counter_params;
   int err;
   
   int16_t num_counter=-1;
   counter_params=alloc_parsed_parameters(parameters, valid_counter_params, &nb_counter_params, &err,1);
   if(counter_params)
   {
      // VERBOSE(9) display_parsed_parameters(counter_params, nb_counter_params);
      num_counter=counter_params->parameters[COUNTER_PARAMS_COUNTER].value.i;
      if(num_counter<0 || num_counter>1)
         goto valid_and_malloc_counter_clean_exit;
      
      for(int16_t i=0;i<4;i++)
         counter->sensor_mem_addr[i]=counters_mem[num_counter][i];
      counter->trap=counters_trap[num_counter];
      
      if(counter_params->parameters[COUNTER_PARAMS_POLLING_PERIODE].value.i>0)
      {
         mea_init_timer(&(counter->timer),counter_params->parameters[COUNTER_PARAMS_POLLING_PERIODE].value.i,1);
      }
      else
      {
         mea_init_timer(&(counter->timer),300,1); // lecture toutes les 5 minutes par défaut
      }
      
      mea_init_timer(&(counter->trap_timer), 600, 1); // 10 mn sans trap
    
      release_parsed_parameters(&counter_params); 
   }
   else
   {
      // traité ici err;
      goto valid_and_malloc_counter_clean_exit;
   }
   
   strcpy(counter->name,(char *)name);
   mea_strtolower(counter->name);
   counter->sensor_id=id_sensor_actuator;
   counter->wh_counter=0;
   counter->kwh_counter=0;
   counter->counter=num_counter;
   counter->t=-1.0;
   mea_start_timer(&(counter->timer));
   counter->lock=(pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
   counter->nbtrap=NULL;
   
   return counter;
   
valid_and_malloc_counter_clean_exit:
   if(counter)
   {
      free(counter);
      counter=NULL;
   }
   if(counter_params)
   {
      mea_log_printf("%s (%s) : %s/%s invalid. Check parameters.\n",ERROR_STR,__func__,name,parameters);

      release_parsed_parameters(&counter_params);
   }
   
   return NULL;
}


void counter_to_xpl2(interface_type_001_t *i001, struct electricity_counter_s *counter)
{
//   char value[20];
   char str[256];

   cJSON *xplMsgJson = cJSON_CreateObject();
   cJSON_AddItemToObject(xplMsgJson, XPLMSGTYPE_STR_C, cJSON_CreateString(XPL_TRIG_STR_C));
   sprintf(str,"%s.%s", get_token_string_by_id(XPL_SENSOR_ID), get_token_string_by_id(XPL_BASIC_ID));
   cJSON_AddItemToObject(xplMsgJson, XPLSCHEMA_STR_C, cJSON_CreateString(str));
   cJSON_AddItemToObject(xplMsgJson, get_token_string_by_id(XPL_TYPE_ID), cJSON_CreateString(get_token_string_by_id(XPL_ENERGY_ID)));
   cJSON_AddItemToObject(xplMsgJson, get_token_string_by_id(XPL_DEVICE_ID), cJSON_CreateString(counter->name));
   sprintf(str,"%d",counter->kwh_counter);
   cJSON_AddItemToObject(xplMsgJson, get_token_string_by_id(XPL_CURRENT_ID), cJSON_CreateString(str));

   // Broadcast the message
   mea_sendXPLMessage2(xplMsgJson);

   (i001->indicators.nbcountersxplsent)++;

   cJSON_Delete(xplMsgJson);
}

/* DBSERVER
int16_t counter_to_db(struct electricity_counter_s *counter)
{
   return dbServer_add_data_to_sensors_values(counter->sensor_id, (double)counter->wh_counter, UNIT_WH, (double)counter->kwh_counter, "WH");
}
*/

int16_t counter_read(interface_type_001_t *i001, struct electricity_counter_s *counter)
{
   uint32_t c;

   char buffer[4];

   char resp[5];
   uint16_t l_resp;
   int16_t comio2_err;
   int ret=0;

   int retry = 0;
   for(int i=0;i<4;i++)
      buffer[i]=(char)counter->sensor_mem_addr[i];
   do
   {
      ret=comio2_cmdSendAndWaitResp(i001->ad, COMIO2_CMD_READMEMORY, buffer, 4, resp, &l_resp, &comio2_err);
      if(ret==0)
      {
         c=     resp[4];
         c=c <<  8;
         c=c |  resp[3];
         c=c <<  8;
         c=c |  resp[2];
         c=c <<  8;
         c=c |  resp[1];
         
         (i001->indicators.nbcountersread)++;
      }
      else
      {
         (i001->indicators.nbcountersreaderr)++;
         if(comio2_err == COMIO2_ERR_DOWN)
         {
            return -1;
         }
      }

   }
   while(ret && retry<5);

   if(ret==0)
   {
      counter->last_counter=counter->counter;
      counter->wh_counter=c;
      counter->kwh_counter=c / 1000;
      counter->counter=c;
   }
   
   return 0;
}


/*
mea_error_t interface_type_001_counters_process_xpl_msg(interface_type_001_t *i001, xPL_ServicePtr theService, xPL_MessagePtr msg, char *device, char *type)
{
   mea_queue_t *counters_list=i001->counters_list;
   struct electricity_counter_s *counter;
   int type_id;

   (i001->indicators.nbcountersxplrecv)++;
   if(type)
      type_id=get_token_id_by_string(type);
   else
   {
      type_id=XPL_ENERGY_ID; // pas de type précisé => type par défaut compteur kw/h
      type=get_token_string_by_id(XPL_ENERGY_ID);
   }

   mea_queue_first(counters_list);
   for(int i=0; i<counters_list->nb_elem; i++)
   {
      mea_queue_current(counters_list, (void **)&counter);

      if(!device || mea_strcmplower(device,counter->name)==0)
      {
         xPL_MessagePtr cntrMessageStat ;
         char value[20];
         char *unit;
         
         if(type_id==XPL_ENERGY_ID)
         {
            sprintf(value,"%d", counter->kwh_counter);
            unit="kWh";
         }
         else if(type_id==XPL_POWER_ID)
         {
            sprintf(value,"%f", counter->power);
            unit="W";
         }
         else
            break;
           
         cntrMessageStat = xPL_createBroadcastMessage(theService, xPL_MESSAGE_STATUS) ;

         xPL_setBroadcastMessage(cntrMessageStat, FALSE);

         xPL_setSchema(cntrMessageStat,  get_token_string_by_id(XPL_SENSOR_ID), get_token_string_by_id(XPL_BASIC_ID));
         xPL_setMessageNamedValue(cntrMessageStat, get_token_string_by_id(XPL_DEVICE_ID),counter->name);
         xPL_setMessageNamedValue(cntrMessageStat, get_token_string_by_id(XPL_TYPE_ID),type);
         xPL_setMessageNamedValue(cntrMessageStat, get_token_string_by_id(XPL_CURRENT_ID),value);
         xPL_setMessageNamedValue(cntrMessageStat, get_token_string_by_id(UNIT_ID),unit);
         xPL_setTarget(cntrMessageStat, xPL_getSourceVendor(msg), xPL_getSourceDeviceID(msg), xPL_getSourceInstanceID(msg));
   
         // Broadcast the message
         mea_sendXPLMessage(cntrMessageStat, NULL);

         (i001->indicators.nbcountersxplsent)++;
         
         xPL_releaseMessage(cntrMessageStat);
      }
      mea_queue_next(counters_list);
   }
   return NOERROR;
}
*/

mea_error_t interface_type_001_counters_process_xpl_msg2(interface_type_001_t *i001, cJSON *xplMsgJson, char *device, char *type)
{
   mea_queue_t *counters_list=i001->counters_list;
   struct electricity_counter_s *counter;
   int type_id;

   (i001->indicators.nbcountersxplrecv)++;
   if(type)
      type_id=get_token_id_by_string(type);
   else
   {
      type_id=XPL_ENERGY_ID; // pas de type précisé => type par défaut compteur kw/h
      type=get_token_string_by_id(XPL_ENERGY_ID);
   }

   mea_queue_first(counters_list);
   for(int i=0; i<counters_list->nb_elem; i++)
   {
      mea_queue_current(counters_list, (void **)&counter);

      if(!device || mea_strcmplower(device,counter->name)==0)
      {
//         xPL_MessagePtr cntrMessageStat;
         char value[20];
         char *unit;
         
         if(type_id==XPL_ENERGY_ID)
         {
            sprintf(value,"%d", counter->kwh_counter);
            unit="kWh";
         }
         else if(type_id==XPL_POWER_ID)
         {
            sprintf(value,"%f", counter->power);
            unit="W";
         }
         else
            break;
        
         char str[256]; 
         cJSON *j = NULL;
         cJSON *msg_json = cJSON_CreateObject();
         
         sprintf(str,"%s.%s", get_token_string_by_id(XPL_SENSOR_ID), get_token_string_by_id(XPL_BASIC_ID));

         cJSON_AddItemToObject(msg_json, XPLSCHEMA_STR_C,  cJSON_CreateString(str)); 
         cJSON_AddItemToObject(msg_json, get_token_string_by_id(XPL_DEVICE_ID),  cJSON_CreateString(counter->name)); 
         cJSON_AddItemToObject(msg_json, get_token_string_by_id(XPL_TYPE_ID),    cJSON_CreateString(type)); 
         cJSON_AddItemToObject(msg_json, get_token_string_by_id(XPL_CURRENT_ID), cJSON_CreateString(value)); 
         cJSON_AddItemToObject(msg_json, get_token_string_by_id(UNIT_ID),        cJSON_CreateString(unit));

         j=cJSON_GetObjectItem(xplMsgJson,XPLSOURCE_STR_C);
         if(j)
            cJSON_AddItemToObject(msg_json, XPLTARGET_STR_C, cJSON_CreateString(j->valuestring));
         else
            cJSON_AddItemToObject(msg_json, XPLTARGET_STR_C, cJSON_CreateString("*"));
          
         mea_sendXPLMessage2(msg_json);

         (i001->indicators.nbcountersxplsent)++;
         
         cJSON_Delete(msg_json);
      }
      mea_queue_next(counters_list);
   }
   return NOERROR;
}


int16_t interface_type_001_counters_poll_inputs2(interface_type_001_t *i001)
{
   mea_queue_t *counters_list=i001->counters_list;
   struct electricity_counter_s *counter;

   mea_queue_first(counters_list);
   for(int16_t i=0; i<counters_list->nb_elem; i++)
   {
      mea_queue_current(counters_list, (void **)&counter);

      pthread_cleanup_push((void *)pthread_mutex_unlock, (void *)&(counter->lock));
      pthread_mutex_lock(&(counter->lock));
      
      if(!mea_test_timer(&(counter->trap_timer))) // traitement delai trop long entre 2 traps.
      {
         struct timeval tv;

         gettimeofday(&tv, NULL);
         
         counter->t=(double)tv.tv_sec+(double)tv.tv_usec/1000000.0;
         counter->last_power=counter->power;
         counter->power=0;

         // envoyer un message xpl 
         char str[256];
         cJSON *xplMsgJson = cJSON_CreateObject();
         cJSON_AddItemToObject(xplMsgJson, XPLMSGTYPE_STR_C, cJSON_CreateString(XPL_TRIG_STR_C));
         sprintf(str,"%s.%s", get_token_string_by_id(XPL_SENSOR_ID), get_token_string_by_id(XPL_BASIC_ID));
         cJSON_AddItemToObject(xplMsgJson, XPLSCHEMA_STR_C, cJSON_CreateString(str));
         cJSON_AddItemToObject(xplMsgJson, get_token_string_by_id(XPL_DEVICE_ID), cJSON_CreateString(counter->name));
         cJSON_AddItemToObject(xplMsgJson, get_token_string_by_id(XPL_TYPE_ID), cJSON_CreateString(get_token_string_by_id(XPL_POWER_ID)));
         cJSON_AddItemToObject(xplMsgJson, get_token_string_by_id(XPL_CURRENT_ID), cJSON_CreateString("0"));

         // Broadcast the message
         mea_sendXPLMessage2(xplMsgJson);

         (i001->indicators.nbcountersxplsent)++;

         cJSON_Delete(xplMsgJson);
      }
      
      pthread_mutex_unlock(&(counter->lock));
      pthread_cleanup_pop(0);

      if(!mea_test_timer(&(counter->timer)))
      {
         if(counter_read(i001, counter)<0)
         {
         }
         else
         {
            if(counter->counter!=counter->last_counter)
            {
//DBSERVER               if(counter->todbflag==1)
//DBSERVER                  counter_to_db(counter);
            }

            counter_to_xpl2(i001, counter);

            VERBOSE(9) mea_log_printf("%s (%s) : counter %s %ld (WH=%ld KWH=%ld)\n", INFO_STR, __func__, counter->name, (long)counter->counter, (long)counter->wh_counter, (long)counter->kwh_counter);
         }
         mea_queue_next(counters_list);
      }
   }
   return 0;
}


void interface_type_001_counters_init(interface_type_001_t *i001)
{
   mea_queue_t *counters_list=i001->counters_list;
   struct electricity_counter_s *counter;

   // initialisation des trap compteurs
   mea_queue_first(counters_list);
   for(int16_t i=0; i<counters_list->nb_elem; i++)
   {
      mea_queue_current(counters_list, (void **)&counter);
      
      counter->nbtrap=&(i001->indicators.nbcounterstraps);
      counter->nbxplout=&(i001->indicators.nbcountersxplsent);
      
      comio2_setTrap(i001->ad, counter->trap, interface_type_001_counters_process_traps2, (void *)counter);

      mea_start_timer(&(counter->timer));
      mea_start_timer(&(counter->trap_timer));
      
      mea_queue_next(counters_list);
   }
}
