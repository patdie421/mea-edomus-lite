//
//  mea-eDomus
//
//  Created by Patrice Dietsch on 04/05/13.
//
//
#ifdef __APPLE__
#include <Python/Python.h>
#else
#include <Python.h>
#endif
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include "mea_verbose.h"
#include <string.h>
#include <pthread.h>

#include "globals.h"
#include "consts.h"
#include "mea_queue.h"
#include "tokens.h"
#include "configuration.h"

#include "pythonPluginServer.h"
#include "processManager.h"

// #define free(x) free(x);

#include "mea_api.h"

char *pythonPlugin_server_name_str="PYTHONPLUGINSERVER";

char *plugin_path=NULL;

// globales pour le fonctionnement du thread
pthread_t *_pythonPluginServer_thread_id=NULL;
int _pythonPluginServer_thread_is_running=0;
int _pythonPluginServer_monitoring_id=-1;

mea_queue_t *pythonPluginCmd_queue;
pthread_cond_t pythonPluginCmd_queue_cond;
pthread_mutex_t pythonPluginCmd_queue_lock;

PyObject *known_modules;

long nbpycall_indicator = 0;
long nbpycallerr_indicator = 0;


static unsigned long _millis()
{
  struct timeval tv;
  gettimeofday(&tv,NULL);

  return 1000 * tv.tv_sec + tv.tv_usec/1000;
}


static unsigned long _diffMillis(unsigned long chrono, unsigned long now)
{
   return now >= chrono ? now - chrono : ULONG_MAX - chrono + now;
}


void set_pythonPluginServer_isnt_running(void *data)
{
   _pythonPluginServer_thread_is_running=0;
}


void _pythonPlugin_thread_cleanup_PyEval_AcquireLock(void *arg)
{
   PyThreadState *tempState=(PyThreadState *)arg;
   
   if(tempState)
      PyThreadState_Swap(tempState);
   PyEval_ReleaseLock();
}


void setPythonPluginPath(char *path)
{
   if(plugin_path) {
      free(plugin_path);
      plugin_path=NULL;
   }
   plugin_path=malloc(strlen(path)+1);
   
   strcpy(plugin_path, path);
}


mea_error_t pythonPluginServer_add_cmd(char *module, void *data, int l_data)
{
   pythonPlugin_cmd_t *e=NULL;
   int ret=NOERROR;
   e=(pythonPlugin_cmd_t *)malloc(sizeof(pythonPlugin_cmd_t));
   if(!e)
      return ERROR;
   e->python_module=NULL;
   e->data=NULL;
   e->l_data=0;
   
   e->python_module=(char *)malloc(strlen(module)+1);
   if(!e->python_module)
      goto exit_pythonPluginServer_add_cmd;
   strcpy(e->python_module, module);

   e->data=(char *)malloc(l_data);
   if(!e->data)
      goto exit_pythonPluginServer_add_cmd;
   memcpy(e->data, data, l_data);
   
   e->l_data=l_data;
   
   pthread_cleanup_push((void *)pthread_mutex_unlock, (void *)&pythonPluginCmd_queue_lock);
   pthread_mutex_lock(&pythonPluginCmd_queue_lock);
   if(pythonPluginCmd_queue) {
      mea_queue_in_elem(pythonPluginCmd_queue, e);
      if(pythonPluginCmd_queue->nb_elem>=1)
         pthread_cond_broadcast(&pythonPluginCmd_queue_cond);
   }
   else
      ret=ERROR;
   pthread_mutex_unlock(&pythonPluginCmd_queue_lock);
   pthread_cleanup_pop(0);

   if(ret!=ERROR)
      return NOERROR;
   
exit_pythonPluginServer_add_cmd:
   if(e) {
      if(e->python_module) {
         free(e->python_module);
         e->python_module=NULL;
      }
      if(e->data) {
         free(e->data);
         e->data=NULL;
      }
      e->l_data=0;
      free(e);
      e=NULL;
   }
   return ERROR;
}


