#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <ctype.h>

#include "cJSON.h"
#include "mea_rest_api.h"

#include "mea_verbose.h"
#include "mea_string_utils.h"
#include "tokens.h"
#include "mongoose.h"
#include "httpServer.h"

#include "configuration.h"


int mea_rest_api_configuration(struct mg_connection *conn, int method, char *tokens[], int l_tokens)
{
   const char *meaSessionId=mg_get_header(conn, "Mea-Session");
   int profile=-1;
   if(checkSession((char *)meaSessionId)!=0) {
      return returnResponse(conn, 401, 99, NOT_AUTHORIZED);
   }

   profile=getSessionProfile((char *)meaSessionId);
   if(profile<0) {
      return returnResponse(conn, 500, 1, "profile error");
   }

   switch(method) {
      case HTTP_GET_ID:
         if(l_tokens==0) {
            char *s=getAppParametersAsString_alloc();
            if(s) {
               httpResponse(conn, 200, NULL, s);
               free(s);
               return 1;
            }
            else {
               return returnResponse(conn, 404, 1, NULL);
            }
         }
         else if(l_tokens==1) {
            char *s=getAppParameterAsString_alloc(tokens[0]);
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
            return returnResponse(conn, 404, 1, NULL);
         }

      case HTTP_PUT_ID: {
         if(profile<1) {
            return returnResponse(conn, 401, 98, NOT_AUTHORIZED);
         }
         cJSON *jsonData=getData_alloc(conn);
         if(!jsonData) {
            return returnResponse(conn, 400, 1, NO_VALID_JSON_DATA);
         }
         if(l_tokens==0) {
            if(updateAppParameters(jsonData)==0) {
               return returnResponseAndDeleteJsonData(conn, 200, 0, NULL, jsonData);
            }
            else {
               return returnResponseAndDeleteJsonData(conn, 404, 1, NULL, jsonData);
            }
         }
         else if(l_tokens==1) {
            if(updateAppParameter(tokens[0], jsonData)==0) {
               return returnResponseAndDeleteJsonData(conn, 200, 0, NULL, jsonData);
            }
            else {
               return returnResponseAndDeleteJsonData(conn, 404, 1, NULL, jsonData);
            }
         }
         else {
            return returnResponseAndDeleteJsonData(conn, 404, 1, NULL, jsonData);
         }
      }
      default:
         return returnResponse(conn, 405, 1, BAD_METHOD);
   }
}
