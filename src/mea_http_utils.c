#include <stdlib.h>

#include "mea_http_utils.h"

#include "cJSON.h"
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


