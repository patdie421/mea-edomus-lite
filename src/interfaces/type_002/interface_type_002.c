/*
 *
 *  Created by Patrice Dietsch on 24/08/12.
 *  Copyright 2012 -. All rights reserved.
 *
 */
#ifdef __APPLE__
#include <Python/Python.h>
#else
#include <Python.h>
#endif

#include "interface_type_002.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>

#include "macros.h"
#include "globals.h"
#include "consts.h"

#include "mea_string_utils.h"
#include "mea_verbose.h"
#include "mea_queue.h"

#include "tokens.h"
#include "tokens_da.h"

#include "parameters_utils.h"

#include "xbee.h"
#include "serial.h"
#include "mea_plugins_utils.h"

#include "processManager.h"
#include "mea_xpl.h"

#include "interfacesServer.h"
#include "interface_type_002.h"

#define DBG_FREE(x) dbg_free((void *)(x), (char *)__func__, (int)__LINE__)

void dbg_free(void *ptr, char *func, int line)
{
   fprintf(MEA_STDERR, "FREE %s %d\n",func, line);
   free(ptr);
}

char *interface_type_002_senttoplugin_str="SENT2PLUGIN";
char *interface_type_002_xplin_str="XPLIN";
char *interface_type_002_xbeedatain_str="XBEEIN";
char *interface_type_002_commissionning_request_str="COMMIN";


typedef void (*thread_f)(void *);

// parametres valide pour les capteurs ou actionneurs pris en compte par le type 2.
char *valid_xbee_plugin_params[]={"S:PLUGIN","S:PARAMETERS", NULL};
#define PLUGIN_PARAMS_PLUGIN      0
#define PLUGIN_PARAMS_PARAMETERS  1


char *printf_mask_addr="%02x%02x%02x%02x-%02x%02x%02x%02x";


struct callback_data_s // donnee "userdata" pour les callbacks
{
   xbee_xd_t       *xd;
   mea_queue_t     *queue;
   pthread_mutex_t *callback_lock;
   pthread_cond_t  *callback_cond;
};


struct callback_commissionning_data_s
{
   interface_type_002_t *i002;
   
   // indicateurs
   uint32_t       *commissionning_request;
   uint32_t       *senttoplugin;
};


struct callback_xpl_data_s
{
};


struct thread_params_s
{
   xbee_xd_t            *xd;
   mea_queue_t          *queue;
   pthread_mutex_t       callback_lock;
   pthread_cond_t        callback_cond;
   parsed_parameters_t  *plugin_params;
   int                   nb_plugin_params;
   data_queue_elem_t    *e;
   interface_type_002_t *i002;
};


int start_interface_type_002(int my_id, void *data, char *errmsg, int l_errmsg);
int stop_interface_type_002(int my_id, void *data, char *errmsg, int l_errmsg);
int restart_interface_type_002(int my_id, void *data, char *errmsg, int l_errmsg);
int16_t check_status_interface_type_002(interface_type_002_t *it002);


void set_interface_type_002_isnt_running(void *data)
{
   interface_type_002_t *i002 = (interface_type_002_t *)data;
   i002->thread_is_running=0;
}


mea_error_t display_frame(int ret, unsigned char *resp, uint16_t l_resp)
{
   DEBUG_SECTION {
      if(!ret) {
         for(int i=0;i<l_resp;i++)
            fprintf(MEA_STDERR, "%02x-[%c](%03d)\n",resp[i],resp[i],resp[i]);
         fprintf(MEA_STDERR, "\n");
      }
   }
   return NOERROR;
}


mea_error_t print_frame(int ret, unsigned char *resp, uint16_t l_resp)
{
   DEBUG_SECTION {
      for(int i=0;i<l_resp;i++)
         fprintf(MEA_STDERR, "[%03x - %c]", resp[i], resp[i]);
      fprintf(MEA_STDERR, "\n");
   }
   return NOERROR;
}


void display_addr(char *a)
{
   DEBUG_SECTION {
      for(int i=0;i<4;i++)
         fprintf(MEA_STDERR, "%02x",a[i]);
   }
}


void _iodata_free_queue_elem(void *d)
{
   data_queue_elem_t *e=(data_queue_elem_t *)d;
   
// /!\ voir s'il ne faut pas nettoyer l'interieur de e
   if(e->cmd) {
      DBG_FREE(e->cmd);
      e->cmd=NULL;
   }
   
   DBG_FREE(e);
   e=NULL;
}


int add_16bits_to_at_uchar_cmd_from_int(unsigned char *at_cmd, uint16_t val)
{
   uint16_t i=2;
   int8_t h,l;
   
   h=(val/0xFF) & 0xFF;
   l=val%0xFF;
   
   if(h>0) {
      at_cmd[i]=h;
      i++;
   }
   at_cmd[i]=l;
   i++;
   at_cmd[i]=-1;
   
   return i;
}


int at_set_16bits_reg_from_int(xbee_xd_t *xd, xbee_host_t *host, char at_cmd[2], int reg_val, int16_t *nerr)
{
   unsigned char resp[254];
   uint16_t l_resp;
   unsigned char at[5];
   uint16_t l_at;
   int16_t ret;
   
   if(!xd)
      return ERROR;
   
   struct xbee_remote_cmd_response_s *map_resp;
   
   at[0]=at_cmd[0];at[1]=at_cmd[1];
   
   l_at=add_16bits_to_at_uchar_cmd_from_int(at, reg_val);
   ret=xbee_atCmdSendAndWaitResp(xd, host, at, l_at, resp, &l_resp, nerr);
   
   if(ret)
      return ERROR;
   map_resp=(struct xbee_remote_cmd_response_s *)resp;
   
   return (int)map_resp->cmd_status;
}


int at_get_local_char_array_reg(xbee_xd_t *xd, unsigned char at_cmd[2], char *reg_val, uint16_t *l_reg_val, int16_t *nerr)
{
   unsigned char resp[254];
   uint16_t l_resp;
   unsigned char at[16];
   uint16_t     l_at;
   int16_t ret;
   
   if(!xd)
      return ERROR;

   struct xbee_cmd_response_s *map_resp;
   
   at[0]=at_cmd[0];at[1]=at_cmd[1];l_at=2;
   
   int16_t err;
   
   ret=xbee_atCmdSendAndWaitResp(xd,NULL,at,l_at,resp,&l_resp, &err);
   if(ret<0)
      return ERROR;
   
   map_resp=(struct xbee_cmd_response_s *)resp;
   if(l_reg_val)
      *l_reg_val=l_resp-5;
   
   if(reg_val) {
      for(int i=0;i<(l_resp-5);i++)
         reg_val[i]=map_resp->at_cmd_data[i];
   }
   
   return (int)map_resp->cmd_status;
}


void addr_64_char_array_to_int(char *h, char *l, uint32_t *addr_64_h, uint32_t *addr_64_l)
{
   *addr_64_h=0;
   for(int i=0;i<4;i++)
      *addr_64_h=(*addr_64_h) | (int32_t)((unsigned char)h[i])<<((3-i)*8);
   
   *addr_64_l=0;
   for(int i=0;i<4;i++)
      *addr_64_l=*addr_64_l | (int32_t)((unsigned char)l[i])<<((3-i)*8);
}


