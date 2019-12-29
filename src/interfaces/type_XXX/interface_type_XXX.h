//
//  interface_type_XXX.h
//  mea-edomus
//
//  Created by Patrice Dietsch on 21/02/2015.
//
//
#ifndef __interface_type_XXX_h
#define __interface_type_XXX_h

#include <sqlite3.h>

#include "interfacesServer.h"
#include "xPLServer.h"
#include "pythonPluginServer.h"

#define INTERFACE_TYPE_XXX 3000

struct interface_type_XXX_indicators_s
{
   uint32_t xplin;
};

extern char *interface_type_XXX_xplin_str;

typedef struct interface_type_XXX_s
{
   int              id_interface;
   int              id_driver;
   char             name[256];
   char             dev[256];
   int              monitoring_id;
   pthread_t       *thread;
   volatile sig_atomic_t thread_is_running;
   xpl2_f           xPL_callback2;
   void            *xPL_callback_data;
   char            *parameters;
   
   struct interface_type_XXX_indicators_s indicators;
} interface_type_XXX_t;


struct interface_type_XXX_data_s
{
   interface_type_XXX_t *iXXX;
   sqlite3 *sqlite3_param_db;
};


xpl2_f get_xPLCallback_interface_type_XXX(void *ixxx);
int get_monitoring_id_interface_type_XXX(void *ixxx);
int set_xPLCallback_interface_type_XXX(void *ixxx, xpl2_f cb);
int set_monitoring_id_interface_type_XXX(void *ixxx, int id);
int get_type_interface_type_XXX(void);

interface_type_XXX_t *malloc_and_init_interface_type_XXX(sqlite3 *sqlite3_param_db, int id_driver, int id_interface, char *name, char *dev, char *parameters, char *description);
int clean_interface_type_XXX(void *ixxx);
int16_t api_interface_type_XXX(void *ixxx, char *cmnd, void *args, int nb_args, void **res, int16_t *nerr, char *err, int l_err);

int get_fns_interface_type_XXX(struct interfacesServer_interfaceFns_s *interfacesFns);

#endif
