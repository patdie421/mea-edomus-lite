#define DEBUGFLAG 0

#include "interface_type_004.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>

#include "globals.h"
#include "consts.h"

#include "tokens.h"
#include "tokens_da.h"
#include "mea_verbose.h"
#include "macros.h"
#include "mea_string_utils.h"
#include "parameters_utils.h"
#include "philipshue.h"
#include "mea_sockets_utils.h"
#include "processManager.h"

#include "cJSON.h"
#include "uthash.h"

#include "interfacesServer.h"

// Parametrage :
// Name : LAMPE001
// Type : OUTPUT
// Interface : PHILIPSHUE01
// Parameters : HUELIGHT=Hue Lamp 1
//   ou
// Parameters : HUELIGHT=Hue Lamp 1;REACHABLE_USE=1
// on/off
// ./xPLSend -m cmnd -c control.basic device=lampe001 current=high type=output
// ./xPLSend -m cmnd -c control.basic device=lampe001 current=low type=output

// changement de couleur
// ./xPLSend -m cmnd -c control.basic device=lampe001 current=#FF0000 type=color

char *valid_hue_params[]={"S:HUELIGHT", "I:REACHABLE_USE", "S:HUEGROUP", NULL};
#define PARAMS_HUELIGH 0
#define PARAMS_REACHABLE 1
#define PARAMS_HUEGROUP 2

#define DEBUG_FLAG 1

char *interface_type_004_xplin_str="XPLIN";
char *interface_type_004_xplout_str="XPLOUT";
char *interface_type_004_lightschanges_str="NBTRANSITIONS";


int start_interface_type_004(int my_id, void *data, char *errmsg, int l_errmsg);
int stop_interface_type_004(int my_id, void *data, char *errmsg, int l_errmsg);
int restart_interface_type_004(int my_id, void *data, char *errmsg, int l_errmsg);
int16_t check_status_interface_type_004(interface_type_004_t *i004);


int16_t sendXPLLightState2(interface_type_004_t *i004, char *xplMsgType, char *deviceName, int16_t newState, int16_t reachable, int16_t on, int16_t last)
{
   char *current_state_str;
   char *last_state_str;
   char *on_str;
   char *reachable_str;
      
   char *high=get_token_string_by_id(HIGH_ID);
   char *low=get_token_string_by_id(LOW_ID);
   
   if(newState)
      current_state_str = high;
   else
      current_state_str = low;
      
   if(last!=-1) {
      if(last)
         last_state_str = low;
      else
         last_state_str = high;
   }
      
   if(on)
      on_str=high;
   else
      on_str=low;
   
   if(reachable)
      reachable_str=TRUE_STR_C;
   else
      reachable_str=FALSE_STR_C;

   char str[256];
   cJSON *xplMsgJson = cJSON_CreateObject();
   cJSON_AddItemToObject(xplMsgJson, XPLMSGTYPE_STR_C, cJSON_CreateString(xplMsgType));
   sprintf(str,"%s.%s", get_token_string_by_id(XPL_SENSOR_ID), get_token_string_by_id(XPL_BASIC_ID));
   cJSON_AddItemToObject(xplMsgJson, XPLSCHEMA_STR_C, cJSON_CreateString(str));
   cJSON_AddItemToObject(xplMsgJson, get_token_string_by_id(XPL_TYPE_ID), cJSON_CreateString(get_token_string_by_id(XPL_INPUT_ID)));
   cJSON_AddItemToObject(xplMsgJson, get_token_string_by_id(XPL_DEVICE_ID), cJSON_CreateString(deviceName));
   cJSON_AddItemToObject(xplMsgJson, get_token_string_by_id(XPL_CURRENT_ID), cJSON_CreateString(current_state_str));
   if(last != -1)
      cJSON_AddItemToObject(xplMsgJson, get_token_string_by_id(XPL_LAST_ID), cJSON_CreateString(last_state_str));
   cJSON_AddItemToObject(xplMsgJson, get_token_string_by_id(REACHABLE_ID), cJSON_CreateString(reachable_str));
   cJSON_AddItemToObject(xplMsgJson, ON_STR_C, cJSON_CreateString(on_str));

   // Broadcast the message
   mea_sendXPLMessage2(xplMsgJson);

   (i004->indicators.xplout)++;

   cJSON_Delete(xplMsgJson);

   return 0;
}


int16_t sendAllxPLTrigger(interface_type_004_t *i004)
{
   cJSON *current = i004->currentHueLightsState;
   
   if(current == NULL)
      return -1;
   
   cJSON *currentLight=current->child;
   while(currentLight) {
      struct lightsListElem_s *e = NULL /*, *tmp = NULL */;
      
      char *huename = my_cJSON_GetItemByName(currentLight, NAME_STR_C)->valuestring;
      
      char *deviceName=NULL;
      HASH_FIND(hh_huename, i004->lightsListByHueName, huename, strlen(huename), e);
      if(e && e->sensorname)
         deviceName = (char *)e->sensorname;
      
      if(deviceName) {
         cJSON *stateCurrent = my_cJSON_GetItemByName(currentLight, STATE_STR_C);
         if(stateCurrent) {
            cJSON *onCurrent        = my_cJSON_GetItemByName(stateCurrent, ON_STR_C);
            cJSON *reachableCurrent = my_cJSON_GetItemByName(stateCurrent, REACHABLE_STR_C);
            int16_t state = 1;
            if(e->reachable_use == 1) {
               state = reachableCurrent->valueint;
            }
            state = state & onCurrent->valueint;
            
            sendXPLLightState2(i004, XPL_TRIG_STR_C, deviceName, state, reachableCurrent->valueint, onCurrent->valueint, -1);
         }
      }
      currentLight=currentLight->next;
   }
   
   return 0;
}