int get_local_xbee_addr(xbee_xd_t *xd, xbee_host_t *local_xbee)
{
   int ret=0;
   int16_t nerr;

   // récupération de l'adresse de l'xbee connecter au PC (pas forcement le coordinateur).
   uint32_t addr_64_h;
   uint32_t addr_64_l;

   unsigned char at_cmd[2];
   uint16_t l_reg_val;
   char addr_l[4];
   char addr_h[4];

   memset(addr_l,0,4);
   memset(addr_h,0,4);

   // lecture de l'adresse de l'xbee local (ie connecté au port serie)
   uint8_t localAddrFound=0;
   for(int i=0;i<5;i++) {
      at_cmd[0]='S';at_cmd[1]='H';
      ret=at_get_local_char_array_reg(xd, at_cmd, (char *)addr_h, &l_reg_val, &nerr);
      if(ret!=-1) {
         at_cmd[0]='S';at_cmd[1]='L';
         ret=at_get_local_char_array_reg(xd, at_cmd, (char *)addr_l, &l_reg_val, &nerr);
         if(ret!=-1) {
            localAddrFound=1;
            break;
         }
      }
      VERBOSE(9) mea_log_printf("%s  (%s) : can't get local xbee address (try %d/5)\n", INFO_STR, __func__, i+1);
      sleep(1);
   }
   if(localAddrFound)
      addr_64_char_array_to_int(addr_h, addr_l, &addr_64_h, &addr_64_l);
   else {
      VERBOSE(9) mea_log_printf("%s  (%s) : can't read local xbee address. Device probably not an xbee, check it.\n", INFO_STR, __func__);
      return -1;
   }

   VERBOSE(9) mea_log_printf("%s  (%s) : local address is : %02x-%02x\n", INFO_STR, __func__, addr_64_h, addr_64_l);

   xbee_get_host_by_addr_64(xd, local_xbee, addr_64_h, addr_64_l, &nerr);

   return 0;
}

cJSON *data_from_json_interface(cJSON *jsonInterface)
{
   cJSON *data=cJSON_CreateObject();
   uint32_t addr_h, addr_l;

   cJSON_AddNumberToObject(data, INTERFACE_ID_STR_C, cJSON_GetObjectItem(jsonInterface, "id_interface")->valuedouble);
   cJSON_AddNumberToObject(data, INTERFACE_TYPE_ID_STR_C, cJSON_GetObjectItem(jsonInterface, "id_type")->valuedouble);
   cJSON_AddStringToObject(data, INTERFACE_NAME_STR_C, jsonInterface->string);
   cJSON_AddNumberToObject(data, INTERFACE_STATE_STR_C, cJSON_GetObjectItem(jsonInterface, "state")->valuedouble);
   if(sscanf((char *)cJSON_GetObjectItem(jsonInterface, "dev")->valuestring, "MESH://%x-%x", &addr_h, &addr_l)==2) {
      cJSON_AddNumberToObject(data, get_token_string_by_id(ADDR_H_ID), addr_h);
      cJSON_AddNumberToObject(data, get_token_string_by_id(ADDR_L_ID), addr_l);
   }
   cJSON_AddNumberToObject(data, API_KEY_STR_C, cJSON_GetObjectItem(jsonInterface, "id_interface")->valuedouble);

   return data;
}


int16_t _interface_type_002_xPL_callback2(cJSON *xplMsgJson, struct device_info_s *device_info, void *userValue)
{
   char *device;
   int err;
   
   interface_type_002_t *interface=(interface_type_002_t *)userValue;
   
   interface->indicators.xplin++;
   cJSON *j = NULL;
   j = cJSON_GetObjectItem(xplMsgJson, get_token_string_by_id(XPL_DEVICE_ID));
   if(!j)
      return ERROR;
   device = j->valuestring;
   if(!device || device[0]==0)
      return ERROR;
   
   parsed_parameters_t *plugin_params=NULL;
   int nb_plugin_params;
       
   plugin_params=alloc_parsed_parameters(device_info->parameters, valid_xbee_plugin_params, &nb_plugin_params, &err, 0);
   if(!plugin_params || !plugin_params->parameters[PLUGIN_PARAMS_PLUGIN].value.s) {
      if(plugin_params)
          release_parsed_parameters(&plugin_params);
      return ERROR; // si pas de parametre (=> pas de plugin) ou pas de fonction ... pas la peine d'aller plus loin pour ce capteur
   }
   
   cJSON *data=cJSON_CreateObject();
   data=device_info_to_json_alloc(device_info);
   cJSON *msg=mea_xplMsgToJson_alloc(xplMsgJson);
   cJSON_AddItemToObject(data, XPLMSG_STR_C, msg);
   if(plugin_params->parameters[PLUGIN_PARAMS_PARAMETERS].value.s)
      cJSON_AddStringToObject(data, DEVICE_PARAMETERS_STR_C, plugin_params->parameters[PLUGIN_PARAMS_PARAMETERS].value.s);
   cJSON_AddNumberToObject(data, DEVICE_TYPE_ID_STR_C, (double)device_info->type_id);
   cJSON_AddNumberToObject(data, get_token_string_by_id(ID_XBEE_ID), (double)((long)interface->xd));
   cJSON_AddNumberToObject(data, API_KEY_STR_C, interface->id_interface);

//   python_cmd_json(plugin_params->parameters[PLUGIN_PARAMS_PLUGIN].value.s, XPLMSG_JSON, data);
   plugin_fireandforget_function_json(plugin_params->parameters[PLUGIN_PARAMS_PLUGIN].value.s, XPLMSG_JSON, data);

   interface->indicators.senttoplugin++;

   release_parsed_parameters(&plugin_params);
   plugin_params=NULL;

   return NOERROR;
}


mea_error_t _inteface_type_002_xbeedata_callback(int id, unsigned char *cmd, uint16_t l_cmd, void *data, char *h, char *l)
{
   struct timeval tv;
   struct callback_data_s *callback_data;
   data_queue_elem_t *e;
   
   callback_data=(struct callback_data_s *)data;
   
   gettimeofday(&tv, NULL);
   
   e=(data_queue_elem_t *)malloc(sizeof(data_queue_elem_t));
   
   memcpy(e->addr_64_h, h, 4);
   memcpy(e->addr_64_l, l, 4);
   e->cmd=(unsigned char *)malloc(l_cmd+1);
   memcpy(e->cmd,cmd,l_cmd);
   e->l_cmd=l_cmd;
   memcpy(&e->tv,&tv,sizeof(struct timeval));
   
   pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)(&callback_data->callback_lock) );
   pthread_mutex_lock(callback_data->callback_lock);
   mea_queue_in_elem(callback_data->queue, e);
   if(callback_data->queue->nb_elem>=1)
      pthread_cond_broadcast(callback_data->callback_cond);
   pthread_mutex_unlock(callback_data->callback_lock);
   pthread_cleanup_pop(0);
   
   return NOERROR;
}


