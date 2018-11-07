#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>
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