int16_t whatChange(interface_type_004_t *i004)
{
   cJSON *current = i004->currentHueLightsState;
   cJSON *last = i004->lastHueLightsState;
   
   if(current == NULL || last == NULL)
      return -1;
   
   cJSON *currentLight=current->child;
   cJSON *lastLight = NULL;
   while(currentLight) {
      struct lightsListElem_s *e = NULL /*, *tmp = NULL */;
      
      char *_id = currentLight->string;
      char *huename = my_cJSON_GetItemByName(currentLight, NAME_STR_C)->valuestring;
      
      char *deviceName=NULL;
      HASH_FIND(hh_huename, i004->lightsListByHueName, huename, strlen(huename), e);
      if(e && e->sensorname)
         deviceName = (char *)e->sensorname;
      
      lastLight = cJSON_GetObjectItem(last, _id);
      if(deviceName && lastLight != NULL) {
         cJSON *stateCurrent = my_cJSON_GetItemByName(currentLight, STATE_STR_C);
         cJSON *stateLast    = my_cJSON_GetItemByName(lastLight,    STATE_STR_C);
         if(stateCurrent && stateLast) {
            cJSON *onCurrent        = my_cJSON_GetItemByName(stateCurrent, ON_STR_C);
            cJSON *onLast           = my_cJSON_GetItemByName(stateLast,    ON_STR_C);
            cJSON *reachableCurrent = my_cJSON_GetItemByName(stateCurrent, REACHABLE_STR_C);
            cJSON *reachableLast    = my_cJSON_GetItemByName(stateLast,    REACHABLE_STR_C);
            int16_t newState = -1;
            if(e->reachable_use == 1) {
               if(reachableCurrent->valueint != reachableLast->valueint)
                  newState = reachableCurrent->valueint;
            }
            if( (onCurrent->valueint != onLast->valueint) || newState != -1) {
               if(e->reachable_use == 1)
                  newState = newState & onCurrent->valueint;
               else
                  newState = onCurrent->valueint;
               if(newState != 0)
                  sendXPLLightState2(i004, XPL_TRIG_STR_C, deviceName, 1, reachableCurrent->valueint, onCurrent->valueint, 0);
               else
                  sendXPLLightState2(i004, XPL_TRIG_STR_C, deviceName, 0, reachableCurrent->valueint, onCurrent->valueint, 1);
               
               (i004->indicators.lightschanges)++;
            }
         }
      }
      currentLight=currentLight->next;
   }
   
   return 0;
}


int16_t interface_type_004_xPL_actuator2(interface_type_004_t *i004, cJSON *xplMsgJson, char *device, char *type)
{
   int type_id = -1;
   int16_t ret=-1;
   char *sensor=NULL;
   struct lightsListElem_s *e = NULL;
   char *current_value = NULL;
   cJSON *j=NULL;
  
   type_id=get_token_id_by_string(type);
   if(type_id != XPL_OUTPUT_ID && type_id != COLOR_ID)
      return -1;

   // récupérer ici tous les éléments du message xpl
   j = cJSON_GetObjectItem(xplMsgJson,get_token_string_by_id(XPL_CURRENT_ID));
   if(j)
      current_value = j->valuestring;
   if(!current_value)
      return -1;
   
   int current_value_id=-1;
   char *color_str=NULL;
   if(type_id == XPL_OUTPUT_ID)
      current_value_id=get_token_id_by_string(current_value);
   else if (type_id == COLOR_ID)
      color_str = current_value;

   int16_t state=-1;
   int16_t on=-1;
   int16_t reachable=-1;
   int16_t reachable_use=-1;
   uint32_t color=0xFFFFFFFF;
   
   int16_t id_sensor=-1;
   
   pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)&(i004->lock) );
   pthread_mutex_lock(&(i004->lock));

   DEBUG_SECTION2(DEBUGFLAG) {
      mea_log_printf("%s  (%s) : processing %s (current_value_id = %d, type_id = %d)\n", DEBUG_STR, __func__, device, current_value_id, type_id);
   }

   HASH_FIND(hh_actuatorname, i004->lightsListByActuatorName, device, strlen(device), e);
   if(e) {
      reachable_use = e->reachable_use;
      id_sensor = e->id_sensor;
      
      switch(current_value_id)
      {
         case HIGH_ID:
            ret=setLightStateByName(i004->currentHueLightsState, (char *)e->huename, 1, i004->server, i004->port, i004->user);
            DEBUG_SECTION2(DEBUGFLAG) mea_log_printf("%s  (%s) : setLightStateByName(%s, high) => ret = %d\n", DEBUG_STR, __func__, (char *)e->huename, ret);
            state=1;
            break;
         case LOW_ID:
            ret=setLightStateByName(i004->currentHueLightsState, (char *)e->huename, 0, i004->server, i004->port, i004->user);
            DEBUG_SECTION2(DEBUGFLAG) mea_log_printf("%s  (%s) : setLightStateByName(%s, low) => ret = %d\n", DEBUG_STR, __func__, (char *)e->huename, ret);
            state=0;
            break;
         default:
            if(type_id == COLOR_ID) {
               int n=0,l=0;
               
               n=sscanf(color_str,"#%6x%n",&color, &l);
               if(n==1 && l==strlen(color_str)) {
                  if(color==0) { // off => on envoie low
                     ret=setLightStateByName(i004->currentHueLightsState, (char *)e->huename, 0, i004->server, i004->port, i004->user);
                     DEBUG_SECTION2(DEBUGFLAG) mea_log_printf("%s  (%s) : setLightStateByName(%s, low) => ret = %d\n", DEBUG_STR, __func__, (char *)e->huename, ret);
                     state=0;
                  }
                  else { // on envoie la couleur
                     ret=setLightColorByName(i004->currentHueLightsState, (char *)e->huename, color, i004->server, i004->port, i004->user);
                     DEBUG_SECTION2(DEBUGFLAG) mea_log_printf("%s  (%s) : setLightColorByName(%s, %d) => ret = %d\n", DEBUG_STR, __func__, (char *)e->huename, color, ret);
                     state=1;
                  }
               }
               else {
                  color=0xFFFFFFFF;
               }
            }
            else
               ret=-1;
            break;
      }
      if(ret!=-1) {
         if(state != ret)
            (i004->indicators.lightschanges)++;
      }
   }
   else {
      DEBUG_SECTION2(DEBUGFLAG) mea_log_printf("%s  (%s) : device %s not found in lights list, try in groups list\n", DEBUG_STR, __func__, device);

      struct groupsListElem_s *g = NULL;
      HASH_FIND(hh_groupname, i004->groupsListByGroupName, device, strlen(device), g);
      if(g) {
         // traiter
         switch(current_value_id)
         {
            case HIGH_ID:
               ret=setGroupStateByName(i004->allGroups, (char *)g->huegroupname, 1, i004->server, i004->port, i004->user);
               break;
            case LOW_ID:
               ret=setGroupStateByName(i004->allGroups, (char *)g->huegroupname, 0, i004->server, i004->port, i004->user);
               break;
            default:
               if(type_id == COLOR_ID) {
                  int n=0,l=0;
                  
                  n=sscanf(color_str,"#%6x%n",&color, &l);
                  if(n==1 && l==strlen(color_str)) {
                     if(color==0) { // off => on envoie low
                        ret=setGroupStateByName(i004->allGroups, (char *)g->huegroupname, 0, i004->server, i004->port, i004->user);
                     }
                     else { // on envoie la couleur
                        ret=setGroupColorByName(i004->allGroups, (char *)g->huegroupname, color, i004->server, i004->port, i004->user);
                     }
                  }
                  else {
                     color=0xFFFFFFFF;
                  }
               }
               else
                  ret=-1;
               break;
         }
      }
/*
      else
      {
         DEBUG_SECTION2(DEBUGFLAG) mea_log_printf("%s  (%s) : device %s not found in groups list, try in scenes list\n", DEBUG_STR, __func__, device);
         struct scenesListElem_s *s = NULL;
         HASH_FIND(hh_scenename, i004->scenesListBySceneName, device, strlen(device), s);
         if(s)
         {
            // traiter
            switch(current_value_id)
            {
               case HIGH_ID:
                  ret=setGroupStateByName(i004->allScenes, (char *)s->huescenename, 1, i004->server, i004->port, i004->user);
                  break;
               case LOW_ID:
                  ret=setGroupStateByName(i004->allScenes, (char *)s->huescenename, 0, i004->server, i004->port, i004->user);
                  break;
               default:
                  ret=-1;
                  break;
            }
         }
         else
         {
            DEBUG_SECTION2(DEBUGFLAG) mea_log_printf("%s  (%s) : device %s not found !!!\n", DEBUG_STR, __func__, device);
            ret = -1;
         }
      }
*/
      else {
         DEBUG_SECTION2(DEBUGFLAG) mea_log_printf("%s  (%s) : device %s not found !!!\n", DEBUG_STR, __func__, device);
         ret = -1;
      }
   }
   
   if(ret!=-1 && id_sensor>0) // si aussi un capteur, on récupère les info avec de libérer le verrou
   {
      getLightStateByName(i004->currentHueLightsState, (char *)e->huename, &on, &reachable);
      if(e->sensorname) {
         sensor = (char *)malloc(strlen(e->sensorname)+1);
         strcpy(sensor,e->sensorname);
      }
   }
   
   pthread_mutex_unlock(&(i004->lock));
   pthread_cleanup_pop(0);
   
   if(ret != -1 && id_sensor>0) {
      if( (on!=-1 && type_id == XPL_OUTPUT_ID) || color != 0xFFFFFFFF) { // si on a des info (on != -1), c'est donc qu'on doit envoyer un message xpl
         if(reachable_use==1)
            state = state & reachable;
         sendXPLLightState2(i004, XPL_TRIG_STR_C, sensor, state, reachable, on, -1);
      }

      if(sensor)
         free(sensor);
   }
   return ret;
}


