/*
 *
 *  Created by Patrice Dietsch on 24/08/12.
 *  Copyright 2012 -. All rights reserved.
 *
 */
#include <Python.h>

#include "interface_type_002.h"

#include <stdio.h>
#include <stdlib.h>k
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
#include "pythonPluginServer.h"
#include "python_utils.h"

#include "processManager.h"

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
#define XBEE_PLUGIN_PARAMS_PLUGIN      0
#define XBEE_PLUGIN_PARAMS_PARAMETERS  1


char *printf_mask_addr="%02x%02x%02x%02x-%02x%02x%02x%02x";


struct callback_data_s // donnee "userdata" pour les callbacks
{
   PyThreadState   *mainThreadState;
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
   PyThreadState  *mainThreadState;
   PyThreadState  *myThreadState;
};


struct thread_params_s
{
   xbee_xd_t            *xd;
   mea_queue_t          *queue;
   pthread_mutex_t       callback_lock;
   pthread_cond_t        callback_cond;
   PyThreadState        *mainThreadState;
   PyThreadState        *myThreadState;
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


PyObject *json_to_pydict_interface(cJSON *jsonInterface)
{
   PyObject *data_dict;
   
   data_dict=PyDict_New();
   if(!data_dict)
      return NULL;
  
   mea_addLong_to_pydict(data_dict, get_token_string_by_id(INTERFACE_ID_ID), (long)cJSON_GetObjectItem(jsonInterface, "id_interface")->valuedouble);
   mea_addLong_to_pydict(data_dict, get_token_string_by_id(INTERFACE_TYPE_ID_ID), (long)cJSON_GetObjectItem(jsonInterface, "id_type")->valuedouble);
   mea_addString_to_pydict(data_dict, get_token_string_by_id(INTERFACE_NAME_ID), (char *)jsonInterface->string);
   mea_addLong_to_pydict(data_dict, get_token_string_by_id(INTERFACE_STATE_ID), (long)cJSON_GetObjectItem(jsonInterface, "state")->valuedouble);

   uint32_t addr_h, addr_l;
   
   if(sscanf((char *)cJSON_GetObjectItem(jsonInterface, "dev")->valuestring, "MESH://%x-%x", &addr_h, &addr_l)==2) {
      mea_addLong_to_pydict(data_dict, get_token_string_by_id(ADDR_H_ID), (long)addr_h);
      mea_addLong_to_pydict(data_dict, get_token_string_by_id(ADDR_L_ID), (long)addr_l);
   }
   
   return data_dict;
}


int16_t _interface_type_002_xPL_callback2(cJSON *xplMsgJson, struct device_info_s *device_info, void *userValue)
{
   char *device;
   int ret;
   int err;
   
   interface_type_002_t *interface=(interface_type_002_t *)userValue;
   struct callback_xpl_data_s *params=(struct callback_xpl_data_s *)interface->xPL_callback_data;
   
   interface->indicators.xplin++;
   
   PyEval_AcquireLock();
   if(!params->mainThreadState)
      params->mainThreadState=PyThreadState_Get();
   if(!params->myThreadState)
      params->myThreadState = PyThreadState_New(params->mainThreadState->interp);
   PyEval_ReleaseLock();
   
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
   if(!plugin_params || !plugin_params->parameters[XBEE_PLUGIN_PARAMS_PLUGIN].value.s) {
      if(plugin_params)
          release_parsed_parameters(&plugin_params);
      return ERROR; // si pas de parametre (=> pas de plugin) ou pas de fonction ... pas la peine d'aller plus loin pour ce capteur
   }

   plugin_queue_elem_t *plugin_elem = (plugin_queue_elem_t *)malloc(sizeof(plugin_queue_elem_t));
   if(plugin_elem) {
      plugin_elem->type_elem=XPLMSG;
      { // appel des fonctions Python
         PyEval_AcquireLock();
         PyThreadState *tempState = PyThreadState_Swap(params->myThreadState);
               
         plugin_elem->aDict=mea_device_info_to_pydict_device(device_info);

         mea_addLong_to_pydict(plugin_elem->aDict, get_token_string_by_id(ID_XBEE_ID), (long)interface->xd);
         PyObject *d=mea_xplMsgToPyDict2(xplMsgJson);
         PyDict_SetItemString(plugin_elem->aDict, XPLMSG_STR_C, d);
         mea_addLong_to_pydict(plugin_elem->aDict, get_token_string_by_id(DEVICE_TYPE_ID_ID), device_info->type_id);
         Py_DECREF(d);
               
         if(plugin_params->parameters[XBEE_PLUGIN_PARAMS_PARAMETERS].value.s)
         mea_addString_to_pydict(plugin_elem->aDict, get_token_string_by_id(DEVICE_PARAMETERS_ID), plugin_params->parameters[XBEE_PLUGIN_PARAMS_PARAMETERS].value.s);
               
         PyThreadState_Swap(tempState);
         PyEval_ReleaseLock();
      } // fin appel des fonctions Python
   }         
   pythonPluginServer_add_cmd(plugin_params->parameters[XBEE_PLUGIN_PARAMS_PLUGIN].value.s, (void *)plugin_elem, sizeof(plugin_queue_elem_t));
   interface->indicators.senttoplugin++;
   release_parsed_parameters(&plugin_params);
   plugin_elem=NULL;

         
   DBG_FREE(plugin_params);
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
   struct xbee_node_identification_response_s *nd_resp;
   int rval=0;
   int err;
   
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

   char *parameters=cJSON_GetObjectItem(jsonInterface,"parameters")->valuestring;

   parsed_parameters_t *plugin_params=NULL;
   int nb_plugin_params;
      
   plugin_params=alloc_parsed_parameters(parameters, valid_xbee_plugin_params, &nb_plugin_params, &err, 0);
   if(!plugin_params || !plugin_params->parameters[XBEE_PLUGIN_PARAMS_PLUGIN].value.s) {
      if(plugin_params)
         release_parsed_parameters(&plugin_params);
      return ERROR;
   }
 
   plugin_queue_elem_t *plugin_elem = (plugin_queue_elem_t *)malloc(sizeof(plugin_queue_elem_t));
   if(plugin_elem) {
      plugin_elem->type_elem=COMMISSIONNING;
         
      { // appel des fonctions Python
         mea_python_lock();

         plugin_elem->aDict = json_to_pydict_interface(jsonInterface);
/////         plugin_elem->aDict=stmt_to_pydict_interface(stmt);

         mea_addLong_to_pydict(plugin_elem->aDict, get_token_string_by_id(ID_XBEE_ID), (long)callback_commissionning->i002->xd);
         mea_addLong_to_pydict(plugin_elem->aDict, "LOCAL_XBEE_ADDR_H", (long)callback_commissionning->i002->local_xbee->l_addr_64_h);
         mea_addLong_to_pydict(plugin_elem->aDict, "LOCAL_XBEE_ADDR_L", (long)callback_commissionning->i002->local_xbee->l_addr_64_l);
            
         if(plugin_params->parameters[XBEE_PLUGIN_PARAMS_PARAMETERS].value.s)
            mea_addString_to_pydict(plugin_elem->aDict, get_token_string_by_id(INTERFACE_PARAMETERS_ID), plugin_params->parameters[XBEE_PLUGIN_PARAMS_PARAMETERS].value.s);
         mea_python_unlock();
      } // fin appel des fonctions Python
         
      pythonPluginServer_add_cmd(plugin_params->parameters[XBEE_PLUGIN_PARAMS_PLUGIN].value.s, (void *)plugin_elem, sizeof(plugin_queue_elem_t));
      *(callback_commissionning->senttoplugin)=*(callback_commissionning->senttoplugin)+1;
   }

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

   if(params->myThreadState) {
      PyEval_AcquireLock();
      PyThreadState_Clear(params->myThreadState);
      PyThreadState_Delete(params->myThreadState);
      PyEval_ReleaseLock();
      params->myThreadState=NULL;
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
   cJSON *jsonDevices = cJSON_GetObjectItem(jsonInterface, "devices");

   if(jsonDevices == NULL)
      return -1;
   cJSON *jsonDevice=jsonDevices->child;

   while(jsonDevice) {

      int err=0;

      char *name=jsonDevice->string;
      int id_sensor_actuator=(int)cJSON_GetObjectItem(jsonDevice, "id_sensor_actuator")->valuedouble;
      int id_type=(int)cJSON_GetObjectItem(jsonDevice, "id_type")->valuedouble;
      char *parameters=(char*)cJSON_GetObjectItem(jsonDevice, "parameters")->valuestring;
      char *dev=(char*)cJSON_GetObjectItem(jsonInterface, "dev")->valuestring;
      int todbflag=0;

      printf("@@@@@ %s %d %d %s %s\n", name, id_sensor_actuator, id_type, parameters, dev);

      params->plugin_params=alloc_parsed_parameters(parameters, valid_xbee_plugin_params, &(params->nb_plugin_params), &err, 0);
      if(!params->plugin_params || !params->plugin_params->parameters[XBEE_PLUGIN_PARAMS_PLUGIN].value.s) {
         if(params->plugin_params)
            release_parsed_parameters(&(params->plugin_params));
         continue; // si pas de paramètre (=> pas de plugin) ou pas de fonction ... pas la peine d'aller plus loin
      }
               
      plugin_queue_elem_t *plugin_elem = (plugin_queue_elem_t *)malloc(sizeof(plugin_queue_elem_t));
      if(plugin_elem) {

         pthread_cleanup_push( (void *)free, (void *)plugin_elem );

         plugin_elem->type_elem=XBEEDATA;
              
         memcpy(plugin_elem->buff, e->cmd, e->l_cmd);
         plugin_elem->l_buff=e->l_cmd;
               
         uint16_t data_type = e->cmd[0]; // 0x90 serie, 0x92 iodata

         { // appel des fonctions Python
            PyEval_AcquireLock();
            pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL); // trop compliquer de traiter avec pthread_cleanup => on interdit les arrets lors des commandes python
            PyThreadState *tempState = PyThreadState_Swap(params->myThreadState);

            uint32_t addr_h, addr_l;

            struct device_info_s device_info;
            device_info_from_json(&device_info, jsonDevice, jsonInterface, NULL);
            plugin_elem->aDict=mea_device_info_to_pydict_device(&device_info);

            if(sscanf((char *)dev, "MESH://%x-%x", &addr_h, &addr_l)==2) {
               mea_addLong_to_pydict(plugin_elem->aDict, get_token_string_by_id(ADDR_H_ID), (long)addr_h);
               mea_addLong_to_pydict(plugin_elem->aDict, get_token_string_by_id(ADDR_L_ID), (long)addr_l);
            }
       
            PyObject *value;

            value = PyByteArray_FromStringAndSize(plugin_elem->buff, (long)plugin_elem->l_buff);
            PyDict_SetItemString(plugin_elem->aDict, "cmd", value);
            Py_DECREF(value);
            mea_addLong_to_pydict(plugin_elem->aDict, "l_cmd", (long)plugin_elem->l_buff);

            value = PyByteArray_FromStringAndSize(&(plugin_elem->buff[12]), (long)plugin_elem->l_buff-12);
            PyDict_SetItemString(plugin_elem->aDict, "cmd_data", value);
            Py_DECREF(value);
            mea_addLong_to_pydict(plugin_elem->aDict, "l_cmd_data", (long)plugin_elem->l_buff-12);

            mea_addLong_to_pydict(plugin_elem->aDict, "data_type", (long)data_type);
            mea_addLong_to_pydict(plugin_elem->aDict, get_token_string_by_id(ID_XBEE_ID), (long)params->xd);
                  
            if(params->plugin_params->parameters[XBEE_PLUGIN_PARAMS_PARAMETERS].value.s)
               mea_addString_to_pydict(plugin_elem->aDict, get_token_string_by_id(DEVICE_PARAMETERS_ID), params->plugin_params->parameters[XBEE_PLUGIN_PARAMS_PARAMETERS].value.s);
                  
            PyThreadState_Swap(tempState);
            PyEval_ReleaseLock();
            pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL); // on réauthorise les arrêts 
            pthread_testcancel(); // on test tout de suite pour être sûr qu'on a pas ratté une demande d'arrêt
         } // fin appel des fonctions Python

         pythonPluginServer_add_cmd(params->plugin_params->parameters[XBEE_PLUGIN_PARAMS_PLUGIN].value.s, (void *)plugin_elem, sizeof(plugin_queue_elem_t));
         params->i002->indicators.senttoplugin++;
         DBG_FREE(plugin_elem);
         plugin_elem=NULL;
               
         pthread_cleanup_pop(0);
      }
      else {
         VERBOSE(2) {
            mea_log_printf("%s (%s) : %s - ", ERROR_STR, __func__, MALLOC_ERROR_STR);
            perror("");
         }
         pthread_exit(PTHREAD_CANCELED);
      }
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
   
   pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
   PyEval_AcquireLock();
   params->mainThreadState = PyThreadState_Get();
   params->myThreadState = PyThreadState_New(params->mainThreadState->interp);
   PyEval_ReleaseLock();
   pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
   pthread_testcancel();
   
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
         mea_log_printf("%s (%s) : %s - ", ERROR_STR, __func__, MALLOC_ERROR_STR);
         perror("");
      }
      goto clean_exit;
   }
   params->queue=(mea_queue_t *)malloc(sizeof(mea_queue_t));
   if(!params->queue) {
      VERBOSE(2) {
         mea_log_printf("%s (%s) : %s - ", ERROR_STR, __func__, MALLOC_ERROR_STR);
         perror("");
      }
      goto clean_exit;
   }
   mea_queue_init(params->queue);

   params->xd=xd;
