//
//  interfacesServer.c
//  mea-edomus
//
//  Created by Patrice Dietsch on 04/11/2013.
//
//
#include <stdio.h>
#include <termios.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <dlfcn.h>

#include "configuration.h"

#include "tokens_da.h"

#include "mea_string_utils.h"
#include "mea_queue.h"
#include "mea_verbose.h"

#include "cJSON.h"
#include "mea_json_utils.h"
#include "mea_file_utils.h"
#include "uthash.h"

#include "processManager.h"

#include "interfacesServer.h"
#include "interface_type_001.h"
#ifndef ASPLUGIN
#include "interface_type_002.h"
#include "interface_type_003.h"
// #include "interface_type_004.h"
// #include "interface_type_005.h"
#include "interface_type_006.h"
#include "interface_type_010.h"
#endif

int update_interface_devices(int id);

int remove_interface(mea_queue_t *interfaces_list, int id_interface);
int prepare_interface(mea_queue_t *interfaces_list, cJSON *params_list, cJSON *jsonInterface, int start);
int link_delegates(mea_queue_t *interfaces_list);
int remove_delegate_links(mea_queue_t *interfaces_list, char *interface);


#define MAX_INTERFACES_PLUGINS 10 // au demarrage et pour les statics

struct interfacesServer_interfaceFns_s *interfacesFns=NULL;
int interfacesFns_nb = 0;
int interfacesFns_max = MAX_INTERFACES_PLUGINS;

mea_queue_t *_interfaces=NULL;

pthread_rwlock_t interfaces_queue_rwlock;
pthread_rwlock_t jsonInterfaces_rwlock;

char *interface_template="{ \
\"name\":\"\", \
\"id_type\":-1, \
\"description\":\"\", \
\"parameters\":\"\", \
\"dev\":\"\", \
\"state\":-1 \
}";


/*
* New functions
*/
cJSON *jsonInterfaces = NULL;
cJSON *jsonTypes      = NULL;

static cJSON *_params_list=NULL;


int nextDeviceId = -1;
int nextInterfaceId = -1;

struct devices_index_s *devices_index = NULL;
struct types_index_s *types_index = NULL;
struct devs_index_s *devs_index = NULL;

/*
int cmpInterfaces(cJSON *i1, cJSON i2*)
{
   cJSON *j1, *j2;
   
   j1=cJSON_GetObjectItem(i1, PARAMETERS_STR_C);
   j2=cJSON_GetObjectItem(i2, PARAMETERS_STR_C);
   if(strcmp(j1->string, j2->string)!=0)
      return 1;
   j1=cJSON_GetObjectItem(i1, DEV_STR_C);
   j2=cJSON_GetObjectItem(i2, DEV_STR_C);
   if(strcmp(j1->string, j2->string)!=0)
      return 2;

   return 0;
}
*/

struct types_index_s *deleteTypesIndex(struct types_index_s **types_index)
{
   struct types_index_s *e = NULL, *tmp = NULL;
 
   HASH_ITER(hh, *types_index, e, tmp) {
      if(e) {
         HASH_DEL(*types_index, e);
         free(e);
      }
   }
   *types_index = NULL;
   
   return *types_index;
}


struct devs_index_s *addInterfaceToDevsIndex(struct devs_index_s *devs_index, cJSON *jsonInterface)
{
   if(!jsonInterface) {
      return NULL;
   }

   char *devName = cJSON_GetObjectItem(jsonInterface, DEV_STR_C)->valuestring;
 
   struct devs_index_s *e=NULL;
   HASH_FIND_STR(devs_index, devName, e);
   if(e) {
      HASH_DEL(devs_index, e);
      free(e);
       e=NULL;
   }

   e=(struct devs_index_s *)malloc(sizeof(struct devs_index_s));
   mea_strncpytrimlower(e->devName, devName, sizeof(e->devName));
   e->interface=jsonInterface;
   HASH_ADD_STR(devs_index, devName, e);

   return devs_index;
}


struct devices_index_s *addInterfaceDevicesToDevicesIndex(struct devices_index_s *devices_index, cJSON *jsonInterface)
{
   if(!jsonInterface) {
      return NULL;
   }

  cJSON *jsonDevices = cJSON_GetObjectItem(jsonInterface, DEVICES_STR_C);
 
  if(jsonDevices) {
     cJSON *jsonDevice = jsonDevices->child;
      while( jsonDevice ) {
         char *name=jsonDevice->string;
         
         struct devices_index_s *e=NULL;
         HASH_FIND_STR(devices_index, name, e);
         if(e) {
            HASH_DEL(devices_index, e);
            free(e);
            e=NULL;
         }

         e=(struct devices_index_s *)malloc(sizeof(struct devices_index_s));
         mea_strncpytrimlower(e->name, name, sizeof(e->name));
         e->device=jsonDevice;
         HASH_ADD_STR(devices_index, name, e);

         jsonDevice=jsonDevice->next;
      }
   }

   return devices_index;
}


void removeTypeFromIndexByTypeId(struct types_index_s **types_index, int type_id)
{
   struct types_index_s *e = NULL, *tmp = NULL;
   
   HASH_ITER(hh, *types_index, e, tmp) {
      if(e) {
         if(e->type) {
            cJSON *id_type=cJSON_GetObjectItem(e->type,ID_TYPE_STR_C);
            if(id_type && (int)(id_type->valuedouble)==type_id) {
               e->type=NULL;
               HASH_DEL(*types_index, e);
               free(e);
               e=NULL;
               return;
            }
         }
      }
   }
}


struct devs_index_s *removeInterfaceFromIndexByInterfaceId(struct devs_index_s *devs_index, int interface_id)
{
   struct devs_index_s *e = NULL, *tmp = NULL;
   
   HASH_ITER(hh, devs_index, e, tmp) {
      if(e) {
         if(e->interface) {
            cJSON *id_interface=cJSON_GetObjectItem(e->interface,ID_INTERFACE_STR_C);
            if(id_interface && (int)(id_interface->valuedouble)==interface_id) {
               e->interface=NULL;
               HASH_DEL(devs_index, e);
               free(e);
               e=NULL;
               break;
            }
         }
      }
   }
   return devs_index;
}


struct devices_index_s *removeDeviceFromIndexByInterfaceId(struct devices_index_s *devices_index, int interface_id)
{
   struct devices_index_s *e = NULL, *tmp = NULL;
   
   HASH_ITER(hh, devices_index, e, tmp) {
      if(e) {
         if(e->device) {
            cJSON *id_interface=cJSON_GetObjectItem(e->device, ID_INTERFACE_STR_C);
            if(id_interface && (int)(id_interface->valuedouble)==interface_id) {
               e->device=NULL;
               HASH_DEL(devices_index, e);
               free(e);
               e=NULL;
            }
         }
      }
   }
   return devices_index;
}


struct devices_index_s *deleteDevicesIndex(struct devices_index_s **devices_index)
{
   struct devices_index_s *e = NULL, *tmp = NULL;
 
   HASH_ITER(hh, *devices_index, e, tmp) {
      if(e) {
         HASH_DEL(*devices_index, e);
         free(e);
      }
   }
   *devices_index = NULL;
   
   return *devices_index;
}


struct devs_index_s *deleteDevsIndex(struct devs_index_s **devs_index)
{
   struct devs_index_s *e = NULL, *tmp = NULL;
 
   HASH_ITER(hh, *devs_index, e, tmp) {
      if(e) {
	      HASH_DEL(*devs_index, e);
	      free(e);
      }
   }
   *devs_index = NULL;
   
   return *devs_index;
}


