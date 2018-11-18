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
#include "mea_json_utils.h"

#include "uthash.h"

#include "processManager.h"

#include "interfacesServer.h"
#include "interface_type_001.h"
#include "interface_type_002.h"
#include "interface_type_003.h"
#include "interface_type_004.h"
#include "interface_type_005.h"
#include "interface_type_006.h"


int update_interfaces(int id);
int remove_interface(int id_interface);

#define MAX_INTERFACES_PLUGINS 10 // au demarrage et pour les statics

struct interfacesServer_interfaceFns_s *interfacesFns=NULL;
int interfacesFns_nb = 0;
int interfacesFns_max = MAX_INTERFACES_PLUGINS;

mea_queue_t *_interfaces=NULL;

pthread_rwlock_t interfaces_queue_rwlock;
pthread_rwlock_t jsonInterfaces_rwlock;

char *interface_defaults="{ \
\"name\":\"\", \
\"id_type\":-1, \
\"description\":\"\", \
\"parameters\":\"\", \
\"state\":-1 \
}";


/*
* New functions
*/
cJSON *jsonInterfaces = NULL;
cJSON *jsonTypes      = NULL;

int nextDeviceId = -1;

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



void deleteDevicesIndex(struct devices_index_s **devices_index)
{
   struct devices_index_s *e = NULL, *tmp = NULL;
 
   HASH_ITER(hh, *devices_index, e, tmp) {
      if(e) {
         HASH_DEL(*devices_index, e);
         free(e);
      }
   }
   *devices_index = NULL;
}


void deleteTypesIndex(struct types_index_s **types_index)
{
   struct types_index_s *e = NULL, *tmp = NULL;
 
   HASH_ITER(hh, *types_index, e, tmp) {
      if(e) {
         HASH_DEL(*types_index, e);
         free(e);
      }
   }
   *types_index = NULL;
}


void deleteDevsIndex(struct devs_index_s **devs_index)
{
   struct devs_index_s *e = NULL, *tmp = NULL;
 
   HASH_ITER(hh, *devs_index, e, tmp) {
      if(e) {
         HASH_DEL(*devs_index, e);
         free(e);
      }
   }
   *devs_index = NULL;
}


int createDevsIndex(struct devs_index_s **devs_index, cJSON *jsonInterfaces)
{
   if(!jsonInterfaces)
      return -1;
 
   cJSON *jsonInterface = jsonInterfaces->child;
   while( jsonInterface ) {
      char *devName = cJSON_GetObjectItem(jsonInterface, "dev")->valuestring;
 
      struct devs_index_s *e=(struct devs_index_s *)malloc(sizeof(struct devs_index_s));
      mea_strncpytrimlower(e->devName, devName, sizeof(e->devName));
      e->interface=jsonInterface;
      HASH_ADD_STR(*devs_index, devName, e);
      jsonInterface=jsonInterface->next;
   }
 
   return 0;
}


int createDevicesIndex(struct devices_index_s **devices_index, cJSON *jsonInterfaces)
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
            HASH_ADD_STR(*devices_index, name, e);
 
            jsonDevice=jsonDevice->next;
         }
      }
      jsonInterface = jsonInterface->next;
   }
 
   return 0;
}


int createTypesIndex(struct types_index_s **types_index, cJSON *jsonTypes)
{
   if(!jsonTypes)
      return -1;
 
   cJSON *jsonType = jsonTypes->child;
   while( jsonType ) {
      int id_type = cJSON_GetObjectItem(jsonType, "id_type")->valuedouble;

      struct types_index_s *e=(struct types_index_s *)malloc(sizeof(struct types_index_s));
      e->id_type = id_type;
      e->type = jsonType;
 
      HASH_ADD_INT(*types_index, id_type, e);

      jsonType=jsonType->next;
   }
 
   return 0;
}


cJSON *findInterfaceByIdThroughLoop_alloc(cJSON *jsonInterfaces, int id)
{
   if(!jsonInterfaces)
      return NULL;

   cJSON *jsonInterface = jsonInterfaces->child;
   while( jsonInterface ) {
      int id_interface = (int)cJSON_GetObjectItem(jsonInterface, "id_interface")->valuedouble;

      if(id_interface==id) {
         return cJSON_Duplicate(jsonInterface, 1);
      }

      jsonInterface=jsonInterface->next;
   }

   return NULL;
}


