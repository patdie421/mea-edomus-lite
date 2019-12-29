//
//  interface_type_002_sensors.c
//  mea-eDomus
//
//  Created by Patrice DIETSCH on 29/11/12.
//
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

#include "globals.h"
#include "mea_verbose.h"
#include "mea_queue.h"
#include "xPLServer.h"
#include "arduino_pins.h"
#include "parameters_utils.h"
#include "mea_string_utils.h"
#include "tokens.h"
#include "tokens_da.h"
#include "interface_type_001_sensors.h"

#include "processManager.h"

// parametres valide pour les capteurs ou actionneurs pris en compte par le type 1.
char *valid_sensor_params[]={"S:PIN","S:TYPE","S:COMPUTE","S:ALGO","I:POLLING_PERIODE",NULL};
#define SENSOR_PARAMS_PIN             0
#define SENSOR_PARAMS_TYPE            1
#define SENSOR_PARAMS_COMPUTE         2
#define SENSOR_PARAMS_ALGO            3
#define SENSOR_PARAMS_POLLING_PERIODE 4


struct assoc_s type_pin_assocs_i001_sensors[] = {
   {DIGITAL_ID, ARDUINO_D4},
   {DIGITAL_ID, ARDUINO_D10},
   {DIGITAL_ID, ARDUINO_D12},
   {ANALOG_ID,  ARDUINO_AI3},
   {-1,-1}
};


struct assoc_s type_compute_assocs_i001_sensors[] = {
   {ANALOG_ID, XPL_TEMP_ID},
   {ANALOG_ID, XPL_VOLTAGE_ID},
   {ANALOG_ID, RAW_ID},
   {-1,-1}
};


struct assoc_s compute_algo_assocs_i001_sensors[] = {
   {XPL_TEMP_ID,XPL_TMP36_ID},
   {XPL_VOLTAGE_ID,XPL_AREF5_ID},
   {XPL_VOLTAGE_ID,XPL_AREF11_ID},
   {-1,-1}
};


float _compute_tmp36(unsigned int value)
{
   return (value * 1.1/1024.0-0.5)*100.0;
}


float _compute_aref5(unsigned int value)
{
   return value * 5/1024.0;
}


float _compute_aref11(unsigned int value)
{
   return value * 1.1/1024.0;
}


float _compute_default(unsigned int value)
{
   return (float)value;
}


int _sensor_pin_type_i001(int token_type_id, int pin_id)
{
   return is_in_assocs_list(type_pin_assocs_i001_sensors, token_type_id, pin_id);
}


int _sensor_type_compute_i001(int token_type_id, int token_compute_id)
{
   if(token_compute_id==-1)
      return 1;
   
   return is_in_assocs_list(type_compute_assocs_i001_sensors, token_type_id, token_compute_id);
}


int _sensor_compute_algo_i001(int token_compute_id, int token_algo_id)
{
   return is_in_assocs_list(compute_algo_assocs_i001_sensors, token_compute_id, token_algo_id);
}


int _valide_sensor_i001(int token_type_id, int pin_id, int token_compute_id, int token_algo_id, int *err)
{
   if(token_type_id==-1) {
      *err=1;
      VERBOSE(5) mea_log_printf("%s (%s) : bad i/o type (%d)\n",ERROR_STR,__func__,token_type_id);
   }
   
   if(pin_id==-1) {
      *err=2;
      VERBOSE(5) mea_log_printf("%s (%s) : bad pin (%d)\n",ERROR_STR,__func__,pin_id);
   }
   
   int ret;
   ret=_sensor_pin_type_i001(token_type_id,pin_id);
   if(!ret) {
      *err=3;
      VERBOSE(5) mea_log_printf("%s (%s) : bad pin (%d) for pin type (%d)\n",ERROR_STR,__func__,pin_id,token_type_id);
      return 0;
   }
   
   if(token_compute_id!=-1) {
      ret=_sensor_type_compute_i001(token_type_id,token_compute_id);
      if(!ret) {
         *err=3;
         VERBOSE(5) mea_log_printf("%s (%s) : bad compute (%d) for pin type (%d)\n",ERROR_STR,__func__,token_compute_id,token_type_id);
         return 0;
      }
      
      if(token_algo_id!=-1) {
         ret=_sensor_compute_algo_i001(token_compute_id,token_algo_id);
         if(!ret) {
            *err=4;
            VERBOSE(5) mea_log_printf("%s (%s) : bad algo (%d) for compute (%d)\n",ERROR_STR,__func__,token_algo_id,token_compute_id);
            return 0;
         }
      }
   }
   else
   {
      if(token_algo_id!=-1) {
         *err=4;
         VERBOSE(5) mea_log_printf("%s (%s) : aglo set (%d) but non compute set\n",ERROR_STR,__func__,token_algo_id);
         return 0;
      }
   }
   
   return 1;
}


