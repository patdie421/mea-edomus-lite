//
//  interface_type_003.c
//  mea-edomus
//
//  Created by Patrice Dietsch on 21/02/2015.
//
//
#include "interface_type_003.h"
#include "interface_type_003_pairing.h"

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
#include "tokens.h"
#include "tokens_da.h"
#include "mea_verbose.h"
#include "mea_string_utils.h"
#include "cJSON.h"
#include "enocean.h"
#include "serial.h"
#include "parameters_utils.h"
#include "python_utils.h"
#include "mea_xpl.h"
#include "processManager.h"

#include "interfacesServer.h"


char *interface_type_003_senttoplugin_str="SENT2PLUGIN";
char *interface_type_003_xplin_str="XPLIN";
char *interface_type_003_enoceandatain_str="ENOCEANIN";


typedef void (*thread_f)(void *);

// parametres valide pour les capteurs ou actionneurs pris en compte par le type 3.
char *valid_enocean_plugin_params[]={"S:PLUGIN","S:PLUGIN_PARAMETERS", NULL};

struct enocean_callback_data_s // donnee "userdata" pour les callbacks
{
   enocean_ed_t    *ed;
   mea_queue_t     *queue;
   pthread_mutex_t *callback_lock;
   pthread_cond_t  *callback_cond;
};


struct xpl_callback_data_s
{
   PyThreadState  *mainThreadState;
   PyThreadState  *myThreadState;
};


struct enocean_thread_data_s
{
   enocean_ed_t         *ed;
   mea_queue_t          *queue;
   pthread_mutex_t       callback_lock;
   pthread_cond_t        callback_cond;
   parsed_parameters_t  *plugin_params;
   int                   nb_plugin_params;
   interface_type_003_t *i003;
};


int start_interface_type_003(int my_id, void *data, char *errmsg, int l_errmsg);
int stop_interface_type_003(int my_id, void *data, char *errmsg, int l_errmsg);
int restart_interface_type_003(int my_id, void *data, char *errmsg, int l_errmsg);


void set_interface_type_003_isnt_running(void *data)
{
   interface_type_003_t *i003 = (interface_type_003_t *)data;
   i003->thread_is_running=0;
}


void _enocean_data_free_queue_elem(void *d)
{
   enocean_data_queue_elem_t *e=(enocean_data_queue_elem_t *)d;

   if(e->data) {
      free(e->data);
      e->data=NULL;
   }

   free(e);
   e=NULL;
}


int16_t _interface_type_003_xPL_callback2(cJSON *xplMsgJson, struct device_info_s *device_info, void *userValue)
{
   int err =0;

   interface_type_003_t *i003=(interface_type_003_t *)userValue;

   char *dev = (char *)device_info->interface_dev;
   int a,b,c,d;
   if(sscanf(dev, "%*[^:]://%x-%x-%x-%x", &a,&b,&c,&d)!=4) {
      VERBOSE(9) mea_log_printf("%s (%s) : scanf - %s\n", ERROR_STR, __func__, dev);
      return -1;
   }

   i003->indicators.xplin++;

   uint32_t enocean_addr = a;
   enocean_addr = (enocean_addr << 8) + b;
   enocean_addr = (enocean_addr << 8) + c;
   enocean_addr = (enocean_addr << 8) + d;

   parsed_parameters_t *plugin_params=NULL;
   int nb_plugin_params;

   plugin_params=alloc_parsed_parameters(device_info->parameters, valid_enocean_plugin_params, &nb_plugin_params, &err, 0);
   if(!plugin_params || !plugin_params->parameters[PLUGIN_PARAMS_PLUGIN].value.s) {
      if(plugin_params)
         release_parsed_parameters(&plugin_params);
      return -1;
   }
   
   cJSON *data=NULL;
   data=device_info_to_json_alloc(device_info);
   cJSON *msg=mea_xplMsgToJson_alloc(xplMsgJson);
   cJSON_AddItemToObject(data, XPLMSG_STR_C, msg);
   cJSON_AddNumberToObject(data, XPL_ENOCEAN_ADDR_STR_C, (double)enocean_addr);
   cJSON_AddNumberToObject(data, API_KEY_STR_C, (double)i003->id_interface);
   if(plugin_params->parameters[PLUGIN_PARAMS_PARAMETERS].value.s)
      cJSON_AddStringToObject(data, DEVICE_PARAMETERS_STR_C, plugin_params->parameters[PLUGIN_PARAMS_PARAMETERS].value.s);

   python_cmd_json(plugin_params->parameters[PLUGIN_PARAMS_PLUGIN].value.s, XPLMSG_JSON, data);

   release_parsed_parameters(&plugin_params);
   plugin_params=NULL;

   return 0;
}