cJSON *getInterfaceById_alloc(int id)
{
   cJSON *jsonInterface=NULL;

   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&jsonInterfaces_rwlock);
   pthread_rwlock_rdlock(&jsonInterfaces_rwlock);

   jsonInterface=findInterfaceByIdThroughLoop_alloc(jsonInterfaces, id);

   pthread_rwlock_unlock(&jsonInterfaces_rwlock);
   pthread_cleanup_pop(0);

   return jsonInterface;
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


cJSON *getInterfaceByDevName_alloc(char *devName)
{
   cJSON *jsonInterface=NULL;

   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&jsonInterfaces_rwlock);
   pthread_rwlock_rdlock(&jsonInterfaces_rwlock);

   jsonInterface=findInterfaceByDevNameThroughIndex_alloc(devs_index, devName);

   pthread_rwlock_unlock(&jsonInterfaces_rwlock);
   pthread_cleanup_pop(0);

   return jsonInterface;
}


cJSON *findDeviceByNameThroughIndex_alloc(struct devices_index_s *devices_index, char *name)
{
   struct devices_index_s *e = NULL;
 
   if(!devices_index)
      return NULL;
 
   cJSON *d = NULL;
 
   HASH_FIND_STR(devices_index, name, e);
   if(e) {
      d=cJSON_Duplicate(e->device, 1);
   }
 
   return d;
}


cJSON *findTypeByIdThroughIndex(struct types_index_s *types_index, int id)
{
   struct types_index_s *e = NULL;
   if(!types_index)
      return NULL;
 
   cJSON *t = NULL;
   HASH_FIND_INT(types_index, &id, e);
   if(e) {
      t=e->type;
   }
   return t;
}


int deleteInterface(char *interface)
{
   cJSON *jsonInterface = NULL;
   int id_interface = -1;

   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&jsonInterfaces_rwlock);
   pthread_rwlock_wrlock(&jsonInterfaces_rwlock);

   jsonInterface = cJSON_GetObjectItem(jsonInterfaces, interface);
   if(jsonInterface)
      id_interface=(int)cJSON_GetObjectItem(jsonInterface, "id_interface")->valuedouble;

   pthread_rwlock_unlock(&jsonInterfaces_rwlock);
   pthread_cleanup_pop(0);

   if(id_interface==-1)
      return -1;

   remove_interface(id_interface);

   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&jsonInterfaces_rwlock);
   pthread_rwlock_wrlock(&jsonInterfaces_rwlock);

   cJSON_DeleteItemFromObject(jsonInterfaces, interface);

   pthread_rwlock_unlock(&jsonInterfaces_rwlock);
   pthread_cleanup_pop(0);

   return 0;
}


int addInterface(cJSON *jsonData)
{
   if(jsonData == NULL || jsonData->type != cJSON_Object)
      return -1;

   cJSON *newInterface=cJSON_Parse(interface_defaults);
   if(!newInterface)
      return -1;

   cJSON *e=jsonData->child;
   while(e) {
      cJSON *d=cJSON_GetObjectItem(newInterface, e->string);
      if(d != NULL && d->type==e->type) {
         cJSON_DeleteItemFromObject(newInterface, e->string);
         cJSON_AddItemToArray(newInterface, cJSON_Duplicate(e,1));
      } 
      e=e->next;
   }

   char name[41];
   int id_type=(int)cJSON_GetObjectItem(newInterface, "id_type")->valuedouble;
   int state=(int)cJSON_GetObjectItem(newInterface, "state")->valuedouble;
   char *_name=(char *)cJSON_GetObjectItem(newInterface, "name")->valuestring;

   if(strlen(_name)>=3) { // ajouter check du nom de l'interface (que des lettres, chiffres et _)
      strncpy(name, _name, sizeof(name)-1);
      mea_strtoupper(name);
      cJSON_DeleteItemFromObject(newInterface, "name");
   }
   else
      goto addInterface_clean_exit;

   if(state<0 && state>2)
      goto addInterface_clean_exit;

   if(id_type<0) // ajouter le check de l'id interface dans jsonTypes
      goto addInterface_clean_exit;

   cJSON_AddItemToObject(newInterface, "devices", cJSON_CreateObject());

   int ret=-1; 
   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&jsonInterfaces_rwlock);
   pthread_rwlock_wrlock(&jsonInterfaces_rwlock);

   if(!cJSON_GetObjectItem(jsonInterfaces, name)) {
      cJSON_AddItemToObject(jsonInterfaces, name, newInterface);
      ret=0;
   }

   pthread_rwlock_unlock(&jsonInterfaces_rwlock);
   pthread_cleanup_pop(0);

   if(ret==0) {
//
// rechargement des interfaces
//
   }

   return ret;

