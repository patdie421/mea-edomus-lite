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

#include "interfacesServer.h"
#include "processManager.h"
#include "configuration.h"

char *_users2 = "{ \
\"admin\": { \"password\":\"admin\", \"fullname\":\"admin\" }, \
\"user\": { \"password\":\"user\", \"fullname\":\"user\" } \
}";

cJSON *users2 = NULL;
cJSON *sessions = NULL;

char *no_valid_json_data_str="no valid json data";
char *bad_method_str="bad method";
char *not_authorized_str="not authorized";

#define NO_VALID_JSON_DATA no_valid_json_data_str
#define BAD_METHOD bad_method_str
#define NOT_AUTHORIZED not_authorized_str


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


int purgeSessions() {
   double tlimit=(double)(time(NULL)-10*60); // 10 mn

   if(sessions) {
      cJSON *session = sessions->child;
      while(session) {
         cJSON *next = session->next;
         if(cJSON_GetObjectItem(session, "time")->valuedouble < tlimit) {
            cJSON_DeleteItemFromObject(sessions, session->string); 
         }
         session = next;
      }
   }

   return 1;
}


int closeSession(struct mg_connection *conn, char *id)
{
   cJSON *json_id = NULL;

   mea_log_printf("%s (%s) : closing session: %s\n",ERROR_STR,__func__, id);

   json_id = cJSON_GetObjectItem(sessions, id);

   if(json_id) {
      cJSON_DeleteItemFromObject(sessions, id);

      mea_log_printf("%s (%s) : session closed\n",ERROR_STR,__func__);
   }
   else
      mea_log_printf("%s (%s) : unknown session\n",ERROR_STR,__func__);

   return 1;
}


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


int openSession(struct mg_connection *conn)
{
   const char *cl=mg_get_header(conn, "Content-Length");
   if(cl) {
      int l_data=atoi(cl);
      if(l_data>0) {
         char *data=(char *)malloc(l_data+1);
         if(data) {
            mg_read(conn, data, l_data);
            data[l_data]=0;

            cJSON *root = cJSON_Parse(data);

            if(root) {
               cJSON *user = cJSON_GetObjectItem(root,"user");
               cJSON *password = cJSON_GetObjectItem(root,"password");

               if(user && password) {
               
                  cJSON *u = cJSON_GetObjectItem(users2, user->valuestring);

                  if(u) {
                     cJSON *p=cJSON_GetObjectItem(u, "password");
                     if(p && p->type==cJSON_String && strcmp(password->valuestring, p->valuestring)==0) {

                        char id[21];

                        for(int i=0;i<20;i++)
                           id[i]='A'+ rand() % ('Z'-'A');
                        id[20]=0;

                        cJSON *idData = cJSON_CreateObject();
                        cJSON_AddItemToObject(idData, "time", cJSON_CreateNumber((double)time(NULL))); 
                        cJSON_AddItemToObject(sessions, id, idData);
                        char jsonSessionId[80];
                        snprintf(jsonSessionId, sizeof(jsonSessionId), "{\"Mea-SessionId\":\"%s\"}", id);
                        httpResponse(conn, 200, NULL, jsonSessionId);
                     }
                     else {
                        returnResponse(conn, 401, 99, NULL);
                     }
                  }
                  else {
                     returnResponse(conn, 401, 99, NULL);
                  }
               }
               else {
                  returnResponse(conn, 400, 1, "no user/password in body data");
               }
               cJSON_Delete(root);
            }
            else {
               returnResponse(conn, 400, 1, NO_VALID_JSON_DATA);
            }

            free(data);
            data=NULL;
         }
      }
      else {
         returnResponse(conn, 400, 1, "no body data");
      }
   }
   else {
      returnResponse(conn, 500, 1, NULL);
   }

   return 1;
}


int checkSession(char *sessionId)
{
   if(sessionId) {
      cJSON *json_sessionId = cJSON_GetObjectItem(sessions, sessionId);
      if(json_sessionId == NULL) {
         return -1;
      }
      else {
         cJSON *json_time=cJSON_GetObjectItem(json_sessionId, "time");
         if(json_time==NULL) {
            return -1;
         }
         cJSON_SetNumberValue(json_sessionId, (double)time(NULL));

         return 0;
      } 
   }
   else {
      return -1;
   }
}


char *get_users_list_alloc()
{
   char *s=cJSON_Print(users2);

   return s;
}


int mea_rest_api_user_GET(struct mg_connection *conn, int method, char *tokens[], int l_tokens)
{
   if(l_tokens==0) {
      char *s=get_users_list_alloc();
      httpResponse(conn, 200, NULL, s);
      free(s);
      return 0;
   }
   else if(l_tokens==1) {
      cJSON *jsonUser=cJSON_GetObjectItem(users2, tokens[0]);
      if(jsonUser) {
         char *s=cJSON_Print(jsonUser);
         httpResponse(conn, 200, NULL, s);
         free(s);
         return 0;
      }
   }
   return returnResponse(conn, 404, 1, NULL);
}


