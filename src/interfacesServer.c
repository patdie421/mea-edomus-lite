//
//  interfacesServer.c
//  mea-edomus
//
//  Created by Patrice Dietsch on 04/11/2013.
//
//
#include <Python.h>

#include <stdio.h>
#include <termios.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#ifndef NOMORESQLITE3
#include <sqlite3.h>
#endif 
#include <dlfcn.h>

#include "globals.h"
#include "configuration.h"
#include "tokens.h"
#include "tokens_da.h"

#include "mea_string_utils.h"
#include "mea_queue.h"
#include "mea_verbose.h"
#include "consts.h"

#include "cJSON.h"
#include "uthash.h"

#include "processManager.h"

#include "interfacesServer.h"
#include "interface_type_001.h"
#include "interface_type_002.h"
#include "interface_type_003.h"
#include "interface_type_004.h"
#include "interface_type_005.h"
#include "interface_type_006.h"

#ifndef NOMORESQLITE3
char *sql_select_device_info="SELECT \
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
sensors_actuators.id_interface \
FROM sensors_actuators INNER JOIN interfaces ON sensors_actuators.id_interface = interfaces.id_interface INNER JOIN types ON sensors_actuators.id_type = types.id_type" ;
#endif

#define MAX_INTERFACES_PLUGINS 10 // au demarrage et pour les statics

struct interfacesServer_interfaceFns_s *interfacesFns;
int interfacesFns_nb = 0;
int interfacesFns_max = MAX_INTERFACES_PLUGINS;

mea_queue_t *_interfaces=NULL;

pthread_rwlock_t interfaces_queue_rwlock;
pthread_rwlock_t jsonInterfaces_rwlock;

/*
* New functions
*/
cJSON *jsonInterfaces = NULL;
cJSON *jsonTypes      = NULL;

struct devices_index_s
{
   char name[41];
   cJSON *device;
   UT_hash_handle hh;
};
struct devices_index_s *devices_index = NULL;


struct types_index_s
{
   int id_type;
  cJSON *type;
   UT_hash_handle hh;
};
struct types_index_s *types_index = NULL;


struct devs_index_s
{
   char devName[41];
   cJSON *interface;
   UT_hash_handle hh;
};
struct devs_index_s *devs_index = NULL;



void deleteDevicesIndex(struct devices_index_s *devices_index)
{
   struct devices_index_s *e = NULL, *tmp = NULL;
 
   HASH_ITER(hh, devices_index, e, tmp) {
      if(e) {
         HASH_DEL(devices_index, e);
         free(e);
      }
   }
   devices_index = NULL;
}


void deleteTypesIndex(struct types_index_s *types_index)
{
   struct types_index_s *e = NULL, *tmp = NULL;
 
   HASH_ITER(hh, types_index, e, tmp) {
      if(e) {
         HASH_DEL(types_index, e);
         free(e);
      }
   }
   types_index = NULL;
}


void deleteDevsIndex(struct devs_index_s *devs_index)
{
   struct devs_index_s *e = NULL, *tmp = NULL;
 
   HASH_ITER(hh, devs_index, e, tmp) {
      if(e) {
         HASH_DEL(devs_index, e);
         free(e);
      }
   }
   devs_index = NULL;
}


int createDevsIndex(struct devs_index_s *devs_index, cJSON *jsonInterfaces)
{
   if(!jsonInterfaces)
      return -1;
 
   cJSON *jsonInterface = jsonInterfaces->child;
   while( jsonInterface ) {
      char *devName = cJSON_GetObjectItem(jsonInterface, "dev")->valuestring;
 
      struct devs_index_s *e=(struct devs_index_s *)malloc(sizeof(struct devs_index_s));
      mea_strncpytrimlower(e->devName, devName, sizeof(e->devName));
      e->interface=jsonInterface;
      HASH_ADD_STR(devs_index, devName, e);
 
      jsonInterface=jsonInterface->next;
   }
 
   return 0;
}


int createDevicesIndex(struct devices_index_s *devices_index, cJSON *jsonInterfaces)
{
   if(!jsonInterfaces)
      return -1;
 
   cJSON *jsonInterface=jsonInterfaces->child;
   while( jsonInterface ) {
      cJSON *jsonDevices = cJSON_GetObjectItem(jsonInterface, "devices");
 
      if(jsonDevices) {
         cJSON *jsonDevice = jsonDevices->child;
         while( jsonDevice ) {
            char *name=jsonDevice->string;
            struct devices_index_s *e=(struct devices_index_s *)malloc(sizeof(struct devices_index_s));
            strncpy(e->name, name, sizeof(e->name));
            e->device=jsonDevice;
            HASH_ADD_STR(devices_index, name, e);
 
            jsonDevice=jsonDevice->next;
         }
      }
      jsonInterface = jsonInterface->next;
   }
 
   return 0;
}


int createTypesIndex(struct types_index_s *types_index, cJSON *jsonTypes)
{
   if(!jsonTypes)
      return -1;
 
   cJSON *jsonType = jsonTypes->child;
   while( jsonType ) {
      int id_type = cJSON_GetObjectItem(jsonType, "id_type")->valuedouble;
      struct types_index_s *e=(struct types_index_s *)malloc(sizeof(struct types_index_s));
      e->id_type = id_type;
      e->type = jsonType;
      HASH_ADD_INT(types_index, id_type, e);
 
      jsonType=jsonType->next;
   }
 
   return 0;
}