addInterface_clean_exit:
   if(newInterface)
      cJSON_Delete(newInterface);

   return -1;
}


int updateInterface(char *interface, cJSON *jsonData)
{
   char *s=cJSON_Print(jsonData);
   if(s) {
      free(s);
   }

   return 0;
}


int updateDevice(char *interface, char *name, cJSON *device)
{
   int ret=-1;

   cJSON *obj = NULL;
   int  id_type=-1;
   char *parameters=NULL;
   char *description=NULL;
   int state=-1;
   int id_interface=-1;

   //
   // verification des parametres
   //
   obj=cJSON_GetObjectItem(device, "id_type");
   if(obj && obj->type==cJSON_Number) {
      id_type=(int)obj->valuedouble;
      cJSON *jsonType=findTypeByIdThroughIndex(types_index, id_type);
      if(jsonType==NULL || (int)cJSON_GetObjectItem(jsonType, "typeoftype")->valuedouble == 10)
         return -3;
   }

   obj=cJSON_GetObjectItem(device, "state");
   if(obj && obj->type==cJSON_Number) {
      if(obj->valuedouble!=0.0 && obj->valuedouble!=1.0)
         return -4;
      else
         state=(int)obj->valuedouble;
   }

   obj=cJSON_GetObjectItem(device, "parameters");
   if(obj && obj->type==cJSON_String)
      parameters=obj->valuestring;

   obj=cJSON_GetObjectItem(device, "description");
   if(obj &&obj->type==cJSON_String)
      description=obj->valuestring;

   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&jsonInterfaces_rwlock);
   pthread_rwlock_wrlock(&jsonInterfaces_rwlock);

   if(jsonInterfaces) {
      ret=-10;
      cJSON *jsonInterface=cJSON_GetObjectItem(jsonInterfaces, interface);
      if(jsonInterface) {
         id_interface=(int)cJSON_GetObjectItem(jsonInterface, "id_interface")->valuedouble;
         ret=-11;
         cJSON *jsonDevices=cJSON_GetObjectItem(jsonInterface, "devices");
         if(jsonDevices) {
            cJSON *d=cJSON_GetObjectItem(jsonDevices, name); 
            ret=-12;
            if(d) {
               if(id_type>=0)
                  cJSON_SetNumberValue(cJSON_GetObjectItem(d, "id_type"),(double)id_type);
               if(state>=0)
                  cJSON_SetNumberValue(cJSON_GetObjectItem(d, "state"),(double)state);
               if(parameters) {
                  cJSON_DeleteItemFromObject(d, "parameters");
                  cJSON_AddStringToObject(d, "parameters", parameters);
               }
               if(description) {
                  cJSON_DeleteItemFromObject(d, "description");
                  cJSON_AddStringToObject(d, "description", description);
               }
               
               ret=0;
            }
         }
      }
   }

   pthread_rwlock_unlock(&jsonInterfaces_rwlock);
   pthread_cleanup_pop(0);

   if(ret==0)
      update_interfaces(id_interface);

   return ret;
}


int deleteDevice(char *interface, char *name)
{
   int ret=-1;
   int id_interface=-1;

   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&jsonInterfaces_rwlock);
   pthread_rwlock_wrlock(&jsonInterfaces_rwlock);

   if(jsonInterfaces) {
      ret=-2;
      cJSON *jsonInterface=cJSON_GetObjectItem(jsonInterfaces, interface);
      if(jsonInterface) {
         id_interface=(int)cJSON_GetObjectItem(jsonInterface, "id_interface")->valuedouble;

         ret=-3;
         cJSON *jsonDevices=cJSON_GetObjectItem(jsonInterface, "devices");
         if(jsonDevices) {
            struct devices_index_s *e=NULL;
            HASH_FIND_STR(devices_index, name, e);
            if(e) {
               HASH_DEL(devices_index, e);
               free(e);
            } 
            cJSON_DeleteItemFromObject(jsonDevices, name); 
            ret=0;
         }
      }
   }

   pthread_rwlock_unlock(&jsonInterfaces_rwlock);
   pthread_cleanup_pop(0);

   if(ret==0)
      update_interfaces(id_interface);

   return ret;
}


