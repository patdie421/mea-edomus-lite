#include <stdlib.h>

#include "mea_http_utils.h"

#include "cJSON.h"
#include "tokens.h"
#include "tokens_da.h"
#include "mongoose.h"
#include "httpServer.h"


cJSON *getData_alloc(struct mg_connection *conn)
{
   cJSON *json=NULL;

   const char *cl=mg_get_header(conn, "Content-Length");
   if(cl) {
      int l_data=atoi(cl);
      if(l_data>0) {
         char *data=(char *)malloc(l_data+1);
         if(data) {
            mg_read(conn, data, l_data);
            data[l_data]=0;
            json = cJSON_Parse(data);
            free(data);
         }
      }
   }
   return json;
}


cJSON *getJsonDataAction(struct mg_connection *conn, cJSON *jsonData)
{
   cJSON *action=cJSON_GetObjectItem(jsonData, ACTION_STR_C);
   if(!action) {
      returnResponseAndDeleteJsonData(conn, 400, 1, "no action", jsonData);
      return NULL;
   }
   if(!(action->type==cJSON_String)) {
      returnResponseAndDeleteJsonData(conn, 400, 1, "action not a string", jsonData);
      return NULL;
   }
   
   return action;
}


cJSON *getJsonDataParameters(struct mg_connection *conn, cJSON *jsonData)
{
   cJSON *parameters=cJSON_GetObjectItem(jsonData, PARAMETERS_STR_C);
   
   if(!parameters) {
      returnResponseAndDeleteJsonData(conn, 400, 1, "no parameter", jsonData);
      return NULL;
   }
   if(parameters->type!=cJSON_Object) {
      returnResponseAndDeleteJsonData(conn, 400, 1, "parameters not an object", jsonData);
      return NULL;
   }
   return parameters;
}

int returnResponseAndDeleteJsonData(struct mg_connection *conn, int httperr, int errnum, char *msg, cJSON *jsonData)
{
   int ret=returnResponse(conn, httperr, errnum, msg);
   if(jsonData)
      cJSON_Delete(jsonData);
   return ret;
}


int returnResponse(struct mg_connection *conn, int httperr, int errnum, char *msg)
{
   char buff[2048];

   if(msg!=NULL)
      snprintf(buff, sizeof(buff)-1, "{\"errno\":%d, \"msg\":\"%s\"}", errnum, msg); 
   else
      snprintf(buff, sizeof(buff)-1, "{\"errno\":%d}", errnum); 

   httpResponse(conn, httperr, NULL, buff);

   return 1;
}