//int16_t interface_type_004_xPL_sensor(interface_type_004_t *i004, xPL_ServicePtr theService, xPL_MessagePtr msg, char *device, char *type)
/**
 * \brief     Traite les demandes xpl de retransmission de la valeur courrante ("sensor.request/request=current") pour interface_type_004
 * \details   La demande sensor.request peut être de la forme est de la forme :
 *            sensor.request
 *            {
 *               request=current
 *               device=<device>
 *               [type=<type>]
 *            }
 *            type est optionnel. Si le type est précisé le type par défaut est "input".
 *
 * \param     i004                contexte de l'interface.
 * \param     theService          xxx
 * \param     ListNomsValeursPtr  xxx
 * \param     device              le périphérique à interroger ou NULL
 * \param     device              le type à interroger ou NULL
 * \return    ERROR en cas d'erreur, NOERROR sinon  */
/*
{
   //   int type_id = -1;
   struct lightsListElem_s *e = NULL;
   int16_t ret=-1;
   
   if(type && get_token_id_by_string(type) != XPL_OUTPUT_ID)
      return -1; // type inconnu, on ne peut pas traiter
   
   int16_t on = 0, reachable = 0;
   int16_t state = 0;
   
   pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)&(i004->lock) );
   pthread_mutex_lock(&(i004->lock));
   
   HASH_FIND(hh_sensorname, i004->lightsListBySensorName, device, strlen(device), e);
   if(e && i004->currentHueLightsState) {
      if(getLightStateByName(i004->currentHueLightsState, (char *)e->huename, &on, &reachable)!=NULL) {
         state = on;
         if(e->reachable_use == 1)
            state = state & reachable;
         
         VERBOSE(9) mea_log_printf("%s  (%s) : sensor %s = %d\n",INFO_STR,__func__,e->sensorname, state);
         
         ret=0;
      }
   }
   
   pthread_mutex_unlock(&(i004->lock));
   pthread_cleanup_pop(0);
   
   if(ret==0)
      sendXPLLightState2(i004, XPL_STAT_STR_C, device, state, reachable, on, 0);
   
   return ret;
}
*/