mea_error_t _interface_type_002_commissionning_callback(int id, unsigned char *cmd, uint16_t l_cmd, void *data, char *addr_h, char *addr_l)
{
   struct xbee_node_identification_response_s *nd_resp=NULL;
   int err=-1;
   
   struct callback_commissionning_data_s *callback_commissionning=(struct callback_commissionning_data_s *)data;
   *(callback_commissionning->commissionning_request)=*(callback_commissionning->commissionning_request)+1;
   
   nd_resp=(struct xbee_node_identification_response_s *)cmd;
   
   char addr[18];
   sprintf(addr,
           // "%02x%02x%02x%02x-%02x%02x%02x%02x",
           printf_mask_addr,
           nd_resp->addr_64_h[0],
           nd_resp->addr_64_h[1],
           nd_resp->addr_64_h[2],
           nd_resp->addr_64_h[3],
           nd_resp->addr_64_l[0],
           nd_resp->addr_64_l[1],
           nd_resp->addr_64_l[2],
           nd_resp->addr_64_l[3]);
   
   VERBOSE(9) mea_log_printf("%s (%s)  : commissionning request received from %s.\n", INFO_STR, __func__, addr);
   
   char devName[256];
   snprintf(devName, sizeof(devName)-1, "%s://%s", callback_commissionning->i002->name, addr);
   cJSON *jsonInterface = getInterfaceByDevName_alloc(devName);
   if(jsonInterface == NULL)
      return ERROR;

   char *parameters=cJSON_GetObjectItem(jsonInterface,PARAMETERS_STR_C)->valuestring;

   parsed_parameters_t *plugin_params=NULL;
   int nb_plugin_params=-1;
      
   plugin_params=alloc_parsed_parameters(parameters, valid_xbee_plugin_params, &nb_plugin_params, &err, 0);
   if(!plugin_params || !plugin_params->parameters[PLUGIN_PARAMS_PLUGIN].value.s) {
      if(plugin_params)
         release_parsed_parameters(&plugin_params);
      return ERROR;
   }

   cJSON *_data=data_from_json_interface(jsonInterface);
   cJSON_AddNumberToObject(_data, get_token_string_by_id(ID_XBEE_ID), (double)((long)callback_commissionning->i002->xd));
   cJSON_AddNumberToObject(_data, "LOCAL_XBEE_ADDR_H", (double)callback_commissionning->i002->local_xbee->l_addr_64_h);
   cJSON_AddNumberToObject(_data, "LOCAL_XBEE_ADDR_L", (double)callback_commissionning->i002->local_xbee->l_addr_64_l);
   if(plugin_params->parameters[PLUGIN_PARAMS_PARAMETERS].value.s) {
      cJSON_AddStringToObject(_data, get_token_string_by_id(INTERFACE_PARAMETERS_ID), plugin_params->parameters[PLUGIN_PARAMS_PARAMETERS].value.s);
   }
   cJSON_AddNumberToObject(_data, API_KEY_STR_C, callback_commissionning->i002->id_interface);

   //python_cmd_json(plugin_params->parameters[PLUGIN_PARAMS_PLUGIN].value.s, COMMISSIONNING, data);
   plugin_fireandforget_function_json(plugin_params->parameters[PLUGIN_PARAMS_PLUGIN].value.s, COMMISSIONNING, data);
   
   *(callback_commissionning->senttoplugin)=*(callback_commissionning->senttoplugin)+1;

   release_parsed_parameters(&plugin_params);
   plugin_params=NULL;
   
   VERBOSE(9) {
      mea_log_printf("%s (%s)  : commissionning request transmitted.\n", INFO_STR, __func__);
   }

   return NOERROR;
}


void *_thread_interface_type_002_xbeedata_cleanup(void *args)
{
   struct thread_params_s *params=(struct thread_params_s *)args;

   mea_log_printf("%s (%s)  : stop.\n", INFO_STR, __func__);
   if(!params)
      return NULL;
   
   if(params->e) {
      DBG_FREE(params->e->cmd);
      params->e->cmd=NULL;
      DBG_FREE(params->e);
      params->e=NULL;
   }

   if(params->plugin_params)
      release_parsed_parameters(&(params->plugin_params));
   
   if(params->queue && params->queue->nb_elem>0) // on vide s'il y a quelque chose avant de partir
      mea_queue_cleanup(params->queue, _iodata_free_queue_elem);
   
   if(params->queue) {
      DBG_FREE(params->queue);
      params->queue=NULL;
   }

   DBG_FREE(params);
   params=NULL;

   mea_log_printf("%s (%s)  : end.\n", INFO_STR, __func__);

   return NULL;
}


mea_error_t _thread_interface_type_002_xbeedata_devices(cJSON *jsonInterface, struct thread_params_s *params, data_queue_elem_t *e)
{
   cJSON *jsonDevices = cJSON_GetObjectItem(jsonInterface, DEVICES_STR_C);

   if(jsonDevices == NULL)
      return -1;
   cJSON *jsonDevice=jsonDevices->child;

   while(jsonDevice) {

      int err=0;

//      char *name=jsonDevice->string;
//      int id_sensor_actuator=(int)cJSON_GetObjectItem(jsonDevice, ID_SENSOR_ACTUATOR_STR_C)->valuedouble;
//      int id_type=(int)cJSON_GetObjectItem(jsonDevice, ID_TYPE_STR_C)->valuedouble;
      char *parameters=(char*)cJSON_GetObjectItem(jsonDevice, PARAMETERS_STR_C)->valuestring;
      char *dev=(char*)cJSON_GetObjectItem(jsonInterface, DEV_STR_C)->valuestring;
//      int todbflag=0;

      params->plugin_params=alloc_parsed_parameters(parameters, valid_xbee_plugin_params, &(params->nb_plugin_params), &err, 0);
      if(!params->plugin_params || !params->plugin_params->parameters[PLUGIN_PARAMS_PLUGIN].value.s) {
         if(params->plugin_params)
            release_parsed_parameters(&(params->plugin_params));
         continue; // si pas de paramètre (=> pas de plugin) ou pas de fonction ... pas la peine d'aller plus loin
      }
               
      uint16_t data_type = e->cmd[0]; // 0x90 serie, 0x92 iodata
      uint32_t addr_h, addr_l;

      struct device_info_s device_info;
      device_info_from_json(&device_info, jsonDevice, jsonInterface, NULL);

      cJSON *data = device_info_to_json_alloc(&device_info);
      if(sscanf((char *)dev, "MESH://%x-%x", &addr_h, &addr_l)==2) {
         cJSON_AddNumberToObject(data, get_token_string_by_id(ADDR_H_ID), (double)addr_h);
         cJSON_AddNumberToObject(data, get_token_string_by_id(ADDR_L_ID), (double)addr_l);
      }
      cJSON_AddItemToObject(data, "cmd", cJSON_CreateByteArray((char *)e->cmd, e->l_cmd));
      cJSON_AddNumberToObject(data, "l_cmd", e->l_cmd);
      cJSON_AddItemToObject(data, "cmd_data", cJSON_CreateByteArray((char *)&(e->cmd[12]), e->l_cmd-12));
      cJSON_AddNumberToObject(data, "l_cmd_data", e->l_cmd-12);
      cJSON_AddNumberToObject(data, "data_type", data_type);
      cJSON_AddNumberToObject(data, get_token_string_by_id(ID_XBEE_ID), (double)((long)params->xd));
      if(params->plugin_params->parameters[PLUGIN_PARAMS_PARAMETERS].value.s) {
         cJSON_AddStringToObject(data, get_token_string_by_id(DEVICE_PARAMETERS_ID), params->plugin_params->parameters[PLUGIN_PARAMS_PARAMETERS].value.s);
      }
      cJSON_AddNumberToObject(data, API_KEY_STR_C, device_info.interface_id);
      
//      python_cmd_json(params->plugin_params->parameters[PLUGIN_PARAMS_PLUGIN].value.s, XBEEDATA, data);
      plugin_fireandforget_function_json(params->plugin_params->parameters[PLUGIN_PARAMS_PLUGIN].value.s, XBEEDATA, data);
      
      params->i002->indicators.senttoplugin++;

      release_parsed_parameters(&(params->plugin_params));

      jsonDevice=jsonDevice->next;
   }

   return 0;
}


