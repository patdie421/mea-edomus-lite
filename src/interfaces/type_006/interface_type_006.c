//
//  interface_type_006.c
//  mea-edomus
//
//  Created by Patrice Dietsch on 16/11/2015.
//
//
#ifdef __APPLE__
#include <Python/Python.h>
#else
#include <Python.h>
#endif

#include "interface_type_006.h"

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

#include "globals.h"
#include "consts.h"

#include "serial.h"

#include "tokens.h"
#include "tokens_da.h"
#include "mea_string_utils.h"
#include "mea_verbose.h"
#include "macros.h"

#include "parameters_utils.h"
#include "pythonPluginServer.h"
#include "python_utils.h"

#include "processManager.h"

#include "interfacesServer.h"


char *interface_type_006_senttoplugin_str="SENT2PLUGIN";
char *interface_type_006_xplin_str="XPLIN";
char *interface_type_006_xplout_str="XPLOUT";
char *interface_type_006_serialin_str="SERIALIN";
char *interface_type_006_serialout_str="SERIALOUT";


typedef void (*thread_f)(void *);

// parametres valide pour les capteurs ou actionneurs pris en compte par le type 2.
char *valid_genericserial_plugin_params[]={"S:PLUGIN","S:PARAMETERS", NULL};
#define GENERICSERIAL_PLUGIN_PARAMS_PLUGIN      0
#define GENERICSERIAL_PLUGIN_PARAMS_PARAMETERS  1

struct callback_xpl_data_s
{
   PyThreadState  *mainThreadState;
   PyThreadState  *myThreadState;
};

struct genericserial_thread_params_s
{
   interface_type_006_t *i006;
};


int start_interface_type_006(int my_id, void *data, char *errmsg, int l_errmsg);
int stop_interface_type_006(int my_id, void *data, char *errmsg, int l_errmsg);
int restart_interface_type_006(int my_id, void *data, char *errmsg, int l_errmsg);
int16_t check_status_interface_type_006(interface_type_006_t *i006);


int interface_type_006_call_serialDataPre(struct genericserial_thread_params_s *params, void *data, int l_data)
{
   int retour=-1;
   if(params->i006->pFunc) {

      pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
      PyEval_AcquireLock();
      PyThreadState *tempState = PyThreadState_Swap(params->i006->myThreadState);

      PyObject *aDict=PyDict_New();
      if(aDict) {
         PyObject *value = PyByteArray_FromStringAndSize((char *)data, l_data);
         PyDict_SetItemString(aDict, "data", value);
         Py_DECREF(value);
         mea_addLong_to_pydict(aDict, "l_data", (long)l_data);

         mea_addLong_to_pydict(aDict, INTERFACE_ID_STR_C, params->i006->id_interface);
         mea_addLong_to_pydict(aDict, "api_key", (long)params->i006->id_interface);

         if(params->i006->pParams)
            PyDict_SetItemString(aDict, "plugin_paramters", params->i006->pParams);

         retour=mea_call_python_function2(params->i006->pFunc, aDict);
         Py_DECREF(aDict);

      }

      PyThreadState_Swap(tempState);
      PyEval_ReleaseLock();
      pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
   }

   return retour;
}