int16_t interface_type_004_xPL_sensor2(interface_type_004_t *i004, cJSON *xplMsgJson, char *device, char *type)
/**
 * \brief     Traite les demandes xpl de retransmission de la valeur courrante ("sensor.request/request=current") pour interface_type_004
 * \details   La demande sensor.request peut être de la forme est de la forme :
 *            sensor.request
 *            {
 *               request=current
 *               device=<device>
 *               [type=<type>]
 *            }
 *            type est optionnel. Si le type est précisé le type par défaut est "input".
 *
 * \param     i004                contexte de l'interface.
 * \param     theService          xxx
 * \param     ListNomsValeursPtr  xxx
 * \param     device              le périphérique à interroger ou NULL
 * \param     device              le type à interroger ou NULL
 * \return    ERROR en cas d'erreur, NOERROR sinon  */
{
   //   int type_id = -1;
   struct lightsListElem_s *e = NULL;
   int16_t ret=-1;
   
   if(type && get_token_id_by_string(type) != XPL_OUTPUT_ID)
      return -1; // type inconnu, on ne peut pas traiter
   
   int16_t on = 0, reachable = 0;
   int16_t state = 0;
   
   pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)&(i004->lock) );
   pthread_mutex_lock(&(i004->lock));
   
   HASH_FIND(hh_sensorname, i004->lightsListBySensorName, device, strlen(device), e);
   if(e && i004->currentHueLightsState) {
      if(getLightStateByName(i004->currentHueLightsState, (char *)e->huename, &on, &reachable)!=NULL) {
         state = on;
         if(e->reachable_use == 1)
            state = state & reachable;
         
         VERBOSE(9) mea_log_printf("%s  (%s) : sensor %s = %d\n",INFO_STR,__func__,e->sensorname, state);
         
         ret=0;
      }
   }
   
   pthread_mutex_unlock(&(i004->lock));
   pthread_cleanup_pop(0);
   
   if(ret==0)
      sendXPLLightState2(i004, XPL_STAT_STR_C, device, state, reachable, on, 0);
   
   return ret;
}


int16_t interface_type_004_xPL_callback2(cJSON *xplMsgJson, struct device_info_s *device_info, void * userValue)
{
   char *schema = NULL, *device = NULL, *type = NULL;
   cJSON *j=NULL;

   interface_type_004_t *i004=(interface_type_004_t *)userValue;

   j = cJSON_GetObjectItem(xplMsgJson, XPLSCHEMA_STR_C);
   if(j)
      schema = j->valuestring;
   else {
      VERBOSE(9) mea_log_printf("%s (%s) : xPL message no schema\n", INFO_STR, __func__);
      return 0;
   }

   j = cJSON_GetObjectItem(xplMsgJson, get_token_string_by_id(XPL_DEVICE_ID));
   if(j)
      device=j->valuestring;

   j = cJSON_GetObjectItem(xplMsgJson, get_token_string_by_id(XPL_TYPE_ID));
   if(j)
      type=j->valuestring;

   mea_strtolower(device);
  
   (i004->indicators.xplin)++;
   
   VERBOSE(9) mea_log_printf("%s  (%s) : xPL Message to process : %s\n",INFO_STR,__func__,schema);
   
   if(mea_strcmplower(schema, XPL_CONTROLBASIC_STR_C) == 0) {
      if(!device) {
         VERBOSE(5) mea_log_printf("%s  (%s) : xPL message no device\n",INFO_STR,__func__);
         return -1;
      }
      if(!type) {
         VERBOSE(5) mea_log_printf("%s  (%s) : xPL message no type\n",INFO_STR,__func__);
         return -1;
      }
      return interface_type_004_xPL_actuator2(i004, xplMsgJson, device, type);
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
         VERBOSE(5) mea_log_printf("%s  (%s) : xPL message request!=current\n",INFO_STR,__func__);
         return -1;
      }
      return interface_type_004_xPL_sensor2(i004, xplMsgJson, device, type);
   }
   
   return 0;
}


void set_interface_type_004_isnt_running(void *data)
{
   interface_type_004_t *i004 = (interface_type_004_t *)data;
   i004->thread_is_running=0;
}


void *_thread_interface_type_004(void *thread_data)
{
   struct thread_interface_type_004_args_s *args = (struct thread_interface_type_004_args_s *)thread_data;
   
   interface_type_004_t *i004=args->i004;
   char server[sizeof(i004->server)];
   strcpy(server,i004->server);
   char user[sizeof(i004->user)];
   strcpy(user,i004->user);
   int port=i004->port;
   
   free(thread_data);
   thread_data=NULL;
  
   pthread_cleanup_push( (void *)set_interface_type_004_isnt_running, (void *)i004 );
   i004->thread_is_running=1;
   
   if(i004->allGroups) {
      cJSON_Delete(i004->allGroups);
      i004->allGroups=NULL;
   }
   i004->allGroups = getAllGroups(server, port, user); // à recharger ensuite régulièrement
//   i004->allScenes = getAllScenes(server, port, user); // à recharger ensuite régulièrement
   mea_timer_t sendAllxPLTriggerTimer, getAllGroupsAndScenesTimer;
   
   mea_init_timer(&sendAllxPLTriggerTimer, 300, 1); // envoi d'un xPLTrigger de toutes les lampes toutes les 5 min (300 secondes)
   mea_start_timer(&sendAllxPLTriggerTimer);
   mea_init_timer(&getAllGroupsAndScenesTimer, 600, 1); // lecture des groupes toutes les 10 min (600 secondes)
   mea_start_timer(&getAllGroupsAndScenesTimer);
   pthread_t *me = i004->thread;
 
   while(1) {
      pthread_testcancel();
      
      process_heartbeat(i004->monitoring_id);
      process_update_indicator(i004->monitoring_id, I004_XPLIN,  i004->indicators.xplin);
      process_update_indicator(i004->monitoring_id, I004_XPLOUT, i004->indicators.xplout);
      process_update_indicator(i004->monitoring_id, I004_LIGHSCHANGES, i004->indicators.lightschanges);
      
      cJSON *allLights = getAllLights(server, port, user);
      if(me != i004->thread)
         pthread_exit(NULL);

      if(!allLights) {
         VERBOSE(2) {
            mea_log_printf("%s (%s) : can't load lights stats list\n", ERROR_STR,__func__);
         }
      }
      else {
         cJSON *tmp;
         
         pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)&(i004->lock) );
         pthread_mutex_lock(&(i004->lock));
         
         tmp = i004->lastHueLightsState;
         i004->lastHueLightsState = i004->currentHueLightsState;
         i004->currentHueLightsState = allLights;
         
         if(mea_test_timer(&sendAllxPLTriggerTimer)==0) {
            sendAllxPLTrigger(i004);
         }
         else if(i004->lastHueLightsState && i004->currentHueLightsState) {
            whatChange(i004);
         }
         
         pthread_mutex_unlock(&(i004->lock));
         pthread_cleanup_pop(0);
         
         if(tmp)
            cJSON_Delete(tmp);
      }
      
      if(mea_test_timer(&getAllGroupsAndScenesTimer)==0)
      {
         cJSON *tmp;
         
         tmp = NULL;
         cJSON *allGroups = getAllGroups(server, port, user);
         if(me != i004->thread)
            pthread_exit(NULL);
         if(allGroups) {
            pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)&(i004->lock) );
            pthread_mutex_lock(&(i004->lock));
            
            tmp = i004->allGroups;
            i004->allGroups=allGroups;
            
            pthread_mutex_unlock(&(i004->lock));
            pthread_cleanup_pop(0);
            
            if(tmp)
               cJSON_Delete(tmp);
         }
