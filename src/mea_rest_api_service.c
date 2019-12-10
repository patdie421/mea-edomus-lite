#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "mea_rest_api.h"
#include "mea_rest_api_service.h"

#include "interfacesServer.h"
#include "mongoose.h"
#include "cJSON.h"
#include "httpServer.h"
#include "tokens.h"
#include "tokens_da.h"
#include "mea_verbose.h"
#include "mea_string_utils.h"
#include "mea_json_utils.h"

#include "processManager.h"


int mea_rest_api_service_GET(struct mg_connection *conn, int method, char *tokens[], int l_tokens)
{
   int ret=0;
   int id=0;
   char json[2048];

   if(l_tokens==0) {
      char buff[40]="";
      int l_buff=0;
      const struct mg_request_info *request_info = mg_get_request_info(conn);

      int detail_flag=0;
      int type=0; 
      if(request_info->query_string)
      {
         ret=mg_get_var(request_info->query_string, strlen(request_info->query_string), "detail", buff, sizeof(buff));
         if(ret) {
            if(mea_strcmplower(buff, "true")==0)
               detail_flag=1;
         }
         ret=mg_get_var(request_info->query_string, strlen(request_info->query_string), "type", buff, sizeof(buff));
         l_buff=(int)strlen(buff);
         if(ret && l_buff>0) {
            if(mea_strcmplower(buff, "all")==0 || mea_strcmplower(buff, "-1")==0) {
               type=-1;
            }
            else {
               for (int i=0;i<l_buff; i++) {
                  if ( isdigit(buff[i]) ) {
                     type=type*(10*i)+(buff[i]-'0');
                  }
               }
            }
         }
      }

      if(detail_flag==0) 
         ret=managed_processes_processes_to_json_mini(json, sizeof(json)-1, type);
      else
         ret=managed_processes_processes_to_json(json, sizeof(json)-1, type);

      if(ret==0) {
         httpResponse(conn, 200, NULL, json);
         return 0;
      }
      else {
         return returnResponse(conn, 500, 1, "very bad error");
      }
   }
   else if(l_tokens==1) {
      for (int i=0;i<strlen(tokens[0]); i++) {
         if ( isdigit(tokens[0][i]) ) {
            id=id*(10*i)+(tokens[0][i]-'0');
         } 
         else {
            return returnResponse(conn, 400, 1, "id not an integer");
         }
      }
      ret=managed_processes_process_to_json(id, json, sizeof(json)-1);
      if(ret==0) {
         httpResponse(conn, 200, NULL, json);
         return 0;
      }
      else {
         return returnResponse(conn, 500, 1, "oups, wath append");
      }
   }
   else {
      return returnResponse(conn, 404, 1, NULL);
   }
}


int mea_rest_api_service_PUT(struct mg_connection *conn, int method, char *tokens[], int l_tokens)
{
   int ret=0;
//   char json[2048]="";
//   char *s=NULL;
   int id=0;

   if(l_tokens==1) {
      for (int i=0;i<strlen(tokens[0]); i++) {
         if ( isdigit(tokens[0][i]) ) {
            id=id*(10*i)+(tokens[0][i]-'0');
         }
         else {
            return returnResponse(conn, 400, 1, "id not an integer");
         }
      }
      cJSON *jsonData=getData_alloc(conn);
      if(jsonData) {
         cJSON *jsonAction=cJSON_GetObjectItem(jsonData, "action");
         if(jsonAction) {
            if(jsonAction->type==cJSON_String) {
               char msg[256];
               if(mea_strcmplower(jsonAction->valuestring, "start")==0) {
                  ret=process_start(id, msg, sizeof(msg)-1);
                  return returnResponseAndDeleteJsonData(conn, 200, 0, msg, jsonData);
               }
               if(mea_strcmplower(jsonAction->valuestring, "stop")==0) {
                  ret=process_stop(id, msg, sizeof(msg)-1);
                  return returnResponseAndDeleteJsonData(conn, 200, 0, msg, jsonData);
               }
               if(mea_strcmplower(jsonAction->valuestring, "restart")==0) {
                  ret=process_restart(id, msg, sizeof(msg)-1);
                  return returnResponseAndDeleteJsonData(conn, 200, 0, msg, jsonData);
               }
               else {
                  process_start(id, msg, sizeof(msg)-1);
                  return returnResponseAndDeleteJsonData(conn, 400, 1,"unknown action", jsonData);
               }
            }
            else {
               return returnResponseAndDeleteJsonData(conn, 400, 1,"action not a string", jsonData);
            }
         }
         else {
            return returnResponseAndDeleteJsonData(conn, 400, 1,"no action", jsonData);
         }
      }
      else {
         return returnResponse(conn, 400, 1, NO_VALID_JSON_DATA);
      }
   }
   else {
      return returnResponse(conn, 404, 1, NULL);
   }

   return returnResponse(conn, 500, 1, NULL);
}


int mea_rest_api_service(struct mg_connection *conn, int method, char *tokens[], int l_tokens)
{
   const char *meaSessionId=mg_get_header(conn, "Mea-Session");

   if(checkSession((char *)meaSessionId)!=0) {
      return returnResponse(conn, 401, 99, NOT_AUTHORIZED);
   }

//   int ret=0;
//   char json[2048]="";
//   char *s=NULL;
//   int id=0;

   switch(method) {
      case HTTP_GET_ID:
         return(mea_rest_api_service_GET(conn, method, tokens, l_tokens));

      case HTTP_POST_ID:
         break;

      case HTTP_PUT_ID:
         return(mea_rest_api_service_PUT(conn, method, tokens, l_tokens));

      case HTTP_DELETE_ID:
         break;

      default:
         return returnResponse(conn, 405, 1, BAD_METHOD);
   }

   return returnResponse(conn, 500, 1, NULL);
}