static int interface_type_006_data_to_plugin(PyThreadState *myThreadState, cJSON *jsonInterface, cJSON *jsonDevice, int data_type, void *data, int l_data)
{
   parsed_parameters_t *plugin_params=NULL;
   int nb_plugin_params;
   int err;
   int retour=-1;
   plugin_queue_elem_t *plugin_elem = NULL;

   char *parameters = cJSON_GetObjectItem(jsonDevice, "parameters")->valuestring;

   plugin_params=alloc_parsed_parameters(parameters, valid_genericserial_plugin_params, &nb_plugin_params, &err, 0);
   if(!plugin_params || !plugin_params->parameters[GENERICSERIAL_PLUGIN_PARAMS_PLUGIN].value.s)
      goto interface_type_006_data_to_plugin_clean_exit;

   plugin_elem = (plugin_queue_elem_t *)malloc(sizeof(plugin_queue_elem_t));
   if(plugin_elem) {
      plugin_elem->type_elem=data_type;

      struct device_info_s device_info;
      device_info_from_json(&device_info, jsonDevice, jsonInterface, NULL);
            
      {
         // appel des fonctions Python => on lock
         pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL); // trop compliquer de traiter avec pthread_cleanup => on interdit les arrêts lors des commandes python
         PyEval_AcquireLock();
         PyThreadState *tempState = PyThreadState_Swap(myThreadState);

         // les infos du "device"
         plugin_elem->aDict = mea_device_info_to_pydict_device(&device_info);
         if(!plugin_elem->aDict)
            goto interface_type_006_data_to_plugin_clean_exit;

         // les datas
         PyObject *value = PyByteArray_FromStringAndSize((char *)data, (long)l_data);
         PyDict_SetItemString(plugin_elem->aDict, "data", value);
         Py_DECREF(value);
         mea_addLong_to_pydict(plugin_elem->aDict, "l_data", (long)l_data);

         // info api
         mea_addLong_to_pydict(plugin_elem->aDict, "api_key", device_info.interface_id);
         
         // parametres spécifiques
         if(plugin_params->parameters[GENERICSERIAL_PLUGIN_PARAMS_PARAMETERS].value.s)
            mea_addString_to_pydict(plugin_elem->aDict, DEVICE_PARAMETERS_STR_C, plugin_params->parameters[GENERICSERIAL_PLUGIN_PARAMS_PARAMETERS].value.s);

         PyThreadState_Swap(tempState);
         PyEval_ReleaseLock();
         pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL); // on réauthorise les arrêts

      } // fin appel des fonctions Python
     
      // commande dans la file de traitement 
      pythonPluginServer_add_cmd(plugin_params->parameters[GENERICSERIAL_PLUGIN_PARAMS_PLUGIN].value.s, (void *)plugin_elem, sizeof(plugin_queue_elem_t));

      retour = 0;
   }

interface_type_006_data_to_plugin_clean_exit:
   if(plugin_elem) {
      free(plugin_elem);
      plugin_elem=NULL;
   }

   if(plugin_params) {
      release_parsed_parameters(&plugin_params);
      nb_plugin_params=0;
      plugin_params=NULL; 
   }
 
   return retour;
}


void set_interface_type_006_isnt_running(void *data)
{
   interface_type_006_t *i006 = (interface_type_006_t *)data;

   i006->thread_is_running=0;
}


int16_t _interface_type_006_xPL_callback2(cJSON *xplMsgJson, struct device_info_s *device_info, void *userValue)
{
   char *device = NULL;
   int err;
   cJSON *j = NULL;
 
   interface_type_006_t *i006=(interface_type_006_t *)userValue;
   struct callback_xpl_data_s *callback_data=(struct callback_xpl_data_s *)i006->xPL_callback_data;
   
   i006->indicators.xplin++;
   
   
   pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
   PyEval_AcquireLock();

   if(!callback_data->mainThreadState)
      callback_data->mainThreadState=PyThreadState_Get();
   if(!callback_data->myThreadState)
      callback_data->myThreadState = PyThreadState_New(callback_data->mainThreadState->interp);

   PyEval_ReleaseLock();
   pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL); // on réauthorise les arrêts         

   j=cJSON_GetObjectItem(xplMsgJson, get_token_string_by_id(XPL_DEVICE_ID));
   if(j)
      device=j->valuestring;
   if(!device) {
      VERBOSE(5) mea_log_printf("%s (%s) : xPL message no device\n", INFO_STR, __func__);
      return -1;
   }
   mea_strtolower(device);
 
   if(!device)
      return -1;

   parsed_parameters_t *plugin_params=NULL;
   int nb_plugin_params;

   plugin_params=alloc_parsed_parameters(device_info->parameters, valid_genericserial_plugin_params, &nb_plugin_params, &err, 0);

   if(!plugin_params || !plugin_params->parameters[GENERICSERIAL_PLUGIN_PARAMS_PLUGIN].value.s) {
      if(plugin_params) {
         release_parsed_parameters(&plugin_params);
         nb_plugin_params=0;
         plugin_params=NULL;
      }
      return -1;
   }

   plugin_queue_elem_t *plugin_elem = (plugin_queue_elem_t *)malloc(sizeof(plugin_queue_elem_t));
   if(plugin_elem) {
      plugin_elem->type_elem=XPLMSG;

      { // appel des fonctions Python
         pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
         PyEval_AcquireLock();
         PyThreadState *tempState = PyThreadState_Swap(callback_data->myThreadState);

         plugin_elem->aDict=mea_device_info_to_pydict_device(device_info);

         if(plugin_params->parameters[GENERICSERIAL_PLUGIN_PARAMS_PARAMETERS].value.s)
            mea_addString_to_pydict(plugin_elem->aDict, DEVICE_PARAMETERS_STR_C, plugin_params->parameters[GENERICSERIAL_PLUGIN_PARAMS_PARAMETERS].value.s);

         mea_addLong_to_pydict(plugin_elem->aDict, "api_key", i006->id_interface);

         PyObject *_xplmsg=mea_xplMsgToPyDict2(xplMsgJson);
         PyDict_SetItemString(plugin_elem->aDict, XPLMSG_STR_C, _xplmsg);

         PyThreadState_Swap(tempState);
         PyEval_ReleaseLock();
         pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
      } // fin appel des fonctions Python

      pythonPluginServer_add_cmd(plugin_params->parameters[GENERICSERIAL_PLUGIN_PARAMS_PLUGIN].value.s, (void *)plugin_elem, sizeof(plugin_queue_elem_t));

      i006->indicators.senttoplugin++;
      free(plugin_elem);
      plugin_elem=NULL;
   }

   release_parsed_parameters(&plugin_params);
   nb_plugin_params=0;
   plugin_params=NULL;

   return 0;
}


