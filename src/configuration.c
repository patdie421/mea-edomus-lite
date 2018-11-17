#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>

#include "configuration.h"
#include "cJSON.h"

cJSON *appParameters = NULL;

char *appParameters_defaults = 
"{ \"MEAPATH\" : \"\", \
   \"PHPCGIPATH\" : \"\", \
   \"PHPINIPATH\" : \"\", \
   \"GUIPATH\" : \"\", \
   \"LOGPATH\" : \"\", \
   \"PLUGINPATH\" : \"\", \
   \"DRIVERSPATH\" : \"\", \
   \"VENDORID\" : \"\", \
   \"DEVICEID\" : \"\", \
   \"INSTANCEID\" : \"\", \
   \"VERBOSELEVEL\" : \"\", \
   \"GUIPORT\" : \"\", \
   \"PHPSESSIONSPATH\" : \"\", \
   \"PARAMSDBVERSION\" : \"\", \
   \"INTERFACE\" : \"\", \
   \"RULESFILE\" : \"\", \
   \"RULESFILESPATH\" : \"\" }";


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


int updateAppParameters(cJSON *jsonData)
{
   return 0;
}


int updateAppParameter(char *name, cJSON *jsonData)
{
   return 0;
}


char *getAppParameterAsString_alloc(char *name)
{
   char *_s=NULL;
   char *s=NULL;

   if(appParameters) {
      cJSON *p=cJSON_GetObjectItem(appParameters, name);
      if(p) {
         _s=cJSON_Print(p);
         if(_s) {
            #define KEYVALUE "{\"value\":%s}"
            s=malloc(strlen(_s)+sizeof(KEYVALUE)+1);
            if(s)
               sprintf(s, KEYVALUE, _s);
            free(_s); 
         } 
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
}