int addDevice(char *interface, cJSON *device)
{
   int ret=-1;

   cJSON *obj = NULL;
   char *name=NULL;
   int  id_type=-1;
   char *parameters=NULL;
   char *description=NULL;
   int state=-1;
   int id_interface=-1;

   //
   // verification des parametres
   //
   obj=cJSON_GetObjectItem(device, "name");
   if(obj && obj->type==cJSON_String)
      name=obj->valuestring;
   else
      return -2;
   mea_strtoupper(name);

   obj=cJSON_GetObjectItem(device, "id_type");
   if(obj && obj->type==cJSON_Number) {
      id_type=(int)obj->valuedouble;
      cJSON *jsonType=findTypeByIdThroughIndex(types_index, id_type);
      if(jsonType==NULL || (int)cJSON_GetObjectItem(jsonType, "typeoftype")->valuedouble == 10)
         return -3;
   }
   else
      return -3;

   obj=cJSON_GetObjectItem(device, "state");
   if(obj && obj->type==cJSON_Number)
      if(obj->valuedouble!=0.0 && obj->valuedouble!=1.0)
         return -4;
      else
         state=(int)obj->valuedouble;
   else
      return -4;

   obj=cJSON_GetObjectItem(device, "parameters");
   if(!obj)
      parameters="";
   else if(obj->type==cJSON_String)
      parameters=obj->valuestring;
   else
      return -5;

   obj=cJSON_GetObjectItem(device, "description");
   if(!obj)
      description="";
   else if(obj->type==cJSON_String)
      description=obj->valuestring;
   else
      return -6;


   //
   // ajouts
   //   
   cJSON *jsonDevice=cJSON_CreateObject();
   cJSON_AddStringToObject(jsonDevice,"parameters",parameters);
   cJSON_AddStringToObject(jsonDevice,"description",description);
   cJSON_AddNumberToObject(jsonDevice,"id_type",id_type);
   cJSON_AddNumberToObject(jsonDevice,"state",state);
   cJSON_AddNumberToObject(jsonDevice,"id_sensor_actuator",(double)(nextDeviceId++));

   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&jsonInterfaces_rwlock);
   pthread_rwlock_wrlock(&jsonInterfaces_rwlock);
 
   ret=-9;
   if(jsonInterfaces) {
      ret=-10;
      cJSON *jsonInterface=cJSON_GetObjectItem(jsonInterfaces, interface);
      if(jsonInterface) {
         id_interface=(int)cJSON_GetObjectItem(jsonInterface, "id_interface")->valuedouble;
         ret=-11;
         cJSON *jsonDevices=cJSON_GetObjectItem(jsonInterface, "devices");
         if(jsonDevices) {
            cJSON *d=cJSON_GetObjectItem(jsonDevices, name); 
            ret=-12;
            if(!d) {
               cJSON_AddNumberToObject(jsonDevice,"id_interface",(int)cJSON_GetObjectItem(jsonInterface,"id_interface")->valuedouble);
               ret=-13; 
               struct devices_index_s *e=(struct devices_index_s *)malloc(sizeof(struct devices_index_s));
               if(e) {
                  strncpy(e->name, name, sizeof(e->name));
                  e->device=jsonDevice;
                  HASH_ADD_STR(devices_index, name, e);

                  cJSON_AddItemToObject(jsonDevices, name, jsonDevice);
                  ret=0;
               }
            }
         }
      }
   }

   if(ret==0)
      update_interfaces(id_interface);

   pthread_rwlock_unlock(&jsonInterfaces_rwlock);
   pthread_cleanup_pop(0);

   return ret; 
}


char *getDevicesAsString_alloc(char *interface)
{
   char *s=NULL;

   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&jsonInterfaces_rwlock);
   pthread_rwlock_rdlock(&jsonInterfaces_rwlock);

   if(jsonInterfaces) {
      cJSON *jsonInterface=cJSON_GetObjectItem(jsonInterfaces, interface);
      if(jsonInterface) {
         cJSON *jsonDevices=cJSON_GetObjectItem(jsonInterface, "devices");
         if(jsonDevices) {
            s=cJSON_Print(jsonDevices);
         }
      }
   }

   pthread_rwlock_unlock(&jsonInterfaces_rwlock);
   pthread_cleanup_pop(0);

   return s;
}


char *getDeviceAsStringByName_alloc(char *interface, char *device)
{
   char *s=NULL;

   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&jsonInterfaces_rwlock);
   pthread_rwlock_rdlock(&jsonInterfaces_rwlock);

   if(jsonInterfaces) {
      cJSON *jsonInterface=cJSON_GetObjectItem(jsonInterfaces, interface);
      if(jsonInterface) {
         cJSON *jsonDevices=cJSON_GetObjectItem(jsonInterface, "devices");
         if(jsonDevices) {
            cJSON *jsonDevice=cJSON_GetObjectItem(jsonDevices, device);
            if(jsonDevices) {
               s=cJSON_Print(jsonDevice);
            }
         }
      }
   }

   pthread_rwlock_unlock(&jsonInterfaces_rwlock);
   pthread_cleanup_pop(0);

   return s;
}


