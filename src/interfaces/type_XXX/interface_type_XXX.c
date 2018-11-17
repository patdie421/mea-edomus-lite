//
//  interface_type_XXX.c
//  mea-edomus
//
//  Created by Patrice Dietsch on 21/02/2015.
//
//
#include <Python.h>

#include "interface_type_XXX.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <sqlite3.h>
#include <errno.h>
#include <pthread.h>

#include "globals.h"
#include "consts.h"
#include "tokens.h"
#include "tokens_da.h"
#include "mea_verbose.h"
#include "macros.h"
#include "mea_string_utils.h"
#include "dbServer.h"
#include "parameters_utils.h"
#include "cJSON.h"
#include "processManager.h"
#include "notify.h"

#include "interfacesServer.h"


char *interface_type_XXX_xplin_str="XPLIN";


struct callback_xpl_data_s
{
};


struct thread_params_s
{
   sqlite3              *param_db;
   interface_type_XXX_t *iXXX;
};


typedef void (*thread_f)(void *);


int start_interface_type_XXX(int my_id, void *data, char *errmsg, int l_errmsg);
int stop_interface_type_XXX(int my_id, void *data, char *errmsg, int l_errmsg);
int restart_interface_type_XXX(int my_id, void *data, char *errmsg, int l_errmsg);


void set_interface_type_XXX_isnt_running(void *data)
{
   interface_type_XXX_t *iXXX = (interface_type_XXX_t *)data;

   iXXX->thread_is_running=0;
}


int16_t _interface_type_XXX_xPL_callback2(cJSON *xplMsgJson, struct device_info_s *device_info, void *userValue)
{
   char *device = NULL;
   int ret = -1;
   int err = 0;

   interface_type_XXX_t *interface=(interface_type_XXX_t *)userValue;
   struct callback_xpl_data_s *params=(struct callback_xpl_data_s *)interface->xPL_callback_data;

   interface->indicators.xplin++;

   return 0;
}


int update_devices_type_XXX(void *ixxx)
{
   printf("update devices type XXX\n");

   return 0;
}


int clean_interface_type_XXX(void *ixxx)
{
   interface_type_XXX_t *iXXX = (interface_type_XXX_t *)ixxx;

   if(iXXX->parameters)
   {
      free(iXXX->parameters);
      iXXX->parameters=NULL;
   }

   if(iXXX->xPL_callback_data)
   {
      free(iXXX->xPL_callback_data);
      iXXX->xPL_callback_data=NULL;
   }

   if(iXXX->xPL_callback2)
      iXXX->xPL_callback2=NULL;

   if(iXXX->thread)
   {
      free(iXXX->thread);
      iXXX->thread=NULL;
   }

   return 0;
}


xpl2_f get_xPLCallback_interface_type_XXX(void *ixxx)
{
   interface_type_XXX_t *iXXX = (interface_type_XXX_t *)ixxx;

   if(iXXX == NULL)
      return NULL;
   else
   {
      return iXXX->xPL_callback2;
   }
}


int get_monitoring_id_interface_type_XXX(void *ixxx)
{
   interface_type_XXX_t *iXXX = (interface_type_XXX_t *)ixxx;

   if(iXXX == NULL)
      return -1;
   else
      return iXXX->monitoring_id;
}


int set_xPLCallback_interface_type_XXX(void *ixxx, xpl2_f cb)
{
   interface_type_XXX_t *iXXX = (interface_type_XXX_t *)ixxx;

   if(iXXX == NULL)
      return -1;
   else
   {
      iXXX->xPL_callback2 = cb;
      return 0;
   }
}


int set_monitoring_id_interface_type_XXX(void *ixxx, int id)
{
   interface_type_XXX_t *iXXX = (interface_type_XXX_t *)ixxx;

   if(iXXX == NULL)
      return -1;
   else
   {
      iXXX->monitoring_id = id;
      return 0;
   }
}


int get_type_interface_type_XXX()
{
   return INTERFACE_TYPE_XXX;
}


int get_interface_id_interface_type_XXX(void *ixxx)
{
   interface_type_XXX_t *iXXX = (interface_type_XXX_t *)ixxx;

   if(iXXX == NULL)
      return -1;
   else
      return iXXX->id_interface;
}


int16_t api_interface_type_XXX(void *ixxx, char *cmnd, void *args, int nb_args, void **res, int16_t *nerr, char *err, int l_err)
{
   interface_type_XXX_t *iXXX = (interface_type_XXX_t *)ixxx;
   PyObject *pyArgs = (PyObject *)args;
   PyObject **pyRes = (PyObject **)res;
   
   if(strcmp(cmnd, "test") == 0)
   {
      *res = PyString_FromString("New style Api call OK !!!");
      *nerr=0;
      strncpy(err, "no error", l_err);

      return 0;
   }
   else
   {
      strncpy(err, "unknown function", l_err);

      return -254;
   }

   return -1;
}