//   params->param_db=db;
   pthread_mutex_init(&params->callback_lock, NULL);
   pthread_cond_init(&params->callback_cond, NULL);
   params->i002=(void *)i002;
   params->mainThreadState = NULL;
   params->myThreadState = NULL;
   
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
         mea_log_printf("%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
         perror("");
      }
      return NULL;
   }
   i002->thread_is_running=0;
                  
   struct interface_type_002_data_s *i002_start_stop_params=(struct interface_type_002_data_s *)malloc(sizeof(struct interface_type_002_data_s));
   if(!i002_start_stop_params) {
      DBG_FREE(i002);
      i002=NULL;
      VERBOSE(2) {
         mea_log_printf("%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
         perror("");
      }  
      return NULL;
   }

   char *name=jsonInterface->string;
   int id_interface=(int)cJSON_GetObjectItem(jsonInterface,"id_interface")->valuedouble;
   char *dev=cJSON_GetObjectItem(jsonInterface,"dev")->valuestring;
   char *parameters=cJSON_GetObjectItem(jsonInterface,"parameters")->valuestring;
   char *description=cJSON_GetObjectItem(jsonInterface,"description")->valuestring;

   strncpy(i002->dev, (char *)dev, sizeof(i002->dev)-1);
   strncpy(i002->name, (char *)name, sizeof(i002->name)-1);
   i002->id_interface=id_interface;
   i002->parameters=(char *)malloc(strlen((char *)parameters)+1);
   strcpy(i002->parameters,(char *)parameters);

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
      struct callback_xpl_data_s *data = (struct callback_xpl_data_s *)i002->xPL_callback_data;
      
      PyEval_AcquireLock();
      if(data->myThreadState) {
         PyThreadState_Clear(data->myThreadState);
         PyThreadState_Delete(data->myThreadState);
         data->myThreadState=NULL;
      }
      PyEval_ReleaseLock();
         
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
      struct callback_xpl_data_s *data = (struct callback_xpl_data_s *)start_stop_params->i002->xPL_callback_data;
      
      PyEval_AcquireLock();
      if(data->myThreadState) {
         PyThreadState_Clear(data->myThreadState);
         PyThreadState_Delete(data->myThreadState);
         data->myThreadState=NULL;
      }
      PyEval_ReleaseLock();
         
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
 * \param     i002           descripteur de l'interface  
 * \param     db             descripteur ouvert de la base de paramétrage  
 * \param     id_interface   identifiant de l'interface
 * \param     dev_and_speed  chemin et vitesse (débit) de l'interface série (sous forme SERIAL://dev:speed
 * \param     parameters     paramètres associés à l'interface
 * \param     md             descripteur ouvert de la base d'historique
 * \return    ERROR ou NOERROR
 **/ 
{
   char dev[81];
   char buff[80];
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
   
   char err_str[128];
   
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
   uint32_t addr_64_h;
   uint32_t addr_64_l;
      
   ret=get_local_xbee_addr(xd, local_xbee);
   if(ret==-1)
      goto clean_exit;

   start_stop_params->i002->local_xbee=local_xbee;
 
   /*
    * exécution du plugin de l'interface
    */
   interface_parameters=alloc_parsed_parameters(start_stop_params->i002->parameters, valid_xbee_plugin_params, &interface_nb_parameters, &err, 0);
   if(!interface_parameters || !interface_parameters->parameters[XBEE_PLUGIN_PARAMS_PLUGIN].value.s) {
      if(interface_parameters) {
         release_parsed_parameters(&interface_parameters);
         
         VERBOSE(9) mea_log_printf("%s (%s) : invalid python plugin parameters (%s)\n", INFO_STR, __func__, start_stop_params->i002->parameters);
      }
      else {
         VERBOSE(9) mea_log_printf("%s  (%s) : no python plugin specified\n", INFO_STR, __func__);
      }
   }
   else {
      PyObject *plugin_params_dict=NULL;
      PyObject *pName, *pModule, *pFunc;
      PyObject *pArgs, *pValue=NULL;
      
      pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
      PyEval_AcquireLock();
      PyThreadState *mainThreadState=PyThreadState_Get();
      PyThreadState *myThreadState = PyThreadState_New(mainThreadState->interp);
      
      PyThreadState *tempState = PyThreadState_Swap(myThreadState);

      PyErr_Clear();
      pName = PyString_FromString(interface_parameters->parameters[XBEE_PLUGIN_PARAMS_PLUGIN].value.s);
      pModule = PyImport_Import(pName);
      if(!pModule) {
         VERBOSE(5) mea_log_printf("%s (%s) : %s not found\n", ERROR_STR, __func__, interface_parameters->parameters[XBEE_PLUGIN_PARAMS_PLUGIN].value.s);
      }
      else { // à mutualiser ... "fonction call_python_function" à écrire ... voir aussi automator ...
         pFunc = PyObject_GetAttrString(pModule, "mea_init");
         if (pFunc && PyCallable_Check(pFunc)) {
            // préparation du parametre du module
            plugin_params_dict=PyDict_New();
            mea_addLong_to_pydict(plugin_params_dict, get_token_string_by_id(ID_XBEE_ID), (long)xd);
            mea_addLong_to_pydict(plugin_params_dict, get_token_string_by_id(INTERFACE_ID_ID), start_stop_params->i002->id_interface);
            if(interface_parameters->parameters[XBEE_PLUGIN_PARAMS_PARAMETERS].value.s)
               mea_addString_to_pydict(plugin_params_dict, get_token_string_by_id(INTERFACE_PARAMETERS_ID), interface_parameters->parameters[XBEE_PLUGIN_PARAMS_PARAMETERS].value.s);

            pArgs = PyTuple_New(1);
            Py_INCREF(plugin_params_dict); // PyTuple_SetItem va voler la référence, on en rajoute une pour pouvoir ensuite faire un Py_DECREF
            PyTuple_SetItem(pArgs, 0, plugin_params_dict);
         
            pValue = PyObject_CallObject(pFunc, pArgs); // appel du plugin
            if (pValue != NULL) {
               DEBUG_SECTION mea_log_printf("%s (%s) : Result of call of mea_init : %ld\n", DEBUG_STR, __func__, PyInt_AsLong(pValue));
            }
            PyErr_Clear(); // à remplacer
            Py_DECREF(pValue);
            Py_DECREF(pArgs);
            Py_DECREF(plugin_params_dict);
         }
         else {
            VERBOSE(5) mea_log_printf("%s (%s) : mea_init not fount in %s module\n", ERROR_STR, __func__, interface_parameters->parameters[XBEE_PLUGIN_PARAMS_PLUGIN].value.s);
         }
         Py_XDECREF(pFunc);
      }
      Py_XDECREF(pModule);
      Py_DECREF(pName);
      PyErr_Clear();
      
      PyThreadState_Swap(tempState);
      PyThreadState_Clear(myThreadState);
      PyThreadState_Delete(myThreadState);
      PyEval_ReleaseLock();
      pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

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
   xpl_callback_params->mainThreadState=NULL;
   xpl_callback_params->myThreadState=NULL;
   
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

   interfacesFns->lib = NULL;
   interfacesFns->type = interfacesFns->get_type();
   interfacesFns->plugin_flag = 0;

   return 0;
}
#endif
