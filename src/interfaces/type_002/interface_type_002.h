/*
 *
 *  Created by Patrice Dietsch on 24/08/12.
 *  Copyright 2012 -. All rights reserved.
 *
 */

#ifndef __interface_type_002_h
#define __interface_type_002_h

#include <signal.h>
/*
#ifdef __APPLE__
#include <Python/Python.h>
#else
#include <Python.h>
#endif
*/
#include "mea_verbose.h"
#include "xbee.h"

#include "interfacesServer.h"
#include "xPLServer.h"
#include "pythonPluginServer.h"

#define INTERFACE_TYPE_002 200

typedef struct commissionning_queue_elem_s
{
   unsigned char addr_64_h[4];
   unsigned char addr_64_l[4];
   char          host_name[21];
   unsigned char node_type;
} commissionning_queue_elem_t;


typedef struct data_queue_elem_s
{
   unsigned char  addr_64_h[4];
   unsigned char  addr_64_l[4];
   unsigned char *cmd;
   int            l_cmd;
   struct timeval tv;
} data_queue_elem_t;

struct interface_type_002_indicators_s
{
   uint32_t senttoplugin;
   uint32_t xplin;
   uint32_t xbeedatain;
   uint32_t commissionning_request;
};

extern char *interface_type_002_senttoplugin_str;
extern char *interface_type_002_xplin_str;
extern char *interface_type_002_xbeedatain_str;
extern char *interface_type_002_commissionning_request_str;

typedef struct interface_type_002_s
{
   int              id_interface;
   char             name[41];
   char             dev[81];
   int              monitoring_id;
   xbee_xd_t       *xd;
   xbee_host_t     *local_xbee;
   pthread_t       *thread;
   volatile sig_atomic_t thread_is_running;
   xpl2_f           xPL_callback2;
   void            *xPL_callback_data;
   char            *parameters;
   
   struct interface_type_002_indicators_s indicators;
} interface_type_002_t;


struct interface_type_002_data_s
{
   interface_type_002_t *i002;
};  


#define PLUGIN_DATA_MAX_SIZE 81


typedef struct plugin_commissionning_queue_elem_s
{
   PyObject      *parameters;
} plugin_commissionning_queue_elem_t;


xpl2_f get_xPLCallback_interface_type_002(void *ixxx);
int get_monitoring_id_interface_type_002(void *ixxx);
int set_xPLCallback_interface_type_002(void *ixxx, xpl2_f cb);
int set_monitoring_id_interface_type_002(void *ixxx, int id);
int get_type_interface_type_002(void);

int clean_interface_type_002(interface_type_002_t *i002);

int get_fns_interface_type_002(struct interfacesServer_interfaceFns_s *interfacesFns);

#endif