void *_thread_interface_type_002_xbeedata(void *args)
/**
 * \brief     Gestion des données asynchrones en provenances d'xbee
 * \details   Les iodata peuvent arriver n'importe quand, il sagit de pouvoir les traiter dés réceptions.
 *            Le callback associé est en charge de poster les données à traiter dans une file qui seront consommées par ce thread.
 * \param     args   ensemble des parametres nécessaires au thread regroupé dans une structure de données (voir struct thread_params_s)
 */
{
   struct thread_params_s *params=(struct thread_params_s *)args;

   pthread_cleanup_push( (void *)_thread_interface_type_002_xbeedata_cleanup, (void *)params );
   pthread_cleanup_push( (void *)set_interface_type_002_isnt_running, (void *)params->i002 );
   
   params->i002->thread_is_running=1;
   process_heartbeat(params->i002->monitoring_id);
   
   data_queue_elem_t *e;
   xbee_xd_t *xd=params->xd;
   int ret;
   
   params->plugin_params=NULL;
   params->nb_plugin_params=0;

   while(1) {
      process_heartbeat(params->i002->monitoring_id);
      process_update_indicator(params->i002->monitoring_id, interface_type_002_senttoplugin_str, params->i002->indicators.senttoplugin);
      process_update_indicator(params->i002->monitoring_id, interface_type_002_xplin_str, params->i002->indicators.xplin);
      process_update_indicator(params->i002->monitoring_id, interface_type_002_xbeedatain_str, params->i002->indicators.xbeedatain);
      process_update_indicator(params->i002->monitoring_id, interface_type_002_commissionning_request_str, params->i002->indicators.commissionning_request);

      if(xd->signal_flag==1) {
         goto _thread_interface_type_002_xbeedata_clean_exit;
      }

      pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)(&params->callback_lock) );
      pthread_mutex_lock(&params->callback_lock);
      
      if(params->queue->nb_elem==0) {
         struct timeval tv;
         struct timespec ts;
         gettimeofday(&tv, NULL);
         ts.tv_sec = tv.tv_sec + 10; // timeout de 10 secondes
         ts.tv_nsec = 0;
         
         ret=pthread_cond_timedwait(&params->callback_cond, &params->callback_lock, &ts);
         if(ret) {
            if(ret==ETIMEDOUT) {
               DEBUG_SECTION mea_log_printf("%s (%s) : Nb elements in queue after TIMEOUT : %ld)\n", DEBUG_STR, __func__, params->queue->nb_elem);
            }
            else {
               // autres erreurs à traiter
            }
         }
      }
      
      ret=mea_queue_out_elem(params->queue, (void **)&e);
      
      pthread_mutex_unlock(&params->callback_lock);
      pthread_cleanup_pop(0);
      
      if(!ret) {
         char devName[256];
         char addr[18];
         
         params->i002->indicators.xbeedatain++;
         
         sprintf(addr,
                 printf_mask_addr, // "%02x%02x%02x%02x-%02x%02x%02x%02x"
                 e->addr_64_h[0],
                 e->addr_64_h[1],
                 e->addr_64_h[2],
                 e->addr_64_h[3],
                 e->addr_64_l[0],
                 e->addr_64_l[1],
                 e->addr_64_l[2],
                 e->addr_64_l[3]);
         VERBOSE(9) mea_log_printf("%s  (%s) : data from = %s received\n",INFO_STR, __func__, addr);

         snprintf(devName, sizeof(devName)-1, "%s://%s", params->i002->name, addr);
         mea_strtolower(devName);

         cJSON *jsonInterface = getInterfaceByDevName_alloc(devName);

         if(jsonInterface) {
            _thread_interface_type_002_xbeedata_devices(jsonInterface, params, e);
            cJSON_Delete(jsonInterface);
         }
         DBG_FREE(e->cmd);
         e->cmd=NULL;
         DBG_FREE(e);
         e=NULL;
         
         pthread_testcancel();
      }
      else {
         DEBUG_SECTION mea_log_printf("%s (%s) : mea_queue_out_elem - no data in queue\n", DEBUG_STR, __func__);
      }
      pthread_testcancel();
   }

_thread_interface_type_002_xbeedata_clean_exit:
   pthread_cleanup_pop(1);
   pthread_cleanup_pop(1);

   process_async_stop(params->i002->monitoring_id);
   for(;;) sleep(1);
  
   return NULL;
}


// xd est déjà dans i002 pourquoi le passer en parametre ...
pthread_t *start_interface_type_002_xbeedata_thread(interface_type_002_t *i002, xbee_xd_t *xd, thread_f function)
/**  
 * \brief     Demarrage du thread de gestion des données (non solicitées) en provenance des xbee
 * \param     i002           descripteur de l'interface
 * \param     xd             descripteur de com. avec l'xbee
 * \param     db             descripteur ouvert de la base de paramétrage  
 * \param     md             descripteur ouvert de la base d'historique
 * \param     function       function principale du thread à démarrer 
 * \return    ERROR ou NOERROR
 **/ 
{
   pthread_t *thread=NULL;
   struct thread_params_s *params=NULL;
   struct callback_data_s *callback_xbeedata=NULL;
   
   params=malloc(sizeof(struct thread_params_s));
   if(!params) {
      VERBOSE(2) {
         char err_str[256];
         strerror_r(errno, err_str, sizeof(err_str)-1);
         mea_log_printf("%s (%s) : %s - %s\n", ERROR_STR, __func__, MALLOC_ERROR_STR,err_str);
      }
      goto clean_exit;
   }
   params->queue=(mea_queue_t *)malloc(sizeof(mea_queue_t));
   if(!params->queue) {
      VERBOSE(2) {
         char err_str[256];
         strerror_r(errno, err_str, sizeof(err_str)-1);
         mea_log_printf("%s (%s) : %s - %s\n", ERROR_STR, __func__, MALLOC_ERROR_STR,err_str);
      }
      goto clean_exit;
   }
   mea_queue_init(params->queue);

   params->xd=xd;
//   params->param_db=db;
   pthread_mutex_init(&params->callback_lock, NULL);
   pthread_cond_init(&params->callback_cond, NULL);
   params->i002=(void *)i002;

   // préparation des données pour les callback io_data et data_flow dont les données sont traitées par le même thread
   callback_xbeedata=(struct callback_data_s *)malloc(sizeof(struct callback_data_s));
   if(!callback_xbeedata) {
      VERBOSE(2) mea_log_printf("%s (%s) : %s\n", ERROR_STR, __func__, MALLOC_ERROR_STR);
      goto clean_exit;
   }
   callback_xbeedata->xd=xd;
//   callback_xbeedata->param_db=db;
   callback_xbeedata->callback_lock=&params->callback_lock;
   callback_xbeedata->callback_cond=&params->callback_cond;
   callback_xbeedata->queue=params->queue;
   
   xbee_set_iodata_callback2(xd, _inteface_type_002_xbeedata_callback, (void *)callback_xbeedata);
   xbee_set_dataflow_callback2(xd, _inteface_type_002_xbeedata_callback, (void *)callback_xbeedata);

   thread=(pthread_t *)malloc(sizeof(pthread_t));
   if(!thread) {
      VERBOSE(2) mea_log_printf("%s (%s) : %s\n", ERROR_STR, __func__, MALLOC_ERROR_STR);
      goto clean_exit;
   }
   
   if(pthread_create (thread, NULL, (void *)function, (void *)params))
      goto clean_exit;
   fprintf(stderr,"INTERFACE_TYPE_002 : %x\n", (unsigned int)*thread);
   pthread_detach(*thread);

   return thread;
   
clean_exit:
   if(thread) {
      DBG_FREE(thread);
      thread=NULL;
   }
   
   if(callback_xbeedata) {
      DBG_FREE(callback_xbeedata);
      callback_xbeedata=NULL;
   }

   if(params && params->queue && params->queue->nb_elem>0) // on vide s'il y a quelque chose avant de partir
      mea_queue_cleanup(params->queue, _iodata_free_queue_elem);

   if(params) {
      if(params->queue) {
         DBG_FREE(params->queue);
         params->queue=NULL;
      }
      DBG_FREE(params);
      params=NULL;
   }
   return NULL;
}