/*         
         tmp = NULL;
         cJSON *allScenes = getAllScenes(server, port, user);
         if(allScenes) {
            pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)&(i004->lock) );
            pthread_mutex_lock(&(i004->lock));
            
            tmp = i004->allScenes;            
            i004->allScenes=allScenes;
            
            pthread_mutex_unlock(&(i004->lock));
            pthread_cleanup_pop(0);
            
            if(tmp)
               cJSON_Delete(tmp);
         }
*/      
      }
      
   _thread_interface_type_004_next_loop:
      sleep(5);
   }
   
_thread_interface_type_004_clean_exit:
   pthread_cleanup_pop(1);
   
   pthread_exit(NULL);
}


int _interface_type_004_clean_configs_lists(interface_type_004_t *i004)
{
   if(i004->lightsListByHueName) {
      struct lightsListElem_s *e = NULL, *te = NULL;
      
      HASH_CLEAR(hh_sensorname, i004->lightsListBySensorName);
      HASH_CLEAR(hh_actuatorname, i004->lightsListByActuatorName);
      
      HASH_ITER(hh_huename, i004->lightsListByHueName, e, te) {
         HASH_DELETE(hh_huename, i004->lightsListByHueName, e);
         if(e->sensorname) {
            free((void *)(e->sensorname));
            e->sensorname=NULL;
         }
         if(e->sensorname) {
            free((void *)(e->actuatorname));
            e->actuatorname=NULL;
         }
         if(e->huename) {
            free((void *)(e->huename));
            e->huename=NULL;
         }
         free(e);
      }
      
      i004->lightsListByHueName=NULL;
      i004->lightsListBySensorName=NULL;
      i004->lightsListByActuatorName=NULL;
   }
   
   if(i004->groupsListByGroupName) {
      struct groupsListElem_s *g = NULL, *tg = NULL;
      
      HASH_ITER(hh_groupname, i004->groupsListByGroupName, g, tg) {
         HASH_DELETE(hh_groupname, i004->groupsListByGroupName, g);
         if(g->groupname) {
            free((void *)(g->groupname));
            g->groupname=NULL;
         }
         free(g);
      }
      i004->groupsListByGroupName=NULL;
   }

/*
   if(i004->scenesListBySceneName)
   {
      struct scenesListElem_s *s = NULL, *ts = NULL;
      
      HASH_ITER(hh_scenename, i004->scenesListBySceneName, s, ts) {
         if(s->scenename) {
            free((void *)(s->scenename));
            s->scenename=NULL;
         }
         HASH_DELETE(hh_scenename, i004->scenesListBySceneName, s);
         free(s);
      }
      i004->scenesListBySceneName=NULL;
   }
*/   
   return 0;
}


