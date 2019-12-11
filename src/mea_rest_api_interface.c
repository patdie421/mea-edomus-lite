#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <ctype.h>

#include "mea_rest_api_interface.h"
#include "mea_rest_api.h"

#include "cJSON.h"
#include "mea_verbose.h"
// #include "mea_string_utils.h"
#include "tokens.h"
#include "tokens_da.h"
#include "mongoose.h"
#include "httpServer.h"
#include "mea_http_utils.h"
#include "interfacesServer.h"


int mea_rest_api_device_DELETE(struct mg_connection *conn, int method, char *interface, char *tokens[], int l_tokens)
{
   if(l_tokens==2) {
      int ret=deleteDevice(interface, tokens[1]);
      if(ret==0) {
         return returnResponse(conn, 200, 0, "deleted");
      }
      else {
         return returnResponse(conn, 404, 1, NULL);
      }
   }
   else {
      return returnResponse(conn, 404, 1, NULL);
   }
}


int mea_rest_api_device_POST_PUT(struct mg_connection *conn, int method, char *interface, char *tokens[], int l_tokens)
{
   int ret=0;
   char *msg="";

   if(l_tokens==1 || l_tokens==2) {
      cJSON *device=getData_alloc(conn);
      if(device) {
         if(l_tokens==1 && method==HTTP_POST_ID) {
            msg="create";
            ret=addDevice(interface, device);
         }
         else if(l_tokens==2 && method==HTTP_PUT_ID){
            msg="updated";
            ret=updateDevice(interface, tokens[1], device);
         }
         cJSON_Delete(device);
         if(ret==0) {
            return returnResponse(conn, 200, 0, msg);
         }
         else {
            return returnResponse(conn, 404, 2, NULL);
         }
      }
      else {
         return returnResponse(conn, 400, 3, NO_VALID_JSON_DATA);
      }
   }
   else {
      return returnResponse(conn, 404, 4, NULL);
   }
}


int mea_rest_api_device_GET(struct mg_connection *conn, int method, char *interface, char *tokens[], int l_tokens)
{
   if(l_tokens==1) {
      char *s=getDevicesAsString_alloc(interface);
      if(s) {
      httpResponse(conn, 200, NULL, s);
      free(s);
      return 1;
      }
      else {
         return returnResponse(conn, 404, 1, "can't find devices");
      }
   }
   else {
      if(l_tokens==2) {
         char *s=getDeviceAsStringByName_alloc(interface, tokens[1]);
         if(s) {
            httpResponse(conn, 200, NULL, s);
            free(s);
            return 1;
         }
         else {
            return returnResponse(conn, 404, 1, "can't find device");
         }
      }
      else {
         return returnResponse(conn, 404, 1, NULL);
      }
   }
}


int mea_rest_api_device(struct mg_connection *conn, int method, char *interface, char *tokens[], int l_tokens)
{
   int ressource=get_token_id_by_string((char *)tokens[0]);

   if(ressource != API_DEVICE_ID) {
      return returnResponse(conn, 404, 1, NULL);
   }

   switch(method) {
      case HTTP_GET_ID:
         return mea_rest_api_device_GET(conn, method, interface, tokens, l_tokens);
      case HTTP_POST_ID:
      case HTTP_PUT_ID:
         return mea_rest_api_device_POST_PUT(conn, method, interface, tokens, l_tokens);
      case HTTP_DELETE_ID:
         return mea_rest_api_device_DELETE(conn, method, interface, tokens, l_tokens);
      default:
         return returnResponse(conn, 405, 1, BAD_METHOD);
   }
}


int mea_rest_api_interface(struct mg_connection *conn, int method, char *tokens[], int l_tokens)
{
   const char *meaSessionId=mg_get_header(conn, MEA_SESSION_STR_C);

   if(checkSession((char *)meaSessionId)!=0) {
      return returnResponse(conn, 401, 99, NOT_AUTHORIZED);
   }

   if(l_tokens>1)
      return mea_rest_api_device(conn, method, tokens[0], &(tokens[1]), l_tokens-1);

   switch(method) {
      case HTTP_DELETE_ID:
         if(l_tokens==0) {
            return returnResponse(conn, 404, 1, NULL);
         }
         else {
            if(deleteInterface(tokens[0])>=0) {
               return returnResponse(conn, 200, 0, NULL);
            }
            else {
               return returnResponse(conn, 404, 1, NULL);
            }
         }
         break;

      case HTTP_GET_ID:
         if(l_tokens==0) {
            char *s=getInterfacesAsString_alloc();
            if(s) {
               httpResponse(conn, 200, NULL, s);
               free(s);
               return 1;
            }
            else {
               return returnResponse(conn, 404, 1, NULL);
            }
         }
         else {
            char *s=getInterfaceAsStringByName_alloc(tokens[0]);
            if(s) {
               httpResponse(conn, 200, NULL, s);
               free(s);
               return 1;
            }
            else {
               return returnResponse(conn, 404, 1, NULL);
            }
         }
         break;

      case HTTP_POST_ID:
         if(l_tokens==0) {
            cJSON *jsonData=getData_alloc(conn);
            if(jsonData) {
               if(addInterface(jsonData)>=0) {
                  return returnResponseAndDeleteJsonData(conn, 200, 0, NULL, jsonData);
               }
               else {
                  return returnResponseAndDeleteJsonData(conn, 404, 1, NULL, jsonData);
               }
            }
            else {
               return returnResponse(conn, 400, 1, NO_VALID_JSON_DATA);
            }
         }
         else {
            return mea_rest_api_device(conn, method, tokens[0], &(tokens[1]), l_tokens-1);
         }
         break;


      case HTTP_PUT_ID:
         if(l_tokens==0) {
            return returnResponse(conn, 404, 1, NULL);
         }
         else if(l_tokens==1) {
            cJSON *jsonData=getData_alloc(conn);
            if(jsonData) {
               if(updateInterface(tokens[0], jsonData)>=0) {
                  return returnResponseAndDeleteJsonData(conn, 200, 0, NULL, jsonData);
               }
               else {
                  return returnResponseAndDeleteJsonData(conn, 400, 1, "can't update interface", jsonData);
               }
            }
            else {
               return returnResponse(conn, 400, 1, NO_VALID_JSON_DATA);
            }
         }
         break;

      default:
         return returnResponse(conn, 405, 1, BAD_METHOD);
   }
   
   return returnResponse(conn, 500, 1, "probably my mistake");
}
