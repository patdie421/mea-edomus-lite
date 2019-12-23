//
//  pythonPlugin.h
//
//  Created by Patrice Dietsch on 04/05/13.
//
//
#ifndef __pythonPluginServer_h
#define __pythonPluginServer_h
#ifdef __APPLE__
#include <Python/Python.h>
#else
#include <Python.h>
#endif

#include <pthread.h>

#include "cJSON.h"
#include "mea_error.h"

/*
#define DEBUG_PyEval_AcquireLock(id, last_time) { \
   printf("CHRONO : Demande Lock par %s a %u ms\n",(id),start_chrono((last_time))); \
   PyEval_AcquireLock(); \
   fprintf(stderr,"CHRONO : Lock obtenu par %s apres %u ms\n",(id),take_chrono((last_time))); \
}

#define DEBUG_PyEval_ReleaseLock(id, last_time) { \
   PyEval_ReleaseLock(); \
   fprintf(stderr,"CHRONO : Liberation Lock par %s apres %u\n",(id), take_chrono((last_time))); \
}
*/

//typedef enum {XBEEDATA=1, XPLMSG=2, COMMISSIONNING=3, ENOCEANDATA=4, GENERICSERIALDATA=5, DATAFROMSENSOR=6 } pythonPlugin_type;
typedef enum {XBEEDATA=1, XPLMSG=2, COMMISSIONNING=3, DATAFROMSENSOR=6, XPLMSG_JSON=12, DATAFROMSENSOR_JSON=16, CUSTOM_JSON=30 } pythonPlugin_type;


struct pythonPluginServer_start_stop_params_s
{
   cJSON *params_list;
};


typedef struct pythonPlugin_cmd_s
{
   char *python_module;
   char *python_function;
   char *data;
   int  l_data;
   cJSON **result;
   pthread_cond_t *exec_cond;
   pthread_mutex_t *exec_lock;
   int reload_flag;
} pythonPlugin_cmd_t;


typedef struct plugin_queue_elem_s
{
   pythonPlugin_type type_elem;
   PyObject *aDict;
   cJSON *aJsonDict;
   char buff[256];
   uint16_t l_buff;
} plugin_queue_elem_t;


mea_error_t pythonPluginServer_add_cmd(char *module, void *data, int l_data);
cJSON      *pythonPluginServer_exec_cmd(char *module, char *function, void *data, int l_data, int reload_flag);

void        setPythonPluginPath(char *path);
int         start_pythonPluginServer(int my_id, void *data, char *errmsg, int l_errmsg);
int         stop_pythonPluginServer(int my_id, void *data, char *errmsg, int l_errmsg);
int         restart_pythonPluginServer(int my_id, void *data, char *errmsg, int l_errmsg);

#endif