char *getInterfaceAsStringByName_alloc(char *name)
{
   char *s=NULL;

   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&jsonInterfaces_rwlock);
   pthread_rwlock_rdlock(&jsonInterfaces_rwlock);

   cJSON *jsonInterface = cJSON_GetObjectItem(jsonInterfaces, name);
   if(jsonInterface) {
      s=cJSON_Print(jsonInterface);
   }

   pthread_rwlock_unlock(&jsonInterfaces_rwlock);
   pthread_cleanup_pop(0);

   return s;
}


char *getInterfacesAsString_alloc()
{
   char *s=NULL;

   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&jsonInterfaces_rwlock);
   pthread_rwlock_rdlock(&jsonInterfaces_rwlock);

   s=cJSON_Print(jsonInterfaces);

   pthread_rwlock_unlock(&jsonInterfaces_rwlock);
   pthread_cleanup_pop(0);

   return s;
}

char *getTypeAsStringByName_alloc(char *name)
{
   char *s=NULL;

   cJSON *jsonType = cJSON_GetObjectItem(jsonTypes, name);
   if(jsonType) {
      s=cJSON_Print(jsonType);
   }

   return s;
}


char *getTypesAsString_alloc()
{
   char *s=NULL;

   s=cJSON_Print(jsonTypes);

   return s;
}


int checkJsonDevices(cJSON *jsonDevices)
{
   if(!jsonDevices)
      return -1;

   if(!(jsonDevices->type == cJSON_Object))
      return -1;

   cJSON *jsonDevice=jsonDevices->child;
   while(jsonDevice) {

      cJSON *json_id_type = cJSON_GetObjectItem(jsonDevice, "id_type");
      cJSON *json_id_description = cJSON_GetObjectItem(jsonDevice, "description");
      cJSON *json_parameters = cJSON_GetObjectItem(jsonDevice, "parameters");
      cJSON *json_state = cJSON_GetObjectItem(jsonDevice, "state");

      if( (json_id_type == NULL) || (!json_id_type->type == cJSON_Number))
         return -1;
      if( (json_id_description == NULL) || !(json_id_description->type == cJSON_String))
         return -1;
      if( (json_parameters == NULL) || !(json_parameters->type == cJSON_String))
         return -1;
      if( (json_state == NULL) || !(json_state->type == cJSON_Number))
         return -1;

      jsonDevice=jsonDevice->next;
   }
   return 0;
}


int checkJsonInterfaces(cJSON *jsonInterfaces)
{
   if(!jsonInterfaces)
      return -1;

   if(!(jsonInterfaces->type == cJSON_Object))
      return -1;

   cJSON *jsonInterface=jsonInterfaces->child;
   while(jsonInterface) {

      cJSON *json_id_type = cJSON_GetObjectItem(jsonInterface, "id_type");
      cJSON *json_id_description = cJSON_GetObjectItem(jsonInterface, "description");
      cJSON *json_dev = cJSON_GetObjectItem(jsonInterface, "dev");
      cJSON *json_parameters = cJSON_GetObjectItem(jsonInterface, "parameters");
      cJSON *json_state = cJSON_GetObjectItem(jsonInterface, "state");
      cJSON *json_devices = cJSON_GetObjectItem(jsonInterface, "devices");

      if( (json_id_type == NULL) || !(json_id_type->type == cJSON_Number)) {
         return -1;
      }
      if( (json_id_description == NULL) || !(json_id_description->type == cJSON_String)) {
         return -1;
      }
      if( (json_dev == NULL) || !(json_dev->type == cJSON_String)) {
         return -1;
      }
      if( (json_parameters == NULL) || !(json_parameters->type == cJSON_String)) {
         return -1;
      }
      if( (json_state == NULL) || !(json_state->type == cJSON_Number)) {
         return -1;
      }

      if(json_devices!=NULL) {
         if(!(json_devices->type == cJSON_Object)) {
            return -1;
         }
         if(checkJsonDevices(json_devices)==-1) {
            return -1;
         }
      }

      jsonInterface=jsonInterface->next;
   }

   return 0;
}


