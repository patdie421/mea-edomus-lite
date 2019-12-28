//
//  interface_type_001_actuators.c
//  mea-eDomus
//
//  Created by Patrice DIETSCH on 29/11/12.
//
//
#ifdef __APPLE__
#include <Python/Python.h>
#else
#include <Python.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

//#include "debug.h"

#include "globals.h"

#include "mea_verbose.h"
#include "arduino_pins.h"
#include "parameters_utils.h"
#include "tokens.h"
#include "tokens_da.h"
#include "mea_string_utils.h"

#include "interface_type_001.h"
#include "interface_type_001_actuators.h"

#include "processManager.h"

#include "cJSON.h"

// PIN=D5;TYPE=DIGITAL_OUT
char *valid_relay_params[]={"S:PIN","S:TYPE",NULL};
#define ACTUATOR_PARAMS_PIN        0
#define ACTUATOR_PARAMS_TYPE       1


struct assoc_s type_pin_assocs_i001_actuators[] = {
   {DIGITAL_ID,ARDUINO_D5},
   {DIGITAL_ID,ARDUINO_D11},
   {DIGITAL_ID,ARDUINO_D13},
   {DIGITAL_ID,ARDUINO_AI0},
   {DIGITAL_ID,ARDUINO_AI1},
   {DIGITAL_ID,ARDUINO_AI2},
   {ANALOG_ID, ARDUINO_D5},
   {ANALOG_ID, ARDUINO_D11},
   {-1,-1}
};


void _interface_type_001_free_actuators_queue_elem(void *d)
{
   struct actuator_s *e=(struct actuator_s *)d;
   
   free(e);
   e=NULL;
}


int actuator_pin_type_i001(int token_type_id, int pin_id)
{
   return is_in_assocs_list(type_pin_assocs_i001_actuators, token_type_id, pin_id);
}


int valide_actuator_i001(int token_type_id, int pin_id, int token_action_id, int *err)
{   
   if(token_type_id==-1) {
      *err=1;
      VERBOSE(5) mea_log_printf("%s (%s) : bad i/o type (%d)\n",ERROR_STR,__func__,token_type_id);
   }
   
   if(pin_id==-1) {
      *err=2;
      VERBOSE(5) mea_log_printf("%s (%s) : bad pin (%d)\n",ERROR_STR,__func__,pin_id);
   }
   
   int ret=actuator_pin_type_i001(token_type_id, pin_id);
   if(!ret) {
      *err=3;
      VERBOSE(5) mea_log_printf("%s (%s) : bad pin (%d) for pin type (%d)\n",ERROR_STR,__func__,pin_id,token_type_id);
      return 0;
   }
   
   return ret;
}


struct actuator_s *interface_type_001_valid_and_malloc_actuator(int16_t id_sensor_actuator, char *name, char *parameters)
{
   unsigned char pin_id=0;
   int16_t type_id=0;
   int16_t action_id=0;
   
   parsed_parameters_t *relay_params=NULL;
   int nb_relay_params;
   int err;
   