mea_error_t call_pythonPlugin(char *module, int type, PyObject *data_dict)
{
   PyObject *pName, *pModule, *pFunc;
   PyObject *pArgs, *pValue;

   PyErr_Clear();

   pName = PyString_FromString(module);
   pModule = PyDict_GetItem(known_modules, pName);
   if(!pModule) {
      pModule = PyImport_Import(pName);
      if(pModule) {
         PyDict_SetItem(known_modules, pName, pModule);
      }
      else {
         VERBOSE(5) {
            mea_log_printf("%s (%s) : module %s not loaded - \n", ERROR_STR, __func__, module);
            PyErr_Print();
         }
         return NOERROR;
      }
   }
   else {
      Py_INCREF(pModule);
      
      char str_module_py[255];
      
      strcpy(str_module_py,plugin_path);
      strcat(str_module_py,"/");
      strcat(str_module_py,module);
      strcat(str_module_py,".");
      strcat(str_module_py,"reload");
     
      int ret=unlink(str_module_py);
      if(!ret) {
         PyObject *m = pModule;
         pModule = PyImport_ReloadModule(m); // possible fute ici voir l'utilisation de Reload
         if(pModule) {
            Py_XDECREF(m);
            PyDict_SetItem(known_modules, pName, pModule);
         }
         else {
            VERBOSE(5) {
               mea_log_printf("%s (%s) : python error - ", ERROR_STR, __func__ );
               PyErr_Print();
               fprintf(MEA_STDERR, "\n");
            }
         }
      }
      else {
         if(errno!=ENOENT)
            perror("");
      }
   }
   
   Py_DECREF(pName);

   int return_code = 0;
   char *fx;
   if (pModule != NULL) {
      switch(type) {
         case XBEEDATA:
            fx="mea_dataFromSensor";
            break;
         case XPLMSG:
            fx="mea_xplCmndMsg";
            break;
         case COMMISSIONNING:
            fx="mea_commissionningRequest";
            break;
//         case ENOCEANDATA:
//            fx="mea_enoceanData";
//            break;
//         case GENERICSERIALDATA:
//            fx="mea_serialData";
//            break;
         case DATAFROMSENSOR:
            fx="mea_dataFromSensor";
            break;
         default:
            return NOERROR;
      }
      pFunc = PyObject_GetAttrString(pModule, fx);

      if (pFunc && PyCallable_Check(pFunc)) {
         unsigned long chrono=_millis();
         pArgs = PyTuple_New(1);
         // data_dict
         Py_INCREF(data_dict); // incrément car PyTuple_SetItem vole la référence
         PyTuple_SetItem(pArgs, 0, data_dict);
         pValue = PyObject_CallObject(pFunc, pArgs);
         Py_DECREF(pArgs);

         process_update_indicator(_pythonPluginServer_monitoring_id, "PYCALL", ++nbpycall_indicator);

         if (pValue != NULL) {
            unsigned long exectime=_diffMillis(chrono, _millis());
            DEBUG_SECTION mea_log_printf("%s (%s) : result of call of %s : %ld (%ld)\n", DEBUG_STR, __func__, fx, PyInt_AsLong(pValue), exectime);
            Py_DECREF(pValue); // verifier si nécessaire
         }
         else {
            VERBOSE(5) {
               mea_log_printf("%s (%s) : python error - ", ERROR_STR, __func__ );
               PyErr_Print();
               fprintf(MEA_STDERR, "\n");
            }
            process_update_indicator(_pythonPluginServer_monitoring_id, "PYCALLERR", ++nbpycallerr_indicator);
            return_code=ERROR;
         }
      }
      else {
         if (PyErr_Occurred()) {
            VERBOSE(5) {
               mea_log_printf("%s (%s) : python error - (%s/%s) - ", ERROR_STR, __func__, module, fx);
               PyErr_Print();
               fprintf(MEA_STDERR, "\n");
            }
            PyErr_Clear();
            return_code=ERROR;
            goto call_pythonPlugin_clean_exit;
         }
         else {
            VERBOSE(5) {
               mea_log_printf("%s (%s) : python unknown error\n", ERROR_STR, __func__);
            }
            return_code=ERROR;
            goto call_pythonPlugin_clean_exit;
         }
      }
   }
   else {
      VERBOSE(5) {
         mea_log_printf("%s (%s) : 4-python error - ", ERROR_STR, __func__);
         PyErr_Print();
         fprintf(MEA_STDERR, "\n");
      }
      return ERROR;
   }
   
call_pythonPlugin_clean_exit:
   Py_XDECREF(pFunc);
   Py_XDECREF(pModule);

   return return_code;
}


void _pythonPluginServer_clean_threadState(void *data)
{
   Py_DECREF(known_modules);
   mea_api_release();
   
   if(data) {
      PyThreadState *myThreadState = (PyThreadState *)data;
      PyEval_AcquireLock();
      PyThreadState_Clear(myThreadState);
      PyThreadState_Delete(myThreadState);
      PyEval_ReleaseLock();
   }
}