int relinkInterfacesDevices(cJSON *jsonInterfaces)
{
   int id_device_cptr=1000;
   int id_interface_cptr=100;

   if(!jsonInterfaces) {
      return -1;
   }
   else {
      cJSON *jsonInterface=jsonInterfaces->child;
      while(jsonInterface) {

         cJSON *json_id_interface = cJSON_GetObjectItem(jsonInterface,"id_interface");
         if(!json_id_interface) {
             json_id_interface=cJSON_CreateNumber((double)id_interface_cptr);
             cJSON_AddItemToObject(jsonInterface, "id_interface", json_id_interface);
         }
         else {
            cJSON_SetNumberValue(json_id_interface, (double)id_interface_cptr);
         }

         int id_interface=id_interface_cptr;

         cJSON *jsonDevices=cJSON_GetObjectItem(jsonInterface, "devices");
         if(jsonDevices) {
            cJSON *jsonDevice=jsonDevices->child;
            while(jsonDevice) {
               cJSON *json_id_sensor_actuator = cJSON_GetObjectItem(jsonDevice,"id_sensor_actuator");
               if(!json_id_sensor_actuator) {
                  json_id_sensor_actuator=cJSON_CreateNumber((double)id_device_cptr);
                  cJSON_AddItemToObject(jsonDevice, "id_sensor_actuator", json_id_sensor_actuator);
               }
               else {
                  cJSON_SetNumberValue(json_id_sensor_actuator, (double)id_device_cptr);
               }
               id_device_cptr++;

               cJSON *json_id_interface = cJSON_GetObjectItem(jsonDevice,"id_interface");
               if(!json_id_interface) {
                  json_id_interface=cJSON_CreateNumber((double)id_interface);
                  cJSON_AddItemToObject(jsonDevice, "id_interface", json_id_interface);
               }
               else {
                  cJSON_SetNumberValue(json_id_interface, (double)id_interface);
               }

               jsonDevice=jsonDevice->next;
            }
         }

         id_interface_cptr++;
         jsonInterface=jsonInterface->next;
      }
   }
//   { char *s=cJSON_Print(jsonInterfaces); printf("%s\n",s); free(s); }
   return id_device_cptr;
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

/*
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
*/

cJSON *jsonInterfacesLoad(char *file)
{
   cJSON *_jsonInterfaces=loadJson(file);
   if(_jsonInterfaces==NULL)
      return NULL;

   if(checkJsonInterfaces(_jsonInterfaces)==-1) {
      cJSON_Delete(_jsonInterfaces);
      return NULL;
   }

   nextDeviceId=relinkInterfacesDevices(_jsonInterfaces);

   return _jsonInterfaces;
}


cJSON *jsonTypesLoad(char *file)
{
   cJSON *_jsonTypes=loadJson(file);
   if(_jsonTypes==NULL)
      return NULL;

   if(checkJsonTypes(_jsonTypes)==-1) {
      cJSON_Delete(_jsonTypes);
      return NULL;
   }

   return _jsonTypes;
}


int resyncDevices(cJSON *jsonInterfaces, char *file)
{
   return 0;
}

 
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

   int type_id = (int)cJSON_GetObjectItem(jsonDevice, "id_type")->valuedouble;

   if(!jsonType) {
      jsonType = findTypeByIdThroughIndex(types_index, type_id);
   }

   if(!jsonType)
      return -1;

   device_info->name              = (char *)jsonDevice->string;
   device_info->id                =    (int)cJSON_GetObjectItem(jsonDevice, "id_sensor_actuator")->valuedouble;
   device_info->parameters        = (char *)cJSON_GetObjectItem(jsonDevice, "parameters")->valuestring;
   device_info->state             =    (int)cJSON_GetObjectItem(jsonDevice, "state")->valuedouble;
   device_info->type_id           = type_id;

   device_info->type_name         = (char *)jsonType->string;
   device_info->type_parameters   = (char *)cJSON_GetObjectItem(jsonType, "parameters")->valuestring;;
   device_info->typeoftype_id     =    (int)cJSON_GetObjectItem(jsonType, "typeoftype")->valuedouble;
 
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
   int ret=0;
 
   interfaces_queue_elem_t *iq;
 
   DEBUG_SECTION mea_log_printf("%s (%s) : reception message xPL\n", INFO_STR, __func__);
 
   cJSON *device = NULL;
   device=cJSON_GetObjectItem(xplMsgJson, XPL_DEVICE_STR_C);
   if(!device) {
      DEBUG_SECTION mea_log_printf("%s (%s) : message discarded\n", INFO_STR, __func__);
      ret=-1;
      goto exit;
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
      ret=-1;
      goto exit;
   }
 
   state=(int)cJSON_GetObjectItem(jsonDevice, "state")->valuedouble;
   id_interface=(int)cJSON_GetObjectItem(jsonDevice, "id_interface")->valuedouble;
   struct device_info_s device_info;
   ret=device_info_from_json(&device_info, jsonDevice, NULL, NULL);
   cJSON_Delete(jsonDevice);
   if(ret==-1) 
      goto exit;

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
            if(ret<0) {
               ret=-1;
               break;
            }
         }
      }
   }

   pthread_rwlock_unlock(&interfaces_queue_rwlock);
   pthread_cleanup_pop(0);

