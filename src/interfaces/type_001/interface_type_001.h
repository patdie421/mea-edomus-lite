/*
 *  interface_type_001.h
 *
 *  Created by Patrice Dietsch on 25/08/12.
 *  Copyright 2012 -. All rights reserved.
 *
 */

#ifndef __interface_type_001_h
#define __interface_type_001_h

#include <inttypes.h>
#include <string.h>
#include <signal.h>

#include "cJSON.h"
#include "mea_verbose.h"
#include "mea_queue.h"
#include "comio2.h"

#include "interfacesServer.h"
#include "xPLServer.h"

#define INTERFACE_TYPE_001 100

#define I001_XPLINNB     "NBXPLIN"
#define I001_SNBTRAPS    "SNBTRAPS"
#define I001_SNBREADS    "SNBREADS"
#define I001_SNBREADSERR "SNBREADSERR"
#define I001_SNBXPLOUT   "SNBXPLOUT"
#define I001_SNBXPLIN    "SNBXPLIN"
#define I001_ANBOUTERR   "ANBOUTERR"
#define I001_ANBOUT      "ANBOUT"
#define I001_ANBXPLIN    "ANBXPLIN"
#define I001_CNBTRAPS    "CNBTRAPS"
#define I001_CNBREADS    "CNBREADS"
#define I001_CNBREADSERR "CNBREADSERR"
#define I001_CNBXPLOUT   "CNBXPLOUT"
#define I001_CNBXPLIN    "CNBXPLIN"

#define UNIT_WH 1 // Watt/Heure
#define UNIT_W  2 // Watt
#define UNIT_C  3 // degré C
#define UNIT_V  4 // Volt
#define UNIT_H  5 // pourcentage humidité

struct interface_type_001_indicators_s
{
   uint32_t nbxplin;
   uint32_t nbactuatorsout;
   uint32_t nbactuatorsxplrecv;
   uint32_t nbactuatorsouterr;
   
   uint32_t nbsensorstraps;
   uint32_t nbsensorsread;
   uint32_t nbsensorsreaderr;
   uint32_t nbsensorsxplsent;
   uint32_t nbsensorsxplrecv;
  
   uint32_t nbcounterstraps;
   uint32_t nbcountersread;
   uint32_t nbcountersreaderr;
   uint32_t nbcountersxplsent;
   uint32_t nbcountersxplrecv;
};

typedef struct interface_type_001_s
{
   uint16_t interface_id;
   char name[41];
   pthread_t  *thread_id; // thread id
   volatile sig_atomic_t thread_is_running;
   int monitoring_id;
   comio2_ad_t *ad; // comio descriptor
   int loaded;
   mea_queue_t *counters_list; // counter sensors attach to interface
   mea_queue_t *actuators_list;
   mea_queue_t *sensors_list;
   xpl2_f xPL_callback2;
   
   struct interface_type_001_indicators_s indicators;

} interface_type_001_t;


struct interface_type_001_start_stop_params_s
{
   interface_type_001_t *i001;
   char dev[81];
};


typedef float (*compute_f)(unsigned int value);

int stop_interface_type_001(int my_id, void *data, char *errmsg, int l_errmsg);
int start_interface_type_001(int my_id, void *data, char *errmsg, int l_errmsg);
int restart_interface_type_001(int my_id, void *data, char *errmsg, int l_errmsg);

xpl2_f get_xPLCallback_interface_type_001(void *ixxx);
int    set_xPLCallback_interface_type_001(void *ixxx, xpl2_f cb);
int    get_monitoring_id_interface_type_001(void *ixxx);
int    set_monitoring_id_interface_type_001(void *ixxx, int id);
int    get_type_interface_type_001(void);

interface_type_001_t *malloc_and_init_interface_type_001(int id_driver, cJSON *jsonInterface);
int clean_interface_type_001(interface_type_001_t *i001);

int get_fns_interface_type_001(struct interfacesServer_interfaceFns_s *interfacesFns);

#endif
