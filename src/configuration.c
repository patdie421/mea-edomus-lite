#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>

#include "configuration.h"
#include "cJSON.h"
#include "mea_verbose.h"

cJSON *appParameters = NULL;

char *appParameters_defaults = 
"{\"MEAPATH\":\".\",\
\"HTMLPATH\":\"./html\",\
\"LOGPATH\":\"./var/log\",\
\"PLUGINPATH\":\"./lib/mea-plugins\",\
\"DRIVERSPATH\":\"./lib/mea-drivers\",\
\"XPL_VENDORID\":\"mea\",\
\"XPL_DEVICEID\":\"edomus\",\
\"XPL_INSTANCEID\":\"0\",\
\"VERBOSELEVEL\":\"1\",\
\"HTTPPORT\":\"8083\",\
\"INTERFACE\":\"eth0\",\
\"RULESFILE\":\"mea-edomus.rules\",\
\"USERSFILE\":\"users.json\",\
\"RULESFILESPATH\":\"./lib/mea-rules\"}";


int appParameters_set(char *key, char *value, cJSON *d);


cJSON *getAppParameters()
{
   return appParameters;
}

 
char *getAppParametersAsString_alloc()
{
   char *s=NULL;

   if(appParameters) {
      s=cJSON_Print(appParameters);
   }

   return s;
}


cJSON *filterByJson_alloc(cJSON *j, cJSON *f)
{
   if(j==NULL || f==NULL)
      return NULL;

   cJSON *_j=cJSON_CreateObject();
   cJSON *e = j->child;

   while(e) {
      if(e->type==cJSON_String && cJSON_GetObjectItem(f,e->string)) {
         cJSON *_e = cJSON_Duplicate(e, 1);
         cJSON_AddItemToArray(_j, _e);
      }
      e=e->next;
   } 

   return _j;
}


int updateAppParameters(cJSON *jsonData)
{
   cJSON *filter=cJSON_Parse(appParameters_defaults);
   if(!filter) {
      return -1;
   }

   cJSON *j=filterByJson_alloc(jsonData, filter);
   if(!j) {
      return -1;
   }
   cJSON_Delete(filter);
   
   cJSON *e=j->child;
   while(e) {
      appParameters_set(e->string, e->valuestring, appParameters);
      e=e->next;
   } 
   cJSON_Delete(j);

   return 0;
}


int updateAppParameter(char *name, cJSON *jsonData)
{
   if(!jsonData || jsonData->type!=cJSON_String)
      return -1;
   
   cJSON *filter=cJSON_Parse(appParameters_defaults);
   if(!filter) {
      return -1;
   }

   int ret=-1;
   if(cJSON_GetObjectItem(filter, name)) {
      if(jsonData->type!=cJSON_String) {
         return -1;
      }
      appParameters_set(name, jsonData->valuestring, appParameters);
      ret=0;
   }
   cJSON_Delete(filter);

   return ret;
}


char *getAppParameterAsString_alloc(char *name)
{
   char *s=NULL;

   if(appParameters) {
      cJSON *p=cJSON_GetObjectItem(appParameters, name);
      if(p) {
         s=cJSON_Print(p);
      }
   }
   return s;
}


void appParameters_clean() {
   if(appParameters) {
      cJSON_Delete(appParameters);
      appParameters = NULL;
   }
}


void appParameters_print(cJSON *p) {
   if(p==NULL)
      p=appParameters;
   char *s = cJSON_Print(p);
   printf("%s\n", s);
   free(s);
}


void appParameters_display()
{
   if(appParameters)
      appParameters_print(appParameters);
}


void appParameters_init() {
   appParameters = cJSON_Parse(appParameters_defaults);
}


cJSON *appParameters_create(void) {
   return cJSON_CreateObject();
}


char *appParameters_get(char *key, cJSON *d)
{
   if(d==NULL)
      d=appParameters;

   cJSON *v=cJSON_GetObjectItem(d, key);
   if(v) {
      if(v->type == cJSON_String) {
         return v->valuestring;
      }
   }

   return NULL;
}


int appParameters_set(char *key, char *value, cJSON *d)
{
   if(d==NULL)
      d=appParameters;

   cJSON *prevValue = cJSON_GetObjectItem(d, key);
   cJSON *newValue = cJSON_CreateString((const char *)value); 
   if(prevValue) {
      cJSON_ReplaceItemInObject(d,(const char *)key, newValue);
   }
   else {
      cJSON_AddItemToObject(d, (const char *)key, newValue);
   }

   return 0;
}