int createDevsIndex(struct devs_index_s **devs_index, cJSON *jsonInterfaces)
{
   if(!jsonInterfaces) {
      return -1;
   }
 
   cJSON *jsonInterface = jsonInterfaces->child;
   while( jsonInterface ) {
      char *devName = cJSON_GetObjectItem(jsonInterface, DEV_STR_C)->valuestring;
 
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
   if(!jsonInterfaces) {
      return -1;
   }
 
   cJSON *jsonInterface=jsonInterfaces->child;
   while( jsonInterface ) {
      cJSON *jsonDevices = cJSON_GetObjectItem(jsonInterface, DEVICES_STR_C);
 
      if(jsonDevices) {
         cJSON *jsonDevice = jsonDevices->child;
         while( jsonDevice ) {
            char *name=jsonDevice->string;
            struct devices_index_s *e=(struct devices_index_s *)malloc(sizeof(struct devices_index_s));
            mea_strncpytrimlower(e->name, name, sizeof(e->name));
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
   if(!jsonTypes) {
      return -1;
   }
 
   cJSON *jsonType = jsonTypes->child;
   while( jsonType ) {
      int id_type = cJSON_GetObjectItem(jsonType, ID_TYPE_STR_C)->valuedouble;

      struct types_index_s *e=(struct types_index_s *)malloc(sizeof(struct types_index_s));
      e->id_type = id_type;
      e->type = jsonType;
 
      HASH_ADD_INT(*types_index, id_type, e);

      jsonType=jsonType->next;
   }
 
   return 0;
}


cJSON *findInterfaceByIdThroughLoop_alloc(cJSON *jsonInterfaces, int _id)
{
   if(!jsonInterfaces) {
      return NULL;
   }

   cJSON *jsonInterface = jsonInterfaces->child;
   while( jsonInterface ) {
      int id_interface = (int)cJSON_GetObjectItem(jsonInterface, ID_INTERFACE_STR_C)->valuedouble;

      if(id_interface==_id) {
	      return cJSON_Duplicate(jsonInterface, 1);
      }

      jsonInterface=jsonInterface->next;
   }

   return NULL;
}


cJSON *findInterfaceByNameThroughLoop_alloc(cJSON *jsonInterfaces, char *name)
{
   if(!jsonInterfaces) {
      return NULL;
   }

   cJSON *jsonInterface = jsonInterfaces->child;
   while( jsonInterface ) {
      if(mea_strcmplower(name, jsonInterface->string)==0) {
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


cJSON *getInterfaceByName_alloc(char *name)
{
   cJSON *jsonInterface=NULL;

   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&jsonInterfaces_rwlock);
   pthread_rwlock_rdlock(&jsonInterfaces_rwlock);

   jsonInterface=findInterfaceByNameThroughLoop_alloc(jsonInterfaces, name);

   pthread_rwlock_unlock(&jsonInterfaces_rwlock);
   pthread_cleanup_pop(0);

   return jsonInterface;
}


cJSON *findInterfaceByDevNameThroughIndex_alloc(struct devs_index_s *_devs_index, char *devName)
{
   struct devs_index_s *e = NULL;
 
   if(!_devs_index) {
      _devs_index=devs_index;
   }
 
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
 
   if(!devices_index) {
      return NULL;
   }
 
   int _name_l=(int)strlen(name)+1;
   char *_name=alloca(_name_l);
   mea_strncpytrimlower(_name, name,_name_l);

   cJSON *d = NULL;
   HASH_FIND_STR(devices_index, _name, e);
   if(e) {
      d=cJSON_Duplicate(e->device, 1);
   }
 
   return d;
}


cJSON *findTypeByIdThroughIndex(struct types_index_s *types_index, int id)
{
   struct types_index_s *e = NULL;

   if(!types_index) {
      return NULL;
   }
 
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
      id_interface=(int)cJSON_GetObjectItem(jsonInterface, ID_INTERFACE_STR_C)->valuedouble;

   pthread_rwlock_unlock(&jsonInterfaces_rwlock);
   pthread_cleanup_pop(0);

   if(id_interface==-1) {
      return -1;
   }

   devices_index=removeDeviceFromIndexByInterfaceId(devices_index, id_interface);
   devs_index=removeInterfaceFromIndexByInterfaceId(devs_index, id_interface);

   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&interfaces_queue_rwlock);
   pthread_rwlock_rdlock(&interfaces_queue_rwlock);

   remove_delegate_links(_interfaces, interface);
   remove_interface(_interfaces, id_interface);

   pthread_rwlock_unlock(&interfaces_queue_rwlock);
   pthread_cleanup_pop(0);

   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&jsonInterfaces_rwlock);
   pthread_rwlock_wrlock(&jsonInterfaces_rwlock);

   cJSON_DeleteItemFromObject(jsonInterfaces, interface);

   pthread_rwlock_unlock(&jsonInterfaces_rwlock);
   pthread_cleanup_pop(0);

#ifdef DEBUGFLAG
   
#endif

   return 0;
}


int addInterface(cJSON *jsonData)
{
   if(jsonData == NULL || jsonData->type != cJSON_Object) {
      return -1;
   }

   cJSON *newInterface=cJSON_Parse(interface_template);
   if(!newInterface) {
      return -1;
   }

   cJSON *e=jsonData->child;
   while(e) {
      cJSON *d=cJSON_GetObjectItem(newInterface, e->string);
      if(d != NULL && d->type==e->type) {
         cJSON_DeleteItemFromObject(newInterface, e->string);
         cJSON_AddItemToArray(newInterface, cJSON_Duplicate(e,1));
      } 
      e=e->next;
   }

   char name[256];
   int id_type=(int)cJSON_GetObjectItem(newInterface, ID_TYPE_STR_C)->valuedouble;
   int state=(int)cJSON_GetObjectItem(newInterface, STATE_STR_C)->valuedouble;
   char *_name=(char *)cJSON_GetObjectItem(newInterface, NAME_STR_C)->valuestring;
   char *dev=(char *)cJSON_GetObjectItem(newInterface, DEV_STR_C)->valuestring;

   if(strlen(_name)>=3) { // ajouter check du nom de l'interface (que des lettres, chiffres et _)
      mea_strcpylower(name, _name);
      cJSON_DeleteItemFromObject(newInterface, NAME_STR_C);
   }
   else {
      goto addInterface_clean_exit;
   }

   if(state<0 || state>2) {
      goto addInterface_clean_exit;
   }

   if(id_type<0) { // ajouter le check de l'id interface dans jsonTypes
      goto addInterface_clean_exit;
   }

   cJSON_AddItemToObject(newInterface, DEVICES_STR_C, cJSON_CreateObject());
   cJSON_AddItemToObject(newInterface, ID_INTERFACE_STR_C, cJSON_CreateNumber(nextInterfaceId));
   nextInterfaceId++;

   int ret=-1; 
   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&jsonInterfaces_rwlock);
   pthread_rwlock_wrlock(&jsonInterfaces_rwlock);

   if(!cJSON_GetObjectItem(jsonInterfaces, name)) {
      cJSON_AddItemToObject(jsonInterfaces, name, newInterface);

      struct devs_index_s *e=(struct devs_index_s *)malloc(sizeof(struct devs_index_s));
      mea_strncpytrimlower(e->devName, dev, sizeof(e->devName));
      e->interface=newInterface;
      HASH_ADD_STR(devs_index, devName, e);

      ret=0;
   }

   pthread_rwlock_unlock(&jsonInterfaces_rwlock);
   pthread_cleanup_pop(0);


   if(ret==0) {
      pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&interfaces_queue_rwlock);
      pthread_rwlock_rdlock(&interfaces_queue_rwlock);

      if(prepare_interface(_interfaces, _params_list, newInterface, 1)==0) {
         link_delegates(_interfaces);
      }

      pthread_rwlock_unlock(&interfaces_queue_rwlock);
      pthread_cleanup_pop(0);
   }

   return ret;

addInterface_clean_exit:
   if(newInterface)
      cJSON_Delete(newInterface);

   return -1;
}


int updateInterface(char *interface, cJSON *jsonData)
{
   int ret=-1;
   int id_interface=-1;
   cJSON *_jsonInterface=NULL, *_jsonInterfaceOld=NULL;

   if(jsonData->type!=cJSON_Object) {
      return -1;
   }

   char *_interface=alloca(strlen(interface)+1);
   mea_strcpylower(_interface, interface);

   cJSON *jsonInterfaceTemplate=cJSON_Parse(interface_template);

   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&jsonInterfaces_rwlock);
   pthread_rwlock_rdlock(&jsonInterfaces_rwlock);

   cJSON *jsonInterface=cJSON_GetObjectItem(jsonInterfaces, _interface);
   if(jsonInterface) {
      id_interface=(int)cJSON_GetObjectItem(jsonInterface, ID_INTERFACE_STR_C)->valuedouble;
      cJSON *e=jsonInterface->child;
      _jsonInterface=cJSON_CreateObject();
      while(e) {
         cJSON *next=e->next;
         cJSON *_e=cJSON_GetObjectItem(jsonData, e->string);

         if(_e && _e->type==e->type && cJSON_GetObjectItem(jsonInterfaceTemplate, _e->string)) {
            cJSON_AddItemToObject(_jsonInterface, _e->string, cJSON_Duplicate(_e,1));
         }
         else {
            cJSON_AddItemToObject(_jsonInterface, e->string, cJSON_Duplicate(e,1));
         }
         e=next;
      }

      char devName[256]="";
      mea_strncpytrimlower(devName, (char *)cJSON_GetObjectItem(jsonInterface, DEV_STR_C)->valuestring, sizeof(devName)-1);

      devices_index=removeDeviceFromIndexByInterfaceId(devices_index, id_interface);
      devs_index=removeInterfaceFromIndexByInterfaceId(devs_index, id_interface);

      _jsonInterfaceOld=cJSON_DetachItemFromObject(jsonInterfaces, _interface);
      
      cJSON_AddItemToObject(jsonInterfaces, _interface, _jsonInterface);

      ret=0;
   }

   pthread_rwlock_unlock(&jsonInterfaces_rwlock);
   pthread_cleanup_pop(0);


   if(ret==0) {
      pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&interfaces_queue_rwlock);
      pthread_rwlock_rdlock(&interfaces_queue_rwlock);

      remove_delegate_links(_interfaces, _jsonInterfaceOld->string);
      remove_interface(_interfaces, id_interface);

      if(prepare_interface(_interfaces, _params_list, _jsonInterface, 1)==0) {
         link_delegates(_interfaces);
      }

      pthread_rwlock_unlock(&interfaces_queue_rwlock);
      pthread_cleanup_pop(0);
   }

   struct devices_index_s *_devices_index=NULL;
   _devices_index=addInterfaceDevicesToDevicesIndex(devices_index, _jsonInterface);
   if(_devices_index) {
      devices_index=_devices_index;
   }

   struct devs_index_s *_devs_index=NULL;
   _devs_index=addInterfaceToDevsIndex(devs_index, _jsonInterface);
   if(_devs_index) {
      devs_index=_devs_index;
   }

   if(_jsonInterfaceOld) {
      cJSON_Delete(_jsonInterfaceOld);
   }
   cJSON_Delete(jsonInterfaceTemplate);
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
   obj=cJSON_GetObjectItem(device, ID_TYPE_STR_C);
   if(obj && obj->type==cJSON_Number) {
      id_type=(int)obj->valuedouble;
      cJSON *jsonType=findTypeByIdThroughIndex(types_index, id_type);
      if(jsonType==NULL || (int)cJSON_GetObjectItem(jsonType, TYPEOFTYPE_STR_C)->valuedouble == TYPEOFTYPE_INTERFACE)
         return -3;
   }

   obj=cJSON_GetObjectItem(device, STATE_STR_C);
   if(obj && obj->type==cJSON_Number) {
      if(obj->valuedouble!=0.0 && obj->valuedouble!=1.0)
         return -4;
      else
         state=(int)obj->valuedouble;
   }

   obj=cJSON_GetObjectItem(device, PARAMETERS_STR_C );
   if(obj && obj->type==cJSON_String)
      parameters=obj->valuestring;

   obj=cJSON_GetObjectItem(device, DESCRIPTION_STR_C);
   if(obj &&obj->type==cJSON_String)
      description=obj->valuestring;

   char *_interface=alloca(strlen(interface)+1);
   mea_strcpylower(_interface, interface);
   char *_name=alloca(strlen(name)+1);
   mea_strcpylower(_name, name);
   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&jsonInterfaces_rwlock);
   pthread_rwlock_wrlock(&jsonInterfaces_rwlock);

   if(jsonInterfaces) {
      ret=-10;
      cJSON *jsonInterface=cJSON_GetObjectItem(jsonInterfaces, _interface);
      if(jsonInterface) {
         id_interface=(int)cJSON_GetObjectItem(jsonInterface, ID_INTERFACE_STR_C)->valuedouble;
         ret=-11;
         cJSON *jsonDevices=cJSON_GetObjectItem(jsonInterface, DEVICES_STR_C);
         if(jsonDevices) {
            cJSON *d=cJSON_GetObjectItem(jsonDevices, _name);
            ret=-12;
            if(d) {
               if(id_type>=0)
                  cJSON_SetNumberValue(cJSON_GetObjectItem(d, ID_TYPE_STR_C),(double)id_type);
               if(state>=0)
                  cJSON_SetNumberValue(cJSON_GetObjectItem(d, STATE_STR_C),(double)state);
               if(parameters) {
                  cJSON_DeleteItemFromObject(d, PARAMETERS_STR_C );
                  cJSON_AddStringToObject(d, PARAMETERS_STR_C , parameters);
               }
               if(description) {
                  cJSON_DeleteItemFromObject(d, DESCRIPTION_STR_C);
                  cJSON_AddStringToObject(d, DESCRIPTION_STR_C, description);
               }
               
               ret=0;
            }
         }
      }
   }

   pthread_rwlock_unlock(&jsonInterfaces_rwlock);
   pthread_cleanup_pop(0);

   if(ret==0) {
      update_interface_devices(id_interface);
   }

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
         id_interface=(int)cJSON_GetObjectItem(jsonInterface, ID_INTERFACE_STR_C)->valuedouble;

         ret=-3;
         cJSON *jsonDevices=cJSON_GetObjectItem(jsonInterface, DEVICES_STR_C);
         if(jsonDevices) {
            struct devices_index_s *e=NULL;
            int _name_l=(int)strlen(name)+1;
            char *_name=alloca(_name_l);
            mea_strncpytrimlower(_name, name, _name_l);
            
            HASH_FIND_STR(devices_index, _name, e);
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

   if(ret==0) {
      update_interface_devices(id_interface);
   }

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
   obj=cJSON_GetObjectItem(device, NAME_STR_C);
   if(obj && obj->type==cJSON_String) {
      name=obj->valuestring;
   }
   else {
      return -2;
   }
   mea_strtolower(name);

   obj=cJSON_GetObjectItem(device, ID_TYPE_STR_C);
   if(obj && obj->type==cJSON_Number) {
      id_type=(int)obj->valuedouble;
      cJSON *jsonType=findTypeByIdThroughIndex(types_index, id_type);
      if(jsonType==NULL || (int)cJSON_GetObjectItem(jsonType, TYPEOFTYPE_STR_C)->valuedouble == TYPEOFTYPE_INTERFACE) {
         return -3;
      }
   }
   else {
      return -3;
   }

   obj=cJSON_GetObjectItem(device, STATE_STR_C);
   if(obj && obj->type==cJSON_Number)
      if(obj->valuedouble!=0.0 && obj->valuedouble!=1.0) {
         return -4;
      }
      else {
         state=(int)obj->valuedouble;
      }
   else {
      return -4;
   }

   obj=cJSON_GetObjectItem(device, PARAMETERS_STR_C );
   if(!obj) {
      parameters="";
   }
   else if(obj->type==cJSON_String) {
      parameters=obj->valuestring;
   }
   else {
      return -5;
   }

   obj=cJSON_GetObjectItem(device, DESCRIPTION_STR_C );
   if(!obj) {
      description="";
   }
   else if(obj->type==cJSON_String) {
      description=obj->valuestring;
   }
   else {
      return -6;
   }


   //
   // ajouts
   //   
   cJSON *jsonDevice=cJSON_CreateObject();
   cJSON_AddStringToObject(jsonDevice,PARAMETERS_STR_C ,parameters);
   cJSON_AddStringToObject(jsonDevice,DESCRIPTION_STR_C,description);
   cJSON_AddNumberToObject(jsonDevice,ID_TYPE_STR_C,id_type);
   cJSON_AddNumberToObject(jsonDevice,STATE_STR_C,state);
   cJSON_AddNumberToObject(jsonDevice,ID_SENSOR_ACTUATOR_STR_C,(double)(nextDeviceId++));

   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&jsonInterfaces_rwlock);
   pthread_rwlock_wrlock(&jsonInterfaces_rwlock);
 
   ret=-9;
   if(jsonInterfaces) {
      ret=-10;
      cJSON *jsonInterface=cJSON_GetObjectItem(jsonInterfaces, interface);
      if(jsonInterface) {
         id_interface=(int)cJSON_GetObjectItem(jsonInterface, ID_INTERFACE_STR_C)->valuedouble;
         ret=-11;
         cJSON *jsonDevices=cJSON_GetObjectItem(jsonInterface, DEVICES_STR_C);
         if(jsonDevices) {
            cJSON *d=cJSON_GetObjectItem(jsonDevices, name);
            ret=-12;
            if(!d) {
               cJSON_AddNumberToObject(jsonDevice,ID_INTERFACE_STR_C,(int)cJSON_GetObjectItem(jsonInterface,ID_INTERFACE_STR_C)->valuedouble);
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

   pthread_rwlock_unlock(&jsonInterfaces_rwlock);
   pthread_cleanup_pop(0);

   if(ret==0) {
      update_interface_devices(id_interface);
   }

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
         cJSON *jsonDevices=cJSON_GetObjectItem(jsonInterface, DEVICES_STR_C);
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
   char *_interface=alloca(strlen(interface)+1);
   char *_device=alloca(strlen(device)+1);
   mea_strcpylower(_interface, interface);
   mea_strcpylower(_device, device);
   
   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&jsonInterfaces_rwlock);
   pthread_rwlock_rdlock(&jsonInterfaces_rwlock);

   if(jsonInterfaces) {
      cJSON *jsonInterface=cJSON_GetObjectItem(jsonInterfaces, _interface);
      if(jsonInterface) {
         cJSON *jsonDevices=cJSON_GetObjectItem(jsonInterface, DEVICES_STR_C);
         if(jsonDevices) {
            cJSON *jsonDevice=cJSON_GetObjectItem(jsonDevices, _device);
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
   char *_name=alloca(strlen(name)+1);
   mea_strcpylower(_name, name);
   
   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&jsonInterfaces_rwlock);
   pthread_rwlock_rdlock(&jsonInterfaces_rwlock);

   cJSON *jsonInterface = cJSON_GetObjectItem(jsonInterfaces, _name);
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

   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&jsonInterfaces_rwlock);
   pthread_rwlock_rdlock(&jsonInterfaces_rwlock);

   cJSON *jsonType = cJSON_GetObjectItem(jsonTypes, name);
   if(jsonType) {
      s=cJSON_Print(jsonType);
   }

   pthread_rwlock_unlock(&jsonInterfaces_rwlock);
   pthread_cleanup_pop(0);

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
   if(!jsonDevices) {
      return -1;
   }

   if(!(jsonDevices->type == cJSON_Object)) {
      return -1;
   }

   cJSON *jsonDevice=jsonDevices->child;
   while(jsonDevice) {

      cJSON *json_id_type = cJSON_GetObjectItem(jsonDevice, ID_TYPE_STR_C);
      cJSON *json_id_description = cJSON_GetObjectItem(jsonDevice, DESCRIPTION_STR_C);
      cJSON *json_parameters = cJSON_GetObjectItem(jsonDevice, PARAMETERS_STR_C );
      cJSON *json_state = cJSON_GetObjectItem(jsonDevice, STATE_STR_C);

      if( (json_id_type == NULL) || !(json_id_type->type == cJSON_Number)) {
         return -1;
      }
      if( (json_id_description == NULL) || !(json_id_description->type == cJSON_String)) {
         return -1;
      }
      if( (json_parameters == NULL) || !(json_parameters->type == cJSON_String)) {
         return -1;
      }
      if( (json_state == NULL) || !(json_state->type == cJSON_Number)) {
         return -1;
      }

      jsonDevice=jsonDevice->next;
   }
   return 0;
}


int checkJsonInterfaces(cJSON *jsonInterfaces)
{
   if(!jsonInterfaces) {
      return -1;
   }

   if(!(jsonInterfaces->type == cJSON_Object)) {
      return -1;
   }

   cJSON *jsonInterface=jsonInterfaces->child;
   while(jsonInterface) {

      cJSON *json_id_type = cJSON_GetObjectItem(jsonInterface, ID_TYPE_STR_C);
      cJSON *json_id_description = cJSON_GetObjectItem(jsonInterface, DESCRIPTION_STR_C);
      cJSON *json_dev = cJSON_GetObjectItem(jsonInterface, DEV_STR_C);
      cJSON *json_parameters = cJSON_GetObjectItem(jsonInterface, PARAMETERS_STR_C );
      cJSON *json_state = cJSON_GetObjectItem(jsonInterface, STATE_STR_C);
      cJSON *json_devices = cJSON_GetObjectItem(jsonInterface, DEVICES_STR_C);

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


int relinkInterfacesDevices(cJSON *jsonInterfaces, int *next_interface_id, int *next_device_id)
{
   int id_device_cptr=1000;
   int id_interface_cptr=100;

   *next_interface_id=-1;
   *next_device_id=-1;

   if(!jsonInterfaces) {
      return -1;
   }
   else {
      cJSON *jsonInterface=jsonInterfaces->child;
      while(jsonInterface) {

         cJSON *json_id_interface = cJSON_GetObjectItem(jsonInterface,ID_INTERFACE_STR_C);
         if(!json_id_interface) {
             json_id_interface=cJSON_CreateNumber((double)id_interface_cptr);
             cJSON_AddItemToObject(jsonInterface, ID_INTERFACE_STR_C, json_id_interface);
         }
         else {
            cJSON_SetNumberValue(json_id_interface, (double)id_interface_cptr);
         }

         int id_interface=id_interface_cptr;

         cJSON *jsonDevices=cJSON_GetObjectItem(jsonInterface, DEVICES_STR_C);
         if(jsonDevices) {
            cJSON *jsonDevice=jsonDevices->child;
            while(jsonDevice) {
               cJSON *json_id_sensor_actuator = cJSON_GetObjectItem(jsonDevice,ID_SENSOR_ACTUATOR_STR_C);
               if(!json_id_sensor_actuator) {
                  json_id_sensor_actuator=cJSON_CreateNumber((double)id_device_cptr);
                  cJSON_AddItemToObject(jsonDevice, ID_SENSOR_ACTUATOR_STR_C, json_id_sensor_actuator);
               }
               else {
                  cJSON_SetNumberValue(json_id_sensor_actuator, (double)id_device_cptr);
               }
               id_device_cptr++;

               cJSON *json_id_interface = cJSON_GetObjectItem(jsonDevice,ID_INTERFACE_STR_C);
               if(!json_id_interface) {
                  json_id_interface=cJSON_CreateNumber((double)id_interface);
                  cJSON_AddItemToObject(jsonDevice, ID_INTERFACE_STR_C, json_id_interface);
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

   *next_interface_id=id_interface_cptr;
   *next_device_id=id_device_cptr;

   return 0;
}


int checkJsonTypes(cJSON *jsonTypes)
{
   if(!jsonTypes) {
      return -1;
   }
   else {
//
// TODO: ajouter ici le controle de la structure et des types de données
//
      return 0;
   }
}


cJSON *findInterfaceById(cJSON *jsonInterfaces, int id)
{
   if(!jsonInterfaces) {
      return NULL;
   }
 
   cJSON *jsonInterface=jsonInterfaces->child;
   while( jsonInterface ) {
      int  id_interface=cJSON_GetObjectItem(jsonInterface, ID_INTERFACE_STR_C)->valuedouble;
      if(id_interface == id) {
         return jsonInterface;
      }
      jsonInterface = jsonInterface->next;
   }
 
   return NULL;
}


int lowerDevicesNames(cJSON *jsonDevices)
{
   if(!jsonDevices || jsonDevices->type!=cJSON_Object) {
      return -1;
   }
   cJSON *jsonDevice=jsonDevices->child;
   while(jsonDevice) {
      mea_strtolower(jsonDevice->string);
      jsonDevice = jsonDevice->next;
   }
   return 0;
}


int lowerInterfacesNames(cJSON *_jsonInterfaces)
{
   cJSON *jsonInterface=_jsonInterfaces->child;
   while(jsonInterface) {
      mea_strtolower(jsonInterface->string);
      lowerDevicesNames(cJSON_GetObjectItem(jsonInterface,"devices"));
      jsonInterface = jsonInterface->next;
   }
   return 0;
}


cJSON *jsonInterfacesLoad(char *file)
{
   cJSON *_jsonInterfaces=loadJson_alloc(file);
   if(_jsonInterfaces==NULL) {
      return NULL;
   }

   if(checkJsonInterfaces(_jsonInterfaces)==-1) {
      cJSON_Delete(_jsonInterfaces);
      return NULL;
   }

   lowerInterfacesNames(_jsonInterfaces);
   relinkInterfacesDevices(_jsonInterfaces, &nextInterfaceId, &nextDeviceId);

   return _jsonInterfaces;
}


cJSON *jsonTypesLoad(char *file)
{
   cJSON *_jsonTypes=loadJson_alloc(file);
   if(_jsonTypes==NULL) {
      return NULL;
   }

   if(checkJsonTypes(_jsonTypes)==-1) {
      cJSON_Delete(_jsonTypes);
      return NULL;
   }

   return _jsonTypes;
}


int interfaceCommit()
{
   char jsonInterfacesFile[1024]="";
   snprintf(jsonInterfacesFile, sizeof(jsonInterfacesFile)-1, "%s/etc/interfaces.json", appParameters_get(MEAPATH_STR_C, NULL));
   cJSON *_jsonInterfaces = NULL;
   
   if(mea_filebackup(jsonInterfacesFile)==0) {
      pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&jsonInterfaces_rwlock);
      pthread_rwlock_wrlock(&jsonInterfaces_rwlock);

      _jsonInterfaces=cJSON_Duplicate(jsonInterfaces, 1);
      
      pthread_rwlock_unlock(&jsonInterfaces_rwlock);
      pthread_cleanup_pop(0);

      int ret=writeJson(jsonInterfacesFile, _jsonInterfaces);
      cJSON_Delete(_jsonInterfaces);

      if(ret==0) {
         return 0;
      }
      else {
         return 3;
      }
   }
   else {
      return 2;
   }
}


int interfaceRollback()
{
   char *interfaceFileName="";
   
   if(interfaceFileName==NULL) {
      return 1;
   }

//   cJSON_Delete(jsonInterfaces);
//   jsonInterfaces=loadJson_alloc(interfaceFileName);
/*
    Charger fichier dans JSON temporaire
    boucle sur JSON
       pour chaque interface
          chercher si l'interface existe actuellement
             OUI: comparer les propriétés (sauf device)
                si différence
                   interface_delete
                   interface_add avec toutes les données de l'interface
                si égale:
                   traiter les devices ...
             NON: ajouter l'interface
      suprimer les interfaces qui ne se trouve pas dans le JSON temporaire
 */
   return 0;
}


int jsonTypesSave()
{
   // TODO: sauvegarde des types

   return 0;
}


cJSON *setPairingState_alloc(char *interfaceName, int state)
{
   int ret=0;
   interfaces_queue_elem_t *iq;
   cJSON *result=NULL;
   
   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&interfaces_queue_rwlock);
   pthread_rwlock_rdlock(&interfaces_queue_rwlock);
   

   if(_interfaces && _interfaces->nb_elem) {
      mea_queue_first(_interfaces);
      while(1) {
         mea_queue_current(_interfaces, (void **)&iq);
         if(iq->context) {
            int monitoring_id = iq->fns->get_monitoring_id(iq->context);
            if(monitoring_id>-1 && process_is_running(monitoring_id)) {
               if(mea_strcmplower(interfaceName, iq->name)==0) {
                  if(iq->fns->pairing) {
                     enum pairing_cmd_e cmd;
                     if(state==0) {
                        cmd=PAIRING_CMD_OFF;
                     }
                     else {
                        cmd=PAIRING_CMD_ON;
                     }
                     result = iq->fns->pairing(cmd, iq->context, NULL);
                     break;
                  }
               }
            }
         }
         ret=mea_queue_next(_interfaces);
         if(ret<0) {
            break;
         }
      }
   }
   
   pthread_rwlock_unlock(&interfaces_queue_rwlock);
   pthread_cleanup_pop(0);
   

   if(result==NULL) {
      result=cJSON_CreateFalse();
   }
   
   return result;
}


cJSON *getAvailablePairing_alloc()
{
   int ret=0;
   interfaces_queue_elem_t *iq=NULL;
   cJSON *result=cJSON_CreateObject();
   
   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&interfaces_queue_rwlock);
   pthread_rwlock_rdlock(&interfaces_queue_rwlock);
   

   if(_interfaces && _interfaces->nb_elem) {
      mea_queue_first(_interfaces);
      while(1) {
         mea_queue_current(_interfaces, (void **)&iq);
         if(iq->context && iq->delegate_flag == 0) {
            int monitoring_id = iq->fns->get_monitoring_id(iq->context);
            if(monitoring_id>-1 && process_is_running(monitoring_id)) {
               if(iq->fns->pairing) {
                  cJSON *state = iq->fns->pairing(PAIRING_CMD_GETSTATE, iq->context, NULL);
                  if(state->type==cJSON_Number) {
                     cJSON_AddNumberToObject(result, iq->name, state->valuedouble);
                     cJSON_Delete(state);
                     state=NULL;
                  }
               }
            }
         }
         ret=mea_queue_next(_interfaces);
         if(ret<0) {
            break;
         }
      }
   }
   
   pthread_rwlock_unlock(&interfaces_queue_rwlock);
   pthread_cleanup_pop(0);
   
   return result;
}


cJSON *device_info_to_json_alloc(struct device_info_s *device_info)
{
   cJSON *jsonDeviceInfo=cJSON_CreateObject();

   if(!jsonDeviceInfo) {
      return NULL;
   }

   cJSON_AddNumberToObject(jsonDeviceInfo, DEVICE_ID_STR_C, device_info->id);
   cJSON_AddStringToObject(jsonDeviceInfo, DEVICE_NAME_STR_C, device_info->name);
   cJSON_AddNumberToObject(jsonDeviceInfo, DEVICE_STATE_STR_C, device_info->state);
   cJSON_AddNumberToObject(jsonDeviceInfo, DEVICE_TYPE_ID_STR_C, device_info->type_id);
   cJSON_AddNumberToObject(jsonDeviceInfo, DEVICE_LOCATION_ID_STR_C, device_info->location_id);
   cJSON_AddNumberToObject(jsonDeviceInfo, INTERFACE_ID_STR_C, device_info->interface_id);
   cJSON_AddStringToObject(jsonDeviceInfo, DEVICE_INTERFACE_NAME_STR_C, device_info->interface_name);
   cJSON_AddStringToObject(jsonDeviceInfo, DEVICE_INTERFACE_TYPE_NAME_STR_C, device_info->type_name);
   cJSON_AddStringToObject(jsonDeviceInfo, DEVICE_TYPE_PARAMETERS_STR_C, device_info->type_parameters);
   cJSON_AddNumberToObject(jsonDeviceInfo, TODBFLAG_STR_C, device_info->todbflag);
   cJSON_AddNumberToObject(jsonDeviceInfo, TYPEOFTYPE_STR_C, device_info->typeoftype_id);
   cJSON_AddStringToObject(jsonDeviceInfo, DEV_STR_C, device_info->interface_dev);

   return jsonDeviceInfo;
}


int device_info_from_json(struct device_info_s *device_info, cJSON *jsonDevice, cJSON *jsonInterface, cJSON *jsonType)
{
   if(!device_info) {
      return -1;
   }

   // BUG:
   if(!jsonInterface) {
      cJSON *_id_interface = cJSON_GetObjectItem(jsonDevice, ID_INTERFACE_STR_C);
      if(!_id_interface) {
         return -2;
      }
      int id_interface=(int)_id_interface->valuedouble;
      jsonInterface=findInterfaceById(jsonInterfaces, id_interface);
   }
   if(!jsonInterface) {
      return -1;
   }

   int type_id = (int)cJSON_GetObjectItem(jsonDevice, ID_TYPE_STR_C)->valuedouble;
   if(!jsonType) {
      jsonType = findTypeByIdThroughIndex(types_index, type_id);
   }

   if(!jsonType) {
      return -4;
   }

   device_info->name              = (char *)jsonDevice->string;
   device_info->id                =    (int)cJSON_GetObjectItem(jsonDevice, ID_SENSOR_ACTUATOR_STR_C)->valuedouble;
   device_info->parameters        = (char *)cJSON_GetObjectItem(jsonDevice, PARAMETERS_STR_C )->valuestring;
   device_info->state             =    (int)cJSON_GetObjectItem(jsonDevice, STATE_STR_C)->valuedouble;
   device_info->type_id           = type_id;

   device_info->type_name         = (char *)jsonType->string;
   device_info->type_parameters   = (char *)cJSON_GetObjectItem(jsonType, PARAMETERS_STR_C )->valuestring;;
   device_info->typeoftype_id     =    (int)cJSON_GetObjectItem(jsonType, TYPEOFTYPE_STR_C)->valuedouble;
 
   device_info->interface_name    = (char *)jsonInterface->string;
   device_info->interface_id      =    (int)cJSON_GetObjectItem(jsonInterface, ID_INTERFACE_STR_C)->valuedouble;
   device_info->interface_type_id =    (int)cJSON_GetObjectItem(jsonInterface, ID_TYPE_STR_C)->valuedouble;
   device_info->interface_dev     = (char *)cJSON_GetObjectItem(jsonInterface, DEV_STR_C)->valuestring;
 
   device_info->location_id       = 0;
   device_info->todbflag          = 0;

   return 0;
}
 
 
int dispatchXPLMessageToInterfaces2(cJSON *xplMsgJson)
{
   int ret=0;
   interfaces_queue_elem_t *iq = NULL;
 
//   DEBUG_SECTION mea_log_printf("%s (%s) : reception message xPL\n", INFO_STR, __func__);
 
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
 
   state=(int)cJSON_GetObjectItem(jsonDevice, STATE_STR_C)->valuedouble;

   if(state==0) {
      DEBUG_SECTION mea_log_printf("%s (%s) : device disabled\n", INFO_STR, __func__);
      ret=-1;
      goto exit;
   }

   id_interface=(int)cJSON_GetObjectItem(jsonDevice, ID_INTERFACE_STR_C)->valuedouble;
   struct device_info_s device_info;
   ret=device_info_from_json(&device_info, jsonDevice, NULL, NULL);
   if(ret<0)  {
      DEBUG_SECTION mea_log_printf("%s (%s) : can't create device info (%d)\n", INFO_STR, __func__, ret);
      goto exit;
   }

   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&interfaces_queue_rwlock);
   pthread_rwlock_rdlock(&interfaces_queue_rwlock);
 
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

   pthread_rwlock_unlock(&interfaces_queue_rwlock);
   pthread_cleanup_pop(0);

   cJSON_Delete(jsonDevice);

exit:
   cJSON_Delete(xplMsgJson);
 
   return ret;
}
 
 
int16_t unload_interfaces_fns(struct interfacesServer_interfaceFns_s *ifns)
{
   for(int i=0; ifns[i].get_type; i++) {
      if(ifns[i].plugin_flag==1) {
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
//   get_fns_interface_type_004(&(ifns[i++]));
//   get_fns_interface_type_005(&(ifns[i++]));
   get_fns_interface_type_006(&(ifns[i++]));
   get_fns_interface_type_010(&(ifns[i++]));
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

/*
#ifndef __APPLE__
   #define DYN_EXT ".so"
#else
   #define DYN_EXT ".dylib"
#endif
*/
#define DYN_EXT ".idrv"
 
struct plugin_info_s plugin_info_defaults[] = {
   { "interface_type_002" DYN_EXT, INTERFACE_TYPE_002, 0 },
   { "interface_type_003" DYN_EXT, INTERFACE_TYPE_003, 0 },
//   { "interface_type_004" DYN_EXT, INTERFACE_TYPE_004, 0 },
//   { "interface_type_005" DYN_EXT, INTERFACE_TYPE_005, 0 },
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
      int  id_type=cJSON_GetObjectItem(jsonInterface, ID_TYPE_STR_C)->valuedouble;

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
         char lib_name[255]="";
         ret=sscanf(p, "[%[^]]]%*[^\n]", lib_name);
         if(ret == 1) {
            if(p[strlen(lib_name)+1]==']') {
               int l=(int)strlen(mea_strtrim(lib_name));
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

   return 0;
}
#endif
 
 
#ifdef ASPLUGIN
int load_interface(int type, char *driversPath)
{
   for(int i=0; interfacesFns[i].get_type; i++) {
      if(interfacesFns[i].get_type() == type) {
         return 0;
      }
   }
 
   for(int i=0; plugins_list[i].name; i++) {
      if(type == plugins_list[i].type) {
//         struct interfacesServer_interfaceFns_s fns;
         get_fns_interface_f fn = NULL;
         char interface_so[256]="";

         snprintf(interface_so, sizeof(interface_so)-1,"%s/%s", driversPath, plugins_list[i].name);

         void *lib = dlopen(interface_so, RTLD_NOW | RTLD_GLOBAL);
         if(lib) {
            char *err=NULL;
 
            fn=dlsym(lib, "get_fns_interface");
            err = dlerror();
            if(err!=NULL) {
               VERBOSE(1) mea_log_printf("%s (%s) : dlsym - %s\n", ERROR_STR, __func__, err);
               return -1;
            }
            else {
               if(interfacesFns_nb >= (interfacesFns_max-1)) {
                  struct interfacesServer_interfaceFns_s *tmp=NULL;
                  interfacesFns_max+=5;
                  tmp = realloc(interfacesFns,  sizeof(struct interfacesServer_interfaceFns_s)*interfacesFns_max);
                  if(tmp == NULL) {
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
         else {
            VERBOSE(2) mea_log_printf("%s (%s) : dlopen - %s\n", ERROR_STR, __func__, dlerror());
            return -1;
         }
      }
   }
   return -1;
}
#endif
 

struct interfacesServer_interfaceFns_s *interfacesServer_get_apis(int id_interface)
{
   int ret=-1;
   interfaces_queue_elem_t *iq=NULL;
   struct interfacesServer_interfaceFns_s *apis=NULL;
   
   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&interfaces_queue_rwlock);
   pthread_rwlock_rdlock(&interfaces_queue_rwlock);
 
   if(_interfaces && _interfaces->nb_elem) {
      mea_queue_first(_interfaces);
      while(1) {
         mea_queue_current(_interfaces, (void **)&iq);
 
         if(iq && iq->context && iq->id == id_interface) {
            apis=iq->fns;
            break;
         }
 
         ret=mea_queue_next(_interfaces);
         if(ret<0)
            break;
      }
   }

   pthread_rwlock_unlock(&interfaces_queue_rwlock);
   pthread_cleanup_pop(0);
 
   return apis;
}


int16_t interfacesServer_call_interface_api(int id_interface, char *cmnd, void *args, int nb_args, void **res, int16_t *nerr, char *err, int l_err)
{
   int ret=-1;
   interfaces_queue_elem_t *iq=NULL;
   int found=0;
 
   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&interfaces_queue_rwlock);
   pthread_rwlock_rdlock(&interfaces_queue_rwlock);
 
   if(_interfaces && _interfaces->nb_elem) {
      mea_queue_first(_interfaces);
      while(1) {
         mea_queue_current(_interfaces, (void **)&iq);
 
         if(iq && iq->context && iq->id == id_interface) {
            found=1;
            break;
         }
 
         ret=mea_queue_next(_interfaces);
         if(ret<0) {
            break;
         }
      }
   }
 
   if(found) {
      ret=iq->fns->api(iq->context, cmnd, args, nb_args, res, nerr, err, l_err);
   }
 
   pthread_rwlock_unlock(&interfaces_queue_rwlock);
   pthread_cleanup_pop(0);
 
   return ret;
}
 

int start_interface(int id_interface)
{
   // TODO: demarrage d'une interface par id

   return 0;
}


int stop_interface(int id_interface)
{
   // TODO: arret d'une interface par id

   return 0;
}


int restart_interface(int id_interface)
{
   if(stop_interface(id_interface)==0) {
      sleep(1);
      return start_interface(id_interface);
   }
   return -1;
}


int remove_interface(mea_queue_t *interfaces_list, int id_interface)
{
   interfaces_queue_elem_t *iq=NULL;
   int ret=-1;

   if(interfaces_list && interfaces_list->nb_elem) {

      mea_queue_first(interfaces_list);
      while(1) {
         ret=mea_queue_current(interfaces_list, (void **)&iq);

         if(ret==ERROR) {
            ret=-1;
            break; 
         }

         if(iq->id == id_interface) {
            int monitoring_id = iq->fns->get_monitoring_id(iq->context);
 
            if(iq->delegate_flag == 0 && monitoring_id>-1) {
               iq->fns->set_xPLCallback(iq->context, NULL);

               void *start_stop_params = process_get_data_ptr(monitoring_id);

               if(process_is_running(monitoring_id)) {
                  process_stop(monitoring_id, NULL, 0);
               }

               process_unregister(monitoring_id);
 
               iq->fns->set_monitoring_id(iq->context, -1);
 
               if(start_stop_params) {
                  free(start_stop_params);
                  start_stop_params=NULL;
               }

            }

            if(iq->context) {
               if(iq->fns->clean) {
                  iq->fns->clean(iq->context);
               }
               free(iq->context);
               iq->context=NULL;
            }
            
            free(iq);
            iq=NULL;

            mea_queue_remove_current(interfaces_list);
            ret=0;
            break;
         }

         if(mea_queue_next(interfaces_list)==ERROR) {
            ret=-1;
            break;
         }
      }
   }

//#ifdef DEBUGFLAG
//   mea_queue_first(interfaces_list);
//   while(1) {
//      ret=mea_queue_current(interfaces_list, (void **)&iq);
//      mea_log_printf("###> %s\n", iq->name);
//      if(mea_queue_next(interfaces_list)==ERROR) {
//         ret=-1;
//         break;
//      }
//   }
///#endif
   return ret;
}

 
void stop_interfaces()
{
   interfaces_queue_elem_t *iq=NULL;

   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&interfaces_queue_rwlock);
   pthread_rwlock_wrlock(&interfaces_queue_rwlock);
 
   if(_interfaces && _interfaces->nb_elem) {
      mea_queue_first(_interfaces);
      while(_interfaces->nb_elem) {
         mea_queue_out_elem(_interfaces, (void **)&iq);
 
         int monitoring_id = iq->fns->get_monitoring_id(iq->context);
 
         if(iq->delegate_flag == 0 && monitoring_id>-1 && process_is_running(monitoring_id)) {
            iq->fns->set_xPLCallback(iq->context, NULL);
 
            int monitoring_id = iq->fns->get_monitoring_id(iq->context);
 
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
 
 
int remove_delegate_links(mea_queue_t *interfaces_list, char *interface)
{
   int ret = 0;
   interfaces_queue_elem_t *iq=NULL;
 
   mea_queue_first(interfaces_list);
   while(1) {
      mea_queue_current(interfaces_list, (void **)&iq);
 
      if(iq && iq->delegate_flag == 1) {
         char name[256], more[256];
         int n;
 
         ret=sscanf(iq->dev,"%[^:]://%[^\n]%n\n", name, more, &n);
         if(ret>0 && strlen(iq->dev) == n) {
            if(mea_strcmplower(name, interface) == 0) {
               iq->context = NULL;
               iq->fns = NULL;
            }
         }
      }
 
      ret=mea_queue_next(interfaces_list);
      if(ret<0) {
         break;
     }
   }

   return 0;
}


int unlink_delegates(mea_queue_t *interfaces_list)
{
   // TODO: suppression des liens avec les interfaces
   return 0;
}

 
int link_delegates(mea_queue_t *interfaces_list)
{
   int ret = 0;
   interfaces_queue_elem_t *iq=NULL;
 
   mea_queue_t _interfaces_list_clone = MEA_QUEUE_T_INIT;
   mea_queue_t *interfaces_list_clone = &_interfaces_list_clone;
   interfaces_queue_elem_t *iq_clone = NULL;
 
   mea_queue_clone(interfaces_list_clone, interfaces_list); // pour le double balayage
 
   mea_queue_first(interfaces_list);
   while(1) {
      mea_queue_current(interfaces_list, (void **)&iq);
 
      if(iq && !iq->context) {
         char name[256]="", more[256]="";
         int n=0;
 
         ret=sscanf(iq->dev,"%[^:]://%[^\n]%n\n", name, more, &n);
         if(ret>0 && strlen(iq->dev) == n) {
            mea_queue_first(interfaces_list_clone);
            while(1) {
               mea_queue_current(interfaces_list_clone, (void **)&iq_clone);
               if(iq_clone->context && iq_clone->delegate_flag == 0) {
                  if(mea_strcmplower(name, iq_clone->name) == 0) {
                     iq->context = iq_clone->context;
                     iq->fns = iq_clone->fns;
                  }
               }
 
               if(mea_queue_next(interfaces_list_clone)<0) {
                  break;
               }
            }
         }
         else {
         }
      }
 
      ret=mea_queue_next(interfaces_list);
      if(ret<0) {
         break;
      }
   }
 
   return 0;
}
 
 
int clean_not_linked(mea_queue_t *interfaces_list)
{
   unlink_delegates(_interfaces);
   link_delegates(_interfaces);
 
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

   char *meapath=appParameters_get(MEAPATH_STR_C, params_list);

   char jsonInterfacesFile[1024]="";
   snprintf(jsonInterfacesFile, sizeof(jsonInterfacesFile)-1, "%s/etc/interfaces.json", meapath);
   jsonInterfaces=jsonInterfacesLoad(jsonInterfacesFile);
   if(!jsonInterfaces) {
      VERBOSE(1) {
         mea_log_printf("%s (%s) : can't load interfaces descriptions from %s\n",ERROR_STR,__func__,jsonInterfacesFile);
      }
      return -1;
   }
   createDevicesIndex(&devices_index, jsonInterfaces);
   createDevsIndex(&devs_index, jsonInterfaces);

   char jsonTypesFile[1024]="";
   snprintf(jsonTypesFile, sizeof(jsonTypesFile)-1, "%s/etc/types.json", meapath);
   jsonTypes=jsonTypesLoad(jsonTypesFile);
   if(!jsonTypes) {
      VERBOSE(1) {
         mea_log_printf("%s (%s) : can't load types descriptions",ERROR_STR,__func__);
      }
      return -1;
   }
   createTypesIndex(&types_index, jsonTypes);

   return 0;
}


int update_interface_devices(int id_interface)
{
   int ret=0;

   printf("Update Interfaces (%d) requested\n", id_interface);

   interfaces_queue_elem_t *iq=NULL;
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
         int _ret=mea_queue_next(_interfaces);
         if(_ret<0) {
            ret=-1;
            break;
         }
      }
   }

   pthread_rwlock_unlock(&interfaces_queue_rwlock);
   pthread_cleanup_pop(0);

   return ret;
}


int prepare_interface(mea_queue_t *interfaces_list, cJSON *params_list, cJSON *jsonInterface, int start)
{
   if(!params_list || !jsonInterface) {
      return -1;
   }

   interfaces_queue_elem_t *iq=NULL;

   char *name=jsonInterface->string;
   int  state=cJSON_GetObjectItem(jsonInterface, STATE_STR_C)->valuedouble;
   int  id_interface=cJSON_GetObjectItem(jsonInterface, ID_INTERFACE_STR_C)->valuedouble;
   int  id_type=cJSON_GetObjectItem(jsonInterface, ID_TYPE_STR_C)->valuedouble;
   char *dev=cJSON_GetObjectItem(jsonInterface, DEV_STR_C)->valuestring;

   if(state==1) {
      iq=(interfaces_queue_elem_t *)malloc(sizeof(interfaces_queue_elem_t));
      if(iq==NULL) {
         VERBOSE(1) {
            char err_str[256];
            strerror_r(errno, err_str, sizeof(err_str)-1);
            mea_log_printf("%s (%s) : %s - %s\n",ERROR_STR,__func__,MALLOC_ERROR_STR,err_str);
         }
         return -1;
      }
      iq->context = NULL;
 
      int monitoring_id=-1;
 
#ifdef ASPLUGIN
      char *driversPath=appParameters_get("DRIVERSPATH", params_list);

      int ret=load_interface(id_type, driversPath);
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
         else {
            free(iq);
            iq=NULL;
            return -1;
         }
      }

      if(monitoring_id!=-1) {
         iq->id=id_interface;
         iq->type=id_type;
         iq->delegate_flag=0;
         strncpy(iq->name, (const char *)name, sizeof(iq->name));
         strncpy(iq->dev, (const char *)dev, sizeof(iq->dev));
         mea_queue_in_elem(interfaces_list, iq);
         if(start>0)
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
            char err_str[256];
            strerror_r(errno, err_str, sizeof(err_str)-1);
            mea_log_printf("%s (%s) : %s - %s\n",ERROR_STR,__func__,MALLOC_ERROR_STR,err_str);
         }
         return -1;
      }
      iq->context = NULL;
      iq->id=id_interface;
      iq->type=id_type;
      iq->delegate_flag=1;
      strncpy(iq->name, (const char *)name, sizeof(iq->name));
      strncpy(iq->dev, (const char *)dev, sizeof(iq->dev));
      mea_queue_in_elem(interfaces_list, iq);
   }
   else {
      VERBOSE(9) mea_log_printf("%s (%s) : %s not activated (state = %d)\n", INFO_STR, __func__, dev, state);
   }

   return 0;
}


mea_queue_t *start_interfaces(cJSON *params_list)
{
   int16_t ret;
   int sortie=0;
   _params_list=params_list;

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
   if(interfacesFns == NULL) {
      goto start_interfaces_clean_exit_S3;
   }

   memset(interfacesFns, 0, sizeof(struct interfacesServer_interfaceFns_s)*interfacesFns_max);
   init_statics_interfaces_fns(interfacesFns, &interfacesFns_nb);
 
   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&interfaces_queue_rwlock);
   pthread_rwlock_wrlock(&interfaces_queue_rwlock);
 
   _interfaces=(mea_queue_t *)malloc(sizeof(mea_queue_t));
   if(!_interfaces) {
      VERBOSE(1) {
         char err_str[256];
         strerror_r(errno, err_str, sizeof(err_str)-1);
         mea_log_printf("%s (%s) : %s - %s\n",ERROR_STR,__func__,MALLOC_ERROR_STR,err_str);
      }
      goto start_interfaces_clean_exit_S2;
   }
   mea_queue_init(_interfaces);
 
   pthread_cleanup_push( (void *)pthread_rwlock_unlock, (void *)&jsonInterfaces_rwlock);
   pthread_rwlock_rdlock(&jsonInterfaces_rwlock);

 
//   char *driversPath=appParameters_get("DRIVERSPATH", params_list);
   cJSON *jsonInterface=jsonInterfaces->child;
   while( jsonInterface ) {
      if(prepare_interface(_interfaces, params_list, jsonInterface, 1)<0) {
         goto start_interfaces_clean_exit_S1;
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