void *_thread_interface_type_006_genericserial_data_cleanup(void *args)
{
   struct genericserial_thread_params_s *params=(struct genericserial_thread_params_s *)args;

   if(!params)
      return NULL;
   
   if(params->i006->fd!=-1) {
      close(params->i006->fd);
   }
   
   if(params->i006->myThreadState) {
      pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
      PyEval_AcquireLock();
      PyThreadState *tempState = PyThreadState_Swap(params->i006->myThreadState);
      if(params->i006->pFunc) {
         Py_XDECREF(params->i006->pFunc);
         params->i006->pFunc=NULL;
      } 
      if(params->i006->pModule) {
         Py_XDECREF(params->i006->pModule);
         params->i006->pModule=NULL;
      }
      if(params->i006->pParams) {
         Py_XDECREF(params->i006->pParams);
         params->i006->pParams=NULL;
      }
      PyThreadState_Swap(tempState);
      PyThreadState_Clear(params->i006->myThreadState);
      PyThreadState_Delete(params->i006->myThreadState);
      params->i006->myThreadState=NULL;
      PyEval_ReleaseLock();
      pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

      params->i006->myThreadState=NULL;
   }
   else
   {
      mea_python_lock();
      if(params->i006->pFunc) {
         Py_XDECREF(params->i006->pFunc);
         params->i006->pFunc=NULL;
      }
      if(params->i006->pModule) {
         Py_XDECREF(params->i006->pModule);
         params->i006->pModule=NULL;
      }
      if(params->i006->pParams) {
         Py_XDECREF(params->i006->pParams);
         params->i006->pParams=NULL;
      }
      mea_python_unlock();
   }

   free(params);
   params=NULL;

   return NULL;
}


int init_interface_type_006_data_preprocessor(interface_type_006_t *i006, char *plugin_name, char *plugin_parameters)
{
   int ret = -1;

   mea_python_lock();
      
   PyObject *pName = PyString_FromString(plugin_name);
   if(!pName) {
      ret=-1;
      goto init_interface_type_006_data_preprocessor_clean_exit;
   }
   else {
      if(!i006->pModule) {
         i006->pModule =  PyImport_Import(pName);
         if(!i006->pModule) {
            ret=-1;
            goto init_interface_type_006_data_preprocessor_clean_exit;
         }
      }
      else {
         PyObject *m = NULL;
         m=i006->pModule;
         i006->pModule=PyImport_ReloadModule(m); // on force le rechargement (c'est pour simplifier)
         Py_DECREF(m);
         if(!i006->pModule) {
            ret=-1;
            goto init_interface_type_006_data_preprocessor_clean_exit;
         }
      }

      if(i006->pFunc) {
         Py_DECREF(i006->pFunc);
         i006->pFunc=NULL;
      }

      i006->pFunc = PyObject_GetAttrString(i006->pModule, "mea_dataPreprocessor");
      if(!i006->pFunc) {
         ret=-1;
         goto init_interface_type_006_data_preprocessor_clean_exit;
      }
      
      if(PyCallable_Check(i006->pFunc)) {
         if(plugin_parameters)
            i006->pParams=PyString_FromString(plugin_parameters);
         else
            i006->pParams=NULL;
         ret = 0;
      }
      else {
         VERBOSE(5) mea_log_printf("%s (%s) : no mea_dataPreprocessor entry point\n", ERROR_STR, __func__);

         Py_XDECREF(i006->pFunc);
         i006->pFunc=NULL;

         Py_XDECREF(i006->pModule);
         i006->pModule=NULL;

         ret = -1;
      }
   }

init_interface_type_006_data_preprocessor_clean_exit:
   if(pName) {
      Py_XDECREF(pName);
      pName=NULL;      
   }
   mea_python_unlock();

   return ret;
}