exit:
   cJSON_Delete(xplMsgJson);
 
   return ret;
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
 
 
int init_interfaces_list(cJSON *jsonInterfaces, cJSON *jsonType)
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
   while(plugin_info_defaults[next_int].type!=-1) {
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

//      cJSON *jsonType=findTypeByIdThroughIndex_alloc(types_index, id_type);
      cJSON *jsonType=findTypeByIdThroughIndex(types_index, id_type);
      if(!jsonType) {
         VERBOSE(2) mea_log_printf("%s (%s) : unknown type - %d\n", WARNING_STR, __func__, id_type);
         goto next;
      }
      char *type_parameters=cJSON_GetObjectItem(jsonType, "parameters")->valuestring;

      for(int i=0; i<next_int; i++) {
         if(plugins_list[i].type == id_type) {
            VERBOSE(2) mea_log_printf("%s (%s) : interface type %d allread loaded\n", INFO_STR, __func__, plugins_list[i].type);
            continue;
         }
      }
 
      char *p = mea_strltrim((char *)type_parameters);
      if(p) {
         char lib_name[255];
         ret=sscanf(p, "[%[^]]]%*[^\n]", lib_name);
         if(ret == 1) {
            if(p[strlen(lib_name)+1]==']') {
               int l=strlen(mea_strtrim(lib_name));
               if(l>0) {
                  plugins_list = (struct plugin_info_s *)realloc(plugins_list, sizeof(struct plugin_info_s)*(next_int+2));
                  plugins_list[next_int].name = malloc(l+1);
                  strcpy(plugins_list[next_int].name, mea_strtrim(lib_name));
                  plugins_list[next_int].type = id_type;
                  plugins_list[next_int].free_flag  = 1;
 
                  next_int++;
               }
            }
            else {
               VERBOSE(2) mea_log_printf("%s (%s) : interface parameter error - %s\n", WARNING_STR, __func__, p);
               goto next;
            }
         }
      }
      else {
         VERBOSE(2) mea_log_printf("%s (%s) : interface parameter error - %s\n", WARNING_STR, __func__, (char *)type_parameters);
        goto next;
      }

next:
//      cJSON_Delete(jsonType);
      jsonInterface = jsonInterface->next;
   }
 
   pthread_rwlock_unlock(&jsonInterfaces_rwlock);
   pthread_cleanup_pop(0);
 
   plugins_list[next_int].type = -1;
   plugins_list[next_int].name = NULL;
   plugins_list[next_int].free_flag  = -1;
/*
   for(int i=0; plugins_list[i].type!=-1; i++)
      printf("%s %d %d\n", plugins_list[i].name, plugins_list[i].type, plugins_list[i].free_flag);
*/
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
 