cJSON *findInterfaceByDevNameThroughIndex_alloc(struct devs_index_s *_devs_index, char *devName)
{
   struct devs_index_s *e = NULL;
 
   if(!_devs_index)
      _devs_index=devs_index;
 
   cJSON *d = NULL;
 
   HASH_FIND_STR(_devs_index, devName, e);
   if(e) {
      d=cJSON_Duplicate(e->interface,1);
   }

   return d;
}


cJSON *findDeviceByNameThroughIndex_alloc(struct devices_index_s *devices_index, char *name)
{
   struct devices_index_s *e = NULL;
 
   if(!devices_index)
      return NULL;
 
   cJSON *d = NULL;
 
   HASH_FIND_STR(devices_index, name, e);
   if(e) {
      d=cJSON_Duplicate(e->device, 0);
   }
 
   return d;
}


cJSON *findTypeByIdThroughIndex_alloc(struct types_index_s *types_index, int id)
{
   struct types_index_s *e = NULL;
 
   if(!types_index)
      return NULL;
 
   cJSON *t = NULL;
 
   HASH_FIND_INT(types_index, &id, e);
   if(e) {
      t=cJSON_Duplicate(e->type, 0);
   }
   return t;
}


int checkJsonInterfaces(cJSON *jsonInterfaces)
{
   if(!jsonInterfaces) {
      return -1;
   }
   else {
//
// ajouter ici le controle de la structure et des types de données
//
      return 0;
   }
}


int checkJsonTypes(cJSON *jsonTypes)
{
   if(!jsonTypes) {
      return -1;
   }
   else {
//
// ajouter ici le controle de la structure et des types de données
//
      return 0;
   }
}


#ifndef NOMORESQLITE3
cJSON *findInterfaceById(cJSON *jsonInterfaces, int id)
{
   if(!jsonInterfaces)
      return NULL;
 
   cJSON *jsonInterface=jsonInterfaces->child;
   while( jsonInterface ) {
      int  id_interface=cJSON_GetObjectItem(jsonInterface, "id_interface")->valuedouble;
      if(id_interface == id)
         return jsonInterface;
      jsonInterface = jsonInterface->next;
   }
 
   return NULL;
}