void *_thread_interface_type_006_genericserial_data(void *args)
{
   struct genericserial_thread_params_s *params=(struct genericserial_thread_params_s *)args;

   pthread_cleanup_push( (void *)_thread_interface_type_006_genericserial_data_cleanup, (void *)params );
   pthread_cleanup_push( (void *)set_interface_type_006_isnt_running, (void *)params->i006 );
   
   params->i006->thread_is_running=1;
   process_heartbeat(params->i006->monitoring_id);
   
   int err_counter=0;
   
   // on se met un context python sous le coude pour ce thread
   pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
   PyEval_AcquireLock();
   params->i006->mainThreadState = PyThreadState_Get();
   params->i006->myThreadState = PyThreadState_New(params->i006->mainThreadState->interp);
   PyEval_ReleaseLock();
   pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
   pthread_testcancel();

   mea_timer_t process_timer;

   mea_init_timer(&process_timer, 5, 1);
   mea_start_timer(&process_timer); 

   params->i006->fd=-1; 

   while(1) {
      if(params->i006->fd<0) // pas ou plus decommunication avec le périphérique serie
      {
         params->i006->fd=serial_open(params->i006->real_dev, params->i006->real_speed);
         if(params->i006->fd<0) {
            VERBOSE(5) {
               mea_log_printf("%s (%s) : can't open %s - ", ERROR_STR, __func__, params->i006->real_dev);
               perror("");
            }
         }
      }

      if(params->i006->fd>=0) {
         char c=0;

         char buffer[4096];
         int buffer_ptr=0;
         struct timeval timeout;
         
         err_counter=0;
         
         while(1) {
            if(mea_test_timer(&process_timer)==0) {
               process_heartbeat(params->i006->monitoring_id);
               process_update_indicator(params->i006->monitoring_id, interface_type_006_senttoplugin_str, params->i006->indicators.senttoplugin);
               process_update_indicator(params->i006->monitoring_id, interface_type_006_xplin_str, params->i006->indicators.xplin);
               process_update_indicator(params->i006->monitoring_id, interface_type_006_xplout_str, params->i006->indicators.xplout);
               process_update_indicator(params->i006->monitoring_id, interface_type_006_serialin_str, params->i006->indicators.serialin);
               process_update_indicator(params->i006->monitoring_id, interface_type_006_serialout_str, params->i006->indicators.serialout);
            }

            fd_set input_set;
            FD_ZERO(&input_set);
            FD_SET(params->i006->fd, &input_set);

            timeout.tv_sec  = 0; // timeout après
            timeout.tv_usec = 200000; // 200ms

            int ret = select(params->i006->fd+1, &input_set, NULL, NULL, &timeout);
            if (ret <= 0) {
               if(ret == 0) {
                  if(buffer_ptr>0)
                     break; // après un "blanc" de 200 ms, si on a des données, on les envoie au plugin
               }
               else {
                  // erreur à traiter ...
                  VERBOSE(5) {
                     mea_log_printf("%s (%s) : select error - ", ERROR_STR, __func__);
                     perror("");
                  }
                  close(params->i006->fd);
                  params->i006->fd=-1;
                  break;
               }
            }
            else {
               ret=(int)read(params->i006->fd, &c, 1);
               if(ret<0) {
                  close(params->i006->fd);
                  params->i006->fd=-1;
                  break;
               }
            }
            if(ret>0) {
               buffer[buffer_ptr++]=c;
               if(buffer_ptr >= sizeof(buffer)-1)
                  break;
            }
         }

         if(buffer_ptr>0) {
            params->i006->indicators.serialin+=buffer_ptr;
            buffer[buffer_ptr]=0;

            int ret=interface_type_006_call_serialDataPre(params, (void *)buffer, buffer_ptr+1);

            cJSON *jsonInterface = getInterfaceById_alloc(params->i006->id_interface);
            if(jsonInterface) {
               cJSON *jsonDevices = cJSON_GetObjectItem(jsonInterface, "devices");
               if(jsonDevices) {
                  cJSON *jsonDevice = jsonDevices->child;
                  while(jsonDevice) {

                     int ret=interface_type_006_data_to_plugin(params->i006->myThreadState, jsonInterface, jsonDevice, DATAFROMSENSOR, (void *)buffer, buffer_ptr+1);

                     jsonDevice=jsonDevice->next;
                  }
               }
            }
            cJSON_Delete(jsonInterface);
         }
         pthread_testcancel();
      }
      else
      {
         err_counter++;
         if(err_counter<5)
            sleep(5);
         else goto _thread_interface_type_006_genericserial_data_clean_exit;
      }

      pthread_testcancel();
   }

_thread_interface_type_006_genericserial_data_clean_exit:
   pthread_cleanup_pop(1);
   pthread_cleanup_pop(1);

   process_async_stop(params->i006->monitoring_id);
   for(;;) sleep(1);
   
   return NULL;
}