void interface_type_001_sensors_free_queue_elem(void *d)
{
   struct sensor_s *e=(struct sensor_s *)d;
   
   free(e);
   e=NULL;
}


int16_t interface_type_001_sensors_process_traps2(int16_t numTrap, char *data, int16_t l_data, void *userdata)
{
   struct sensor_s *sensor;
   
   sensor=(struct sensor_s *)userdata;
   
   sensor->val=(unsigned char)data[0];
   
   {
      *(sensor->nbtrap)=*(sensor->nbtrap)+1;
      
      char value[20];
      value[sizeof(value)-1]=0;
      
      if(data[0]==0) {
         sensor->val=0;
         snprintf(value, sizeof(value)-1, "low");
      }
      else {
         sensor->val=1;
         snprintf(value, sizeof(value)-1, "high");
      }
      
      VERBOSE(9) mea_log_printf("%s (%s) : sensor %s = %s\n", INFO_STR, __func__, sensor->name, value);

      char str[256];
      str[sizeof(str)-1]=0;
      cJSON *xplMsgJson = cJSON_CreateObject(); 
      cJSON_AddItemToObject(xplMsgJson, XPLMSGTYPE_STR_C, cJSON_CreateString(XPL_TRIG_STR_C));
      snprintf(str, sizeof(str)-1, "%s.%s", get_token_string_by_id(XPL_SENSOR_ID), get_token_string_by_id(XPL_BASIC_ID));
      cJSON_AddItemToObject(xplMsgJson, XPLSCHEMA_STR_C, cJSON_CreateString(str));
      cJSON_AddItemToObject(xplMsgJson, get_token_string_by_id(XPL_TYPE_ID), cJSON_CreateString(get_token_string_by_id(XPL_INPUT_ID)));
      cJSON_AddItemToObject(xplMsgJson, get_token_string_by_id(XPL_DEVICE_ID), cJSON_CreateString(sensor->name));
      cJSON_AddItemToObject(xplMsgJson, get_token_string_by_id(XPL_CURRENT_ID), cJSON_CreateString(value));
 
      // Broadcast the message
      mea_sendXPLMessage2(xplMsgJson);
      
      *(sensor->nbxplout)=*(sensor->nbxplout)+1;
      
      cJSON_Delete(xplMsgJson);
   }
   
   return NOERROR;
}




struct sensor_s *interface_type_001_sensors_valid_and_malloc_sensor(int16_t id_sensor_actuator, char *name, char *parameters)
{
   int pin_id;
   int type_id;
   int compute_id;
   int algo_id;
   
   parsed_parameters_t *sensor_params=NULL;
   int nb_sensor_params;
   int err;
   
