//
//  interfaces.h
//
//  Created by Patrice DIETSCH on 13/09/12.
//
//

#ifndef __interfacesServer_h
#define __interfacesServer_h

// #include <Python.h>
#include <termios.h>
#include <inttypes.h>
#include <sqlite3.h>

#include "xPLServer.h"

#include "mea_queue.h"
//DBSERVER #include "dbServer.h"
#include "cJSON.h"

#define INTERFACE_TYPE_001 100
#define INTERFACE_TYPE_002 200
#define INTERFACE_TYPE_003 300
#define INTERFACE_TYPE_004 400
#define INTERFACE_TYPE_005 455
#define INTERFACE_TYPE_006 465

extern char *sql_select_device_info;
extern char *sql_select_interface_info;

/*
sensors_actuators.id_sensor_actuator, \
sensors_actuators.id_location, \
sensors_actuators.state, \
sensors_actuators.parameters, \
types.parameters, \
sensors_actuators.id_type, \
lower(sensors_actuators.name), \
lower(interfaces.name), \
interfaces.id_type, \
(SELECT lower(types.name) FROM types WHERE types.id_type = interfaces.id_type), \
interfaces.dev, \
sensors_actuators.todbflag, \
types.typeoftype, \
sensors_actuators.id_interface
*/
struct device_info_s
{
   uint16_t id;
   char *name;
   char *parameters;
   uint16_t state;

   uint16_t type_id;
   uint16_t typeoftype_id;
   char *type_name;
   char *type_parameters;

   uint16_t interface_id;
   char *interface_name;
   char *interface_dev;
   uint16_t interface_type_id;

   uint16_t location_id;
   uint16_t todbflag;

//   void *userdata;
};

typedef int16_t (*xpl2_f)(cJSON *xplMsgJson, struct device_info_s *device_info, void *userdata);

typedef void * (*malloc_and_init_f)(sqlite3 *, int, int, char *, char *, char *, char *);
typedef void * (*malloc_and_init2_f)(cJSON *);
typedef int    (*get_monitoring_id_f)(void *);
typedef int    (*set_monitoring_id_f)(void *, int);
typedef xpl2_f (*get_xPLCallback_f)(void *);
typedef int    (*set_xPLCallback_f)(void *, xpl2_f);
typedef int    (*clean_f)(void *);
typedef int    (*get_type_f)(void);
typedef int    (*get_interface_id_f)(void *);
typedef int    (*api_f)(void *ixxx, char *, void *, int, void **, int16_t *, char *, int); // *cmnd, void *args, int nb_args, void **res, char *err, int l_err


struct interfacesServer_interfaceFns_s {
   void *lib;
   int type;
   int plugin_flag;

   malloc_and_init_f malloc_and_init;
   malloc_and_init2_f malloc_and_init2;
   get_monitoring_id_f get_monitoring_id;
   set_monitoring_id_f set_monitoring_id;
   get_xPLCallback_f get_xPLCallback;
   set_xPLCallback_f set_xPLCallback;
   get_type_f get_type;
//   get_interface_id_f get_interface_id;

   clean_f clean;

   api_f api;
};

#ifdef ASPLUGIN
typedef int (*get_fns_interface_f)(void *, struct interfacesServer_interfaceFns_s *);
#else
typedef int (*get_fns_interface_f)(struct interfacesServer_interfaceFns_s *);
#endif


typedef struct interfaces_queue_elem_s
{
   int id;
   int type;
   char name[17];
   char dev[256];

   uint16_t delegate_flag;
   void *context;
   struct interfacesServer_interfaceFns_s *fns;
} interfaces_queue_elem_t;


struct interfacesServerData_s
{
//   char **params_list;
   cJSON *params_list;
   sqlite3 *sqlite3_param_db;
//   dbServer_md_t *myd;
};


int16_t      interfacesServer_call_interface_api(int id_interface, char *cmnd, void *args, int nb_args, void **res, int16_t *nerr, char *err, int l_err);
//mea_queue_t *start_interfaces(char **params_list, sqlite3 *sqlite3_param_db);
mea_queue_t *start_interfaces(cJSON *params_list, sqlite3 *sqlite3_param_db);
void         stop_interfaces(void);
int          dispatchXPLMessageToInterfaces2(cJSON *xplMsgJson);
int          restart_interfaces(int my_id, void *data, char *errmsg, int l_errmsg);

#endif
