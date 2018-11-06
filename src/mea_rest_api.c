#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "cJSON.h"
#include "mea_rest_api.h"
#include "mea_verbose.h"
#include "tokens.h"
#include "mongoose.h"
#include "guiServer.h"

char *_users = "{\"admin\":\"admin\",\"user\":\"user\"}";

cJSON *users = NULL;
cJSON *sessions = NULL;


cJSON *loadFromFile(char *filename)
{
   // Ouvrir le fichier

   // Obtenir la taille 
      // fseek(fp, 0L, SEEK_END);
      // sz = ftell(fp);

   // allouer la memoire
      // char *data=malloc(sz);

   // parser data en json

   // free(data)

   // return
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
}


int closeSession(struct mg_connection *conn, char *id)
{
   cJSON *json_id = NULL;

   mea_log_printf("closing session: %s\n", id);

   json_id = cJSON_GetObjectItem(sessions, id);

   if(json_id) {
      cJSON_DeleteItemFromObject(sessions, id);

      mea_log_printf("Session closed\n");
   }
   else
      mea_log_printf("Unknown session\n");

   return 1;
}


int openSession(struct mg_connection *conn)
{
   int ret=0;

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
               
                  cJSON *u = cJSON_GetObjectItem(users, user->valuestring);

                  if(u) {
                     if(u->type=cJSON_String && strcmp(password->valuestring, u->valuestring)==0) {

                        char id[21];

                        for(int i=0;i<20;i++)
                           id[i]='A'+ rand() % ('Z'-'A');
                        id[20]=0;

                        cJSON *idData = cJSON_CreateObject();
                        cJSON_AddItemToObject(idData, "time", cJSON_CreateNumber((double)time(NULL))); 
                        cJSON_AddItemToObject(sessions, id, idData);
                        char jsonSessionId[40];
                        snprintf(jsonSessionId, sizeof(jsonSessionId), "{\"sessionId\":\"%s\"}", id);
                        httpResponse(conn, 200, NULL, jsonSessionId);
                        ret=1;
                     }
                     else {
                        httpResponse(conn, 401, NULL, "{}");
                        ret=1;
                     }
                  }
                  else {
                     httpResponse(conn, 401, NULL, "{}");
                     ret=1;
                  }
               }
               else {
                  httpResponse(conn, 401, NULL, "{}");
                  ret=1;
               }

               cJSON_Delete(root);
            }
            else {
               httpResponse(conn, 400, NULL, "{}");
               ret=1;
            }

            free(data);
            data=NULL;
         }
      }
   }
   else {
      httpResponse(conn, 400, NULL, "{}");
      ret=1;
   }

   return ret;
}


int mea_rest_api_interface(struct mg_connection *conn, int method, char *tokens[], int l_tokens)
{
   httpResponse(conn, 200, NULL, "{}");
   return 1;
}


int mea_rest_api_session(struct mg_connection *conn, int method, char *tokens[], int l_tokens)
{
   switch(method) {
      case HTTP_GET_ID:
         return 0;
         break;
      case HTTP_POST_ID:
         if(l_tokens==0)
            return openSession(conn);
         else
            return 0;
         break;
      case HTTP_PUT_ID:
         return 0;
         break;
      case HTTP_DELETE_ID:
         if(l_tokens==1) {
            return closeSession(conn, tokens[0]);
         }
         return 0;
         break;
      default:
         return 0;
   }

   httpResponse(conn, 404, NULL, "{}");
   return 1;
}


int mea_rest_api_init()
{
   srand(time(NULL));
   sessions=cJSON_CreateObject();
   users=cJSON_Parse(_users);
}


int mea_rest_api_exit()
{
   cJSON_Delete(sessions);
   sessions=NULL;
   cJSON_Delete(users);
   users=NULL;
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
         break;
      case API_INTERFACE_ID:
         return mea_rest_api_interface(conn, method, _tokens, _l_tokens);
         break;

      default:
         return 1;
   } 
 
   return 1;
}