void linkInterfacesDevices(cJSON *jsonInterfaces, cJSON *jsonDevices)
{
   cJSON *jsonDevice=jsonDevices->child;
   while( jsonDevice ) {
      int id_interface=cJSON_GetObjectItem(jsonDevice, "id_interface")->valuedouble;
      char *name=jsonDevice->string;
 
      cJSON *jsonInterface=findInterfaceById(jsonInterfaces, id_interface);
      char *s=NULL;
      s=cJSON_Print(jsonInterfaces);
      free(s);
      if(jsonInterface) {
         cJSON *_jsonDevice=cJSON_Duplicate(jsonDevice, 1);
         cJSON *jsonInterfaceDevices=cJSON_GetObjectItem(jsonInterface, "devices");
         cJSON_AddItemToObject(jsonInterfaceDevices, name, _jsonDevice);
      }
 
      jsonDevice = jsonDevice->next;
   }
}
 
 
cJSON *typesTableToJson(sqlite3 *sqlite3_param_db)
{
   char sql[256];
   sqlite3_stmt * stmt;
   int ret = 0;
 
   cJSON *jsonTypes=cJSON_CreateObject();
 
   if(!jsonTypes) {
      VERBOSE(1) {
         mea_log_printf("%s (%s) : %s - cJSON_CreateObject: ",ERROR_STR,__func__,"creation error");
      }
      return NULL;
   }
 
   sprintf(sql,"SELECT * FROM types");
   ret = sqlite3_prepare_v2(sqlite3_param_db, sql, (int)(strlen(sql)+1), &stmt, NULL);
   if(ret)
   {
      VERBOSE(1) mea_log_printf("%s (%s) : sqlite3_prepare_v2 - %s\n", ERROR_STR,__func__,sqlite3_errmsg(sqlite3_param_db));
      return NULL;
   }
 
   while (1)
   {
      int s = sqlite3_step (stmt); // sqlite function need int
      if (s == SQLITE_ROW)
      {
         int16_t id_type;
         const unsigned char *name;
         const unsigned char *description;
         const unsigned char *parameters;
         int16_t flag;
         int16_t typeoftype;
 
         id_type = sqlite3_column_int(stmt, 1);
         name = sqlite3_column_text(stmt, 2);
         description = sqlite3_column_text(stmt, 3);
         parameters = sqlite3_column_text(stmt, 4);
         flag = sqlite3_column_int(stmt, 5);
         typeoftype = sqlite3_column_int(stmt, 6);
 
         cJSON *jsonType=cJSON_CreateObject();
         cJSON_AddItemToObject(jsonType, "id_type",     cJSON_CreateNumber((double)id_type));
         cJSON_AddItemToObject(jsonType, "description", cJSON_CreateString((const char *)description));
         cJSON_AddItemToObject(jsonType, "parameters",  cJSON_CreateString((const char *)parameters));
         cJSON_AddItemToObject(jsonType, "flag",        cJSON_CreateNumber((double)flag));
         cJSON_AddItemToObject(jsonType, "typeoftype",  cJSON_CreateNumber((double)typeoftype));
         cJSON_AddItemToObject(jsonTypes, (const char *)name, jsonType);
      }
      else if (s == SQLITE_DONE)
      {
         sqlite3_finalize(stmt);
         break;
      }
      else
      {
         VERBOSE(2) mea_log_printf("%s (%s) : sqlite3_step - %s\n", ERROR_STR,__func__,sqlite3_errmsg (sqlite3_param_db));
         sqlite3_finalize(stmt);
         return NULL;
      }
   }
 
   return jsonTypes;
}
 
 
cJSON *devicesTableToJson(sqlite3 *sqlite3_param_db)
{
   char sql[256];
   sqlite3_stmt * stmt;
   int ret = 0;
 
   cJSON *jsonDevices=cJSON_CreateObject();
 
   if(!jsonDevices) {
      VERBOSE(1) {
         mea_log_printf("%s (%s) : %s - cJSON_CreateObject: ",ERROR_STR,__func__,"creation error");
      }
      return NULL;
   }
 
   sprintf(sql,"SELECT * FROM sensors_actuators");
   ret = sqlite3_prepare_v2(sqlite3_param_db, sql, (int)(strlen(sql)+1), &stmt, NULL);
   if(ret)
   {
      VERBOSE(1) mea_log_printf("%s (%s) : sqlite3_prepare_v2 - %s\n", ERROR_STR,__func__,sqlite3_errmsg(sqlite3_param_db));
      return NULL;
   }
 
   while (1)
   {
      int s = sqlite3_step (stmt); // sqlite function need int
      if (s == SQLITE_ROW)
      {
         int16_t id_sensor_actuator;
         int16_t id_type;
         int16_t id_interface;
         const unsigned char *name;
         const unsigned char *description;
         const unsigned char *parameters;
         int16_t state;
 
         id_sensor_actuator = sqlite3_column_int(stmt, 1);
         id_type = sqlite3_column_int(stmt, 2);
         id_interface = sqlite3_column_int(stmt, 3);
         name = sqlite3_column_text(stmt, 4);
         description = sqlite3_column_text(stmt, 5);
         parameters = sqlite3_column_text(stmt, 7);
         state = sqlite3_column_int(stmt, 8);
 
         cJSON *jsonDevice=cJSON_CreateObject();
         cJSON_AddItemToObject(jsonDevice, "id_sensor_actuator", cJSON_CreateNumber((double)id_sensor_actuator));
         cJSON_AddItemToObject(jsonDevice, "id_type",            cJSON_CreateNumber((double)id_type));
         cJSON_AddItemToObject(jsonDevice, "id_interface",       cJSON_CreateNumber((double)id_interface));
         cJSON_AddItemToObject(jsonDevice, "description",        cJSON_CreateString((const char *)description));
         cJSON_AddItemToObject(jsonDevice, "parameters",         cJSON_CreateString((const char *)parameters));
         cJSON_AddItemToObject(jsonDevice, "state",              cJSON_CreateNumber((double)state));
         cJSON_AddItemToObject(jsonDevices, (const char *)name, jsonDevice);
 
         char *s=cJSON_Print(jsonDevice);
         free(s);
      }
      else if (s == SQLITE_DONE)
      {
         sqlite3_finalize(stmt);
         break;
      }
      else
      {
         VERBOSE(2) mea_log_printf("%s (%s) : sqlite3_step - %s\n", ERROR_STR,__func__,sqlite3_errmsg (sqlite3_param_db));
         sqlite3_finalize(stmt);
         return NULL;
      }
   }
 
   return jsonDevices;
}
 
 
cJSON *interfacesTableToJson(sqlite3 *sqlite3_param_db)
{
   char sql[256];
   sqlite3_stmt * stmt;
   int ret = 0;
 
   cJSON *jsonInterfaces=cJSON_CreateObject();
 
   if(!jsonInterfaces) {
      VERBOSE(1) {
         mea_log_printf("%s (%s) : %s - cJSON_CreateObject: ",ERROR_STR,__func__,"creation error");
      }
      return NULL;
   }
 
   sprintf(sql,"SELECT * FROM interfaces");
   ret = sqlite3_prepare_v2(sqlite3_param_db, sql, (int)(strlen(sql)+1), &stmt, NULL);
   if(ret)
   {
      VERBOSE(1) mea_log_printf("%s (%s) : sqlite3_prepare_v2 - %s\n", ERROR_STR,__func__,sqlite3_errmsg(sqlite3_param_db));
      return NULL;
   }
 
   while (1)
   {
      int s = sqlite3_step (stmt); // sqlite function need int
      if (s == SQLITE_ROW)
      {
         int16_t id_interface;
         int16_t id_type;
         const unsigned char *dev;
         const unsigned char *parameters;
         const unsigned char *name;
         const unsigned char *description;
         int16_t state;
 
         id_interface = sqlite3_column_int(stmt, 1);
         id_type = sqlite3_column_int(stmt, 2);
         name = sqlite3_column_text(stmt, 3);
         description = sqlite3_column_text(stmt, 4);
         dev = sqlite3_column_text(stmt, 5);
         parameters = sqlite3_column_text(stmt, 6);
         state = sqlite3_column_int(stmt, 7);
 
         cJSON *jsonInterface=cJSON_CreateObject();
         cJSON_AddItemToObject(jsonInterface, "id_interface", cJSON_CreateNumber((double)id_interface));
         cJSON_AddItemToObject(jsonInterface, "id_type",      cJSON_CreateNumber((double)id_type));
         cJSON_AddItemToObject(jsonInterface, "description",  cJSON_CreateString((const char *)description));
         cJSON_AddItemToObject(jsonInterface, "dev",          cJSON_CreateString((const char *)dev));
         cJSON_AddItemToObject(jsonInterface, "parameters",   cJSON_CreateString((const char *)parameters));
         cJSON_AddItemToObject(jsonInterface, "state",        cJSON_CreateNumber((double)state));
         cJSON_AddItemToObject(jsonInterface, "devices",      cJSON_CreateObject());
 
         cJSON_AddItemToObject(jsonInterfaces, (const char *)name, jsonInterface);
      }
      else if (s == SQLITE_DONE)
      {
         sqlite3_finalize(stmt);
         break;
      }
      else
      {
         VERBOSE(2) mea_log_printf("%s (%s) : sqlite3_step - %s\n", ERROR_STR,__func__,sqlite3_errmsg (sqlite3_param_db));
         sqlite3_finalize(stmt);
         return NULL;
      }
   }
 
   return jsonInterfaces;
}
 
 
int resyncDevices(cJSON *jsonInterfaces, sqlite3 *sqlite3_param_db)
{
/*
* A faire :
* recharger les "devices" pour les interfaces présentes
*/
   return 0;
}
 