int16_t _inteface_type_003_enocean_data_callback(uint8_t *data, uint16_t l_data, uint32_t enocean_addr, void *callbackdata)
{
   struct timeval tv;
   struct enocean_callback_data_s *callback_data;
   enocean_data_queue_elem_t *e;

   callback_data=(struct enocean_callback_data_s *)callbackdata;

   gettimeofday(&tv, NULL);

   e=(enocean_data_queue_elem_t *)malloc(sizeof(enocean_data_queue_elem_t));

   e->enocean_addr=enocean_addr;

   e->data=(uint8_t *)malloc(l_data+1);
   memcpy(e->data,data,l_data);
   e->l_data=l_data;
   memcpy(&e->tv,&tv,sizeof(struct timeval));

   pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)(&callback_data->callback_lock) );
   pthread_mutex_lock(callback_data->callback_lock);
   mea_queue_in_elem(callback_data->queue, e);
   if(callback_data->queue->nb_elem>=1)
      pthread_cond_broadcast(callback_data->callback_cond);
   pthread_mutex_unlock(callback_data->callback_lock);
   pthread_cleanup_pop(0);

   return 0;
}


void *_thread_interface_type_003_enocean_data_cleanup(void *args)
{
   struct enocean_thread_data_s *udata=(struct enocean_thread_data_s *)args;

   if(!udata)
      return NULL;

   if(udata->queue && udata->queue->nb_elem>0) // on vide s'il y a quelque chose avant de partir
      mea_queue_cleanup(udata->queue, _enocean_data_free_queue_elem);

   if(udata->queue) {
      free(udata->queue);
      udata->queue=NULL;
   }

   if(udata->plugin_params) {
      release_parsed_parameters(&(udata->plugin_params));
      udata->plugin_params=NULL;
   }

   free(udata);
   udata=NULL;

   return NULL;
}


