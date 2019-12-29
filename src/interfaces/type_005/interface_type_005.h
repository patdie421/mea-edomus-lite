/*
 *  interface_type_005.h
 *
 *  Created by Patrice Dietsch on 27/02/15.
 *  Copyright 2012 -. All rights reserved.
 *
 */

#ifndef __interface_type_005_h
#define __interface_type_005_h

/*
#ifdef ASPLUGIN
#define NAME(f) f ## _ ## PLGN
#else
#define NAME(f) f
#endif
*/

#include <inttypes.h>
#include <pthread.h>
#include <signal.h>
#include "uthash.h"
#include "cJSON.h"

#include "interfacesServer.h"

#include "xPLServer.h"

#include "mea_queue.h"
#include "netatmo.h"

#define INTERFACE_TYPE_005 455

extern char *interface_type_005_xplin_str;
extern char *interface_type_005_xplout_str;
extern char *interface_type_005_nbreaderror_str;
extern char *interface_type_005_nbread_str;

#define I005_XPLIN  interface_type_005_xplin_str
#define I005_XPLOUT interface_type_005_xplout_str
#define I005_NBREADERROR interface_type_005_nbreaderror_str
#define I005_NBREAD interface_type_005_nbread_str

struct interface_type_005_indicators_s
{
   uint32_t xplin;
   uint32_t xplout;
   uint32_t nbreaderror;
   uint32_t nbread;
};


typedef struct interface_type_005_s
{
   int              id_interface;
   char             name[256];
   char             dev[256];
   char             user[256];
   char             password[256];
   char            *parameters;
   xpl2_f           xPL_callback2;
   mea_queue_t      devices_list;
   int              monitoring_id;
   pthread_t       *thread;
   volatile sig_atomic_t
                    thread_is_running;
   pthread_mutex_t  lock;
   struct interface_type_005_indicators_s
                    indicators;
   struct netatmo_token_s token;
} interface_type_005_t;


struct interface_type_005_start_stop_params_s
{
   interface_type_005_t *i005;
};


struct thread_interface_type_005_args_s {
   interface_type_005_t *i005;
};


xpl2_f get_xPLCallback_interface_type_005(void *ixxx);
int get_monitoring_id_interface_type_005(void *ixxx);
int set_xPLCallback_interface_type_005(void *ixxx, xpl2_f cb);
int set_monitoring_id_interface_type_005(void *ixxx, int id);
int get_type_interface_type_005(void);

int clean_interface_type_005(interface_type_005_t *i005);

#ifndef ASPLUGIN
int get_fns_interface_type_005(struct interfacesServer_interfaceFns_s *interfacesFns);
#endif

#endif