cJSON *jsonInterfacesLoad(sqlite3 *sqlite3_param_db)
{
   cJSON *_jsonInterfaces=interfacesTableToJson(sqlite3_param_db);
   if(_jsonInterfaces==NULL)
      return NULL;
 
   cJSON *_jsonDevices=devicesTableToJson(sqlite3_param_db);
 
   linkInterfacesDevices(_jsonInterfaces, _jsonDevices);
 
   cJSON_Delete(_jsonDevices);
 
   if(checkJsonInterfaces(_jsonInterfaces)==-1) {
      cJSON_Delete(_jsonInterfaces);
      return NULL;
   }

   createDevicesIndex(devices_index, jsonInterfaces);
 
   return _jsonInterfaces;
}

cJSON *jsonTypesLoad(sqlite3 *sqlite3_param_db)
{
   cJSON *_jsonInterfaces=typesTableToJson(sqlite3_param_db);
   if(_jsonTypes==NULL)
      return NULL;
 
   if(checkJsonTypes(_jsonTypes)==-1) {
      cJSON_Delete(_jsonTypes);
      return NULL;
   }

   return _jsonTypes;
}
#endif
 
 
int jsonInterfacesSave()
{
   return 0;
}

int jsonTypesSave()
{
   return 0;
}


/*
* end new functions
*/
 
 
int device_info_from_json(struct device_info_s *device_info, cJSON *jsonDevice, cJSON *jsonInterface, cJSON *jsonType)
{
   if(!device_info)
      return -1;
 
   if(!jsonInterface)
      jsonInterface = cJSON_GetObjectItem(jsonDevice, "interface");
   if(!jsonInterface)
      return -1;
 
   device_info->name              = (char *)jsonDevice->string;
   device_info->id                =    (int)cJSON_GetObjectItem(jsonDevice, "id_sensor_actuator")->valuedouble;
   device_info->parameters        = (char *)cJSON_GetObjectItem(jsonDevice, "parameters")->valuestring;
   device_info->state             =    (int)cJSON_GetObjectItem(jsonDevice, "state")->valuedouble;
   device_info->type_id           =    (int)cJSON_GetObjectItem(jsonDevice, "id_type")->valuedouble;
 
   if(!jsonType)
      jsonType = findTypeByIdThroughIndex(types_index, device_info->id);
   if(!jsonType)
      return -1;
   
   device_info->type_name         = (char *)jsonType->string;
   device_info->typeoftype_id     =    (int)cJSON_GetObjectItem(jsonType, "typeoftype")->valuedouble;
   device_info->parameters        = (char *)cJSON_GetObjectItem(jsonType, "parameters")->valuestring;
 
   device_info->interface_name    = (char *)jsonInterface->string;
   device_info->interface_id      =    (int)cJSON_GetObjectItem(jsonInterface, "id_interface")->valuedouble;
   device_info->interface_type_id =    (int)cJSON_GetObjectItem(jsonInterface, "id_type")->valuedouble;
   device_info->interface_dev     = (char *)cJSON_GetObjectItem(jsonInterface, "dev")->valuestring;
 
   device_info->location_id       = 0;
   device_info->todbflag          = 0;
 
   return 0;
}
 
 
int dispatchXPLMessageToInterfaces2(cJSON *xplMsgJson)
{
   int ret;
 
   interfaces_queue_elem_t *iq;
 
   DEBUG_SECTION mea_log_printf("%s (%s) : reception message xPL\n", INFO_STR, __func__);
 
   cJSON *device = NULL;
   device=cJSON_GetObjectItem(xplMsgJson, XPL_DEVICE_STR_C);
   if(!device) {
      DEBUG_SECTION mea_log_printf("%s (%s) : message discarded\n", INFO_STR, __func__);
      return -1;
   }

   int id_interface = -1;
   int state = -1;
   cJSON *jsonDevice = NULL; 

   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&jsonInterfaces_rwlock);
   pthread_rwlock_rdlock(&jsonInterfaces_rwlock);
   jsonDevice=findDeviceByNameThroughIndex_alloc(devices_index, device->valuestring);
   pthread_rwlock_unlock(&jsonInterfaces_rwlock);
   pthread_cleanup_pop(0);

   if(!jsonDevice) {
      DEBUG_SECTION mea_log_printf("%s (%s) : unknown device\n", INFO_STR, __func__);
      return -1;
   }
 
   state=(int)cJSON_GetObjectItem(jsonDevice, "state")->valuedouble;
   id_interface=(int)cJSON_GetObjectItem(jsonDevice, "id_interface")->valuedouble;
   struct device_info_s device_info;
   device_info_from_json(&device_info, jsonDevice, NULL, NULL);
   cJSON_Delete(jsonDevice);

   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&interfaces_queue_rwlock);
   pthread_rwlock_rdlock(&interfaces_queue_rwlock);
 
   while(1) {
 
      if(_interfaces && _interfaces->nb_elem) {
         mea_queue_first(_interfaces);
         while(1) {
            mea_queue_current(_interfaces, (void **)&iq);
            if(iq->context && id_interface == iq->id) {
               int monitoring_id = iq->fns->get_monitoring_id(iq->context);
               if(monitoring_id>-1 && process_is_running(monitoring_id)) {
                  xpl2_f xPL_callback2 = iq->fns->get_xPLCallback(iq->context);
                  if(xPL_callback2) {
                     xPL_callback2(xplMsgJson, &device_info, iq->context);
                     break;
                  }
               }
               break;
            }
            ret=mea_queue_next(_interfaces);
            if(ret<0)
               break;
         }
      }
   }
 
   pthread_rwlock_unlock(&interfaces_queue_rwlock);
   pthread_cleanup_pop(0);
 
   cJSON_Delete(xplMsgJson);
 
   return 0;
}
 
 
int16_t unload_interfaces_fns(struct interfacesServer_interfaceFns_s *ifns)
{
   for(int i=0; ifns[i].get_type; i++)
   {
      if(ifns[i].plugin_flag==1)
      {
         dlclose(ifns[i].lib);
         ifns[i].lib=NULL;
      }
   }
 
   return 0;
}
 
 
int16_t init_statics_interfaces_fns(struct interfacesServer_interfaceFns_s *ifns, int *ifns_nb)
{
   int i=0;
   *ifns_nb=0;
 
// chargement des interfaces "standard"
   get_fns_interface_type_001(&(ifns[i++]));
#ifndef ASPLUGIN
   get_fns_interface_type_002(&(ifns[i++]));
   get_fns_interface_type_003(&(ifns[i++]));
   get_fns_interface_type_004(&(ifns[i++]));
   get_fns_interface_type_005(&(ifns[i++]));
   get_fns_interface_type_006(&(ifns[i++]));
#endif
 
// chargement des interfaces "externes"
 
   *ifns_nb=i;
   interfacesFns[*ifns_nb].get_type = NULL;
 
   return 0;
}
 
 
#ifdef ASPLUGIN
struct plugin_info_s {
   char   *name;
   int     type;
   int16_t free_flag;
};
struct plugin_info_s *plugins_list = NULL;