   struct actuator_s *actuator=(struct actuator_s *)malloc(sizeof(struct actuator_s));
   if(!actuator) {
      VERBOSE(2) {
         mea_log_printf("%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
         perror(""); }
      goto valid_and_malloc_relay_clean_exit;
   }

   relay_params=alloc_parsed_parameters((char *)parameters, valid_relay_params, &nb_relay_params, &err,1);
   if(relay_params) {
      type_id=get_token_id_by_string(relay_params->parameters[ACTUATOR_PARAMS_TYPE].value.s);
      pin_id=mea_getArduinoPin(relay_params->parameters[ACTUATOR_PARAMS_PIN].value.s);
      
      if(valide_actuator_i001(type_id,pin_id,action_id,&err)) {
         strncpy(actuator->name, (char *)name, sizeof(actuator->name)-1);
         actuator->name[sizeof(actuator->name)-1]=0;
         mea_strtolower(actuator->name);
         actuator->actuator_id=id_sensor_actuator;
         actuator->arduino_pin=pin_id;
         actuator->arduino_pin_type=type_id;
         actuator->old_val=0;
       
         release_parsed_parameters(&relay_params); 
         
         return actuator;
      }
      else {
         VERBOSE(2) mea_log_printf("%s (%s) : parametres (%s) non valides\n",ERROR_STR,__func__,parameters);
         goto valid_and_malloc_relay_clean_exit;
      }
   }
   else {
      VERBOSE(1) {
         mea_log_printf("%s (%s) : %s/%s invalid. Check parameters.\n",ERROR_STR,__func__,name,parameters);
      }
   }
   
valid_and_malloc_relay_clean_exit:
   if(actuator) {
      free(actuator);
      actuator=NULL;
   }
   if(relay_params)
      release_parsed_parameters(&relay_params);

   return NULL;
}


mea_error_t xpl_actuator2(interface_type_001_t *i001, cJSON *xplMsgJson, char *device, char *type)
{
   int ret;
   int type_id;
   unsigned char sval[2];
   int16_t comio2_err;

   (i001->indicators.nbactuatorsxplrecv)++;

   type_id=get_token_id_by_string(type);
   if(type_id != XPL_OUTPUT_ID && type_id !=VARIABLE_ID)
      return ERROR;
   
   if(mea_queue_first(i001->actuators_list)==-1) {
      return ERROR;
   }
   
   struct actuator_s *iq;
   while(1) {
      mea_queue_current(i001->actuators_list, (void **)&iq);
      if(mea_strcmplower(iq->name, device)==0) {  // OK, c'est bien pour moi ...
         char *current=NULL;
         cJSON *j = NULL;
         j=cJSON_GetObjectItem(xplMsgJson, get_token_string_by_id(XPL_CURRENT_ID));
         if(j) {
            current = j->valuestring;
         }
         if(!current) {
            return ERROR;
         }
         int current_id=get_token_id_by_string(current);
         if(type_id==XPL_OUTPUT_ID && iq->arduino_pin_type==DIGITAL_ID) {
            switch(current_id) {
               case XPL_PULSE_ID:
               {
                  int pulse_width;
                  char *data1=NULL;
                  j=cJSON_GetObjectItem(xplMsgJson, get_token_string_by_id(XPL_DATA1_ID));
                  if(j)
                     data1=j->valuestring;
                  if(data1) {
                     pulse_width=atoi(data1);
                     if(pulse_width<=0)
                        pulse_width=250;
                  }
                  else
                     pulse_width=250;

                  VERBOSE(9) mea_log_printf("%s (%s) : %s PLUSE %d ms on %d\n", INFO_STR, __func__, device, pulse_width, iq->arduino_pin);
                  
                  sval[0]=iq->arduino_pin;
                  sval[1]=((pulse_width / 100) & 0xFF);
                  ret=comio2_call_proc(i001->ad, 0, (char *)sval, 2, &comio2_err);
                  if(ret!=0) {
                     VERBOSE(9) mea_log_printf("%s (%s) : comio2_call_proc error (comio2_err=%d)\n", INFO_STR, __func__, comio2_err);
                     (i001->indicators.nbactuatorsouterr)++;
                     return ERROR;
                  }
                  
                  (i001->indicators.nbactuatorsout)++;
                  return NOERROR;
               }
               
               case HIGH_ID:
               case LOW_ID:
               {
                  int o=0;
                  
                  if(current_id==HIGH_ID)
                     o=255;
                  
                  VERBOSE(9) mea_log_printf("%s (%s) : %s set %d on pin %d\n", INFO_STR, __func__, device, o, iq->arduino_pin);
                  
                  sval[0]=iq->arduino_pin;
                  sval[1]=o;
                  ret=comio2_call_proc(i001->ad, 1, (char *)sval, 2, &comio2_err);
                  if(ret!=0) {
                     VERBOSE(9) mea_log_printf("%s (%s) : comio2_call_proc error (comio2_err=%d)\n", INFO_STR, __func__, comio2_err);
                     (i001->indicators.nbactuatorsouterr)++;

                     return ERROR;
                  }
                  /*
                     ajouter ici l'emission d'un message xpl
                   */
                  (i001->indicators.nbactuatorsout)++;
                  return NOERROR;
               }
               
               default:
                  VERBOSE(9) mea_log_printf("%s (%s) : incorrect request for %s (%s)\n", INFO_STR, __func__, device, type);
                  return ERROR;
            }
            
         }
         else if(type_id==VARIABLE_ID && iq->arduino_pin_type==ANALOG_ID) {
            int o=0;
            switch(current_id)
            {
               case DEC_ID:
                  o=(int)iq->old_val-1;
                  break;
               case INC_ID:
                  o=iq->old_val+1;
                  break;
               default:
               {
                  int16_t inc_dec=0; // +1 = inc ; -1 = dec
                  char *str;
                  if(current[0]=='-') {
                     inc_dec=-1;
                     str=&current[1];
                  }
                  else if (current[0]=='+') {
                     inc_dec=1;
                     str=&current[1];
                  }
                  else str=current;
                        
                  int n;
                  ret=sscanf(str,"%d%n", &o, &n);
                  if(o>255) o=255;
                        
                  if(ret==1 && !(strlen(str)-n)) {
                     if(inc_dec) {
                        o=iq->old_val+(uint16_t)o*(uint16_t)inc_dec;
                     }
                     if(o>255) o=255;
                     if(o<  0) o=0;
                  }
                  else {
                     VERBOSE(9) mea_log_printf("%s (%s) : %s ???\n", INFO_STR, __func__, current);
                     return ERROR; // erreur de syntaxe ...
                  }
               }
               break;
            }
            if(o>255) {
               o=255;
            }
            else if(o<0) {
               o=0;
            }
            iq->old_val=(uint16_t)o;
                  
            VERBOSE(9) mea_log_printf("%s (%s) : %s set %d on pin %d\n", INFO_STR, __func__, device, o, iq->arduino_pin);
            
            sval[0]=iq->arduino_pin;
            sval[1]=o;
            ret=comio2_call_proc(i001->ad, 2, (char *)sval, 2, &comio2_err);
            if(ret!=0) {
               VERBOSE(9) mea_log_printf("%s (%s) : comio2_call_proc error (comio2_err=%d)\n", INFO_STR, __func__, comio2_err);
               (i001->indicators.nbactuatorsouterr)++;
               return ERROR;
            }
            (i001->indicators.nbactuatorsout)++;
            /*
               ajouter ici l'emission d'un message xpl
             */
            return NOERROR;
         }
         return ERROR;
      }
      ret=mea_queue_next(i001->actuators_list);
      if(ret<0) {
         break;
      }
   }
   return ERROR;
}
