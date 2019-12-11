#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <ctype.h>

#include "mea_rest_api_session.h"
#include "mea_rest_api.h"
#include "mea_rest_api_user.h"

#include "cJSON.h"
#include "mea_verbose.h"
#include "mea_string_utils.h"
#include "tokens.h"
#include "tokens_da.h"
#include "mongoose.h"
#include "httpServer.h"
#include "mea_http_utils.h"


cJSON *sessions = NULL;

int purgeSessions() {
   double tlimit=(double)(time(NULL)-10*60); // 10 mn

   if(sessions) {
      cJSON *session = sessions->child;
      while(session) {
         cJSON *next = session->next;
         if(cJSON_GetObjectItem(session, TIME_STR_C)->valuedouble < tlimit) {
            cJSON_DeleteItemFromObject(sessions, session->string); 
         }
         session = next;
      }
   }

   return 1;
}


int purgeSessionsByUsername(char *username) {
   if(sessions) {
      cJSON *session = sessions->child;
      while(session) {
         cJSON *next = session->next;
         if(mea_strcmplower(cJSON_GetObjectItem(session, USER_STR_C)->valuestring, username)==0) {
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

   VERBOSE(9) mea_log_printf("%s (%s) : closing session: %s\n",INFO_STR,__func__, id);

   json_id = cJSON_GetObjectItem(sessions, id);

   if(json_id) {
      cJSON_DeleteItemFromObject(sessions, id);

      VERBOSE(9) mea_log_printf("%s (%s) : session closed\n",INFO_STR,__func__);
   }
   else
      VERBOSE(0) mea_log_printf("%s (%s) : unknown session\n",WARNING_STR,__func__);

   return 1;
}


int openSession(struct mg_connection *conn)
{
   const char *cl=mg_get_header(conn, "Content-Length");
   int ret=1;
   
   if(cl) {
      int l_data=atoi(cl);
      if(l_data>0) {
         char *data=(char *)malloc(l_data+1);
         if(data) {
            mg_read(conn, data, l_data);
            data[l_data]=0;

            cJSON *root = cJSON_Parse(data);

            if(root) {
               cJSON *user = cJSON_GetObjectItem(root, USER_STR_C);
               cJSON *password = cJSON_GetObjectItem(root, PASSWORD_STR_C);
               if(user && password) {
               
                  cJSON *u = cJSON_GetObjectItem(users2, user->valuestring);
                  if(u) {
                     cJSON *p=cJSON_GetObjectItem(u, PASSWORD_STR_C);

                     if(p && p->type==cJSON_String && strcmp(password->valuestring, p->valuestring)==0) {

                        char id[21];

                        for(int i=0;i<20;i++)
                           id[i]='A'+ rand() % ('Z'-'A');
                        id[20]=0;

                        double _profile=0.0;
                        cJSON *profile=cJSON_GetObjectItem(u, PROFILE_STR_C);
                        if(profile && profile->type==cJSON_Number) {
                           _profile=profile->valuedouble;
                        }

                        cJSON *idData = cJSON_CreateObject();
                        cJSON_AddItemToObject(idData, TIME_STR_C, cJSON_CreateNumber((double)time(NULL))); 
                        cJSON_AddItemToObject(idData, PROFILE_STR_C, cJSON_CreateNumber(_profile));
                        cJSON_AddItemToObject(idData, USER_STR_C, cJSON_CreateString(user->valuestring));
                        cJSON_AddItemToObject(sessions, id, idData);
                        char jsonSessionId[256];
                        snprintf(jsonSessionId, sizeof(jsonSessionId), "{\"%s\":\"%s\"}", MEA_SESSIONID_STR_C, id);
                        httpResponse(conn, 200, NULL, jsonSessionId);
                     }
                     else {
                        ret=-1;
                        returnResponse(conn, 401, 99, NULL);
                     }
                  }
                  else {
                     ret=-1;
                     returnResponse(conn, 401, 99, NULL);
                  }
               }
               else {
                  ret=-1;
                  returnResponse(conn, 400, 1, "no user/password in body data");
               }
               cJSON_Delete(root);
            }
            else {
               ret=-1;
               returnResponse(conn, 400, 1, NO_VALID_JSON_DATA);
            }

            free(data);
            data=NULL;
         }
      }
      else {
         ret=-1;
         returnResponse(conn, 400, 1, "no body data");
      }
   }
   else {
      ret=-1;
      returnResponse(conn, 500, 1, "that's the end of adventure");
   }

   return ret;
}


int getSessionProfile(char *sessionId)
{
   if(sessionId) {
      cJSON *json_sessionId = cJSON_GetObjectItem(sessions, sessionId);
      if(json_sessionId == NULL) {
         return -1;
      }
      else {
         cJSON *json_profile=cJSON_GetObjectItem(json_sessionId, PROFILE_STR_C);
         if(json_profile==NULL || json_profile->type!=cJSON_Number) {
            return 0;
         }
         else {
            return (int)(json_profile->valuedouble);
         }
      }
   }
   else {
      return -1;
   }
}


char *getSessionUser_alloc(char *sessionId)
{
   char *user=NULL;
   
   if(sessionId) {
      cJSON *json_sessionId = cJSON_GetObjectItem(sessions, sessionId);
      if(json_sessionId == NULL) {
         return NULL;
      }
      else {
         cJSON *json_user=cJSON_GetObjectItem(json_sessionId, USER_STR_C);
         if(json_user==NULL || json_user->type!=cJSON_String) {
            return 0;
         }
         else {
            user=malloc(strlen(json_user->valuestring)+1);
            if(user) {
               strcpy(user, json_user->valuestring);
               return user;
            }
         }
      }
   }
   return NULL;
}


int checkSession(char *sessionId)
{
   if(sessionId) {
      cJSON *json_sessionId = cJSON_GetObjectItem(sessions, sessionId);
      if(json_sessionId == NULL) {
         return -1;
      }
      else {
         cJSON *json_time=cJSON_GetObjectItem(json_sessionId, TIME_STR_C);
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


int mea_rest_api_session(struct mg_connection *conn, int method, char *tokens[], int l_tokens)
{
   switch(method) {

      case HTTP_GET_ID:
         if(l_tokens==0) {
            const char *meaSessionId=mg_get_header(conn, MEA_SESSION_STR_C);
            if(checkSession((char *)meaSessionId)!=0) {
               return returnResponse(conn, 401, 99, NOT_AUTHORIZED);
            }
            return returnResponse(conn, 200, 0, NULL);
         }
         else {
            return returnResponse(conn, 404, 1, NULL);
         }

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
   return returnResponse(conn, 500, 1, "It's not my fault ! Sorry ...");
}