#ifndef __APPLE__
   #define DYN_EXT ".so"
#else
   #define DYN_EXT ".dylib"
#endif
 
struct plugin_info_s plugin_info_defaults[] = {
   { "interface_type_002" DYN_EXT, INTERFACE_TYPE_002, 0 },
   { "interface_type_003" DYN_EXT, INTERFACE_TYPE_003, 0 },
   { "interface_type_004" DYN_EXT, INTERFACE_TYPE_004, 0 },
   { "interface_type_005" DYN_EXT, INTERFACE_TYPE_005, 0 },
   { "interface_type_006" DYN_EXT, INTERFACE_TYPE_006, 0 },
   { NULL, -1, -1 }
};
 
 
int init_interfaces_list(cJSON *jsonInterfaces)
{
  int ret = -1;

   if(plugins_list) {
      for(int i=0;plugins_list[i].name;i++) {
         if(plugins_list[i].free_flag) {
            free(plugins_list[i].name);
            plugins_list[i].name=NULL;
         }
      }
      free(plugins_list);
      plugins_list = NULL;
   }
   int next_int=0;
   plugins_list = (struct plugin_info_s *)realloc(plugins_list, sizeof(plugin_info_defaults));
   while(plugin_info_statics[next_int].type!=-1) {
      plugins_list[next_int].type = plugin_info_defaults[next_int].type;
      plugins_list[next_int].name = plugin_info_defaults[next_int].name;
      plugins_list[next_int].free_flag = 0;
      next_int++;
   }
 
   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&jsonInterfaces_rwlock);
   pthread_rwlock_rdlock(&jsonInterfaces_rwlock);
 
   cJSON *jsonInterface=jsonInterfaces->child;
   while( jsonInterface ) {
 
      int  state=cJSON_GetObjectItem(jsonInterface, "state")->valuedouble;
      int  id_type=cJSON_GetObjectItem(jsonInterface, "id_type")->valuedouble;
      char *parameters=cJSON_GetObjectItem(jsonInterface, "parameters")->valuestring;

      for(int i=0; i<next_int; i++) {
         if(plugins_list[i].type == id_type) {
            VERBOSE(2) mea_log_printf("%s (%s) : interface type %d allread loaded\n", INFO_STR, __func__, plugins_list[i].type);
            continue;
         }
      }
 
      char *p = mea_strltrim((char *)parameters);
      if(p) {
         char lib_name[255];
         ret=sscanf(p, "[%[^]]]%*[^\n]", lib_name);
         if(ret == 1) {
            if(p[strlen(lib_name)+1]==']') {
               int l=strlen(mea_strtrim(lib_name));
               if(l>0) {
                  plugins_list = (struct plugin_info_s *)realloc(plugins_list, sizeof(struct plugin_info_s)*(next_int+1));
                  plugins_list[next_int].name = malloc(l+1);
                  strcpy(plugins_list[next_int].name, mea_strtrim(lib_name));
                  plugins_list[next_int].type = id_type;
                  plugins_list[next_int].free_flag  = 1;
 
                  next_int++;
               }
            }
            else {
               VERBOSE(2) mea_log_printf("%s (%s) : interface parameter error - %s\n", WARNING_STR, __func__, p);
               continue;
            }
         }
      }
      else {
         VERBOSE(2) mea_log_printf("%s (%s) : interface parameter error - %s\n", WARNING_STR, __func__, (char *)parameters);
        continue;
      }
      jsonInterface = jsonInterface->next;
   }
 
   pthread_rwlock_unlock(&jsonInterfaces_rwlock);
   pthread_cleanup_pop(0);
 
   plugins_list[next_int].type = -1;
   plugins_list[next_int].name = NULL;
   plugins_list[next_int].free_flag  = -1;
 
   return 0;
}
#endif
 
 
#ifdef ASPLUGIN
// int load_interface(int type, cJSON *params_list)
int load_interface(int type, char *driversPath)
{
   for(int i=0; interfacesFns[i].get_type; i++)
   {
      if(interfacesFns[i].get_type() == type)
         return 0;
   }
 
   for(int i=0; plugins_list[i].name; i++)
   {
      if(type == plugins_list[i].type)
      {
         struct interfacesServer_interfaceFns_s fns;
         get_fns_interface_f fn = NULL;
         char interface_so[256];

//         snprintf(interface_so, sizeof(interface_so)-1,"%s/%s", appParameters_get("DRIVERSPATH", params_list), plugins_list[i].name);
         snprintf(interface_so, sizeof(interface_so)-1,"%s/%s", driversPath, plugins_list[i].name);

         void *lib = dlopen(interface_so, RTLD_NOW | RTLD_GLOBAL);
         if(lib)
         {
            char *err;
 
            fn=dlsym(lib, "get_fns_interface");
            err = dlerror();
            if(err!=NULL)
            {
               VERBOSE(1) mea_log_printf("%s (%s) : dlsym - %s\n", ERROR_STR, __func__, err);
               return -1;
            }
            else
            {
               if(interfacesFns_nb >= (interfacesFns_max-1))
               {
                  struct interfacesServer_interfaceFns_s *tmp;
                  interfacesFns_max+=5;
                  tmp = realloc(interfacesFns,  sizeof(struct interfacesServer_interfaceFns_s)*interfacesFns_max);
                  if(tmp == NULL)
                  {
                     dlclose(lib);
                     return -1;
                  }
                  interfacesFns = tmp;
                  memset(&(interfacesFns[interfacesFns_nb+1]), 0, 5*sizeof(struct interfacesServer_interfaceFns_s));
               }
               fn(lib, &(interfacesFns[interfacesFns_nb]));
               interfacesFns[interfacesFns_nb].lib = lib;
               interfacesFns[interfacesFns_nb].type = type;
               interfacesFns_nb++;
               return 1;
            }
         }
         else
         {
            VERBOSE(2) mea_log_printf("%s (%s) : dlopen - %s\n", ERROR_STR, __func__, dlerror());
            return -1;
         }
         break;
      }
   }
   return -1;
}
#endif
 
 
int16_t interfacesServer_call_interface_api(int id_interface, char *cmnd, void *args, int nb_args, void **res, int16_t *nerr, char *err, int l_err)
{
   int ret;
//   void *context = NULL;
   interfaces_queue_elem_t *iq;
   int found=0;
 
   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&interfaces_queue_rwlock);
   pthread_rwlock_rdlock(&interfaces_queue_rwlock);
 
   if(_interfaces && _interfaces->nb_elem)
   {
      mea_queue_first(_interfaces);
      while(1)
      {
         mea_queue_current(_interfaces, (void **)&iq);
 
         if(iq && iq->context && iq->id == id_interface)
         {
            found=1;
            break;
         }
 
         ret=mea_queue_next(_interfaces);
         if(ret<0)
            break;
      }
   }
   else
      ret=-1;
 
   if(found)
      ret=iq->fns->api(iq->context, cmnd, args, nb_args, res, nerr, err, l_err);
 
   pthread_rwlock_unlock(&interfaces_queue_rwlock);
   pthread_cleanup_pop(0);
 
   return ret;
}
 
 
void stop_interfaces()
{
   interfaces_queue_elem_t *iq;
 
   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&interfaces_queue_rwlock);
   pthread_rwlock_wrlock(&interfaces_queue_rwlock);
 
   if(_interfaces && _interfaces->nb_elem)
   {
      mea_queue_first(_interfaces);
      while(_interfaces->nb_elem)
      {
         mea_queue_out_elem(_interfaces, (void **)&iq);
 
         int monitoring_id = iq->fns->get_monitoring_id(iq->context);
 
         if(iq->delegate_flag == 0 && monitoring_id>-1 && process_is_running(monitoring_id))
         {
            iq->fns->set_xPLCallback(iq->context, NULL);
 
            int monitoring_id = iq->fns->get_monitoring_id(iq->context);
 
            if(monitoring_id!=-1)
            {
               void *start_stop_params = process_get_data_ptr(monitoring_id);
 
               process_stop(monitoring_id, NULL, 0);
               process_unregister(monitoring_id);
 
               iq->fns->set_monitoring_id(iq->context, -1);
 
               if(start_stop_params)
               {
                  free(start_stop_params);
                  start_stop_params=NULL;
               }
            }
            iq->fns->clean(iq->context);
            free(iq->context);
            iq->context=NULL;
         }
 
         free(iq);
         iq=NULL;
      }
      free(_interfaces);
      _interfaces=NULL;
   }
 
   unload_interfaces_fns(interfacesFns);
 
   free(interfacesFns);
   interfacesFns=NULL;
 
   pthread_rwlock_unlock(&interfaces_queue_rwlock);
   pthread_cleanup_pop(0);
 
 
   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&jsonInterfaces_rwlock);
   pthread_rwlock_wrlock(&jsonInterfaces_rwlock);
 
   if(jsonInterfaces) {
      cJSON_Delete(jsonInterfaces);
      jsonInterfaces=NULL;
   }
 
   if(jsonTypes) {
      cJSON_Delete(jsonTypes);
      jsonTypes=NULL;
   }
 
   if(devices_index) {
      deleteDevicesIndex(devices_index);
      devices_index=NULL;
   }

   if(devices_index) {
      deleteDevsIndex(devs_index);
      devs_index=NULL;
   }

   if(types_index) {
      deleteTypesIndex(types_index);
      devices_index=NULL;
   }
 
   pthread_rwlock_unlock(&jsonInterfaces_rwlock);
   pthread_cleanup_pop(0);
 
   return;
}
 
 
int clean_not_linked(mea_queue_t *interfaces_list)
{
   // à faire
 
   return 0;
}
 
 
int link_delegates(mea_queue_t *interfaces_list)
{
   int ret = 0;
   interfaces_queue_elem_t *iq;
 
   mea_queue_t _interfaces_list_clone;
   mea_queue_t *interfaces_list_clone=&_interfaces_list_clone;
   interfaces_queue_elem_t *iq_clone;
 
   mea_queue_clone(interfaces_list_clone, interfaces_list); // pour le double balayage
 
   mea_queue_first(interfaces_list);
   while(1)
   {
      mea_queue_current(interfaces_list, (void **)&iq);
 
      if(iq && !iq->context)
      {
         char name[256], more[256];
         int n;
 
         ret=sscanf(iq->dev,"%[^:]://%[^\n]%n\n", name, more, &n);
         if(ret>0 && strlen(iq->dev) == n)
         {
            mea_queue_first(interfaces_list_clone);
            while(1)
            {
               mea_queue_current(interfaces_list_clone, (void **)&iq_clone);
               if(iq_clone->context && iq_clone->delegate_flag == 0)
               {
                  if(mea_strcmplower(name, iq_clone->name) == 0)
                  {
                     iq->context = iq_clone->context;
                     iq->fns = iq_clone->fns;
                  }
               }
 
               if(mea_queue_next(interfaces_list_clone)<0)
                  break;
            }
         }
         else
         {
         }
      }
 
      ret=mea_queue_next(interfaces_list);
      if(ret<0)
         break;
   }
 
   return 0;
}
 
 
 