int mea_rest_api_user_POST(struct mg_connection *conn, int method, char *tokens[], int l_tokens)
{
   if(l_tokens==0) {
      cJSON *jsonData=getData_alloc(conn);
      if(jsonData) {
         return returnResponse(conn, 200, 0, "user post");
      }
   }
   return returnResponse(conn, 404, 1, NULL);
}


int mea_rest_api_user_PUT(struct mg_connection *conn, int method, char *tokens[], int l_tokens)
{
   if(l_tokens==1) {
      cJSON *jsonData=getData_alloc(conn);
      if(jsonData) {
         return returnResponse(conn, 200, 0, "user put");
      }
   }
   return returnResponse(conn, 404, 1, NULL);
}


int mea_rest_api_user_DELETE(struct mg_connection *conn, int method, char *tokens[], int l_tokens)
{
   if(l_tokens==1) {
      return returnResponse(conn, 200, 0, "user delete");
   }
   return returnResponse(conn, 404, 1, NULL);
}


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
         l_buff=strlen(buff);
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
   return 1;
}


int mea_rest_api_service_PUT(struct mg_connection *conn, int method, char *tokens[], int l_tokens)
{
   int ret=0;
   char json[2048]="";
   char *s=NULL;
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
                  cJSON_Delete(jsonData);
                  return returnResponse(conn, 200, 0, msg);
               }
               if(mea_strcmplower(jsonAction->valuestring, "stop")==0) {
                  ret=process_stop(id, msg, sizeof(msg)-1);
                  cJSON_Delete(jsonData);
                  return returnResponse(conn, 200, 0, msg);
               }
               if(mea_strcmplower(jsonAction->valuestring, "restart")==0) {
                  ret=process_restart(id, msg, sizeof(msg)-1);
                  cJSON_Delete(jsonData);
                  return returnResponse(conn, 200, 0, msg);
               }
               else {
                  process_start(id, msg, sizeof(msg)-1);
                  cJSON_Delete(jsonData);
                  return returnResponse(conn, 400, 1,"unknown action");
               }
            }
            else {
               cJSON_Delete(jsonData);
               return returnResponse(conn, 400, 1,"action not a string");
            }
         }
         else {
            cJSON_Delete(jsonData);
            return returnResponse(conn, 400, 1,"no action");
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


int mea_rest_api_type(struct mg_connection *conn, int method, char *tokens[], int l_tokens)
{
   const char *meaSessionId=mg_get_header(conn, "Mea-Session");

   if(checkSession((char *)meaSessionId)!=0) {
      return returnResponse(conn, 401, 99, NOT_AUTHORIZED);
   }

   switch(method) {
      case HTTP_GET_ID:
         if(l_tokens==0) {
            char *s=getTypesAsString_alloc();
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
            char *s=getTypeAsStringByName_alloc(tokens[0]);
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

      default:
         return returnResponse(conn, 405, 1, BAD_METHOD);
   }
 
   return 0;
}


int mea_rest_api_service(struct mg_connection *conn, int method, char *tokens[], int l_tokens)
{
   const char *meaSessionId=mg_get_header(conn, "Mea-Session");

   if(checkSession((char *)meaSessionId)!=0) {
      return returnResponse(conn, 401, 99, NOT_AUTHORIZED);
   }

   int ret=0;
   char json[2048]="";
   char *s=NULL;
   int id=0;

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
            return returnResponse(conn, 404, 1, NULL);
         }
      }
      else {
         return returnResponse(conn, 400, 1, NO_VALID_JSON_DATA);
      }
   }
   else {
      return returnResponse(conn, 404, 1, NULL);
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
   const char *meaSessionId=mg_get_header(conn, "Mea-Session");

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
         else if(l_tokens==1) {
            cJSON *jsonData=getData_alloc(conn);
            if(jsonData) {
            if(deleteInterface(tokens[0])>=0) {
                  return returnResponse(conn, 200, 0, NULL);
               }
               else {
                  return returnResponse(conn, 404, 1, NULL);
               }
            }
         }
         return returnResponse(conn, 404, 1, NULL);


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
         else if(l_tokens==1) {
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
         return returnResponse(conn, 500, 1, NULL);

      case HTTP_POST_ID:
         if(l_tokens==0) {
            cJSON *jsonData=getData_alloc(conn);
            if(jsonData) {
               if(addInterface(jsonData)>=0) {
                  return returnResponse(conn, 200, 0, NULL);
               }
               else {
                  return returnResponse(conn, 404, 1, NULL);
               }
            }
            else {
               return returnResponse(conn, 400, 1, NO_VALID_JSON_DATA);
            }
         }
         else {
            return mea_rest_api_device(conn, method, tokens[0], &(tokens[1]), l_tokens-1);
         }

      case HTTP_PUT_ID:
         if(l_tokens==0) {
            return returnResponse(conn, 404, 1, NULL);
         }
         else if(l_tokens==1) {
            cJSON *jsonData=getData_alloc(conn);
            if(jsonData) {
               if(updateInterface(tokens[0], jsonData)>=0) {
                  return returnResponse(conn, 200, 0, NULL);
               }
               else {
                  return returnResponse(conn, 404, 1, NULL);
               }
            }
            else {
               return returnResponse(conn, 400, 1, NO_VALID_JSON_DATA);
            }
         }

      default:
         return returnResponse(conn, 405, 1, BAD_METHOD);
   }
}