void *_thread_interface_type_003_enocean_data(void *args)
/**
 * \brief     Gestion des données asynchrones en provenances de equipement enocean
 * \details   Les data peuvent arriver n'importe quand, il sagit de pouvoir les traiter dès réceptions.
 *            Le callback associé est en charge de poster les données à traiter dans une file qui seront consommées par ce thread.
 * \param     args   ensemble des parametres nécessaires au thread regroupé dans une structure de données (voir struct enocean_thread_params_s)
 */
{
   struct enocean_thread_data_s *udata=(struct enocean_thread_data_s *)args;

   pthread_cleanup_push( (void *)_thread_interface_type_003_enocean_data_cleanup, (void *)udata );
   pthread_cleanup_push( (void *)set_interface_type_003_isnt_running, (void *)udata->i003 );

   udata->i003->thread_is_running=1;
   process_heartbeat(udata->i003->monitoring_id);

   enocean_ed_t *ed=udata->ed;
   enocean_data_queue_elem_t *e;
   int ret;

   while(1) {
      if(ed->signal_flag==1)
         goto _thread_interface_type_003_enocean_data_clean_exit;

      process_heartbeat(udata->i003->monitoring_id);
      process_update_indicator(udata->i003->monitoring_id, interface_type_003_senttoplugin_str, udata->i003->indicators.senttoplugin);
      process_update_indicator(udata->i003->monitoring_id, interface_type_003_xplin_str, udata->i003->indicators.xplin);
      process_update_indicator(udata->i003->monitoring_id, interface_type_003_enoceandatain_str, udata->i003->indicators.enoceandatain);

      pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)(&udata->callback_lock) );
      pthread_mutex_lock(&udata->callback_lock);

      if(udata->queue->nb_elem==0) {
         struct timeval tv;
         struct timespec ts;
         gettimeofday(&tv, NULL);
         ts.tv_sec = tv.tv_sec + 10; // timeout de 10 secondes
         ts.tv_nsec = 0;

         ret=pthread_cond_timedwait(&udata->callback_cond, &udata->callback_lock, &ts);
         if(ret) {
            if(ret==ETIMEDOUT) {
//               DEBUG_SECTION mea_log_printf("%s (%s) : Nb elements in queue after TIMEOUT : %ld)\n", DEBUG_STR, __func__, params->queue->nb_elem);
            }
            else {
               // autres erreurs à traiter
            }
         }
      }

      if(udata->i003->pairing_state == ENOCEAN_PAIRING_ON) {
         if(mea_now() - udata->i003->pairing_startms > 60.0*1000.0) { // 60 secondes pour l'appairage
            udata->i003->pairing_startms=-1.0;
            udata->i003->pairing_state = ENOCEAN_PAIRING_OFF;
         }
      }

      ret=mea_queue_out_elem(udata->queue, (void **)&e);

      pthread_mutex_unlock(&udata->callback_lock);
      pthread_cleanup_pop(0);

      if(!ret) {
         cJSON *jsonInterface=NULL;
         char interfaceDevName[256]="";
         uint32_t addr=0;
         uint8_t _addr[4];
         // uint8_t a=0,b=0,c=0,d=0;
         int pairing_done=0;
         
         udata->i003->indicators.enoceandatain++;
         addr = e->enocean_addr;
//         d=addr & 0xFF;
//         addr = addr >> 8;
//         c=addr & 0xFF;
//         addr = addr >> 8;
//         b=addr & 0xFF;
//         addr = addr >> 8;
//         a=addr & 0xFF;

         _addr[3]=addr & 0xFF;
         addr = addr >> 8;
         _addr[2]=addr & 0xFF;
         addr = addr >> 8;
         _addr[1]=addr & 0xFF;
         addr = addr >> 8;
         _addr[0]=addr & 0xFF;
         snprintf(interfaceDevName, sizeof(interfaceDevName)-1, "%s://%02x-%02x-%02x-%02x", udata->i003->name, _addr[0], _addr[1], _addr[2], _addr[3]);
         mea_strtolower(interfaceDevName);
         jsonInterface = getInterfaceByDevName_alloc(interfaceDevName);
         
         mea_log_printf("%s (%s) : enocean data from %02x-%02x-%02x-%02x\n", INFO_STR, __func__, _addr[0], _addr[1], _addr[2], _addr[3]);

         if(udata->i003->pairing_state==ENOCEAN_PAIRING_ON && jsonInterface==NULL) {
            mea_log_printf("%s (%s) : pairing in progress for %02x-%02x-%02x-%02x\n", INFO_STR, __func__, _addr[0], _addr[1], _addr[2], _addr[3]);
            
            uint8_t eep[3]="";
            
            int _ret=enocean_pairing(udata->i003, e, addr, eep);
            if(_ret<0) {
               mea_log_printf("%s (%s) : can't pair %02x-%02x-%02x-%02x\n", INFO_STR, __func__, _addr[0], _addr[1], _addr[2], _addr[3]);
            }
            else {
               enocean_update_interfaces((void *)udata->i003, interfaceDevName, _addr, eep);
               pairing_done=1;
            }
         }
         
         if(jsonInterface && pairing_done==0) {
            cJSON *jsonDevices = cJSON_GetObjectItem(jsonInterface, DEVICES_STR_C);
            if(jsonDevices) {
               cJSON *jsonDevice = jsonDevices->child;
               while(jsonDevice) {
                  int err;
                  char *parameters = cJSON_GetObjectItem(jsonDevice, PARAMETERS_STR_C)->valuestring;
                     udata->plugin_params=alloc_parsed_parameters(parameters, valid_enocean_plugin_params, &(udata->nb_plugin_params), &err, 0);
                  if(!udata->plugin_params || !udata->plugin_params->parameters[PLUGIN_PARAMS_PLUGIN].value.s) {
                     goto _thread_interface_type_003_enocean_next_device_loop;
                  }
                  
                  struct device_info_s device_info;
                  device_info_from_json(&device_info, jsonDevice, jsonInterface, NULL);
                  cJSON *data=NULL;
                  data=device_info_to_json_alloc(&device_info);
                  cJSON_AddNumberToObject(data, XPL_ENOCEAN_ADDR_STR_C, (double)e->enocean_addr);
                  cJSON_AddItemToObject(data, DATA_STR_C, cJSON_CreateByteArray((char *)e->data, e->l_data));
                  cJSON_AddNumberToObject(data, L_DATA_STR_C, (double)e->l_data);
                  cJSON_AddNumberToObject(data, API_KEY_STR_C, (double)(long)udata->i003->id_interface);
                  if(udata->plugin_params->parameters[PLUGIN_PARAMS_PARAMETERS].value.s)
                     cJSON_AddStringToObject(data, DEVICE_PARAMETERS_STR_C, udata->plugin_params->parameters[PLUGIN_PARAMS_PARAMETERS].value.s);
                  python_cmd_json(udata->plugin_params->parameters[PLUGIN_PARAMS_PLUGIN].value.s, XPLMSG_JSON, data);
                  udata->i003->indicators.senttoplugin++;

_thread_interface_type_003_enocean_next_device_loop:
                  if(udata->plugin_params) {
                     release_parsed_parameters(&(udata->plugin_params));
                     udata->plugin_params=NULL;
                  }
                  jsonDevice=jsonDevice->next;
               }
            }
            cJSON_Delete(jsonInterface);
         }


         free(e->data);
         e->data=NULL;
         free(e);
         e=NULL;

         pthread_testcancel();
      }
      else {
         // pb d'accès aux données de la file
         DEBUG_SECTION mea_log_printf("%s (%s) : mea_queue_out_elem - no data in queue\n", DEBUG_STR, __func__);
      }
      pthread_testcancel();
   }

