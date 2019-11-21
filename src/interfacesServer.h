//
//  interfaces.h
//
//  Created by Patrice DIETSCH on 13/09/12.
//
//

#ifndef __interfacesServer_h
#define __interfacesServer_h

#include <termios.h>
#include <inttypes.h>

#include "xPLServer.h"

#include "mea_queue.h"
#include "cJSON.h"
#include "uthash.h"

#define INTERFACE_TYPE_001 100
#define INTERFACE_TYPE_002 200
#define INTERFACE_TYPE_003 300
#define INTERFACE_TYPE_004 400
#define INTERFACE_TYPE_005 455
#define INTERFACE_TYPE_006 465


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
};

struct devices_index_s
{
   char name[41];
   cJSON *device;
   UT_hash_handle hh;
};

struct types_index_s
{
   int id_type;
   cJSON *type;
   UT_hash_handle hh;
};

struct devs_index_s
{
   char devName[81];
   cJSON *interface;
   UT_hash_handle hh;
};

typedef int16_t (*xpl2_f)(cJSON *xplMsgJson, struct device_info_s *device_info, void *userdata);

typedef void * (*malloc_and_init2_f)(int, cJSON *);
typedef int    (*get_monitoring_id_f)(void *);
typedef int    (*set_monitoring_id_f)(void *, int);
typedef xpl2_f (*get_xPLCallback_f)(void *);
typedef int    (*set_xPLCallback_f)(void *, xpl2_f);
typedef int    (*clean_f)(void *);
typedef int    (*get_type_f)(void);
typedef int    (*get_interface_id_f)(void *);
typedef int    (*update_devices_f)(void *);
typedef int    (*api_f)(void *ixxx, char *, void *, int, void **, int16_t *, char *, int); // *cmnd, void *args, int nb_args, void **res, char *err, int l_err

struct interfacesServer_interfaceFns_s {
   void *lib;
   int type;
   int plugin_flag;

   malloc_and_init2_f malloc_and_init2;
   get_monitoring_id_f get_monitoring_id;
   set_monitoring_id_f set_monitoring_id;
   get_xPLCallback_f get_xPLCallback;
   set_xPLCallback_f set_xPLCallback;

   update_devices_f update_devices;
   get_type_f get_type;

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
   cJSON *params_list;
};


int16_t      interfacesServer_call_interface_api(int id_interface, char *cmnd, void *args, int nb_args, void **res, int16_t *nerr, char *err, int l_err);
mea_queue_t *start_interfaces(cJSON *params_list);
void         stop_interfaces(void);
int          dispatchXPLMessageToInterfaces2(cJSON *xplMsgJson);
int          restart_interfaces(int my_id, void *data, char *errmsg, int l_errmsg);
int          device_info_from_json(struct device_info_s *device_info, cJSON *jsonDevice, cJSON *jsonInterface, cJSON *jsonType);

// wrapper
cJSON       *getInterfaceByDevName_alloc(char *devName);
cJSON       *getInterfaceById_alloc(int id);
char        *getInterfacesAsString_alloc(void);
char        *getInterfaceAsStringByName_alloc(char *name);
char        *getDevicesAsString_alloc(char *interface);
char        *getDeviceAsStringByName_alloc(char *interface, char *device);

char        *getTypesAsString_alloc(void);
char        *getTypeAsStringByName_alloc(char *name);

int         addDevice(char *interface, cJSON *device);
int         deleteDevice(char *interface, char *name);
int         updateDevice(char *interface, char *name, cJSON *device);

int         deleteInterface(char *interface);
int         addInterface(cJSON *jsonData);
int         updateInterface(char *interface, cJSON *jsonData);

#endif