int mea_rest_api_configuration(struct mg_connection *conn, int method, char *tokens[], int l_tokens)
{
   const char *meaSessionId=mg_get_header(conn, "Mea-Session");

   if(checkSession((char *)meaSessionId)!=0) {
      return returnResponse(conn, 401, 99, NOT_AUTHORIZED);
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
         cJSON *jsonData=getData_alloc(conn);
         if(!jsonData) {
            return returnResponse(conn, 400, 1, NO_VALID_JSON_DATA);
         }
         if(l_tokens==0) {
            if(updateAppParameters(jsonData)==0) {
               return returnResponse(conn, 200, 0, NULL);
            }
            else {
               return returnResponse(conn, 404, 1, NULL);
            }
         }
         else if(l_tokens==1) {
            if(updateAppParameter(tokens[0], jsonData)==0) {
               return returnResponse(conn, 200, 0, NULL);
            }
            else {
               return returnResponse(conn, 404, 1, NULL);
            }
         }
         else {
            return returnResponse(conn, 404, 1, NULL);
         }
      }
      default:
         return returnResponse(conn, 405, 1, BAD_METHOD);
   }
}


int mea_rest_api_user(struct mg_connection *conn, int method, char *tokens[], int l_tokens)
{
   const char *meaSessionId=mg_get_header(conn, "Mea-Session");

   if(checkSession((char *)meaSessionId)!=0) {
      return returnResponse(conn, 401, 99, NOT_AUTHORIZED);
   }

   switch(method) {

      case HTTP_GET_ID:
         return mea_rest_api_user_GET(conn, method, tokens, l_tokens);

      case HTTP_POST_ID:
         return mea_rest_api_user_POST(conn, method, tokens, l_tokens);

      case HTTP_PUT_ID:
         return mea_rest_api_user_PUT(conn, method, tokens, l_tokens);

      case HTTP_DELETE_ID:
         return mea_rest_api_user_DELETE(conn, method, tokens, l_tokens);

      default:
         return returnResponse(conn, 405, 1, BAD_METHOD);
   }
}


int mea_rest_api_session(struct mg_connection *conn, int method, char *tokens[], int l_tokens)
{
   switch(method) {

      case HTTP_POST_ID:
         if(l_tokens==0) {
            return openSession(conn);
         }
         else {
            return returnResponse(conn, 404, 1, NULL);
         }

      case HTTP_DELETE_ID:
         if(l_tokens==1) {
            return closeSession(conn, tokens[0]);
         }
         else {
            return returnResponse(conn, 404, 1, NULL);
         }

      default:
         return returnResponse(conn, 405, 1, BAD_METHOD);
   }
   return returnResponse(conn, 500, 1, NULL);
}


int mea_rest_api_init()
{
   srand(time(NULL));
   sessions=cJSON_CreateObject();
   users2=cJSON_Parse(_users2);

   return 1;
}


int mea_rest_api_exit()
{
   cJSON_Delete(sessions);
   sessions=NULL;
   cJSON_Delete(users2);
   users2=NULL;

   return 1;
}


int mea_rest_api(struct mg_connection *conn, int method, char *tokens[], int l_tokens)
{
   if(sessions==NULL)
      mea_rest_api_init();

   purgeSessions();

   int ressource=get_token_id_by_string((char *)tokens[0]);

   char **_tokens;
   int _l_tokens=l_tokens-1;

   if(_l_tokens<=0) {
      _l_tokens=0;
      _tokens=NULL;
   }
   else {
      _tokens=&tokens[1];
   }

   switch(ressource) {
      case API_SESSION_ID:
         return mea_rest_api_session(conn, method, _tokens, _l_tokens);
      case API_INTERFACE_ID:
         return mea_rest_api_interface(conn, method, _tokens, _l_tokens);
      case API_SERVICE_ID:
         return mea_rest_api_service(conn, method, _tokens, _l_tokens);
      case API_TYPE_ID:
         return mea_rest_api_type(conn, method, _tokens, _l_tokens);
      case API_CONFIGURATION_ID:
         return mea_rest_api_configuration(conn, method, _tokens, _l_tokens);
      case API_USER_ID:
         return mea_rest_api_user(conn, method, _tokens, _l_tokens);
      default:
         return returnResponse(conn, 404, 1, NULL);
   } 
}