   struct sensor_s *sensor=(struct sensor_s *)malloc(sizeof(struct sensor_s));
   if(!sensor) {
      VERBOSE(1) {
         mea_log_printf("%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
         perror("");
      }
      goto interface_type_001_sensors_valid_and_malloc_sensor_clean_exit;
   }
   
   sensor_params=alloc_parsed_parameters((char *)parameters, valid_sensor_params, &nb_sensor_params, &err,1);
   
   if(sensor_params) {
      type_id=get_token_id_by_string(sensor_params->parameters[SENSOR_PARAMS_TYPE].value.s);
      pin_id=mea_getArduinoPin(sensor_params->parameters[SENSOR_PARAMS_PIN].value.s);
      compute_id=get_token_id_by_string(sensor_params->parameters[SENSOR_PARAMS_COMPUTE].value.s);
      algo_id=get_token_id_by_string(sensor_params->parameters[SENSOR_PARAMS_ALGO].value.s);
      
      switch(type_id)
      {
         case ANALOG_ID:
            sensor->arduino_function=5;
            break;
         case DIGITAL_ID:
            sensor->arduino_function=6;
            break;
         default:
            VERBOSE(1) mea_log_printf("%s (%s) : bad sensor type (%s)\n",ERROR_STR,__func__,sensor_params->parameters[SENSOR_PARAMS_TYPE].value.s);
            goto interface_type_001_sensors_valid_and_malloc_sensor_clean_exit;
      }
      
      if(_valide_sensor_i001(type_id,pin_id,compute_id,algo_id,&err)) {
         strncpy(sensor->name, (char *)name, sizeof(sensor->name)-1);
         sensor->name[sizeof(sensor->name)-1]=0;
         mea_strtolower(sensor->name);
         sensor->sensor_id=id_sensor_actuator;
         sensor->arduino_pin=pin_id;
         sensor->arduino_pin_type=type_id;
         sensor->compute=compute_id;
         sensor->algo=algo_id;
         
         switch(algo_id) {
            case XPL_TMP36_ID:
               sensor->compute_fn=_compute_tmp36;
               break;
            case XPL_AREF5_ID:
               sensor->compute_fn=_compute_aref5;
               break;
            case XPL_AREF11_ID:
               sensor->compute_fn=_compute_aref11;
               break;
            default:
               sensor->compute_fn=NULL;
               break;
         }
         
         if(sensor_params->parameters[SENSOR_PARAMS_POLLING_PERIODE].value.i>0) {
            mea_init_timer(&(sensor->timer),sensor_params->parameters[SENSOR_PARAMS_POLLING_PERIODE].value.i,1);
         }
         else {
            mea_init_timer(&(sensor->timer),60,1); // lecture toutes les minutes par défaut
         }
         // start_timer(&(sensor->timer));
         sensor->nbtrap=NULL;
         sensor->nbxplout=NULL;
         release_parsed_parameters(&sensor_params);
         return sensor;
      }
      else {
         VERBOSE(1) mea_log_printf("%s (%s) : parametres (%s) non valides\n", ERROR_STR, __func__, parameters);
         goto interface_type_001_sensors_valid_and_malloc_sensor_clean_exit;
      }
   }
   else {
      VERBOSE(1) {
         mea_log_printf("%s (%s) : \"%s\"/\"%s\" invalid. Check parameters.\n",ERROR_STR, __func__, name, parameters);
      }
   }
   
interface_type_001_sensors_valid_and_malloc_sensor_clean_exit:
   if(sensor) {
      free(sensor);
      sensor=NULL;
   }

   if(sensor_params) {
      release_parsed_parameters(&sensor_params);
   }

   return NULL;
}


mea_error_t interface_type_001_sensors_process_xpl_msg2(interface_type_001_t *i001, cJSON *xplMsgJson, char *device, char *type)
/**
  * \brief     Traite les demandes xpl de retransmission de la valeur courrante ("sensor.request/request=current") pour interface_type_001
  * \details   La demande sensor.request peut être de la forme est de la forme :
  *            sensor.request
  *            {
  *               request=current
  *               [device=<device>]
  *               [type=<type>]
  *            }
  *            device et type sont optionnels. S'il ne sont pas précisé, le statut tous les capteurs sera émis. Si le type est précisé sans device,
  *            tous les statuts des capteurs du type "type" seront transmis.
  *
  * \param     i001                contexte de l'interface.
  * \param     xplMsgJson
  * \param     device              le périphérique à interroger ou NULL
  * \param     type                le type à interroger ou NULL
  * \return    ERROR en cas d'erreur, NOERROR sinon  */  
{
   mea_queue_t *sensors_list=i001->sensors_list;
   struct sensor_s *sensor;
   int type_id=0;
   uint16_t send_xpl_flag=0;
   int16_t no_type=0;

   i001->indicators.nbsensorsxplrecv++;
   if(type) {
      type_id=get_token_id_by_string(type);
      if(type_id==-1) {
         return ERROR; // type inconnu, on ne peut pas traiter
      }
   }
   else {
      no_type=1; // type par defaut, le type est celui du capteur
   }
   
   mea_queue_first(sensors_list);
   for(int i=0; i<sensors_list->nb_elem; i++) {
      mea_queue_current(sensors_list, (void **)&sensor);
      if(!device || mea_strcmplower(device,sensor->name)==0) { // pas de device, on transmettra le statut de tous les capteurs du type demandé (si précisé)
         char value[20];
         value[sizeof(value)-1]=0;
         char *unit=NULL;
         if(no_type==1) {  // pas de type demandé, on "calcule" le type du capteur
            if(sensor->arduino_pin_type==ANALOG_ID) {
               if(sensor->algo==XPL_AREF5_ID || sensor->algo==XPL_AREF11_ID) {
                  type_id=XPL_VOLTAGE_ID;
               }
               else if(sensor->algo==XPL_TMP36_ID) {
                  type_id=XPL_TEMP_ID;
               }
               else {
                  type_id=GENERIC_ID; // juste une valeur ...
               }
            }
            else if(sensor->arduino_pin_type==DIGITAL_ID) {
               type_id=XPL_INPUT_ID;
            }
            type=get_token_string_by_id(type_id);
         }
         
         // préparation de la réponse
         send_xpl_flag=0;
         switch(type_id) {
            case XPL_TEMP_ID: {
               if(sensor->arduino_pin_type==ANALOG_ID) {
                  if(sensor->algo==XPL_TMP36_ID) {
                     snprintf(value, sizeof(value)-1, "%0.2f", sensor->computed_val);
                     unit="c";
                  }
                  else {
                     snprintf(value, sizeof(value)-1, "%d", sensor->val);
                     unit=NULL;
                  }
                  send_xpl_flag=1;
               }
               break;
            }
            
            case XPL_VOLTAGE_ID:
            {
               if(sensor->arduino_pin_type==ANALOG_ID) {
                  if(sensor->algo==XPL_AREF5_ID || sensor->algo==XPL_AREF11_ID) {
                     snprintf(value, sizeof(value)-1, "%0.2f", sensor->computed_val);
                     unit="v";
                  }
                  else {
                     snprintf(value, sizeof(value)-1, "%d", sensor->val);
                     unit=NULL;
                  }
                  send_xpl_flag=1;
               }
               break;
            }
            
            case XPL_INPUT_ID:
            {
               if(sensor->arduino_pin_type==DIGITAL_ID) {
                  if(sensor->val==0) {
                     snprintf(value, sizeof(value)-1, "%s", LOW_STR_C);
                  }
                  else {
                     snprintf(value, sizeof(value)-1, "%s", HIGH_STR_C);
                  }
                  unit=NULL;
                  send_xpl_flag=1;
               }
               break;
            }
            
            case GENERIC_ID:
            {
               if(sensor->arduino_pin_type==ANALOG_ID) {
                  snprintf(value, sizeof(value)-1, "%d", sensor->val);
                  unit=NULL;
                  send_xpl_flag=1;
                  break;
               }
            }
            default:
               break;
         }
         
         if(send_xpl_flag) {
            char str[256];
            str[sizeof(str)-1]=0;
            cJSON *j = NULL;
            cJSON *msg_json = cJSON_CreateObject();

            VERBOSE(9) mea_log_printf("%s (%s) : sensor %s = %s\n", INFO_STR, __func__, sensor->name, value);

            cJSON_AddItemToObject(msg_json, XPLMSGTYPE_STR_C,  cJSON_CreateString(XPL_STAT_STR_C));

            snprintf(str, sizeof(str)-1, "%s.%s", get_token_string_by_id(XPL_SENSOR_ID), get_token_string_by_id(XPL_BASIC_ID));

            cJSON_AddItemToObject(msg_json, XPLSCHEMA_STR_C,  cJSON_CreateString(str));
            cJSON_AddItemToObject(msg_json, get_token_string_by_id(XPL_DEVICE_ID),  cJSON_CreateString(sensor->name));
            cJSON_AddItemToObject(msg_json, get_token_string_by_id(XPL_TYPE_ID),    cJSON_CreateString(type));
            cJSON_AddItemToObject(msg_json, get_token_string_by_id(XPL_CURRENT_ID), cJSON_CreateString(value));
            if(unit) {
               cJSON_AddItemToObject(msg_json, get_token_string_by_id(UNIT_ID),     cJSON_CreateString(unit));
            }
            j=cJSON_GetObjectItem(xplMsgJson, XPLSOURCE_STR_C);
            if(j) {
               cJSON_AddItemToObject(msg_json, XPLTARGET_STR_C, cJSON_CreateString(j->valuestring));
            }
            else {
               cJSON_AddItemToObject(msg_json, XPLTARGET_STR_C, cJSON_CreateString("*"));
            }

            mea_sendXPLMessage2(msg_json);

            i001->indicators.nbsensorsxplsent++;
         }
         
         if(device) { // demande ciblée ?
            return NOERROR; // oui => fin de traitement, sinon on continu
         }
      }
      mea_queue_next(sensors_list);
   }
   return NOERROR;
}


int16_t interface_type_001_sensors_poll_inputs2(interface_type_001_t *i001)
{
   mea_queue_t *sensors_list=i001->sensors_list;
   struct sensor_s *sensor;

   int16_t comio2_err;
   int unit=0;

   mea_queue_first(sensors_list);
   for(int16_t i=0; i<sensors_list->nb_elem; i++)
   {
      mea_queue_current(sensors_list, (void **)&sensor);
      if(!mea_test_timer(&(sensor->timer))) {
         if(sensor->arduino_pin_type==ANALOG_ID) {
            int v;

            unsigned char buffer[8], resp[8];
            uint16_t l_resp;
            buffer[0]=sensor->arduino_pin;

            int ret=comio2_call_fn(i001->ad, (uint16_t)sensor->arduino_function, (char *)buffer, 1, &v, resp, &l_resp, &comio2_err);
            if(ret<0) {
               VERBOSE(5) {
                  mea_log_printf("%s (%s) : comio2 error = %d.\n", ERROR_STR, __func__, comio2_err);
               }
               i001->indicators.nbsensorsreaderr++;
               if(comio2_err == COMIO2_ERR_DOWN) {
                  return -1;
               }
               continue;
            }
            else if(ret>0) {
               VERBOSE(5) {
                  mea_log_printf("%s (%s) : function %d return error = %d.\n", ERROR_STR, __func__, sensor->arduino_function, comio2_err);
               }
               i001->indicators.nbsensorsreaderr++;
               continue;
            }
            
            i001->indicators.nbsensorsread++;
            if(sensor->val!=v) {
               int16_t last=sensor->val;
               float computed_last;
               
               sensor->val=v;
               if(sensor->compute_fn==NULL) {
                  sensor->computed_val=v;
                  computed_last=last;
                  sensor->compute=RAW_ID;
               }
               else {
                  sensor->computed_val=sensor->compute_fn(v);
                  computed_last=sensor->compute_fn(last);
               }
               
               if(sensor->compute==XPL_TEMP_ID) {
                  VERBOSE(9) mea_log_printf("%s (%s) : temperature sensor %s =  %.1f °C (%d) \n", INFO_STR, __func__, sensor->name, sensor->computed_val, sensor->val);
                  unit = UNIT_C;
               }
               else if(sensor->compute==XPL_VOLTAGE_ID) {
                  VERBOSE(9) mea_log_printf("%s (%s) : voltage sensor %s =  %.1f V (%d) \n", INFO_STR, __func__, sensor->name, sensor->computed_val, sensor->val);
                  unit = UNIT_V;
               }
               else  {
                  VERBOSE(9) mea_log_printf("%s (%s) : raw sensor %s = %d\n", INFO_STR, __func__, sensor->name, sensor->val);
               }
                  
               char str[256];
               str[sizeof(str)-1]=0;

               cJSON *xplMsgJson = cJSON_CreateObject();
               cJSON_AddItemToObject(xplMsgJson, XPLMSGTYPE_STR_C, cJSON_CreateString(XPL_TRIG_STR_C));
               snprintf(str, sizeof(str)-1, "%s.%s", get_token_string_by_id(XPL_SENSOR_ID), get_token_string_by_id(XPL_BASIC_ID));
               cJSON_AddItemToObject(xplMsgJson, XPLSCHEMA_STR_C, cJSON_CreateString(str));
               cJSON_AddItemToObject(xplMsgJson, get_token_string_by_id(XPL_DEVICE_ID), cJSON_CreateString(sensor->name));
               cJSON_AddItemToObject(xplMsgJson, get_token_string_by_id(XPL_TYPE_ID), cJSON_CreateString(get_token_string_by_id(XPL_POWER_ID)));
               snprintf(str, sizeof(str)-1, "%0.1f",sensor->computed_val);
               cJSON_AddItemToObject(xplMsgJson, get_token_string_by_id(XPL_CURRENT_ID), cJSON_CreateString(str));
               snprintf(str, sizeof(str)-1, "%0.1f",computed_last);
               cJSON_AddItemToObject(xplMsgJson, get_token_string_by_id(XPL_LAST_ID), cJSON_CreateString(str));

               // Broadcast the message
               mea_sendXPLMessage2(xplMsgJson);

               (i001->indicators.nbsensorsxplsent)++;
               
               cJSON_Delete(xplMsgJson);   
            }
         }
         else {
            // traiter ici les capteurs logiques
         }
      }
      mea_queue_next(sensors_list);
   }
   return 0;
}


void interface_type_001_sensors_init(interface_type_001_t *i001)
{
   mea_queue_t *sensors_list=i001->sensors_list;
   struct sensor_s *sensor;

   mea_queue_first(sensors_list);
   for(int16_t i=0; i<sensors_list->nb_elem; i++) {
      mea_queue_current(sensors_list, (void **)&sensor);

      sensor->nbtrap=&(i001->indicators.nbsensorstraps);
      sensor->nbxplout=&(i001->indicators.nbsensorsxplsent);
      
      comio2_setTrap(i001->ad, sensor->arduino_pin+10, interface_type_001_sensors_process_traps2, (void *)sensor);

      mea_start_timer(&(sensor->timer));

      mea_queue_next(sensors_list);
   }
}