_thread_interface_type_003_enocean_data_clean_exit:
   pthread_cleanup_pop(1);
   pthread_cleanup_pop(1);

   process_async_stop(udata->i003->monitoring_id);
   for(;;) sleep(1);

   return NULL;
}


pthread_t *start_interface_type_003_enocean_data_thread(interface_type_003_t *i003, enocean_ed_t *ed, thread_f function)
/**
 * \brief     Demarrage du thread de gestion des données (non solicitées) en provenance des enocean
 * \param     i003           descripteur de l'interface
 * \param     ed             descripteur de com. avec l'enocean
 * \param     function       function principale du thread à démarrer
 * \return    adresse du thread ou NULL en cas d'erreur
 **/
{
   pthread_t *thread=NULL;
   struct enocean_thread_data_s *udata=NULL;
   struct enocean_callback_data_s *enocean_callback_data=NULL;

   udata=malloc(sizeof(struct enocean_thread_data_s));
   if(!udata) {
      VERBOSE(2) {
         mea_log_printf("%s (%s) : %s - ", ERROR_STR, __func__, MALLOC_ERROR_STR);
         perror("");
      }
      goto clean_exit;
   }
   udata->queue=(mea_queue_t *)malloc(sizeof(mea_queue_t));
   if(!udata->queue) {
      VERBOSE(2) {
         mea_log_printf("%s (%s) : %s - ", ERROR_STR, __func__, MALLOC_ERROR_STR);
         perror("");
      }
      goto clean_exit;
   }
   mea_queue_init(udata->queue);

   udata->ed=ed;
   pthread_mutex_init(&udata->callback_lock, NULL);
   pthread_cond_init(&udata->callback_cond, NULL);
   udata->i003=(void *)i003;

   // préparation des données pour les callback io_data et data_flow dont les données sont traitées par le même thread
   enocean_callback_data=(struct enocean_callback_data_s *)malloc(sizeof(struct enocean_callback_data_s));
   if(!enocean_callback_data) {
      VERBOSE(2) mea_log_printf("%s (%s) : %s\n", ERROR_STR, __func__, MALLOC_ERROR_STR);
      goto clean_exit;
   }
   enocean_callback_data->ed=ed;
   enocean_callback_data->callback_lock=&udata->callback_lock;
   enocean_callback_data->callback_cond=&udata->callback_cond;
   enocean_callback_data->queue=udata->queue;

   enocean_set_data_callback2(ed, _inteface_type_003_enocean_data_callback, (void *)enocean_callback_data);

   thread=(pthread_t *)malloc(sizeof(pthread_t));
   if(!thread) {
      VERBOSE(2) mea_log_printf("%s (%s) : %s\n", ERROR_STR, __func__, MALLOC_ERROR_STR);
      goto clean_exit;
   }

   if(pthread_create (thread, NULL, (void *)function, (void *)udata))
      goto clean_exit;
   pthread_detach(*thread);

   return thread;

clean_exit:
   if(thread) {
      free(thread);
      thread=NULL;
   }

   if(enocean_callback_data) {
      free(enocean_callback_data);
      enocean_callback_data=NULL;
   }

   if(udata && udata->queue && udata->queue->nb_elem>0) // on vide s'il y a quelque chose avant de partir
      mea_queue_cleanup(udata->queue, _enocean_data_free_queue_elem);

   if(udata) {
      if(udata->queue) {
         free(udata->queue);
         udata->queue=NULL;
      }
      free(udata);
      udata=NULL;
   }
   return NULL;
}


int update_devices_type_003(void *ixxx)
{
   printf("update devices type 003\n");

   return 0;
}