xpl2_f get_xPLCallback_interface_type_002(void *ixxx)
{
   interface_type_002_t *i002 = (interface_type_002_t *)ixxx;

   if(i002 == NULL)
      return NULL;
   else
      return i002->xPL_callback2;
}


int get_monitoring_id_interface_type_002(void *ixxx)
{
   interface_type_002_t *i002 = (interface_type_002_t *)ixxx;

   if(i002 == NULL)
      return -1;
   else
      return i002->monitoring_id;
}


int set_xPLCallback_interface_type_002(void *ixxx, xpl2_f cb)
{
   interface_type_002_t *i002 = (interface_type_002_t *)ixxx;

   if(i002 == NULL)
      return -1;
   else {
      i002->xPL_callback2 = cb;
      return 0;
   }
}


int set_monitoring_id_interface_type_002(void *ixxx, int id)
{
   interface_type_002_t *i002 = (interface_type_002_t *)ixxx;

   if(i002 == NULL)
      return -1;
   else {
      i002->monitoring_id = id;
      return 0;
   }
}


int get_type_interface_type_002()
{
   return INTERFACE_TYPE_002;
}


interface_type_002_t *malloc_and_init2_interface_type_002(int id_driver, cJSON *jsonInterface)
{
   interface_type_002_t *i002;
                  
   i002=(interface_type_002_t *)malloc(sizeof(interface_type_002_t));
   if(!i002) {
      VERBOSE(2) {
         char err_str[256];
         strerror_r(errno, err_str, sizeof(err_str)-1);
         mea_log_printf("%s (%s) : %s - %s\n",ERROR_STR,__func__,MALLOC_ERROR_STR,err_str);
      }
      return NULL;
   }
   i002->thread_is_running=0;
                  
   struct interface_type_002_data_s *i002_start_stop_params=(struct interface_type_002_data_s *)malloc(sizeof(struct interface_type_002_data_s));
   if(!i002_start_stop_params) {
      DBG_FREE(i002);
      i002=NULL;
      VERBOSE(2) {
         char err_str[256];
         strerror_r(errno, err_str, sizeof(err_str)-1);
         mea_log_printf("%s (%s) : %s - %s\n",ERROR_STR,__func__,MALLOC_ERROR_STR,err_str);
      }  
      return NULL;
   }

   char *name=jsonInterface->string;
   int id_interface=(int)cJSON_GetObjectItem(jsonInterface,ID_INTERFACE_STR_C)->valuedouble;
   char *dev=cJSON_GetObjectItem(jsonInterface,DEV_STR_C)->valuestring;
   char *parameters=cJSON_GetObjectItem(jsonInterface,PARAMETERS_STR_C)->valuestring;
   char *description=cJSON_GetObjectItem(jsonInterface,DESCRIPTION_STR_C)->valuestring;

   strncpy(i002->dev, (char *)dev, sizeof(i002->dev)-1);
   strncpy(i002->name, (char *)name, sizeof(i002->name)-1);
   i002->id_interface=id_interface;
   int l_parameters=(int)strlen((char *)parameters)+1;
   i002->parameters=(char *)malloc(l_parameters);
   i002->parameters[l_parameters-1]=0;
   strncpy(i002->parameters, (char *)parameters, l_parameters-1);

   i002->indicators.senttoplugin=0;
   i002->indicators.xplin=0;
   i002->indicators.xbeedatain=0;
   i002->indicators.commissionning_request=0;

   // initialisation du process
   i002_start_stop_params->i002=i002;
   i002->monitoring_id=process_register((char *)name);
   
   i002->xd=NULL;
   i002->local_xbee=NULL;
   i002->thread=NULL;
   i002->xPL_callback2=NULL;
   i002->xPL_callback_data=NULL;

   process_set_group(i002->monitoring_id, 1);
   process_set_start_stop(i002->monitoring_id, start_interface_type_002, stop_interface_type_002, (void *)i002_start_stop_params, 1);
   process_set_watchdog_recovery(i002->monitoring_id, restart_interface_type_002, (void *)i002_start_stop_params);
   process_set_description(i002->monitoring_id, (char *)description);
   process_set_heartbeat_interval(i002->monitoring_id, 60); // chien de garde au bout de 60 secondes sans heartbeat

   process_add_indicator(i002->monitoring_id, interface_type_002_senttoplugin_str, 0);
   process_add_indicator(i002->monitoring_id, interface_type_002_xplin_str, 0);
   process_add_indicator(i002->monitoring_id, interface_type_002_xbeedatain_str, 0);
   process_add_indicator(i002->monitoring_id, interface_type_002_commissionning_request_str, 0);

   return i002;
}               


static uint32_t _indianConvertion(uint32_t val_x86)
{
   uint32_t val_xbee;
   char *val_x86_ptr;
   char *val_xbee_ptr;
   
   val_x86_ptr = (char *)&val_x86;
   val_xbee_ptr = (char *)&val_xbee;
   
   // conversion little vers big indian
   for(int16_t i=0,j=3;i<sizeof(uint32_t);i++)
      val_xbee_ptr[i]=val_x86_ptr[j-i];

   return val_xbee;
}


