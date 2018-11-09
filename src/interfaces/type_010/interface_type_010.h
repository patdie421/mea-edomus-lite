//
//  interface_type_010.h
//  mea-edomus
//
//  Created by Patrice Dietsch on 21/02/2015.
//
//
#ifndef __interface_type_010_h
#define __interface_type_010_h

#include <signal.h>
#include <Python.h>
#include <sqlite3.h>

#include "interfacesServer.h"
//DBSERVER #include "dbServer.h"
#include "xPLServer.h"
#include "pythonPluginServer.h"

#define INTERFACE_TYPE_010 3010

#define INIT_LINE_BUFFER_SIZE 10
#define INC_LINE_BUFFER_SIZE  10

struct interface_type_010_indicators_s
{
   uint32_t xplin;
   uint32_t senttoplugin;
};

extern char *interface_type_010_xplin_str;

enum file_type_e { FT_PIPE=1 };
enum direction_e { DIR_IN=1, DIR_OUT, DIR_BOTH };

typedef struct interface_type_010_s
{
   int              id_interface;
   int              id_driver;
   char             name[41];
   char             dev[81];
   int              monitoring_id;
   pthread_t       *thread;
   volatile sig_atomic_t thread_is_running;
   xpl2_f           xPL_callback2;
   void            *xPL_callback_data;
   char            *parameters;

   int              fduration;
   char            *fendstr;
   char            *fstartstr;
   int              fsize;

   enum direction_e direction;
   enum file_type_e file_type;
   char            *file_name;
   char            *in_ext, *out_ext;
   int              file_desc_in;
   int              file_desc_out;

   char            *line_buffer;
   int              line_buffer_l;
   int              line_buffer_ptr;
 
   struct interface_type_010_indicators_s indicators;

   PyThreadState   *mainThreadState;
   PyThreadState   *myThreadState;

   PyObject        *pModule;
   PyObject        *pFunc;
   PyObject        *pParams;
 
} interface_type_010_t;


struct interface_type_010_data_s
{
   interface_type_010_t *i010;
   sqlite3 *sqlite3_param_db;
};


xpl2_f  get_xPLCallback_interface_type_010(void *ixxx);
int     get_monitoring_id_interface_type_010(void *ixxx);
int     set_xPLCallback_interface_type_010(void *ixxx, xpl2_f cb);
int     set_monitoring_id_interface_type_010(void *ixxx, int id);
int     get_type_interface_type_010(void);

int     clean_interface_type_010(void *ixxx);
interface_type_010_t *malloc_and_init_interface_type_010(sqlite3 *sqlite3_param_db, int id_driver, int id_interface, char *name, char *dev, char *parameters, char *description);

int16_t api_interface_type_010(void *ixxx, char *cmnd, void *args, int nb_args, void **res, int16_t *nerr, char *err, int l_err);

int     get_fns_interface_type_010(struct interfacesServer_interfaceFns_s *interfacesFns);

#endif
