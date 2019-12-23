//
//  interface_type_003.h
//  mea-edomus
//
//  Created by Patrice Dietsch on 21/02/2015.
//
//
#ifndef __interface_type_003_h
#define __interface_type_003_h
#include "pythonPluginServer.h"

#include <signal.h>

#include "interface_type_003.h"
#include "enocean.h"
#include "interfacesServer.h"
#include "xPLServer.h"
#include "cJSON.h"


#define INTERFACE_TYPE_003 300

typedef struct enocean_data_queue_elem_s {
   uint8_t  *data;
   uint16_t l_data;
   uint32_t enocean_addr;
   struct timeval tv;
} enocean_data_queue_elem_t;


struct interface_type_003_indicators_s {
   uint32_t senttoplugin;
   uint32_t xplin;
   uint32_t enoceandatain;
};

extern char *interface_type_003_senttoplugin_str;
extern char *interface_type_003_xplin_str;
extern char *interface_type_003_enoceandatain_str;

typedef struct interface_type_003_s {
   int              id_interface;
   int              id_driver;
   char             name[256];
   char             dev[256];
   int              monitoring_id;
   enocean_ed_t    *ed;
   pthread_t       *thread;
   volatile sig_atomic_t
                    thread_is_running;
   volatile sig_atomic_t
                    pairing_state;
   double           pairing_startms;
   xpl2_f           xPL_callback2;
   void            *xPL_callback_data;
   char            *parameters;
   
   struct interface_type_003_indicators_s indicators;
} interface_type_003_t;


struct interface_type_003_data_s {
   interface_type_003_t *i003;
};

extern char *valid_enocean_plugin_params[];
#define PLUGIN_PARAMS_PLUGIN      0
#define PLUGIN_PARAMS_PARAMETERS  1

#define PLUGIN_DATA_MAX_SIZE 81

xpl2_f get_xPLCallback_interface_type_003(void *ixxx);
int get_monitoring_id_interface_type_003(void *ixxx);
int set_xPLCallback_interface_type_003(void *ixxx, xpl2_f cb);
int set_monitoring_id_interface_type_003(void *ixxx, int id);
int get_type_interface_type_003(void);

// interface_type_003_t *malloc_and_init_interface_type_003(int id_driver, cJSON *jsonInterface);
int clean_interface_type_003(void *ixxx);
int16_t api_interface_type_003(void *ixxx, char *cmnd, void *args, int nb_args, void **res, int16_t *nerr, char *err, int l_err);

int get_fns_interface_type_003(struct interfacesServer_interfaceFns_s *interfacesFns);

#endif