int mea_sendAtCmdAndWaitResp_json(interface_type_002_t *i002, cJSON *args, cJSON **res, int16_t *_nerr, char *_err, int l_err)
//cJSON *mea_sendAtCmdAndWaitResp_json(PyObject *self, PyObject *args)
{
   unsigned char at_cmd[256];
   uint16_t l_at_cmd;
   unsigned char resp[256];
   uint16_t l_resp;
   int16_t ret;
   cJSON *arg;
   xbee_host_t *host=NULL;
   xbee_xd_t *xd=i002->xd;

   *_nerr=255;

   // récupération des paramètres et contrôle des types
   if(args->type!=cJSON_Array || cJSON_GetArraySize(args)!=6) {
      return -255;
   }
 
   uint32_t addr_h;
   arg=cJSON_GetArrayItem(args, 2);
   if(arg->type==cJSON_Number) {
      addr_h=(uint32_t)arg->valuedouble;
   }
   else {
      return -255;
   }

   uint32_t addr_l;
   arg=cJSON_GetArrayItem(args, 3);
   if(arg->type==cJSON_Number) {
      addr_l=(uint32_t)arg->valuedouble;
   }
   else {
      return -255;
   }

   char *at;
   arg=cJSON_GetArrayItem(args, 4);
   if(arg->type==cJSON_String || arg->type==cJSON_ByteArray) {
      at=arg->valuestring;
      if(arg->type==cJSON_ByteArray) {
         at[arg->valueint]=0;
      }
   }
   else {
      return -255;
   }
   at_cmd[0]=at[0];
   at_cmd[1]=at[1];
   
   arg=cJSON_GetArrayItem(args, 5);
   if(arg->type==cJSON_Number) {
      uint32_t val=(uint32_t)arg->valuedouble;
      uint32_t val_xbee=_indianConvertion(val);
      char *val_xbee_ptr=(char *)&val_xbee;
      
      for(int16_t i=0;i<sizeof(uint32_t);i++)
         at_cmd[2+i]=val_xbee_ptr[i];
      l_at_cmd=6;
   }
   else if(arg->type==cJSON_String || arg->type==cJSON_ByteArray) {
      int l=0;
      char *at_arg=(char *)arg->valuestring;
      if(arg->type==cJSON_String) {
         l=(int)strlen(at_arg);
      }
      else {
         l=arg->valueint;
      }
      uint16_t i;
      for(i=0;i<l;i++)
         at_cmd[2+i]=at_arg[i];
      if(i>0)
         l_at_cmd=2+i;
      else
         l_at_cmd=2;
   }
   else {
      return -255;
   }
   
   // recuperer le host dans la table (necessite d'avoir accès à xd
   int16_t err;
   
   host=NULL;
   host=(xbee_host_t *)malloc(sizeof(xbee_host_t)); // description de l'xbee directement connecté
   xbee_get_host_by_addr_64(xd, host, addr_h, addr_l, &err);
   if(err==XBEE_ERR_NOERR) {
   }
   else {
      DEBUG_SECTION mea_log_printf("%s (%s) : host not found.\n", DEBUG_STR ,__func__);
      if(host) {
         free(host);
         host=NULL;
      }
      return -255;
   }

   ret=xbee_atCmdSendAndWaitResp(xd, host, at_cmd, l_at_cmd, resp, &l_resp, &err);
   if(ret==-1) {
      DEBUG_SECTION mea_log_printf("%s (%s) : error %d.\n", DEBUG_STR, __func__, err);
      if(host) {
         free(host);
         host=NULL;
      }
      return -255;
   }
   
   struct xbee_remote_cmd_response_s *mapped_resp=(struct xbee_remote_cmd_response_s *)resp;
   
   cJSON *t=cJSON_CreateArray();
   cJSON_AddItemToArray(t, cJSON_CreateNumber((double)mapped_resp->cmd_status));
   cJSON_AddItemToArray(t, cJSON_CreateByteArray((char *)&resp, l_resp));
   cJSON_AddItemToArray(t, cJSON_CreateNumber((double)l_resp));

   free(host);
   host=NULL;

   *res=t;
   
   return 0; // return True
}


static int mea_sendAtCmd_json(interface_type_002_t *i002, cJSON *args)
{
   unsigned char at_cmd[256];
   uint16_t l_at_cmd;
   xbee_xd_t *xd=i002->xd;
   cJSON *arg;
   
   xbee_host_t *host=NULL;

   // récupération des paramètres et contrôle des types
   if(args->type!=cJSON_Array || cJSON_GetArraySize(args)!=3) {
      return -255;
   }

   uint32_t addr_h;
   arg=cJSON_GetArrayItem(args, 2);
   if(arg->type==cJSON_Number) {
      addr_h=(uint32_t)arg->valuedouble;
   }
   else {
      return -255;
   }

   uint32_t addr_l;
   arg=cJSON_GetArrayItem(args, 3);
   if(arg->type==cJSON_Number) {
      addr_l=(uint32_t)arg->valuedouble;
   }
   else {
      return -255;
   }

   char *at;
   arg=cJSON_GetArrayItem(args, 4);
   if(arg->type==cJSON_String || arg->type==cJSON_ByteArray) {
      at=arg->valuestring;
      if(arg->type==cJSON_ByteArray) {
         at[arg->valueint]=0;
      }
   }
   else {
      return -255;
   }
   at_cmd[0]=at[0];
   at_cmd[1]=at[1];

   arg=cJSON_GetArrayItem(args, 5);
   if(arg->type==cJSON_Number) {
      uint32_t val=(uint32_t)arg->valuedouble;
      uint32_t val_xbee=_indianConvertion(val);
      char *val_xbee_ptr=(char *)&val_xbee;
      
      for(int16_t i=0;i<sizeof(uint32_t);i++)
         at_cmd[2+i]=val_xbee_ptr[i];
      l_at_cmd=6;
   }
   else if(arg->type==cJSON_String || arg->type==cJSON_ByteArray) {
      int l=0;
      char *at_arg=(char *)arg->valuestring;
      if(arg->type==cJSON_String) {
         l=(int)strlen(at_arg);
      }
      else {
         l=arg->valueint;
      }
      uint16_t i;
      for(i=0;i<l;i++)
         at_cmd[2+i]=at_arg[i];
      if(i>0)
         l_at_cmd=2+i;
      else
         l_at_cmd=2;
   }
   else {
      return -255;
   }

   int16_t err;
   
   if(addr_l != 0xFFFFFFFF && addr_h != 0xFFFFFFFF) {
      host=(xbee_host_t *)malloc(sizeof(xbee_host_t)); // description de l'xbee directement connecté
      xbee_get_host_by_addr_64(xd, host, addr_h, addr_l, &err);
      if(err!=XBEE_ERR_NOERR) {
         DEBUG_SECTION mea_log_printf("%s (%s) : host not found.\n", DEBUG_STR,__func__);
         return -254;
      }
   }
   else
      host=NULL;
   
   int16_t nerr;
   // ret=
   xbee_atCmdSend(xd, host, at_cmd, l_at_cmd, &nerr);
   
   if(host) {
      free(host);
      host=NULL;
   }
   return 0;
}


int16_t api_interface_type_002_json(void *ixxx, char *cmnd, void *args, int nb_args, void **res, int16_t *nerr, char *err, int l_err)
{
   interface_type_002_t *i002 = (interface_type_002_t *)ixxx;

   cJSON *jsonArgs = (cJSON *)args;
   cJSON **jsonRes = (cJSON **)res;
   
   if(strcmp(cmnd, "sendXbeeCmdAndWaitResp") == 0) {
      int ret=0;
      ret=mea_sendAtCmdAndWaitResp_json((void *)i002, jsonArgs, jsonRes, nerr, err, l_err);
      if(ret<0) {
         strncpy(err, "error", l_err);
         return -1;
      }
      else {
         strncpy(err, "no error", l_err);
         *nerr=0;
         return 0;
      }
   }
   else if(strcmp(cmnd, "sendXbeeCmd") == 0) {
      int ret=0;
      ret=mea_sendAtCmd_json((void *)i002, jsonArgs);
      if(ret<0) {
         strncpy(err, "error", l_err);
         return -1;
      }
      else {
         strncpy(err, "no error", l_err);
         *nerr=0;
         return 0;
      }
   }
   else {
      strncpy(err, "unknown function", l_err);

      return -254;
   }
}


int update_devices_type_002(void *ixxx)
{
   printf("update devices 002\n");

   return 0;
}


int clean_interface_type_002(interface_type_002_t *i002)
{
   if(i002->parameters) {
      DBG_FREE(i002->parameters);
      i002->parameters=NULL;
   }
   
   if(i002->xPL_callback_data) {
      DBG_FREE(i002->xPL_callback_data);
      i002->xPL_callback_data=NULL;
   }
   
   if(i002->xPL_callback2)
      i002->xPL_callback2=NULL;
   
   if(i002->xd && i002->xd->dataflow_callback_data &&
     (i002->xd->dataflow_callback_data == i002->xd->io_callback_data)) {
      DBG_FREE(i002->xd->dataflow_callback_data);
      i002->xd->io_callback_data=NULL;
      i002->xd->dataflow_callback_data=NULL;
   }
   else {
      if(i002->xd && i002->xd->dataflow_callback_data) {
         DBG_FREE(i002->xd->dataflow_callback_data);
         i002->xd->dataflow_callback_data=NULL;
      }
      if(i002->xd && i002->xd->io_callback_data) {
         DBG_FREE(i002->xd->io_callback_data);
         i002->xd->io_callback_data=NULL;
      }
   }

   if(i002->thread) {
      DBG_FREE(i002->thread);
      i002->thread=NULL;
   }

   if(i002->xd && i002->xd->commissionning_callback_data) {
      DBG_FREE(i002->xd->commissionning_callback_data);
      i002->xd->commissionning_callback_data=NULL;
   }

   if(i002->xd) {
      DBG_FREE(i002->xd);
      i002->xd=NULL;
   }
   
   if(i002->local_xbee) {
      DBG_FREE(i002->local_xbee);
      i002->local_xbee=NULL;
   }
   
   return 0;
}