void *_pythonPlugin_thread(void *data)
{
   int ret;
   
   pythonPlugin_cmd_t *e=NULL;
   PyThreadState *mainThreadState, *myThreadState=NULL;
   
   pthread_cleanup_push( (void *)_pythonPluginServer_clean_threadState, (void *)myThreadState );
   pthread_cleanup_push( (void *)set_pythonPluginServer_isnt_running, (void *)NULL );

   _pythonPluginServer_thread_is_running=1;
   process_heartbeat(_pythonPluginServer_monitoring_id);

   pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
   PyEval_AcquireLock();
   mainThreadState = PyThreadState_Get();
   myThreadState = PyThreadState_New(mainThreadState->interp);
   PyEval_ReleaseLock();
   pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
   pthread_testcancel();

   // chemin vers les plugins rajoutés dans le path de l'interpréteur Python
   PyObject* sysPath = PySys_GetObject((char*)"path");
   PyObject* pluginsDir = PyString_FromString(plugin_path);
   PyList_Append(sysPath, pluginsDir);
   Py_DECREF(pluginsDir);
   
   known_modules=PyDict_New(); // initialisation du cache de module
   
   mea_api_init(); // initialisation du module mea mis à disposition du plugin

   int16_t pass;
   
   while(1) {
      process_heartbeat(_pythonPluginServer_monitoring_id);
      pthread_cleanup_push((void *)pthread_mutex_unlock, (void *)&pythonPluginCmd_queue_lock);
      pthread_mutex_lock(&pythonPluginCmd_queue_lock);
   
      pass=0; // pour faire en sorte de n'avoir qu'un seul pthread_mutex_unlock en face du pthread_mutex_lock ci-dessus
//      ret=0;
      if(pythonPluginCmd_queue && pythonPluginCmd_queue->nb_elem==0) {
         struct timeval tv;
         struct timespec ts;
         gettimeofday(&tv, NULL);
         ts.tv_sec = tv.tv_sec + 10; // timeout de 10 secondes
         ts.tv_nsec = 0;
         
         ret=pthread_cond_timedwait(&pythonPluginCmd_queue_cond, &pythonPluginCmd_queue_lock, &ts);
         if(ret!=0) {
            if(ret==ETIMEDOUT) {
               pass=1;
            }
            else {
               // autres erreurs à traiter
               DEBUG_SECTION mea_log_printf("%s (%s) : pthread_cond_timedwait error - ",DEBUG_STR, __func__);
               perror("");
               pass=1;
            }
         }
      }
      
      if (pythonPluginCmd_queue && !pass) // pas d'erreur, on récupère un élément dans la queue
         ret=mea_queue_out_elem(pythonPluginCmd_queue, (void **)&e);
      else
         ret=-1;

      pthread_mutex_unlock(&pythonPluginCmd_queue_lock);
      pthread_cleanup_pop(0);

      if (pass) { // une erreur, retour au début de la boucle
         pthread_testcancel();
         continue;
      }
      
      if(!ret) { // on a sortie un élément de la queue
         plugin_queue_elem_t *data = (plugin_queue_elem_t *)e->data;

         PyThreadState *tempState=NULL;
         pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
         PyEval_AcquireLock();
         tempState = PyThreadState_Swap(myThreadState);
         
         PyObject *pydict_data=data->aDict;

         call_pythonPlugin(e->python_module, data->type_elem, pydict_data);
         
         Py_DECREF(pydict_data);

         PyThreadState_Swap(tempState);
         PyEval_ReleaseLock();
         pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

         if(e) {
            if(e->python_module) {
               free(e->python_module);
               e->python_module=NULL;
            }
            if(e->data) {
               free(e->data);
               e->data=NULL;
            }
            e->l_data=0;
            free(e);
            e=NULL;
         }
      }
      else {
         // pb d'accés aux données de la file
         VERBOSE(9) mea_log_printf("%s (%s) : mea_queue_out_elem - can't access queue element\n", ERROR_STR, __func__);
      }
      
      pthread_testcancel();
   }
   
   pthread_cleanup_pop(1);
   pthread_cleanup_pop(1);

   pthread_exit(NULL);
   
   return NULL;
}


pthread_t *pythonPluginServer()
{
   int py_init_flag=0;
   pthread_t *pythonPlugin_thread=NULL;
   
   pythonPluginCmd_queue=(mea_queue_t *)malloc(sizeof(mea_queue_t));
   if(!pythonPluginCmd_queue) {
      VERBOSE(1) {
         mea_log_printf("%s (%s) : %s - ", ERROR_STR, __func__, MALLOC_ERROR_STR);
         perror("");
      }
      return NULL;
   }
   mea_queue_init(pythonPluginCmd_queue);
   pthread_mutex_init(&pythonPluginCmd_queue_lock, NULL);
   pthread_cond_init(&pythonPluginCmd_queue_cond, NULL);
 
   pythonPlugin_thread=(pthread_t *)malloc(sizeof(pthread_t));
   if(!pythonPlugin_thread) {
      VERBOSE(1) {
         mea_log_printf("%s (%s) : %s - ",FATAL_ERROR_STR, __func__, MALLOC_ERROR_STR);
         perror("");
      }
      goto pythonPluginServer_clean_exit;
   }
   
   Py_Initialize();
   PyEval_InitThreads(); // voir ici http://www.codeproject.com/Articles/11805/Embedding-Python-in-C-C-Part-I
   PyEval_ReleaseLock();
   py_init_flag=1;

   if(pthread_create (pythonPlugin_thread, NULL, _pythonPlugin_thread, NULL)) {
      VERBOSE(1) mea_log_printf("%s (%s) : pthread_create - can't start thread - ", FATAL_ERROR_STR, __func__);
      perror("");
      goto pythonPluginServer_clean_exit;
   }
   fprintf(stderr,"PYTHONPLUGINSERVER : %x\n", (unsigned int)*pythonPlugin_thread);

   pthread_detach(*pythonPlugin_thread);
   
   if(pythonPlugin_thread)   
      return pythonPlugin_thread;

pythonPluginServer_clean_exit:
   if(pythonPlugin_thread) {
      free(pythonPlugin_thread);
      pythonPlugin_thread=NULL;
   }

   if(pythonPluginCmd_queue) {
      free(pythonPluginCmd_queue);
      pythonPluginCmd_queue=NULL;
   }

   if(py_init_flag) {
      PyEval_AcquireLock();
      Py_Finalize();
   }
   return NULL;
}


