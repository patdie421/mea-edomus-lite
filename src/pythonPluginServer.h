//
//  pythonPlugin.h
//
//  Created by Patrice Dietsch on 04/05/13.
//
//

#ifndef __pythonPluginServer_h
#define __pythonPluginServer_h
#include <Python.h>

#include <pthread.h>
#include <sqlite3.h>


#define DEBUG_PyEval_AcquireLock(id, last_time) { \
   printf("CHRONO : Demande Lock par %s a %u ms\n",(id),start_chrono((last_time))); \
   PyEval_AcquireLock(); \
   fprintf(stderr,"CHRONO : Lock obtenu par %s apres %u ms\n",(id),take_chrono((last_time))); \
}

#define DEBUG_PyEval_ReleaseLock(id, last_time) { \
   PyEval_ReleaseLock(); \
   fprintf(stderr,"CHRONO : Liberation Lock par %s apres %u\n",(id), take_chrono((last_time))); \
}


//typedef enum {XBEEDATA=1, XPLMSG=2, COMMISSIONNING=3, ENOCEANDATA=4, GENERICSERIALDATA=5, DATAFROMSENSOR=6 } pythonPlugin_type;
typedef enum {XBEEDATA=1, XPLMSG=2, COMMISSIONNING=3, DATAFROMSENSOR=6 } pythonPlugin_type;


struct pythonPluginServer_start_stop_params_s
{
   cJSON *params_list;
   sqlite3 *sqlite3_param_db;
};


typedef struct pythonPlugin_cmd_s
{
   char *python_module;
   char *data;
   int  l_data;
} pythonPlugin_cmd_t;


typedef struct plugin_queue_elem_s
{
   pythonPlugin_type type_elem;
   PyObject *aDict;
   char buff[128];
   uint16_t l_buff;
} plugin_queue_elem_t;


mea_error_t pythonPluginServer_add_cmd(char *module, void *data, int l_data);
void        setPythonPluginPath(char *path);
int         start_pythonPluginServer(int my_id, void *data, char *errmsg, int l_errmsg);
int         stop_pythonPluginServer(int my_id, void *data, char *errmsg, int l_errmsg);
int         restart_pythonPluginServer(int my_id, void *data, char *errmsg, int l_errmsg);

#endif