pthread_t *start_interface_type_006_genericserial_data_thread(interface_type_006_t *i006, parsed_parameters_t *interface_parameters, thread_f function)
{
   pthread_t *thread=NULL;
   struct genericserial_thread_params_s *params=NULL;
   struct genericserial_callback_data_s *genericserial_callback_data=NULL;
   
   params=malloc(sizeof(struct genericserial_thread_params_s));
   if(!params)
   {
      VERBOSE(2) {
         mea_log_printf("%s (%s) : %s - ", ERROR_STR, __func__, MALLOC_ERROR_STR);
         perror("");
      }
      goto start_interface_type_006_genericserial_data_thread_clean_exit;
   }

   params->i006=(void *)i006;

   if(interface_parameters && interface_parameters->parameters[GENERICSERIAL_PLUGIN_PARAMS_PLUGIN].value.s) {
      init_interface_type_006_data_preprocessor(params->i006,
                                                interface_parameters->parameters[GENERICSERIAL_PLUGIN_PARAMS_PLUGIN].value.s,
                                                interface_parameters->parameters[GENERICSERIAL_PLUGIN_PARAMS_PARAMETERS].value.s);
   }

   thread=(pthread_t *)malloc(sizeof(pthread_t));
   if(!thread) {
      VERBOSE(2) mea_log_printf("%s (%s) : %s\n", ERROR_STR, __func__, MALLOC_ERROR_STR);
      goto start_interface_type_006_genericserial_data_thread_clean_exit;
   }
   
   if(pthread_create (thread, NULL, (void *)function, (void *)params))
      goto start_interface_type_006_genericserial_data_thread_clean_exit;
   pthread_detach(*thread);
   fprintf(stderr,"INTERFACE_TYPE_006 : %x\n", (unsigned int)*thread);

   return thread;
   
start_interface_type_006_genericserial_data_thread_clean_exit:
   if(thread) {
      free(thread);
      thread=NULL;
   }
   
   if(genericserial_callback_data) {
      free(genericserial_callback_data);
      genericserial_callback_data=NULL;
   }

   if(params) {
      free(params);
      params=NULL;
   }
   return NULL;
}


int update_devices_type_006(void *ixxx)
{
   printf("update devices type 006\n");

   return 0;
}


int clean_interface_type_006(void *ixxx)
{
   interface_type_006_t *i006 = (interface_type_006_t *)ixxx;

   if(i006->parameters) {
      free(i006->parameters);
      i006->parameters=NULL;
   }

   if(i006->xPL_callback2)
      i006->xPL_callback2=NULL;

   if(i006->thread) {
      free(i006->thread);
      i006->thread=NULL;
   }

   return 0;
}