int clean_interface_type_003(void *ixxx)
{
   interface_type_003_t *i003 = (interface_type_003_t *)ixxx;

   if(i003->parameters) {
      free(i003->parameters);
      i003->parameters=NULL;
   }

   if(i003->xPL_callback2)
      i003->xPL_callback2=NULL;

   if(i003->ed && i003->ed->enocean_callback_data) {
         free(i003->ed->enocean_callback_data);
         i003->ed->enocean_callback_data=NULL;
   }

   if(i003->ed) {
      free(i003->ed);
      i003->ed=NULL;
   }

   if(i003->thread) {
      free(i003->thread);
      i003->thread=NULL;
   }

   return 0;
}


xpl2_f get_xPLCallback_interface_type_003(void *ixxx)
{
   interface_type_003_t *i003 = (interface_type_003_t *)ixxx;

   if(i003 == NULL)
      return NULL;
   else
      return i003->xPL_callback2;
}


int get_monitoring_id_interface_type_003(void *ixxx)
{
   interface_type_003_t *i003 = (interface_type_003_t *)ixxx;

   if(i003 == NULL)
      return -1;
   else
      return i003->monitoring_id;
}


int set_xPLCallback_interface_type_003(void *ixxx, xpl2_f cb)
{
   interface_type_003_t *i003 = (interface_type_003_t *)ixxx;

   if(i003 == NULL)
      return -1;
   else {
      i003->xPL_callback2 = cb;
      return 0;
   }
}


int set_monitoring_id_interface_type_003(void *ixxx, int id)
{
   interface_type_003_t *i003 = (interface_type_003_t *)ixxx;

   if(i003 == NULL)
      return -1;
   else {
      i003->monitoring_id = id;
      return 0;
   }
}


int get_type_interface_type_003()
{
   return INTERFACE_TYPE_003;
}


int get_interface_id_interface_type_003(void *ixxx)
{
   interface_type_003_t *i003 = (interface_type_003_t *)ixxx;

   if(i003 == NULL)
      return -1;
   else
      return i003->id_interface;
}


int api_sendEnoceanRadioErp1Packet_json(interface_type_003_t *i003, cJSON *args, cJSON **res, int16_t *nerr, char *err, int l_err)
{
   cJSON *arg;
   int16_t ret;

   *nerr=255;
   
   // récupération des paramètres et contrôle des types
   if(args->type!=cJSON_Array || cJSON_GetArraySize(args)!=6) {
      return -255;
   }

   // rorg
   uint32_t rorg;
   arg=cJSON_GetArrayItem(args, 2);
   
   if(arg->type==cJSON_Number) {
      rorg=(uint32_t)arg->valuedouble;
   }
   else
      return -255;

   // sub_id
   uint32_t sub_id;
   arg=cJSON_GetArrayItem(args, 3);
   if(arg->type==cJSON_Number) {
      sub_id=(uint32_t)arg->valuedouble;
   }
   else
      return -255;

   // dest addr
   uint32_t dest_addr;
   arg=cJSON_GetArrayItem(args, 4);
   if(arg->type==cJSON_Number) {
      dest_addr=(uint32_t)arg->valuedouble;
   }
   else
      return -255;

   arg=cJSON_GetArrayItem(args, 5);
   int l=0;
   if(arg->type==cJSON_ByteArray || arg->type==cJSON_String) {
      if(arg->type==cJSON_String) {
         l=(int)strlen(arg->valuestring);
      }
      else {
         l=arg->valueint;
      }
   }
   else
      return -255;

   *nerr = 0;
   ret = enocean_send_radio_erp1_packet(i003->ed, rorg, i003->ed->id, sub_id, dest_addr, (uint8_t *)arg->valuestring, l, 0, nerr);
   if(ret<0)
      strncpy(err, "error", l_err);
   else
      strncpy(err, "no error", l_err);

   *res = NULL;

   return ret;
}


int16_t api_interface_type_003_json(void *ixxx, char *cmnd, void *args, int nb_args, void **res, int16_t *nerr, char *err, int l_err)
{
   interface_type_003_t *i003 = (interface_type_003_t *)ixxx;

   cJSON *jsonArgs = (cJSON *)args;
   cJSON **jsonRes = (cJSON **)res;

   if(strcmp(cmnd, "sendEnoceanRadioErp1Packet") == 0) {
      api_sendEnoceanRadioErp1Packet_json(i003, jsonArgs, jsonRes, nerr, err, l_err);
   }
#ifdef DEBUG
   else if(strcmp(cmnd, "test") == 0) {
      *jsonRes = cJSON_CreateString("New style Api call OK !!!");
      *nerr=0;
      strncpy(err, "no error", l_err);

      return 0;
   }
#endif
   else {
      strncpy(err, "unknown function", l_err);
      return -254;
   }

   return -1;
}