int stop_interface_type_002(int my_id, void *data, char *errmsg, int l_errmsg)
/**
 * \brief     arrêt d'une interface de type 2
 * \details   RAZ de tous les structures de données et libération des mémoires allouées
 * \param     i002           descripteur de l'interface  
 * \return    ERROR ou NOERROR
 **/ 
{
   if(!data)
      return -1;

   struct interface_type_002_data_s *start_stop_params=(struct interface_type_002_data_s *)data;

   VERBOSE(1) mea_log_printf("%s  (%s) : %s shutdown thread ... ", INFO_STR, __func__, start_stop_params->i002->name);

   if(start_stop_params->i002->xPL_callback_data) {
      DBG_FREE(start_stop_params->i002->xPL_callback_data);
      start_stop_params->i002->xPL_callback_data=NULL;
   }
   
   if(start_stop_params->i002->xPL_callback2) {
      start_stop_params->i002->xPL_callback2=NULL;
   }

   if(start_stop_params->i002->xd &&
      start_stop_params->i002->xd->dataflow_callback_data &&
     (start_stop_params->i002->xd->dataflow_callback_data == start_stop_params->i002->xd->io_callback_data)) {
      DBG_FREE(start_stop_params->i002->xd->dataflow_callback_data);
      start_stop_params->i002->xd->io_callback_data=NULL;
      start_stop_params->i002->xd->dataflow_callback_data=NULL;
   }
   else {
      if(start_stop_params->i002->xd && start_stop_params->i002->xd->dataflow_callback_data) {
         DBG_FREE(start_stop_params->i002->xd->dataflow_callback_data);
         start_stop_params->i002->xd->dataflow_callback_data=NULL;
      }
      if(start_stop_params->i002->xd && start_stop_params->i002->xd->io_callback_data) {
         DBG_FREE(start_stop_params->i002->xd->io_callback_data);
         start_stop_params->i002->xd->io_callback_data=NULL;
      }
   }

   if(start_stop_params->i002->thread) {
      pthread_cancel(*(start_stop_params->i002->thread));

      int counter=100;
      while(counter--) {
         if(start_stop_params->i002->thread_is_running) {  // pour éviter une attente "trop" active
            usleep(100); // will sleep for 10 ms
         }
         else {
            break;
         }
      }
      DEBUG_SECTION mea_log_printf("%s (%s) : %s, fin après %d itération(s)\n",DEBUG_STR, __func__,start_stop_params->i002->name,100-counter);

      DBG_FREE(start_stop_params->i002->thread);
      start_stop_params->i002->thread=NULL;
   }

   xbee_remove_commissionning_callback(start_stop_params->i002->xd);

   if(start_stop_params->i002->xd && start_stop_params->i002->xd->commissionning_callback_data) {
      DBG_FREE(start_stop_params->i002->xd->commissionning_callback_data);
      start_stop_params->i002->xd->commissionning_callback_data=NULL;
   }

   xbee_close(start_stop_params->i002->xd);

   if(start_stop_params->i002->xd) {
      DBG_FREE(start_stop_params->i002->xd);
      start_stop_params->i002->xd=NULL;
   }

   if(start_stop_params->i002->local_xbee) {
      DBG_FREE(start_stop_params->i002->local_xbee);
      start_stop_params->i002->local_xbee=NULL;
   }

   return 0;
}


int restart_interface_type_002(int my_id, void *data, char *errmsg, int l_errmsg)
{
   process_stop(my_id, NULL, 0);
   sleep(5);
   return process_start(my_id, NULL, 0);
}


int16_t check_status_interface_type_002(interface_type_002_t *i002)
/**  
 * \brief     indique si une anomalie a généré l'emission d'un signal SIGHUP
 * \param     i002           descripteur de l'interface  
 * \return    ERROR signal émis ou NOERROR sinon
 **/ 
{
   if(i002->xd->signal_flag!=0)
      return -1;
   return 0;
}