void pythonPluginCmd_queue_free_queue_elem(void *d)
{
   pythonPlugin_cmd_t *e=(pythonPlugin_cmd_t *)d;

   free(e->python_module);
   e->python_module=NULL;
   free(e->data);
   e->data=NULL;
} 


int stop_pythonPluginServer(int my_id, void *data, char *errmsg, int l_errmsg)
{
   if(_pythonPluginServer_thread_id) {
      pthread_cancel(*_pythonPluginServer_thread_id);
      int counter=100;
//      int stopped=-1;
      while(counter--)
      {
         if(_pythonPluginServer_thread_is_running) {
            usleep(100); // will sleep for 1 ms
         }
         else {
//            stopped=0;
            break;
         }
      }
      DEBUG_SECTION mea_log_printf("%s (%s) : %s, fin après %d itération(s)\n",DEBUG_STR, __func__, pythonPlugin_server_name_str, 100-counter);

      
      free(_pythonPluginServer_thread_id);
      _pythonPluginServer_thread_id=NULL;
   }

   pthread_cleanup_push((void *)pthread_mutex_unlock, (void *)&pythonPluginCmd_queue_lock);
   pthread_mutex_lock(&pythonPluginCmd_queue_lock);
   if(pythonPluginCmd_queue) {
      mea_queue_cleanup(pythonPluginCmd_queue, pythonPluginCmd_queue_free_queue_elem);
      free(pythonPluginCmd_queue);
      pythonPluginCmd_queue=NULL;
   }
   pthread_mutex_unlock(&pythonPluginCmd_queue_lock);
   pthread_cleanup_pop(0);

   PyEval_AcquireLock();
   Py_Finalize();

   _pythonPluginServer_monitoring_id=-1;

   VERBOSE(1) mea_log_printf("%s (%s) : %s %s.\n", INFO_STR, __func__, pythonPlugin_server_name_str, stopped_successfully_str);

   return 0;
}


int start_pythonPluginServer(int my_id, void *data, char *errmsg, int l_errmsg)
{
   struct pythonPluginServer_start_stop_params_s *pythonPluginServer_start_stop_params = (struct pythonPluginServer_start_stop_params_s *)data;

   char err_str[80], notify_str[256];

   if(appParameters_get("PLUGINPATH", pythonPluginServer_start_stop_params->params_list)) {
      setPythonPluginPath(appParameters_get("PLUGINPATH", pythonPluginServer_start_stop_params->params_list));

      _pythonPluginServer_thread_id=pythonPluginServer();
      if(_pythonPluginServer_thread_id==NULL) {
#ifdef _POSIX_SOURCE
         char *ret;
#else
         int ret;
#endif
         ret=strerror_r(errno, err_str, sizeof(err_str));
         VERBOSE(1) {
            mea_log_printf("%s (%s) : can't start %s (thread error) - %s\n", ERROR_STR, __func__, pythonPlugin_server_name_str, notify_str);
         }

         return -1;
      }
      _pythonPluginServer_monitoring_id=my_id;
   }
   else {
      VERBOSE(1) {
         mea_log_printf("%s (%s) : can't start %s (incorrect plugin path).\n", ERROR_STR, __func__, pythonPlugin_server_name_str);
      }
      return -1;
   }

   VERBOSE(2) mea_log_printf("%s (%s) : %s %s.\n", INFO_STR, __func__, pythonPlugin_server_name_str, launched_successfully_str);

   return 0;
}


int restart_pythonPluginServer(int my_id, void *data, char *errmsg, int l_errmsg)
{
   int ret=0;
   ret=stop_pythonPluginServer(my_id, data, errmsg, l_errmsg);
   if(ret==0) {
      return start_pythonPluginServer(my_id, data, errmsg, l_errmsg);
   }
   return ret;
}