int load_interface_type_004(interface_type_004_t *i004, cJSON *jsonInterface)
{
   int ret = -1;
   parsed_parameters_t *hue_params=NULL;
   int nb_hue_params=0;
   int nerr=0;
   
   i004->loaded=0;
   
   
   // on vide d'abord les listes s'il y a déjà des données
   _interface_type_004_clean_configs_lists(i004);

   cJSON *jsonDevices = cJSON_GetObjectItem(jsonInterface, "devices");
   if(jsonDevices == NULL)
      goto  load_interface_type_004_clean_exit;

   cJSON *jsonDevice = jsonDevices->child;

   // récupération des parametrages des capteurs dans la base
   while (jsonDevice) // boucle de traitement du résultat de la requete
   {
         // les valeurs dont on a besoin
      char *name=jsonDevice->string;
      int id_sensor_actuator=(int)cJSON_GetObjectItem(jsonDevice,"id_sensor_actuator")->valuedouble;
      int id_type=(int)cJSON_GetObjectItem(jsonDevice,"id_type")->valuedouble;
      char *parameters=cJSON_GetObjectItem(jsonDevice,"parameters")->valuestring;
      int todbflag = 0;

      hue_params=alloc_parsed_parameters((char *)parameters, valid_hue_params, &nb_hue_params, &nerr, 0);
      if(hue_params && nb_hue_params>0) {
         int params_test = (hue_params->parameters[PARAMS_HUELIGH].value.s != NULL) + (hue_params->parameters[PARAMS_HUEGROUP].value.s != NULL);
            
         if(params_test != 1) {
            VERBOSE(2) {
               mea_log_printf("%s (%s) : a HUELIGHT, HUEGROUP or HUESCENE parameter is mandatory and only one ... (device %s skipped)", WARNING_STR, __func__, name);
            }
            continue;
         }
            
         if(hue_params->parameters[PARAMS_HUELIGH].value.s) { // c'est une lampe ?
            struct lightsListElem_s *e = NULL;
               
            if(id_type != 500 && id_type != 501) {
               VERBOSE(2) {
                  mea_log_printf("%s (%s) : incorrect type (INPUT or OUTPUT only allowed) ... (device %s skipped)", WARNING_STR, __func__, name);
                  continue;
               }
            }
            // on cherche dans la liste si elle existe déjà
            HASH_FIND(hh_huename, i004->lightsListByHueName, hue_params->parameters[PARAMS_HUELIGH].value.s, strlen(hue_params->parameters[PARAMS_HUELIGH].value.s), e);
            if (e == NULL) {
               // elle n'existe pas on va la créer
               e = (struct lightsListElem_s *)malloc(sizeof(struct lightsListElem_s));
               char *_huename=malloc(strlen(hue_params->parameters[PARAMS_HUELIGH].value.s)+1);
               if(_huename==NULL) {
                  VERBOSE(2) {
                     mea_log_printf("%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
                     perror("");
                  }
                  goto load_interface_type_004_clean_exit;
               }
               strcpy(_huename, hue_params->parameters[PARAMS_HUELIGH].value.s);
               e->huename = _huename;
               e->sensorname=NULL;
               e->actuatorname=NULL;
               e->id_sensor=-1;
               e->todbflag_sensor=-1;
               e->id_actuator=-1;
               e->todbflag_actuator=-1;
               e->reachable_use=0;
               HASH_ADD_KEYPTR( hh_huename, i004->lightsListByHueName, e->huename, strlen(e->huename), e );
            }
            
            char *_name=NULL;
            _name=malloc(strlen((char *)name)+1);
            if(_name==NULL) {
               VERBOSE(2) {
                  mea_log_printf("%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
                  perror("");
               }
               goto load_interface_type_004_clean_exit;
            }
            strcpy(_name, (char *)name);
            mea_strtolower(_name);
            
            switch(id_type)
            {
               case 500: // c'est une entrée
                  if(e->id_sensor != -1) {
                     // warning déjà initialisé comme capteur
                  }
                  else {
                     e->sensorname = _name;
                     e->id_sensor = id_sensor_actuator;
                     e->todbflag_sensor=todbflag;
                     e->reachable_use=hue_params->parameters[PARAMS_REACHABLE].value.i;
                     HASH_ADD_KEYPTR( hh_sensorname, i004->lightsListBySensorName, e->sensorname, strlen(e->sensorname), e );
                  }
                  break;
               case 501:
                  if(e->id_actuator != -1) {
                     // erreur déjà initialisé comme actionneur
                  }
                  else {
                     e->actuatorname = _name;
                     e->id_actuator = id_sensor_actuator;
                     e->todbflag_actuator=todbflag;
                     HASH_ADD_KEYPTR( hh_actuatorname, i004->lightsListByActuatorName, e->actuatorname, strlen(e->actuatorname), e );
                  }
                  break;
               default:
                  VERBOSE(2) {
                     mea_log_printf("%s (%s) : type error, skipped",WARNING_STR,__func__);
                  }
                  // erreur, pas type autorisé
                  break;
            }
         }
         else if(hue_params->parameters[PARAMS_HUEGROUP].value.s) { // c'est un groupe ?
            if(id_type != 501) {
               VERBOSE(2) {
                  mea_log_printf("%s (%s) : group is output only (device %s skipped)", WARNING_STR, __func__, name);
               }
               continue;
            }
               
            struct groupsListElem_s *g = NULL;
            HASH_FIND(hh_groupname, i004->groupsListByGroupName, (char *)name, strlen((char *)name), g);
            if (g == NULL) {
               // il n'existe pas on va le créer
               g = (struct groupsListElem_s *)malloc(sizeof(struct groupsListElem_s));
               char *_groupname=malloc(strlen((char *)name)+1);
               if(_groupname==NULL) {
                  VERBOSE(2) {
                     mea_log_printf("%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
                     perror("");
                  }
                  goto load_interface_type_004_clean_exit;
               }
               char *_huegroupname=malloc(strlen(hue_params->parameters[PARAMS_HUEGROUP].value.s)+1);
               if(_huegroupname==NULL) {
                  VERBOSE(2) {
                     mea_log_printf("%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
                     perror("");
                  }
                  free(_groupname);
                  _groupname=NULL;
                  goto load_interface_type_004_clean_exit;
               }
               strcpy(_groupname, (char *)name);
               mea_strtolower(_groupname);
               g->groupname = _groupname;
               strcpy(_huegroupname, hue_params->parameters[PARAMS_HUEGROUP].value.s);
               g->huegroupname = _huegroupname;
               
               HASH_ADD_KEYPTR( hh_groupname, i004->groupsListByGroupName, g->groupname, strlen(g->groupname), g );
            }
            else {
               VERBOSE(2) {
                  mea_log_printf("%s (%s) : group already defined, skipped",WARNING_STR,__func__);
               }
            }
         }
         release_parsed_parameters(&hue_params);
      }
      else {
         VERBOSE(2) {
            mea_log_printf("%s (%s) : parameter error ... (device %s skipped)", WARNING_STR, __func__, name);
         }
      }

      jsonDevice=jsonDevice->next;
   }
   
   i004->loaded=1;
   return 0;
   
load_interface_type_004_clean_exit:
   _interface_type_004_clean_configs_lists(i004);
   i004->xPL_callback2=NULL;
   if(hue_params) {
      release_parsed_parameters(&hue_params);
      hue_params=NULL;
   }
   return -1;
}


int16_t get_huesystem_connection_parameters(char *device, char *server, uint16_t l_server, int *port, char *user, uint16_t l_user)
{
   char _server[41];
   int  _port=0;
   char _user[41];
   int n=0, r=0;
   
   char *_device=mea_strtrim(device); // on eleve les blancs devant et derrière
   
   // on recherche "HTTP://<server>:<port>@<user>" ou "HTTP://<server>@<user>"
   n=sscanf(_device,"HTTP://%40[^:]:%d@%40s%n",_server,&_port,_user, &r);
   if(n!=3) {
      _port=80;
      n=sscanf(_device,"HTTP://%40[^@]@%40s%n",_server,_user, &r);
      if(n!=2)
         return -1;
   }
   
   if(r != strlen(_device)) // il reste des données qu'on a pas traitées => erreur
      return -1;
   
   strncpy(server, _server, l_server);
   server[l_server-1]=0;
   *port=_port;
   strncpy(user,_user,l_user);
   server[l_user-1]=0;
   
   return 0;
}


xpl2_f get_xPLCallback_interface_type_004(void *ixxx)
{
   interface_type_004_t *i004 = (interface_type_004_t *)ixxx;

   if(i004 == NULL)
      return NULL;
   else
      return i004->xPL_callback2;
}


int get_monitoring_id_interface_type_004(void *ixxx)
{
   interface_type_004_t *i004 = (interface_type_004_t *)ixxx;

   if(i004 == NULL)
      return -1;
   else
      return i004->monitoring_id;
}


int set_xPLCallback_interface_type_004(void *ixxx, xpl2_f cb)
{
   interface_type_004_t *i004 = (interface_type_004_t *)ixxx;

   if(i004 == NULL)
      return -1;
   else {
      i004->xPL_callback2 = cb;
      return 0;
   }
}


int set_monitoring_id_interface_type_004(void *ixxx, int id)
{
   interface_type_004_t *i004 = (interface_type_004_t *)ixxx;

   if(i004 == NULL)
      return -1;
   else {
      i004->monitoring_id = id;
      return 0;
   }
}


int get_type_interface_type_004()
{
   return INTERFACE_TYPE_004;
}


interface_type_004_t *malloc_and_init2_interface_type_004(int id_driver, cJSON *jsonInterface)
{
   interface_type_004_t *i004=NULL;
   
   i004=(interface_type_004_t *)malloc(sizeof(interface_type_004_t));
   if(!i004) {
      VERBOSE(2) {
         mea_log_printf("%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
         perror("");
      }
      return NULL;
   }
   i004->thread_is_running=0;
   pthread_mutex_init(&i004->lock,NULL);
   struct interface_type_004_start_stop_params_s *i004_start_stop_params=(struct interface_type_004_start_stop_params_s *)malloc(sizeof(struct interface_type_004_start_stop_params_s));
   if(!i004_start_stop_params) {
      free(i004);
      i004=NULL;
      VERBOSE(2) {
         mea_log_printf("%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
         perror("");
      }
      return NULL;
   }

   char *parameters=cJSON_GetObjectItem(jsonInterface,"parameters")->valuestring;

   i004->parameters=(char *)malloc(strlen((char *)parameters)+1);
   if(!i004->parameters) {
      free(i004);
      i004=NULL;
      free(i004_start_stop_params);
      i004_start_stop_params=NULL;
      
      VERBOSE(2) {
         mea_log_printf("%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
         perror("");
      }
      return NULL;
   }

   int id_interface=(int)cJSON_GetObjectItem(jsonInterface,"id_interface")->valuedouble;
   char *name=jsonInterface->string;
   char *dev=cJSON_GetObjectItem(jsonInterface,"dev")->valuestring;
   char *description=cJSON_GetObjectItem(jsonInterface,"description")->valuestring;

   strcpy(i004->parameters,(char *)parameters);
   strncpy(i004->dev, (char *)dev, sizeof(i004->dev)-1);
   strncpy(i004->name, (char *)name, sizeof(i004->name)-1);
   i004->id_interface=id_interface;
   i004->indicators.xplin=0;
   i004->indicators.xplout=0;
   i004->indicators.lightschanges=0;
   i004->lightsListByHueName=NULL;
   i004->lightsListByActuatorName=NULL;
   i004->lightsListBySensorName=NULL;
   i004->groupsListByGroupName=NULL;
   i004->allGroups=NULL;
   i004->lastHueLightsState=NULL;
   i004->currentHueLightsState=NULL;
   i004->server[0]=0;
   i004->user[0]=0;
   i004->port=0;
   i004->xPL_callback2=NULL;
   i004->thread=NULL;
   i004->loaded=0;
   
   i004->monitoring_id=process_register((char *)name);
   i004_start_stop_params->i004=i004;
   
   process_set_group(i004->monitoring_id, 1);
   process_set_start_stop(i004->monitoring_id, start_interface_type_004, stop_interface_type_004, (void *)i004_start_stop_params, 1);
   process_set_watchdog_recovery(i004->monitoring_id, restart_interface_type_004, (void *)i004_start_stop_params);
   process_set_description(i004->monitoring_id, (char *)description);
   process_set_heartbeat_interval(i004->monitoring_id, 60); // chien de garde au bout de 60 secondes sans heartbeat
   
   process_add_indicator(i004->monitoring_id, interface_type_004_xplout_str, 0);
   process_add_indicator(i004->monitoring_id, interface_type_004_xplin_str, 0);
   process_add_indicator(i004->monitoring_id, interface_type_004_lightschanges_str, 0);
   
   return i004;
}


int update_devices_type_004(void *ixxx)
{
   printf("update devices type 004\n");

   return 0;
}


int clean_interface_type_004(void *ixxx)
{
   interface_type_004_t *i004 = (interface_type_004_t *)ixxx;

   if(i004->parameters) {
      free(i004->parameters);
      i004->parameters=NULL;
   }
   
   i004->xPL_callback2=NULL;
   
   if(i004->thread) {
      free(i004->thread);
      i004->thread=NULL;
   }

   _interface_type_004_clean_configs_lists(i004);

   if(i004->lastHueLightsState) {
      cJSON_Delete(i004->lastHueLightsState);
      i004->lastHueLightsState=NULL;
   }
   
   if(i004->currentHueLightsState) {
      cJSON_Delete(i004->currentHueLightsState);
      i004->lastHueLightsState=NULL;
   }
   
   if(i004->allGroups) {
      cJSON_Delete(i004->allGroups);
      i004->allGroups=NULL;
   }
   
   return 0;
}


int stop_interface_type_004(int my_id, void *data, char *errmsg, int l_errmsg)
{
   if(!data)
      return -1;
   
   struct interface_type_004_start_stop_params_s *start_stop_params=(struct interface_type_004_start_stop_params_s *)data;
   
   VERBOSE(1) mea_log_printf("%s  (%s) : %s shutdown thread ...\n", INFO_STR, __func__, start_stop_params->i004->name);
   
   if(start_stop_params->i004->xPL_callback2) {
      start_stop_params->i004->xPL_callback2=NULL;
   }
  
   if(start_stop_params->i004->thread) {
      pthread_cancel(*(start_stop_params->i004->thread));
      
      int counter=100;
      while(counter--) {
         if(start_stop_params->i004->thread_is_running) {  // pour éviter une attente "trop" active
            usleep(100); // will sleep for 10 ms
         }
         else
            break;
      }
      DEBUG_SECTION2(DEBUGFLAG) mea_log_printf("%s (%s) : %s, fin après %d itération(s)\n", DEBUG_STR, __func__, start_stop_params->i004->name, 100-counter);
      
      free(start_stop_params->i004->thread);
      start_stop_params->i004->thread=NULL;
   }
   
   return 0;
}


int restart_interface_type_004(int my_id, void *data, char *errmsg, int l_errmsg)
{
   process_stop(my_id, NULL, 0);
   sleep(5);
   return process_start(my_id, NULL, 0);
}


int16_t check_status_interface_type_004(interface_type_004_t *it004)
/**
 * \brief     indique si une anomalie a généré l'emission d'un signal SIGHUP
 * \param     i004           descripteur de l'interface
 * \return    toujours 0, pas d'emission de signal
 **/
{
   return 0;
}


int start_interface_type_004(int my_id, void *data, char *errmsg, int l_errmsg)
{
   int16_t ret;
   
   char server[41];
   int  port=0;
   char user[41];
   
   pthread_t *interface_type_004_thread_id=NULL; // descripteur du thread
   
   struct interface_type_004_start_stop_params_s *start_stop_params=(struct interface_type_004_start_stop_params_s *)data; // données pour la gestion des arrêts/relances
   struct thread_interface_type_004_args_s *interface_type_004_thread_args=NULL; // parametre à transmettre au thread

   cJSON *jsonInterface = getInterfaceById_alloc(start_stop_params->i004->id_interface);
   if(jsonInterface == NULL) {
      VERBOSE(2) mea_log_printf("%s (%s) : can't get interface description.\n", ERROR_STR,__func__);
      return -1;
   }
 
   if(start_stop_params->i004->loaded!=1) {
      // les données sont elles déjà chargées ?
      ret=load_interface_type_004(start_stop_params->i004, jsonInterface);
      if(ret<0) {
         VERBOSE(2) mea_log_printf("%s (%s) : can not load lights.\n", ERROR_STR,__func__);
         return -1;
      }
   }
   
   // si on a trouvé une config
   if(start_stop_params->i004->loaded==1) {
      // avec dev qui doit contenir "HTTP://<server>:<port>@<user>" ou "HTTP://<server>@<user>"
      ret=get_huesystem_connection_parameters((char *)start_stop_params->i004->dev, server, sizeof(server), &port, user, sizeof(user));
      if(ret==-1) {
         VERBOSE(2) mea_log_printf("%s (%s) : unknow interface device - %s\n", ERROR_STR,__func__, start_stop_params->i004->dev);
         goto start_interface_type_004_clean_exit;
      }
   }
   
   interface_type_004_thread_args=malloc(sizeof(struct thread_interface_type_004_args_s));
   if(!interface_type_004_thread_args) {
      VERBOSE(2) mea_log_printf("%s (%s) : malloc - %s\n", ERROR_STR,__func__);
      perror("");
      goto start_interface_type_004_clean_exit;
   }
   interface_type_004_thread_args->i004=start_stop_params->i004;
   
   strncpy(start_stop_params->i004->server, server, sizeof(start_stop_params->i004->server));
   start_stop_params->i004->server[sizeof(start_stop_params->i004->server)-1]=0;
   
   strncpy(start_stop_params->i004->user, user, sizeof(start_stop_params->i004->user));
   start_stop_params->i004->user[sizeof(start_stop_params->i004->user)-1]=0;
   start_stop_params->i004->port=port;
   
   start_stop_params->i004->xPL_callback2=interface_type_004_xPL_callback2;
   
   interface_type_004_thread_id=(pthread_t *)malloc(sizeof(pthread_t));
   if(!interface_type_004_thread_id) {
      VERBOSE(2) mea_log_printf("%s (%s) : malloc - %s\n", ERROR_STR,__func__);
      goto start_interface_type_004_clean_exit;
   }
   
   if(pthread_create (interface_type_004_thread_id, NULL, _thread_interface_type_004, (void *)interface_type_004_thread_args)) {
      VERBOSE(2) mea_log_printf("%s (%s) : pthread_create - can't start thread\n",ERROR_STR,__func__);
      goto start_interface_type_004_clean_exit;
   }
   fprintf(stderr,"INTERFACE_TYPE_004 : %x\n", (unsigned int)*interface_type_004_thread_id);

   start_stop_params->i004->thread=interface_type_004_thread_id;

   pthread_detach(*interface_type_004_thread_id);

   VERBOSE(2) mea_log_printf("%s  (%s) : %s %s.\n", INFO_STR, __func__, start_stop_params->i004->name, launched_successfully_str);
   
   return 0;
   
start_interface_type_004_clean_exit:
   if(interface_type_004_thread_args) {
      free(interface_type_004_thread_args);
      interface_type_004_thread_args=NULL;
   }
   if(interface_type_004_thread_id) {
      free(interface_type_004_thread_id);
      interface_type_004_thread_id=NULL;
   }
   
   return -1;
}

#ifndef ASPLUGIN
int get_fns_interface_type_004(struct interfacesServer_interfaceFns_s *interfacesFns)
{
   interfacesFns->malloc_and_init2 = (malloc_and_init2_f)&malloc_and_init2_interface_type_004;
   interfacesFns->get_monitoring_id = (get_monitoring_id_f)&get_monitoring_id_interface_type_004;
   interfacesFns->get_xPLCallback = (get_xPLCallback_f)&get_xPLCallback_interface_type_004;
   interfacesFns->update_devices = (update_devices_f)&update_devices_type_004;
   interfacesFns->clean = (clean_f)&clean_interface_type_004;
   interfacesFns->set_monitoring_id = (set_monitoring_id_f)&set_monitoring_id_interface_type_004;
   interfacesFns->set_xPLCallback = (set_xPLCallback_f)&set_xPLCallback_interface_type_004;
   interfacesFns->get_type = (get_type_f)&get_type_interface_type_004;
   interfacesFns->api = NULL;
   interfacesFns->lib = NULL;
   interfacesFns->type = interfacesFns->get_type();
   interfacesFns->plugin_flag = 0;

   return 0;
}
#endif