interface_type_003_t *malloc_and_init_interface_type_003(int id_driver, cJSON *jsonInterface)
{
   interface_type_003_t *i003;

   i003=(interface_type_003_t *)malloc(sizeof(interface_type_003_t));
   if(!i003) {
      VERBOSE(2) {
        mea_log_printf("%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
        perror("");
      }
      return NULL;
   }
   i003->thread_is_running=0;

   struct interface_type_003_data_s *i003_start_stop_params=(struct interface_type_003_data_s *)malloc(sizeof(struct interface_type_003_data_s));
   if(!i003_start_stop_params) {
      free(i003);
      i003=NULL;
      VERBOSE(2) {
         mea_log_printf("%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
         perror("");
      }
      return NULL;
   }

   int id_interface=(int)cJSON_GetObjectItem(jsonInterface,ID_INTERFACE_STR_C)->valuedouble;
   char *name=jsonInterface->string;
   char *dev=cJSON_GetObjectItem(jsonInterface,DEV_STR_C)->valuestring;
   char *parameters=cJSON_GetObjectItem(jsonInterface,PARAMETERS_STR_C)->valuestring;
   char *description=cJSON_GetObjectItem(jsonInterface,DESCRIPTION_STR_C)->valuestring;

   strncpy(i003->dev, (char *)dev, sizeof(i003->dev)-1);
   strncpy(i003->name, (char *)name, sizeof(i003->name)-1);
   i003->id_interface=id_interface;
   i003->id_driver=id_driver;
   int l_parameters=(int)strlen((char *)parameters)+1;
   i003->parameters=(char *)malloc(l_parameters);
   i003->parameters[l_parameters-1]=0;
   strncpy(i003->parameters,(char *)parameters,l_parameters-1);

   i003->indicators.senttoplugin=0;
   i003->indicators.xplin=0;
   i003->indicators.enoceandatain=0;

   i003->ed=NULL;
   i003->thread=NULL;
   i003->xPL_callback2=NULL;
   i003->xPL_callback_data=NULL;

   i003->monitoring_id=process_register((char *)name);
   i003_start_stop_params->i003=i003;

   process_set_group(i003->monitoring_id, 1);
   process_set_start_stop(i003->monitoring_id, start_interface_type_003, stop_interface_type_003, (void *)i003_start_stop_params, 1);
   process_set_watchdog_recovery(i003->monitoring_id, restart_interface_type_003, (void *)i003_start_stop_params);
   process_set_description(i003->monitoring_id, (char *)description);
   process_set_heartbeat_interval(i003->monitoring_id, 60); // chien de garde au bout de 60 secondes sans heartbeat

   process_add_indicator(i003->monitoring_id, interface_type_003_senttoplugin_str, 0);
   process_add_indicator(i003->monitoring_id, interface_type_003_xplin_str, 0);
   process_add_indicator(i003->monitoring_id, interface_type_003_enoceandatain_str, 0);

   return i003;
}


int stop_interface_type_003(int my_id, void *data, char *errmsg, int l_errmsg)
/**
 * \brief     arrêt d'une interface de type 3
 * \details   RAZ de tous les structures de données et libération des zones de mémoires allouées
 * \param     my_id
 * \param     data
 * \param     errmsg
 * \param     l_errmsg
 * \return    0 pas d'erreur ou -1 sinon
 **/
{
   if(!data)
      return -1;

   struct interface_type_003_data_s *start_stop_params=(struct interface_type_003_data_s *)data;

   VERBOSE(1) mea_log_printf("%s (%s) : %s shutdown thread ... ", INFO_STR, __func__, start_stop_params->i003->name);

   if(start_stop_params->i003->xPL_callback_data) {
      free(start_stop_params->i003->xPL_callback_data);
      start_stop_params->i003->xPL_callback_data=NULL;
   }

   if(start_stop_params->i003->xPL_callback2)
      start_stop_params->i003->xPL_callback2=NULL;

   if(start_stop_params->i003->ed->enocean_callback_data) {
      free(start_stop_params->i003->ed->enocean_callback_data);
      start_stop_params->i003->ed->enocean_callback_data=NULL;
   }

   if(start_stop_params->i003->thread) {
      pthread_cancel(*(start_stop_params->i003->thread));

      int counter=100;
      while(counter--) {
         if(start_stop_params->i003->thread_is_running) {
            // pour éviter une attente "trop" active
            usleep(100); // will sleep for 10 ms
         }
         else
            break;
      }
      DEBUG_SECTION mea_log_printf("%s (%s) : %s, fin après %d itération(s)\n",DEBUG_STR, __func__,start_stop_params->i003->name,100-counter);

      free(start_stop_params->i003->thread);
      start_stop_params->i003->thread=NULL;
   }

   enocean_close(start_stop_params->i003->ed);
   enocean_clean_ed(start_stop_params->i003->ed);

   if(start_stop_params->i003->ed) {
      free(start_stop_params->i003->ed);
      start_stop_params->i003->ed=NULL;
   }

   return 0;
}


int restart_interface_type_003(int my_id, void *data, char *errmsg, int l_errmsg)
/**
 * \brief     redémarrage d'une interface de type 3
 * \details   Arrêt puis relance de l'interface type 3
 * \param     my_id
 * \param     data
 * \param     errmsg
 * \param     l_errmsg
 * \return    0 pas d'erreur ou -1 sinon
 **/
{
   process_stop(my_id, NULL, 0);
   sleep(5);
   return process_start(my_id, NULL, 0);
}


int start_interface_type_003(int my_id, void *data, char *errmsg, int l_errmsg)
/**
 * \brief     Demarrage d'une interface de type 3
 * \details   ouverture de la communication avec un dongle USB300 ou équivalant, démarrage du thread de gestion des données
 *            "non sollicitées" et mise en place des callback xpl et données
 * \param     my_id          id attribué par processManager
 * \param     data           données nécessaire au arret/relance (à caster sur struct interface_type_003_data_s)
 * \param     errmsg
 * \param     l_errmsg
 * \return    0 = Ok ou -1 = KO
 **/
{
   char dev[81];
   char buff[80];
   speed_t speed;

   int fd=-1;
   int err;
   int ret;

   enocean_ed_t *ed=NULL;

   struct xpl_callback_data_s *xpl_callback_data=NULL;

   struct interface_type_003_data_s *start_stop_params=(struct interface_type_003_data_s *)data;

   start_stop_params->i003->thread=NULL;

   int interface_nb_parameters=0;
   parsed_parameters_t *interface_parameters=NULL;

   char err_str[128];

   ret=get_dev_and_speed((char *)start_stop_params->i003->dev, buff, sizeof(buff), &speed);
   if(!ret) {
      int n=snprintf(dev,sizeof(buff)-1,"/dev/%s",buff);
      if(n<0 || n==(sizeof(buff)-1)) {
         strerror_r(errno, err_str, sizeof(err_str));
         VERBOSE(2) {
            mea_log_printf("%s (%s) : snprintf - %s\n", ERROR_STR, __func__, err_str);
         }
         goto clean_exit;
      }
   }
   else {
      VERBOSE(2) mea_log_printf("%s (%s) : incorrect device/speed interface - %s\n", ERROR_STR,__func__,start_stop_params->i003->dev);
      goto clean_exit;
   }

   ed=(enocean_ed_t *)malloc(sizeof(enocean_ed_t));
   if(!ed) {
      strerror_r(errno, err_str, sizeof(err_str));
      VERBOSE(2) {
         mea_log_printf("%s (%s) : %s - %s\n", ERROR_STR, __func__, MALLOC_ERROR_STR, err_str);
      }
      goto clean_exit;
   }

   fd=enocean_init(ed, dev);
   if (fd == -1) {
      strerror_r(errno, err_str, sizeof(err_str));
      VERBOSE(2) {
         mea_log_printf("%s (%s) : enocean_init - Unable to open serial port (%s).\n", ERROR_STR, __func__, dev);
      }
      goto clean_exit;
   }
   start_stop_params->i003->ed=ed;

   /*
    * exécution du plugin d'interface
    */
   interface_parameters=alloc_parsed_parameters(start_stop_params->i003->parameters, valid_enocean_plugin_params, &interface_nb_parameters, &err, 0);
   if(!interface_parameters || !interface_parameters->parameters[PLUGIN_PARAMS_PLUGIN].value.s) {
      if(interface_parameters) {
         // pas de plugin spécifié
         release_parsed_parameters(&interface_parameters);
         interface_parameters=NULL;
         VERBOSE(9) mea_log_printf("%s (%s) : no python plugin specified\n", INFO_STR, __func__);
      }
      else {
         VERBOSE(5) mea_log_printf("%s (%s) : invalid or no python plugin parameters (%s)\n", ERROR_STR, __func__, start_stop_params->i003->parameters);
      }
   }
   else {
      cJSON *result=NULL;
      cJSON *data=cJSON_CreateObject();
      if(data) {
         cJSON_AddNumberToObject(data, INTERFACE_ID_STR_C, start_stop_params->i003->id_interface);
         if(interface_parameters->parameters[PLUGIN_PARAMS_PARAMETERS].value.s) {
            cJSON_AddStringToObject(data, INTERFACE_PARAMETERS_STR_C, interface_parameters->parameters[PLUGIN_PARAMS_PARAMETERS].value.s);
         }

         result=python_call_function_json_alloc(interface_parameters->parameters[PLUGIN_PARAMS_PLUGIN].value.s, "mea_init", data);
         if(result) {
            char *s=cJSON_Print(result);
            DEBUG_SECTION mea_log_printf("%s (%s) : Result of call of mea_init : %s\n", DEBUG_STR, __func__, s);
            free(s);

            cJSON_Delete(result);
         }
      }

      if(interface_parameters)
         release_parsed_parameters(&interface_parameters);
      interface_nb_parameters=0;
   }

   /*
    * Gestion des sous-interfaces
    */
   start_stop_params->i003->thread=start_interface_type_003_enocean_data_thread(start_stop_params->i003, ed, (thread_f)_thread_interface_type_003_enocean_data);

   //
   // gestion des demandes xpl : ajouter une zone de donnees specifique au callback xpl (pas simplement passe i003).
   //
   xpl_callback_data=(struct xpl_callback_data_s *)malloc(sizeof(struct xpl_callback_data_s));
   if(!xpl_callback_data) {
      strerror_r(errno, err_str, sizeof(err_str));
      VERBOSE(2) {
         mea_log_printf("%s (%s) : %s - %s\n", ERROR_STR, __func__, MALLOC_ERROR_STR, err_str);
      }
      goto clean_exit;
   }
   xpl_callback_data->mainThreadState=NULL;
   xpl_callback_data->myThreadState=NULL;

   start_stop_params->i003->xPL_callback_data=xpl_callback_data;
   start_stop_params->i003->xPL_callback2=_interface_type_003_xPL_callback2;

   VERBOSE(2) mea_log_printf("%s (%s) : %s %s.\n", INFO_STR, __func__, start_stop_params->i003->name, launched_successfully_str);

   return 0;

clean_exit:
   if(ed)
      enocean_remove_data_callback(ed);

   if(start_stop_params->i003->thread)
      stop_interface_type_003(start_stop_params->i003->monitoring_id, start_stop_params, NULL, 0);

   if(interface_parameters) {
      release_parsed_parameters(&interface_parameters);
      interface_nb_parameters=0;
   }

   if(xpl_callback_data) {
      free(xpl_callback_data);
      xpl_callback_data=NULL;
   }

   if(ed) {
      if(fd>=0)
         enocean_close(ed);
      enocean_free_ed(ed);
      ed=NULL;
   }

   return -1;
}


#ifndef ASPLUGIN
int get_fns_interface_type_003(struct interfacesServer_interfaceFns_s *interfacesFns)
{
   interfacesFns->malloc_and_init2 = (malloc_and_init2_f)&malloc_and_init_interface_type_003;
   interfacesFns->get_monitoring_id = (get_monitoring_id_f)&get_monitoring_id_interface_type_003;
   interfacesFns->get_xPLCallback = (get_xPLCallback_f)&get_xPLCallback_interface_type_003;
   interfacesFns->update_devices = (update_devices_f)&update_devices_type_003;
   interfacesFns->clean = (clean_f)&clean_interface_type_003;
   interfacesFns->set_monitoring_id = (set_monitoring_id_f)&set_monitoring_id_interface_type_003;
   interfacesFns->set_xPLCallback = (set_xPLCallback_f)&set_xPLCallback_interface_type_003;
   interfacesFns->get_type = (get_type_f)&get_type_interface_type_003;
   interfacesFns->api = (api_f)&api_interface_type_003_json;
   interfacesFns->lib = NULL;
   interfacesFns->type = interfacesFns->get_type();
   interfacesFns->plugin_flag = 0;

   return 0;
}
#endif