#ifndef NOMORESQLITE3
mea_queue_t *start_interfaces(cJSON *params_list, sqlite3 *sqlite3_param_db)
#else
mea_queue_t *start_interfaces(cJSON *params_list)
#endif
{
   char sql[255];
#ifndef NOMORESQLITE3
   sqlite3_stmt * stmt;
#endif
   int16_t ret;
   int sortie=0;
   interfaces_queue_elem_t *iq;

   pthread_rwlock_init(&interfaces_queue_rwlock, NULL);
   pthread_rwlock_init(&jsonInterfaces_rwlock, NULL);
 
   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&jsonInterfaces_rwlock);
   pthread_rwlock_wrlock(&jsonInterfaces_rwlock);
 
    if(jsonInterfaces) {
      cJSON_Delete(jsonInterfaces);
      jsonInterfaces=NULL;
   }

   if(jsonTypes) {
      cJSON_Delete(jsonTypes);
      jsonTypes=NULL;
   }
#ifndef NOMORESQLITE3
   jsonInterfaces=jsonInterfacesLoad(sqlite3_param_db);
   createDevicesIndex(devices_index, jsonInterfaces);
   jsonTypes=jsonTypesLoad(sqlite3_param_db);
   createTypeIndex(types_index, jsonTypes);
   createDevsIndex(devs_index, jsonInterfaces);