int remove_interface(int id_interface)
{
   interfaces_queue_elem_t *iq;
   int ret=-1;

   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&interfaces_queue_rwlock);
   pthread_rwlock_wrlock(&interfaces_queue_rwlock);
 
   if(_interfaces && _interfaces->nb_elem) {

      mea_queue_first(_interfaces);
      while(1) {
         ret=mea_queue_current(_interfaces, (void **)&iq);

         if(ret==ERROR) {
            ret=-1;
            break; 
         }

         if(iq->id == id_interface) {
 
            int monitoring_id = iq->fns->get_monitoring_id(iq->context);
 
            if(iq->delegate_flag == 0 && monitoring_id>-1 && process_is_running(monitoring_id)) {
               iq->fns->set_xPLCallback(iq->context, NULL);
 
               if(monitoring_id!=-1) {
                  void *start_stop_params = process_get_data_ptr(monitoring_id);
 
                  process_stop(monitoring_id, NULL, 0);
                  process_unregister(monitoring_id);
 
                  iq->fns->set_monitoring_id(iq->context, -1);
 
                  if(start_stop_params) {
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

            mea_queue_remove_current(_interfaces);
            ret=0;
            break;
         }

         if(mea_queue_next(_interfaces)==ERROR) {
            ret=-1;
            break;
         }
      }
   }

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

   if(interfacesFns) {
      unload_interfaces_fns(interfacesFns);
      free(interfacesFns);
      interfacesFns=NULL;
   }
 
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
      deleteDevicesIndex(&devices_index);
      devices_index=NULL;
   }

   if(devs_index) {
      deleteDevsIndex(&devs_index);
      devs_index=NULL;
   }

   if(types_index) {
      deleteTypesIndex(&types_index);
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
 
 
int start_interfaces_load_json(cJSON *params_list)
{
    if(jsonInterfaces) {
      cJSON_Delete(jsonInterfaces);
      jsonInterfaces=NULL;
   }

   if(jsonTypes) {
      cJSON_Delete(jsonTypes);
      jsonTypes=NULL;
   }

   if(1) {
      char jsonInterfacesFile[256];
      snprintf(jsonInterfacesFile, sizeof(jsonInterfacesFile)-1, "%s/etc/interfaces.json", appParameters_get("MEAPATH", params_list));
      jsonInterfaces=jsonInterfacesLoad(jsonInterfacesFile);
      if(!jsonInterfaces) {
         VERBOSE(1) {
            mea_log_printf("%s (%s) : can't load interfaces descriptions\n",ERROR_STR,__func__);
         }
         return -1;
      }
      createDevicesIndex(&devices_index, jsonInterfaces);
      createDevsIndex(&devs_index, jsonInterfaces);
   }

   if(1) {
      char jsonTypesFile[256];
      snprintf(jsonTypesFile, sizeof(jsonTypesFile)-1, "%s/etc/types.json", appParameters_get("MEAPATH", params_list));
      jsonTypes=jsonTypesLoad(jsonTypesFile);
      if(!jsonTypes) {
         VERBOSE(1) {
            mea_log_printf("%s (%s) : can't load types descriptions",ERROR_STR,__func__);
         }
         return -1;
      }
      createTypesIndex(&types_index, jsonTypes);
   }
}


int update_interfaces(int id_interface)
{
   printf("Update Interfaces (%d) requested\n", id_interface);

   interfaces_queue_elem_t *iq;
   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&interfaces_queue_rwlock);
   pthread_rwlock_rdlock(&interfaces_queue_rwlock);

   if(_interfaces && _interfaces->nb_elem) {
      mea_queue_first(_interfaces);
      while(1) {
         mea_queue_current(_interfaces, (void **)&iq);
         if(iq->context && id_interface == iq->id) {
            if(iq->fns->update_devices)
               iq->fns->update_devices(iq->context);
            break;
         }
         int ret=mea_queue_next(_interfaces);
         if(ret<0) {
            ret=-1;
            break;
         }
      }
   }

   pthread_rwlock_unlock(&interfaces_queue_rwlock);
   pthread_cleanup_pop(0);
   return 0;
}


mea_queue_t *start_interfaces(cJSON *params_list)
{
   int16_t ret;
   int sortie=0;
   interfaces_queue_elem_t *iq;


   pthread_rwlock_init(&interfaces_queue_rwlock, NULL);
   pthread_rwlock_init(&jsonInterfaces_rwlock, NULL);
 
   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&jsonInterfaces_rwlock);
   pthread_rwlock_wrlock(&jsonInterfaces_rwlock);

   ret=start_interfaces_load_json(params_list);

   pthread_rwlock_unlock(&jsonInterfaces_rwlock);
   pthread_cleanup_pop(0);

   if(ret==-1 || !jsonInterfaces || !jsonTypes) {
      goto start_interfaces_clean_exit_S3;
   }
    
#ifdef ASPLUGIN
   init_interfaces_list(jsonInterfaces, jsonTypes);
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
      int  id_interface=cJSON_GetObjectItem(jsonInterface, "id_interface")->valuedouble;
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
         int i=0;
         for(;interfacesFns[i].get_type;i++) {
            int type=interfacesFns[i].get_type();
            if(type==id_type) {
               iq->fns = &(interfacesFns[i]);
            }
         }
 
         if(iq->fns) {
            void *ptr = NULL;
            if(iq->fns->malloc_and_init2 != NULL) {
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
   start_interfaces(interfacesServerData->params_list);
 
   return 0;
}