int start_interface_type_002(int my_id, void *data, char *errmsg, int l_errmsg)
/**
 * \brief     Demarrage d'une interface de type 2
 * \details   ouverture de la communication avec l'xbee point d'entrée MESH, démarrage du thread de gestion des données iodata et xbeedata, mise en place des callback xpl et commissionnement
 * \return    ERROR ou NOERROR
 **/ 
{
   char dev[256];
   char buff[256];
   speed_t speed;

   int fd=-1;
   int16_t nerr;
   int err;
   int ret;
  
   xbee_xd_t *xd=NULL;
   xbee_host_t *local_xbee=NULL;
   
   struct callback_commissionning_data_s *commissionning_callback_params=NULL;
   struct callback_xpl_data_s *xpl_callback_params=NULL;
   
   struct interface_type_002_data_s *start_stop_params=(struct interface_type_002_data_s *)data;

   start_stop_params->i002->thread=NULL;

   int interface_nb_parameters=0;
   parsed_parameters_t *interface_parameters=NULL;
   
   char err_str[256];
   
   ret=get_dev_and_speed((char *)start_stop_params->i002->dev, buff, sizeof(buff), &speed);
   if(!ret) {
      int n=snprintf(dev,sizeof(buff)-1,"/dev/%s",buff);
      if(n<0 || n==(sizeof(buff)-1)) {
#ifdef _POSIX_SOURCE
         char *ret;
#else
         int ret;
#endif
         ret=strerror_r(errno, err_str, sizeof(err_str));
         VERBOSE(2) {
            mea_log_printf("%s (%s) : snprintf - %s\n", ERROR_STR, __func__, err_str);
         }
         goto clean_exit;
      }
   }
   else {
      VERBOSE(2) mea_log_printf("%s (%s) : incorrect device/speed interface - %s\n", ERROR_STR,__func__,start_stop_params->i002->dev);
      goto clean_exit;
   }

   xd=(xbee_xd_t *)malloc(sizeof(xbee_xd_t));
   if(!xd) {
#ifdef _POSIX_SOURCE
      char *ret;
#else
      int ret;
#endif
      ret=strerror_r(errno, err_str, sizeof(err_str));
      VERBOSE(2) {
         mea_log_printf("%s (%s) : %s - %s\n", ERROR_STR, __func__, MALLOC_ERROR_STR, err_str);
      }
      goto clean_exit;
   }
   
   fd=xbee_init(xd, dev, (int)speed);
   if (fd == -1) {
#ifdef _POSIX_SOURCE
      char *ret;
#else
      int ret;
#endif
      ret=strerror_r(errno, err_str, sizeof(err_str));
      VERBOSE(2) {
         mea_log_printf("%s (%s) : init_xbee - Unable to open serial port (%s).\n", ERROR_STR, __func__, dev);
      }
      goto clean_exit;
   }
   start_stop_params->i002->xd=xd;
   
   /*
    * Préparation du réseau XBEE
    */
   local_xbee=(xbee_host_t *)malloc(sizeof(xbee_host_t)); // description de l'xbee directement connecté
   if(!local_xbee) {
#ifdef _POSIX_SOURCE
      char *ret;
#else
      int ret;
#endif
      ret=strerror_r(errno, err_str, sizeof(err_str));
      VERBOSE(2) {
         mea_log_printf("%s (%s) : %s - %s.\n", ERROR_STR, __func__, MALLOC_ERROR_STR, err_str);
      }
      goto clean_exit;
   }
   
   // récupération de l'adresse de l'xbee connecter au PC (pas forcement le coordinateur).
//   uint32_t addr_64_h;
//   uint32_t addr_64_l;
      
   ret=get_local_xbee_addr(xd, local_xbee);
   if(ret==-1)
      goto clean_exit;

   start_stop_params->i002->local_xbee=local_xbee;
 
   /*
    * exécution du plugin de l'interface
    */
   interface_parameters=alloc_parsed_parameters(start_stop_params->i002->parameters, valid_xbee_plugin_params, &interface_nb_parameters, &err, 0);
   if(!interface_parameters || !interface_parameters->parameters[PLUGIN_PARAMS_PLUGIN].value.s) {
      if(interface_parameters) {
         release_parsed_parameters(&interface_parameters);
         
         VERBOSE(9) mea_log_printf("%s (%s) : invalid python plugin parameters (%s)\n", INFO_STR, __func__, start_stop_params->i002->parameters);
      }
      else {
         VERBOSE(9) mea_log_printf("%s  (%s) : no python plugin specified\n", INFO_STR, __func__);
      }
   }
   else {
      cJSON *data=cJSON_CreateObject();
      cJSON_AddNumberToObject(data, get_token_string_by_id(ID_XBEE_ID), (double)((long)xd));
      cJSON_AddNumberToObject(data, get_token_string_by_id(INTERFACE_ID_ID), start_stop_params->i002->id_interface);
      cJSON_AddNumberToObject(data, API_KEY_STR_C, (double)start_stop_params->i002->id_interface);
      if(interface_parameters->parameters[PLUGIN_PARAMS_PARAMETERS].value.s) {
         cJSON_AddStringToObject(data, get_token_string_by_id(INTERFACE_PARAMETERS_ID), interface_parameters->parameters[PLUGIN_PARAMS_PARAMETERS].value.s);
      }
      cJSON_AddNumberToObject(data, API_KEY_STR_C, start_stop_params->i002->id_interface);
      
      //python_call_function_json_alloc(interface_parameters->parameters[PLUGIN_PARAMS_PLUGIN].value.s, "mea_init", data);
      plugin_call_function_json_alloc(interface_parameters->parameters[PLUGIN_PARAMS_PLUGIN].value.s, "mea_init", data);

      if(interface_parameters)
         release_parsed_parameters(&interface_parameters);
      interface_nb_parameters=0;
   }

   /*
    * parametrage du réseau
    */
   // déouverte des xbee du réseau
   xbee_start_network_discovery(xd, &nerr); // lancement de la découverte "asynchrone"
      
   /*
    * Gestion des sous-interfaces
    */
   start_stop_params->i002->thread=start_interface_type_002_xbeedata_thread(start_stop_params->i002, xd, (thread_f)_thread_interface_type_002_xbeedata);

   //
   // gestion du commissionnement : mettre une zone donnees specifique au commissionnement
   //
   commissionning_callback_params=(struct callback_commissionning_data_s *)malloc(sizeof(struct callback_commissionning_data_s));
   if(!commissionning_callback_params) {
#ifdef _POSIX_SOURCE
      char *ret;
#else
      int ret;
#endif
      ret=strerror_r(errno, err_str, sizeof(err_str));
      VERBOSE(2) {
         mea_log_printf("%s (%s) : %s - %s", ERROR_STR, __func__, MALLOC_ERROR_STR, err_str);
      }
      goto clean_exit;
   }
   commissionning_callback_params->i002=start_stop_params->i002;
   commissionning_callback_params->senttoplugin=&(start_stop_params->i002->indicators.senttoplugin); // indicateur requete vers plugin
   commissionning_callback_params->commissionning_request=&(start_stop_params->i002->indicators.commissionning_request); // indicateur nb demande commissionnement
   xbee_set_commissionning_callback2(xd, _interface_type_002_commissionning_callback, (void *)commissionning_callback_params);
   
   //
   // gestion des demandes xpl : ajouter une zone de donnees specifique au callback xpl (pas simplement passe i002).
   //
   xpl_callback_params=(struct callback_xpl_data_s *)malloc(sizeof(struct callback_xpl_data_s));
   if(!xpl_callback_params) {
#ifdef _POSIX_SOURCE
      char *ret;
#else
      int ret;
#endif
      ret=strerror_r(errno, err_str, sizeof(err_str));
      VERBOSE(2) {
         mea_log_printf("%s (%s) : %s - %s\n", ERROR_STR, __func__, MALLOC_ERROR_STR, err_str);
      }
      goto clean_exit;
   }

   start_stop_params->i002->xPL_callback_data=xpl_callback_params;
   start_stop_params->i002->xPL_callback2=_interface_type_002_xPL_callback2;
   
   VERBOSE(2) mea_log_printf("%s  (%s) : %s %s.\n", INFO_STR, __func__,start_stop_params->i002->name, launched_successfully_str);
   
   return 0;
   
clean_exit:
   if(xd) {
      xbee_remove_commissionning_callback(xd);
      xbee_remove_iodata_callback(xd);
      xbee_remove_dataflow_callback(xd);
   }
   
   if(start_stop_params->i002->thread) {
      stop_interface_type_002(start_stop_params->i002->monitoring_id, start_stop_params, NULL, 0);
   }

   
   if(interface_parameters) {
      release_parsed_parameters(&interface_parameters);
      interface_nb_parameters=0;

   }
   
   if(commissionning_callback_params) {
      DBG_FREE(commissionning_callback_params);
      commissionning_callback_params=NULL;
   }
   
   if(xpl_callback_params) {
      DBG_FREE(xpl_callback_params);
      xpl_callback_params=NULL;
   }
   
   if(local_xbee) {
      DBG_FREE(local_xbee);
      local_xbee=NULL;
   }
   
   if(xd) {
      if(fd>=0)
         xbee_close(xd);
      xbee_clean_xd(xd);
      DBG_FREE(xd);
      xd=NULL;
   }
   
   return -1;
}

#ifndef ASPLUGIN
int get_fns_interface_type_002(struct interfacesServer_interfaceFns_s *interfacesFns)
{
   interfacesFns->malloc_and_init2 = (malloc_and_init2_f)&malloc_and_init2_interface_type_002;
   interfacesFns->get_monitoring_id = (get_monitoring_id_f)&get_monitoring_id_interface_type_002;
   interfacesFns->get_xPLCallback = (get_xPLCallback_f)&get_xPLCallback_interface_type_002;
   interfacesFns->update_devices = (update_devices_f)&update_devices_type_002;
   interfacesFns->clean = (clean_f)&clean_interface_type_002;
   interfacesFns->set_monitoring_id = (set_monitoring_id_f)&set_monitoring_id_interface_type_002;
   interfacesFns->set_xPLCallback = (set_xPLCallback_f)&set_xPLCallback_interface_type_002;
   interfacesFns->get_type = (get_type_f)&get_type_interface_type_002;
   interfacesFns->api = (api_f)&api_interface_type_002_json;
   interfacesFns->pairing = NULL;
   interfacesFns->lib = NULL;
   interfacesFns->type = interfacesFns->get_type();
   interfacesFns->plugin_flag = 0;

   return 0;
}
#endif