#endif

   pthread_rwlock_unlock(&jsonInterfaces_rwlock);
   pthread_cleanup_pop(0);

   if(!jsonInterfaces || !jsonTypes)
      return NULL;
    
#ifdef ASPLUGIN
   init_interfaces_list(jsonInterfaces);
#endif
 
   interfacesFns_max = MAX_INTERFACES_PLUGINS;
   interfacesFns = (struct interfacesServer_interfaceFns_s *)malloc(sizeof(struct interfacesServer_interfaceFns_s) * interfacesFns_max);
   if(interfacesFns == NULL)
      goto start_interfaces_clean_exit_S3;
 
   memset(interfacesFns, 0, sizeof(struct interfacesServer_interfaceFns_s)*interfacesFns_max);
   init_statics_interfaces_fns(interfacesFns, &interfacesFns_nb);
 
   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&interfaces_queue_rwlock);
   pthread_rwlock_wrlock(&interfaces_queue_rwlock);
 
   _interfaces=(mea_queue_t *)malloc(sizeof(mea_queue_t));
   if(!_interfaces)
   {
      VERBOSE(1) {
         mea_log_printf("%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
         perror("");
      }
      goto start_interfaces_clean_exit_S2;
   }
   mea_queue_init(_interfaces);
 
 
   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&jsonInterfaces_rwlock);
   pthread_rwlock_rdlock(&jsonInterfaces_rwlock);
 
   char *driversPath=appParameters_get("DRIVERSPATH", params_list);
   cJSON *jsonInterface=jsonInterfaces->child;
   while( jsonInterface ) {
      char *name=jsonInterface->string;
      int  state=cJSON_GetObjectItem(jsonInterface, "state")->valuedouble;
      int  id_interface=cJSON_GetObjectItem(jsonInterface, "state")->valuedouble;
      int  id_type=cJSON_GetObjectItem(jsonInterface, "id_type")->valuedouble;
      char *dev=cJSON_GetObjectItem(jsonInterface, "dev")->valuestring;
      char *parameters=cJSON_GetObjectItem(jsonInterface, "parameters")->valuestring;
      char *description=cJSON_GetObjectItem(jsonInterface, "description")->valuestring;
 
      if(state==1) {
 
         iq=(interfaces_queue_elem_t *)malloc(sizeof(interfaces_queue_elem_t));
         if(iq==NULL) {
            VERBOSE(1) {
               mea_log_printf("%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
               perror("");
            }
            goto start_interfaces_clean_exit_S1;
         }
         iq->context = NULL;
 
         int monitoring_id=-1;
         int i=0;
 
#ifdef ASPLUGIN
         ret=load_interface(id_type, driversPath);
         if(ret<0) {
            VERBOSE(2) mea_log_printf("%s (%s) : can't load interface type %d\n", ERROR_STR, __func__, id_type);
         }
         else if(ret>0) {
            VERBOSE(2) mea_log_printf("%s (%s) : new interface loaded (%d)\n", INFO_STR, __func__, id_type);
         }
#endif
         iq->fns = NULL;
         for(;interfacesFns[i].get_type;i++) {
            int type=interfacesFns[i].get_type();
            if(type==id_type) {
               iq->fns = &(interfacesFns[i]);
            }
         }
 
         if(iq->fns) {
            void *ptr = NULL;
            if(iq->fns->malloc_and_init != NULL) {
               ptr = iq->fns->malloc_and_init(sqlite3_param_db, i, id_interface, name, dev, parameters, description);
            }
            else {
               ptr = iq->fns->malloc_and_init2(i, jsonInterface);
            }
 
            if(ptr) {
               iq->context=ptr;
               monitoring_id = iq->fns->get_monitoring_id(ptr);
            }
         }
 
         if(monitoring_id!=-1) {
            iq->id=id_interface;
            iq->type=id_type;
            iq->delegate_flag=0;
            strncpy(iq->name, (const char *)name, sizeof(iq->name));
            strncpy(iq->dev, (const char *)dev, sizeof(iq->dev));
            mea_queue_in_elem(_interfaces, iq);
            process_start(monitoring_id, NULL, 0);
         }
         else {
            free(iq);
            iq=NULL;
         }
      }
      else if(state==2) {
         iq=(interfaces_queue_elem_t *)malloc(sizeof(interfaces_queue_elem_t));
         if(iq==NULL) {
            VERBOSE(1) {
               mea_log_printf("%s (%s) : %s - ",ERROR_STR,__func__,MALLOC_ERROR_STR);
               perror("");
            }
            goto start_interfaces_clean_exit_S1;
         }
         iq->context = NULL;
         iq->id=id_interface;
         iq->type=id_type;
         iq->delegate_flag=1;
         strncpy(iq->name, (const char *)name, sizeof(iq->name));
         strncpy(iq->dev, (const char *)dev, sizeof(iq->dev));
         mea_queue_in_elem(_interfaces, iq);
      }
     else {
         VERBOSE(9) mea_log_printf("%s (%s) : %s not activated (state = %d)\n", INFO_STR, __func__, dev, state);
      }
      jsonInterface = jsonInterface->next;
   }
   sortie=1;
 
start_interfaces_clean_exit_S1:
   pthread_rwlock_unlock(&jsonInterfaces_rwlock);
   pthread_cleanup_pop(0);
 
start_interfaces_clean_exit_S2:
   pthread_rwlock_unlock(&interfaces_queue_rwlock);
   pthread_cleanup_pop(0);
 
start_interfaces_clean_exit_S3:
   if(sortie==0) {
      stop_interfaces(); // stop fait le free de interfaces.
      return NULL;
   }
 
   // associer ici les délégués aux interfaces réelles ou suppression si pas d'interface réelle chargée
   link_delegates(_interfaces);
   clean_not_linked(_interfaces);
 
   return _interfaces;
}
 
 
int restart_interfaces(int my_id, void *data, char *errmsg, int l_errmsg)
{
   struct interfacesServerData_s *interfacesServerData = (struct interfacesServerData_s *)data;
 
   stop_interfaces();
   sleep(1);
   start_interfaces(interfacesServerData->params_list, interfacesServerData->sqlite3_param_db);
 
   return 0;
}

