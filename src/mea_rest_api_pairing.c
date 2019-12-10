//
//  mea_rest_api_pairing.c
//  mea-edomus-lite
//
//  Created by Patrice Dietsch on 03/12/2019.
//  Copyright © 2019 Patrice Dietsch. All rights reserved.
//
#include <stdio.h>
#include <stdlib.h>

#include "mea_rest_api_pairing.h"
#include "mea_rest_api.h"

#include "interfacesServer.h"
#include "mongoose.h"
#include "cJSON.h"
#include "httpServer.h"
#include "tokens.h"
#include "tokens_da.h"
#include "mea_verbose.h"


int mea_rest_api_pairing_GET(struct mg_connection *conn, int method, char *tokens[], int l_tokens)
{
   if(l_tokens==0) {
      cJSON *interfaces=getAvailablePairing_alloc();
      char *s=cJSON_Print(interfaces);
      httpResponse(conn, 200, NULL, s);
      free(s);
      cJSON_Delete(interfaces);
      return 0;
   }
   else if(l_tokens==1) {
      cJSON *interfaces=getAvailablePairing_alloc();
      cJSON *interface=cJSON_GetObjectItem(interfaces, tokens[0]);
      if(interface) {
         char *s=cJSON_Print(interface);
         httpResponse(conn, 200, NULL, s);
         free(s);
         cJSON_Delete(interfaces);
      }
      else {
         return returnResponse(conn, 404, 1, NULL);
      }
      return 0;
   }
   return returnResponse(conn, 404, 1, NULL);
}


int mea_rest_api_pairing_POST(struct mg_connection *conn, int method, char *tokens[], int l_tokens)
{
   cJSON *jsonData=NULL;
   if(l_tokens==1) {
      jsonData=getData_alloc(conn);
      if(jsonData && jsonData->type==cJSON_Object) {
         cJSON *state=cJSON_GetObjectItem(jsonData,"state");
         if(state) {
            if(state->type==cJSON_Number) {
               int v=0;
               char *msg="";
               int ret=0;

               if(state->valuedouble!=0.0) {
                  v=1;
               }
               cJSON *r=setPairingState_alloc(tokens[0], v);

               if(r && r->type==cJSON_True) {
                  msg="done";
                  ret=0;
               }
               else {
                  msg="error";
                  ret=1;
               }
               
               if(r) {
                  cJSON_Delete(r);
                  r=NULL;
               }

               return returnResponseAndDeleteJsonData(conn, 200, ret, msg, jsonData);
            }
            else {
               return returnResponseAndDeleteJsonData(conn, 400, 1, "\"state\" is not a number", jsonData);
            }
         }
         else {
            return returnResponseAndDeleteJsonData(conn, 400, 1, "no \"state\" in body", jsonData);
         }
      }
      else {
         return returnResponseAndDeleteJsonData(conn, 400, 1, "no valid body data", jsonData);
      }
   }
   return returnResponse(conn, 404, 1, NULL);
}

/*
int mea_rest_api_pairing_PUT(struct mg_connection *conn, int method, char *tokens[], int l_tokens)
{
   if(l_tokens==1) {
      cJSON *jsonData=getData_alloc(conn);
      if(jsonData) {
      
         cJSON_Delete(jsonData);
         return returnResponse(conn, 200, 0, "pairing put");
      }
   }
   return returnResponse(conn, 404, 1, NULL);
}
*/

/*
int mea_rest_api_pairing_DELETE(struct mg_connection *conn, int method, char *tokens[], int l_tokens)
{
   if(l_tokens==1) {
      return returnResponse(conn, 200, 0, "pairing delete");
   }
   return returnResponse(conn, 404, 1, NULL);
}
*/


int mea_rest_api_pairing(struct mg_connection *conn, int method, char *tokens[], int l_tokens)
{
   const char *meaSessionId=mg_get_header(conn, "Mea-Session");

   if(checkSession((char *)meaSessionId)!=0) {
      return returnResponse(conn, 401, 99, NOT_AUTHORIZED);
   }

   switch(method) {

      case HTTP_GET_ID:
         return mea_rest_api_pairing_GET(conn, method, tokens, l_tokens);

      case HTTP_POST_ID:
         return mea_rest_api_pairing_POST(conn, method, tokens, l_tokens);

//      case HTTP_PUT_ID:
//         return mea_rest_api_pairing_PUT(conn, method, tokens, l_tokens);

//      case HTTP_DELETE_ID:
//         return mea_rest_api_pairing_DELETE(conn, method, tokens, l_tokens);

      default:
         return returnResponse(conn, 405, 1, BAD_METHOD);
   }
}