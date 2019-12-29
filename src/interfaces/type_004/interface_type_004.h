/*
 *  interface_type_004.h
 *
 *  Created by Patrice Dietsch on 27/02/15.
 *  Copyright 2012 -. All rights reserved.
 *
 */

#ifndef __interface_type_004_h
#define __interface_type_004_h
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

#define INTERFACE_TYPE_004 400

extern char *interface_type_004_xplin_str;
extern char *interface_type_004_xplout_str;
extern char *interface_type_004_lightschanges_str;

#define I004_XPLIN  interface_type_004_xplin_str
#define I004_XPLOUT interface_type_004_xplout_str
#define I004_LIGHSCHANGES interface_type_004_lightschanges_str

struct interface_type_004_indicators_s
{
   uint32_t xplin;
   uint32_t xplout;
   uint32_t lightschanges;
};


struct lightsListElem_s {
    const char *sensorname; /* key */
    const char *actuatorname; /* key */
    const char *huename;
    int16_t id_sensor;
    int16_t id_actuator;
    int16_t todbflag_sensor;
    int16_t todbflag_actuator;
    int16_t reachable_use;
    UT_hash_handle hh_sensorname;/* makes this structure hashable */
    UT_hash_handle hh_actuatorname;/* makes this structure hashable */
    UT_hash_handle hh_huename;/* makes this structure hashable */
};


struct groupsListElem_s {
    const char *groupname; /* key */
    const char *huegroupname;
    UT_hash_handle hh_groupname;/* makes this structure hashable */
};

/*
struct scenesListElem_s {
    const char *scenename; // key
    const char *huescenename;
    UT_hash_handle hh_scenename; // makes this structure hashable
};
*/

typedef struct interface_type_004_s
{
   int              id_interface;
   char             name[256];
   char             dev[256];
   char             server[256];
   char             user[256];
   int              port;
   int              monitoring_id;
   pthread_t       *thread;
   volatile sig_atomic_t
                    thread_is_running;
                    
   pthread_mutex_t  lock;
   
   struct lightsListElem_s
                   *lightsListByHueName;
   struct lightsListElem_s
                   *lightsListByActuatorName;
   struct lightsListElem_s
                   *lightsListBySensorName;
                   
   struct groupsListElem_s
                   *groupsListByGroupName;
   cJSON *lastHueLightsState;
   cJSON *currentHueLightsState;
   cJSON *allGroups;

   struct interface_type_004_indicators_s
                    indicators;
   int              loaded;
   xpl2_f           xPL_callback2;
   char            *parameters;

} interface_type_004_t;


struct interface_type_004_start_stop_params_s
{
   interface_type_004_t *i004;
};


struct thread_interface_type_004_args_s
{
   interface_type_004_t *i004;
};


xpl2_f get_xPLCallback_interface_type_004(void *ixxx);
int get_monitoring_id_interface_type_004(void *ixxx);
int set_xPLCallback_interface_type_004(void *ixxx, xpl2_f cb);
int set_monitoring_id_interface_type_004(void *ixxx, int id);
int get_type_interface_type_004(void);

int clean_interface_type_004(void *ixxx);

#ifndef ASPLUGIN
int get_fns_interface_type_004(struct interfacesServer_interfaceFns_s *interfacesFns);
#endif

#endif