xpl2_f get_xPLCallback_interface_type_006(void *ixxx)
{
   interface_type_006_t *i006 = (interface_type_006_t *)ixxx;

   if(i006 == NULL)
      return NULL;
   else
      return i006->xPL_callback2;
}


int get_monitoring_id_interface_type_006(void *ixxx)
{
   interface_type_006_t *i006 = (interface_type_006_t *)ixxx;

   if(i006 == NULL)
      return -1;
   else
      return i006->monitoring_id;
}


int set_xPLCallback_interface_type_006(void *ixxx, xpl2_f cb)
{
   interface_type_006_t *i006 = (interface_type_006_t *)ixxx;

   if(i006 == NULL)
      return -1;
   else {
      i006->xPL_callback2 = cb;
      return 0;
   }
}


int set_monitoring_id_interface_type_006(void *ixxx, int id)
{
   interface_type_006_t *i006 = (interface_type_006_t *)ixxx;

   if(i006 == NULL)
      return -1;
   else {
      i006->monitoring_id = id;
      return 0;
   }
}


int get_type_interface_type_006()
{
   return INTERFACE_TYPE_006;
}


static int api_write_data(interface_type_006_t *i006, PyObject *args, PyObject **res, int16_t *nerr, char *err, int l_err)
{
   err="";
   l_err=0;

   if(i006->fd == -1) {
      *nerr=253;
      return -253;
   }
   PyObject *arg;
   int16_t ret;
   *nerr=255;
   *res = NULL;

   // récupération des paramètres et contrôle des types
   if(PyTuple_Size(args)!=3)
      return -255;

   Py_buffer py_packet;
   arg=PyTuple_GetItem(args, 2);
   if(PyObject_CheckBuffer(arg)) {
      ret=PyObject_GetBuffer(arg, &py_packet, PyBUF_SIMPLE);
      if(ret<0)
         return -255;
   }
   else
      return -255;

   ret=write(i006->fd, py_packet.buf, py_packet.len);
   if(ret<0) {
      VERBOSE(5) {
         mea_log_printf("%s (%s) : write - ", ERROR_STR, __func__);
         perror("");
      }
   }
 
   PyBuffer_Release(&py_packet);
 
   nerr=0;

   return 0;
}