void *_thread_interface_type_XXX_cleanup(void *args)
{
   struct thread_params_s *params=(struct thread_params_s *)args;

   if(!params)
      return NULL;

   return NULL;
}


interface_type_XXX_t *malloc_and_init_interface_type_XXX(sqlite3 *sqlite3_param_db, int id_driver, int id_interface, char *name, char *dev, char *parameters, char *description)
{
   interface_type_XXX_t *iXXX;
                  
   iXXX=(interface_type_XXX_t *)malloc(sizeof(interface_type_XXX_t));
   if(!iXXX)
   {
      VERBOSE(2) {
        mea_log_printf("%s (%s) : %s - ", ERROR_STR, __func__, MALLOC_ERROR_STR);
        perror("");
      }
      return NULL;
   }
   iXXX->thread_is_running=0;
                  
   struct interface_type_XXX_data_s *iXXX_start_stop_params=(struct interface_type_XXX_data_s *)malloc(sizeof(struct interface_type_XXX_data_s));
   if(!iXXX_start_stop_params)
   {
      free(iXXX);
      iXXX=NULL;

      VERBOSE(2) {
         mea_log_printf("%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
         perror("");
      }  
      return NULL;
   }

   strncpy(iXXX->dev, (char *)dev, sizeof(iXXX->dev)-1);
   strncpy(iXXX->name, (char *)name, sizeof(iXXX->name)-1);
   iXXX->id_interface=id_interface;
   iXXX->id_driver=id_driver;
   iXXX->parameters=(char *)malloc(strlen((char *)parameters)+1);
   strcpy(iXXX->parameters,(char *)parameters);

   iXXX->indicators.xplin=0;

   iXXX->thread=NULL;
   iXXX->xPL_callback2=NULL;
   iXXX->xPL_callback_data=NULL;
   iXXX->monitoring_id=process_register((char *)name);

   iXXX_start_stop_params->sqlite3_param_db = sqlite3_param_db;
   iXXX_start_stop_params->iXXX=iXXX;
                  
   process_set_group(iXXX->monitoring_id, 1);
   process_set_start_stop(iXXX->monitoring_id, start_interface_type_XXX, stop_interface_type_XXX, (void *)iXXX_start_stop_params, 1);
   process_set_watchdog_recovery(iXXX->monitoring_id, restart_interface_type_XXX, (void *)iXXX_start_stop_params);
   process_set_description(iXXX->monitoring_id, (char *)description);
   process_set_heartbeat_interval(iXXX->monitoring_id, 60); // chien de garde au bout de 60 secondes sans heartbeat

   process_add_indicator(iXXX->monitoring_id, interface_type_XXX_xplin_str, 0);

   return iXXX;
}


void *_thread_interface_type_XXX(void *args)
{
   struct thread_params_s *params=(struct thread_params_s *)args;

   pthread_cleanup_push( (void *)_thread_interface_type_XXX_cleanup, (void *)params );
   pthread_cleanup_push( (void *)set_interface_type_XXX_isnt_running, (void *)params->iXXX );

   params->iXXX->thread_is_running=1;
   process_heartbeat(params->iXXX->monitoring_id);

   sqlite3 *params_db=params->param_db;
   int ret;

   while(1)
   {
      process_heartbeat(params->iXXX->monitoring_id);
      process_update_indicator(params->iXXX->monitoring_id, interface_type_XXX_xplin_str, params->iXXX->indicators.xplin);

      sleep(1);

      // traiter les données en provenance des périphériques
      fprintf(stderr, "BOUCLE\n");

      pthread_testcancel();
   }

   pthread_cleanup_pop(1);
   pthread_cleanup_pop(1);
}


pthread_t *start_interface_type_XXX_thread(interface_type_XXX_t *iXXX, void *fd, sqlite3 *db, thread_f thread_function)
{
   pthread_t *thread=NULL;
   struct thread_params_s *thread_params=NULL;
   struct callback_data_s *callback_data=NULL;

   thread_params=malloc(sizeof(struct thread_params_s));
   if(!thread_params)
   {
      VERBOSE(2) {
         mea_log_printf("%s (%s) : %s - ", ERROR_STR, __func__, MALLOC_ERROR_STR);
         perror("");
      }
      goto clean_exit;
   }

   thread_params->param_db=db;
   thread_params->iXXX=(void *)iXXX;

   thread=(pthread_t *)malloc(sizeof(pthread_t));
   if(!thread)
   {
      VERBOSE(2) mea_log_printf("%s (%s) : %s\n", ERROR_STR, __func__, MALLOC_ERROR_STR);
      goto clean_exit;
   }

   if(pthread_create (thread, NULL, (void *)thread_function, (void *)thread_params))
      goto clean_exit;

   pthread_detach(*thread);

   return thread;

clean_exit:
   if(thread)
   {
      free(thread);
      thread=NULL;
   }

   if(thread_params)
   {
      free(thread_params);
      thread_params=NULL;
   }
   return NULL;
}


int stop_interface_type_XXX(int my_id, void *data, char *errmsg, int l_errmsg)
{
   if(!data)
      return -1;

   struct interface_type_XXX_data_s *start_stop_params=(struct interface_type_XXX_data_s *)data;

   VERBOSE(1) mea_log_printf("%s (%s) : %s shutdown thread ... ", INFO_STR, __func__, start_stop_params->iXXX->name);

   if(start_stop_params->iXXX->xPL_callback_data)
   {
      free(start_stop_params->iXXX->xPL_callback_data);
      start_stop_params->iXXX->xPL_callback_data=NULL;
   }

   if(start_stop_params->iXXX->xPL_callback2)
      start_stop_params->iXXX->xPL_callback2=NULL;

   if(start_stop_params->iXXX->thread)
   {
      pthread_cancel(*(start_stop_params->iXXX->thread));

      int counter=100;
      while(counter--)
      {
         if(start_stop_params->iXXX->thread_is_running)
         {
            usleep(100);
         }
         else
            break;
      }
      DEBUG_SECTION mea_log_printf("%s (%s) : %s, end after %d loop(s)\n", DEBUG_STR, __func__, start_stop_params->iXXX->name, 100-counter);

      free(start_stop_params->iXXX->thread);
      start_stop_params->iXXX->thread=NULL;
   }

   mea_notify_printf('S', "%s %s", start_stop_params->iXXX->name, stopped_successfully_str);

   return 0;
}


int restart_interface_type_XXX(int my_id, void *data, char *errmsg, int l_errmsg)
{
   process_stop(my_id, NULL, 0);
   sleep(5);
   return process_start(my_id, NULL, 0);
}


int start_interface_type_XXX(int my_id, void *data, char *errmsg, int l_errmsg)
{
   struct callback_xpl_data_s *xpl_callback_params=NULL;
   struct interface_type_XXX_data_s *start_stop_params=(struct interface_type_XXX_data_s *)data;

   char err_str[128];

   start_stop_params->iXXX->thread=NULL;

   start_stop_params->iXXX->thread=start_interface_type_XXX_thread(start_stop_params->iXXX, NULL, start_stop_params->sqlite3_param_db, (thread_f)_thread_interface_type_XXX);

   xpl_callback_params=(struct callback_xpl_data_s *)malloc(sizeof(struct callback_xpl_data_s)); 
   if(!xpl_callback_params)
   {
      strerror_r(errno, err_str, sizeof(err_str));
      VERBOSE(2) {
         mea_log_printf("%s (%s) : %s - %s\n", ERROR_STR, __func__, MALLOC_ERROR_STR, err_str);
      }
      mea_notify_printf('E', "%s can't be launched - %s.\n", start_stop_params->iXXX->name, err_str);
      goto clean_exit;
   }
//   xpl_callback_params->param_db=start_stop_params->sqlite3_param_db;

   start_stop_params->iXXX->xPL_callback_data=xpl_callback_params;
   start_stop_params->iXXX->xPL_callback2=_interface_type_XXX_xPL_callback2;

   return 0;

clean_exit:
   if(xpl_callback_params)
   {
      free(xpl_callback_params);
      xpl_callback_params=NULL;
   }

   return -1; 
}


#ifndef ASPLUGIN
int get_fns_interface_type_XXX(struct interfacesServer_interfaceFns_s *interfacesFns)
{
   interfacesFns->malloc_and_init = (malloc_and_init_f)&malloc_and_init_interface_type_XXX;
   interfacesFns->get_monitoring_id = (get_monitoring_id_f)&get_monitoring_id_interface_type_XXX;
   interfacesFns->get_xPLCallback = (get_xPLCallback_f)&get_xPLCallback_interface_type_XXX;
   interfacesFns->update_devices = (update_devices_f)&update_devices_type_XXX;
   interfacesFns->clean = (clean_f)&clean_interface_type_XXX;
   interfacesFns->set_monitoring_id = (set_monitoring_id_f)&set_monitoring_id_interface_type_XXX;
   interfacesFns->set_xPLCallback = (set_xPLCallback_f)&set_xPLCallback_interface_type_XXX;
   interfacesFns->get_type = (get_type_f)&get_type_interface_type_XXX;
//   interfacesFns->get_interface_id = (get_interface_id_f)&get_interface_id_interface_type_XXX;
   interfacesFns->api = (api_f)&api_interface_type_XXX;
   interfacesFns->lib = NULL;
   interfacesFns->type = interfacesFns->get_type();
   interfacesFns->plugin_flag = 0;

   return 0;
}
#endif