int16_t api_interface_type_006(void *ixxx, char *cmnd, void *args, int nb_args, void **res, int16_t *nerr, char *err, int l_err)
{
   interface_type_006_t *i006 = (interface_type_006_t *)ixxx;

   PyObject *pyArgs = (PyObject *)args;
   PyObject **pyRes = (PyObject **)res;
   
   if(strcmp(cmnd, "mea_writeData") == 0) {
      int ret=api_write_data((void *)i006, pyArgs, pyRes, nerr, err, l_err);
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


interface_type_006_t *malloc_and_init2_interface_type_006(int id_driver, cJSON *jsonInterface)
{
   interface_type_006_t *i006;
                  
   i006=(interface_type_006_t *)malloc(sizeof(interface_type_006_t));
   if(!i006) {
      VERBOSE(2) {
        mea_log_printf("%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
        perror("");
      }
      return NULL;
   }
   i006->thread_is_running=0;
                  
   struct interface_type_006_data_s *i006_start_stop_params=(struct interface_type_006_data_s *)malloc(sizeof(struct interface_type_006_data_s));
   if(!i006_start_stop_params) {
      free(i006);
      i006=NULL;
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

   strncpy(i006->dev, (char *)dev, sizeof(i006->dev)-1);
   strncpy(i006->name, (char *)name, sizeof(i006->name)-1);
   i006->id_interface=id_interface;
   i006->parameters=(char *)malloc(strlen((char *)parameters)+1);
   if(i006->parameters)
      strncpy(i006->parameters,(char *)parameters, strlen((char *)parameters));
   i006->indicators.senttoplugin=0;
   i006->indicators.xplin=0;
   i006->indicators.serialin=0;
   i006->indicators.serialout=0;
   
   i006->thread=NULL;
   i006->xPL_callback2=NULL;
   i006->xPL_callback_data=NULL;

   i006->myThreadState=NULL;
   i006->myThreadState=NULL;
   i006->pModule=NULL;
   i006->pFunc=NULL;
   i006->pParams=NULL;
   
   i006->monitoring_id=process_register((char *)name);
   i006_start_stop_params->i006=i006;
                  
   process_set_group(i006->monitoring_id, 1);
   process_set_start_stop(i006->monitoring_id, start_interface_type_006, stop_interface_type_006, (void *)i006_start_stop_params, 1);
   process_set_watchdog_recovery(i006->monitoring_id, restart_interface_type_006, (void *)i006_start_stop_params);
   process_set_description(i006->monitoring_id, (char *)description);
   process_set_heartbeat_interval(i006->monitoring_id, 60); // chien de garde au bout de 60 secondes sans heartbeat

   process_add_indicator(i006->monitoring_id, interface_type_006_senttoplugin_str, 0);
   process_add_indicator(i006->monitoring_id, interface_type_006_xplin_str, 0);
   process_add_indicator(i006->monitoring_id, interface_type_006_serialin_str, 0);
   process_add_indicator(i006->monitoring_id, interface_type_006_serialout_str, 0);

   return i006;
}


int stop_interface_type_006(int my_id, void *data, char *errmsg, int l_errmsg)
{
   if(!data)
      return -1;

   struct interface_type_006_data_s *start_stop_params=(struct interface_type_006_data_s *)data;

   VERBOSE(1) mea_log_printf("%s (%s) : %s shutdown thread ...\n", INFO_STR, __func__, start_stop_params->i006->name);

   if(start_stop_params->i006->xPL_callback2) {
      start_stop_params->i006->xPL_callback2=NULL;
   }

   if(start_stop_params->i006->xPL_callback_data) {
      struct callback_xpl_data_s *callback_data = (struct callback_xpl_data_s *)start_stop_params->i006->xPL_callback_data;
      if(callback_data->myThreadState) {
         PyEval_AcquireLock();
         PyThreadState_Clear(callback_data->myThreadState);
         PyThreadState_Delete(callback_data->myThreadState);
         callback_data->myThreadState=NULL;
         PyEval_ReleaseLock();
      }

      free(start_stop_params->i006->xPL_callback_data);
      start_stop_params->i006->xPL_callback_data=NULL;
   }
   
   if(start_stop_params->i006->thread)
   {
      pthread_cancel(*(start_stop_params->i006->thread));
      
      int counter=100;
      while(counter--) {
         if(start_stop_params->i006->thread_is_running) { // pour éviter une attente "trop" active
            usleep(100); // will sleep for 10 ms
         }
         else {
            break;
         }
      }
      DEBUG_SECTION mea_log_printf("%s (%s) : %s, fin après %d itération(s)\n",DEBUG_STR, __func__,start_stop_params->i006->name,100-counter);

      if(!start_stop_params->i006->thread)
         free(start_stop_params->i006->thread);
      start_stop_params->i006->thread=NULL;
   }
 
   return 0;
}


int restart_interface_type_006(int my_id, void *data, char *errmsg, int l_errmsg)
{
   process_stop(my_id, NULL, 0);
   sleep(5);
   return process_start(my_id, NULL, 0);
}


int start_interface_type_006(int my_id, void *data, char *errmsg, int l_errmsg)
{
   char dev[81];
   char buff[80];
   speed_t speed;
   int err=0;
   char err_str[128];
   int ret=0;
   int interface_nb_parameters=0;
   parsed_parameters_t *interface_parameters=NULL;
    
   struct interface_type_006_data_s *start_stop_params=(struct interface_type_006_data_s *)data;
   start_stop_params->i006->thread=NULL;
   start_stop_params->i006->xPL_callback_data=NULL;
   
   ret=get_dev_and_speed((char *)start_stop_params->i006->dev, buff, sizeof(buff), &speed);
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
   else
   {
      VERBOSE(2) mea_log_printf("%s (%s) : incorrect device/speed interface - %s\n", ERROR_STR, __func__, start_stop_params->i006->dev);
      goto clean_exit;
   }
   strncpy(start_stop_params->i006->real_dev, dev, sizeof( start_stop_params->i006->real_dev)-1);
   start_stop_params->i006->real_speed=(int)speed;

   interface_parameters=alloc_parsed_parameters(start_stop_params->i006->parameters, valid_genericserial_plugin_params, &interface_nb_parameters, &err, 0);
   if(!interface_parameters || !interface_parameters->parameters[GENERICSERIAL_PLUGIN_PARAMS_PLUGIN].value.s) {
      if(interface_parameters) {
         // pas de plugin spécifié
         release_parsed_parameters(&interface_parameters);
         interface_nb_parameters=0;
         interface_parameters=NULL;
         VERBOSE(9) mea_log_printf("%s (%s) : no python plugin specified\n", INFO_STR, __func__);
      }
      else
         VERBOSE(5) mea_log_printf("%s (%s) : invalid or no python plugin parameters (%s)\n", ERROR_STR, __func__, start_stop_params->i006->parameters);
   }
   else {
      mea_python_lock(); // attention python_lock / python_unlock définissent un block ({ }) les variables déclérées restent locales au bloc

      PyObject *plugin_params_dict=PyDict_New();

      mea_addLong_to_pydict(plugin_params_dict, INTERFACE_ID_STR_C, start_stop_params->i006->id_interface);
      if(interface_parameters->parameters[GENERICSERIAL_PLUGIN_PARAMS_PARAMETERS].value.s)
         mea_addString_to_pydict(plugin_params_dict, INTERFACE_PARAMETERS_STR_C, interface_parameters->parameters[GENERICSERIAL_PLUGIN_PARAMS_PARAMETERS].value.s);
      mea_call_python_function(interface_parameters->parameters[GENERICSERIAL_PLUGIN_PARAMS_PLUGIN].value.s, "mea_init", plugin_params_dict);
      Py_DECREF(plugin_params_dict);

      mea_python_unlock();
   }
   // données pour les callbacks xpl
   struct callback_xpl_data_s *xpl_callback_params=(struct callback_xpl_data_s *)malloc(sizeof(struct callback_xpl_data_s));
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

   start_stop_params->i006->xPL_callback_data=xpl_callback_params;
   start_stop_params->i006->xPL_callback2=_interface_type_006_xPL_callback2;

   start_stop_params->i006->thread=start_interface_type_006_genericserial_data_thread(start_stop_params->i006, interface_parameters, (thread_f)_thread_interface_type_006_genericserial_data);
   
   if(start_stop_params->i006->thread!=0) {
      VERBOSE(2) mea_log_printf("%s (%s) : %s %s.\n", INFO_STR, __func__,start_stop_params->i006->name, launched_successfully_str);
      return 0;
   }

   {
#ifdef _POSIX_SOURCE
      char *ret;
#else
      int ret;
#endif
      ret=strerror_r(errno, err_str, sizeof(err_str));
      VERBOSE(2) mea_log_printf("%s  (%s) : %s can't start - %s.\n", ERROR_STR, __func__, start_stop_params->i006->name, err_str);
   } 

clean_exit:
   if(start_stop_params->i006->thread) {
      stop_interface_type_006(start_stop_params->i006->monitoring_id, start_stop_params, NULL, 0);
   }

   if(start_stop_params->i006->xPL_callback_data) {
      free(start_stop_params->i006->xPL_callback_data);
      start_stop_params->i006->xPL_callback_data=NULL;
   }

   if(interface_parameters) {
      release_parsed_parameters(&interface_parameters);
      interface_parameters=NULL;
      interface_nb_parameters=0;
   }
   
   return -1;
}

#ifndef ASPLUGIN
int get_fns_interface_type_006(struct interfacesServer_interfaceFns_s *interfacesFns)
{
   interfacesFns->malloc_and_init2 = (malloc_and_init2_f)&malloc_and_init2_interface_type_006;
   interfacesFns->get_monitoring_id = (get_monitoring_id_f)&get_monitoring_id_interface_type_006;
   interfacesFns->get_xPLCallback = (get_xPLCallback_f)&get_xPLCallback_interface_type_006;
   interfacesFns->update_devices = (update_devices_f)&update_devices_type_006;
   interfacesFns->clean = (clean_f)&clean_interface_type_006;
   interfacesFns->set_monitoring_id = (set_monitoring_id_f)&set_monitoring_id_interface_type_006;
   interfacesFns->set_xPLCallback = (set_xPLCallback_f)&set_xPLCallback_interface_type_006;
   interfacesFns->get_type = (get_type_f)&get_type_interface_type_006;
   interfacesFns->api = (api_f)&api_interface_type_006;
   interfacesFns->lib = NULL;
   interfacesFns->type = interfacesFns->get_type();
   interfacesFns->plugin_flag = 0;

   return 0;
}
#endif
