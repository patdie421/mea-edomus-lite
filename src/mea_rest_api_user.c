#include <stdio.h>
#include <stdlib.h>

#include "mea_rest_api.h"
#include "mea_rest_api_user.h"

#include "interfacesServer.h"
#include "mongoose.h"
#include "cJSON.h"
#include "httpServer.h"
#include "mea_http_utils.h"
#include "tokens.h"
#include "tokens_da.h"
#include "mea_verbose.h"
#include "mea_string_utils.h"
#include "mea_json_utils.h"
#include "users.h"


char *get_users_list_alloc()
{
   cJSON *_users2=cJSON_Duplicate(users2, 1);
   
   cJSON *user=_users2->child;
   while(user) {
      cJSON_DeleteItemFromObject(user, "password");
      user=user->next;
   }

   char *s=cJSON_Print(_users2);
   cJSON_Delete(_users2);

   return s;
}


int _usersCommit()
{
   char *usersFileName=user_getFileFullPath();
   
   if(usersFileName==NULL) {
      return 1;
   }
   
   if(backupJson(usersFileName)==0) {
      if(writeJson(usersFileName, users2)==0) {
         return 0;
      }
      else {
         return 3;
      }
   }
   else {
      return 2;
   }
}


int _usersRollback()
{
   char *usersFileName=user_getFileFullPath();
   
   if(usersFileName==NULL) {
      return 1;
   }

   cJSON_Delete(users2);
   users2=loadJson_alloc(usersFileName);
   
   return 0;
}


int _userUpdatePassword(char *user, cJSON *parameters)
{
   cJSON *oldpassword=cJSON_GetObjectItem(parameters, "oldpassword");
   cJSON *newpassword=cJSON_GetObjectItem(parameters, "newpassword");

   if(oldpassword==NULL || oldpassword->type!=cJSON_String) {
      return 1;
   }

   if(newpassword==NULL || newpassword->type!=cJSON_String) {
      return 2;
   }

   cJSON *_user=cJSON_GetObjectItem(users2, user);
   if(_user==NULL) {
      return 3;
   }

   cJSON *password=cJSON_GetObjectItem(_user, "password");
   if(password==NULL || password->type!=cJSON_String) {
      return 4;
   }

   if(strcmp(password->valuestring, oldpassword->valuestring)==0) {
      cJSON_DeleteItemFromObject(_user, "password");
      cJSON_AddStringToObject(_user, "password", newpassword->valuestring);
   }
   else {
      return 5;
   }

   return 0;
}


int _userUpdate(char *username, cJSON *jsonData)
{
   if(!jsonData || jsonData->type!=cJSON_Object)
      return 1;

   cJSON *user=cJSON_GetObjectItem(users2, username);
   if(!user || user->type!=cJSON_Object)
      return 2;

   cJSON *profile=cJSON_GetObjectItem(jsonData, "profile");
   cJSON *fullname=cJSON_GetObjectItem(jsonData, "fullname");
   cJSON *password=cJSON_GetObjectItem(jsonData, "password");

   if( !profile && !fullname && !password ) {
      return 3;
   }

   if(profile) {
      if(profile->type==cJSON_Number) {
         cJSON_DeleteItemFromObject(user,"profile");
         cJSON_AddNumberToObject(user,"profile",profile->valuedouble);
      }
      else
         return 4;
   }

   if(fullname) {
      if(fullname->type==cJSON_String) {
         cJSON_DeleteItemFromObject(user,"fullname");
         cJSON_AddStringToObject(user, "fullname", fullname->valuestring);
      }
      else
         return 5;
   }
   
   if(password) {
      if(password->type==cJSON_String) {
         cJSON_DeleteItemFromObject(user,"password");
         cJSON_AddStringToObject(user, "password", fullname->valuestring);
      }
      else
         return 6;
   }

   return 0;
}


int _userCreate(cJSON *jsonData)
{
   if(!jsonData || jsonData->type!=cJSON_Object)
      return 1;

   cJSON *username=cJSON_GetObjectItem(jsonData, "username");
   if(!username || username->type!=cJSON_String)
      return 2;
      
   cJSON *user=cJSON_GetObjectItem(users2, username->valuestring);
   if(user) {
      return 3;
   }

   cJSON *profile=cJSON_GetObjectItem(jsonData, "profile");
   if(!profile || profile->type!=cJSON_Number)
      return 4;
   cJSON *fullname=cJSON_GetObjectItem(jsonData, "fullname");
   if(fullname && fullname->type!=cJSON_String)
      return 5;
   cJSON *password=cJSON_GetObjectItem(jsonData, "password");
   if(!password || password->type!=cJSON_String)
      return 6;
      
   user=cJSON_CreateObject();
   cJSON_AddItemToObject(user, "profile", cJSON_CreateNumber(profile->valuedouble));
   cJSON_AddItemToObject(user, "fullname", cJSON_CreateString(fullname->valuestring));
   cJSON_AddItemToObject(user, "password", cJSON_CreateString(password->valuestring));
   cJSON_AddItemToObject(users2, username->valuestring, user);
   
   return 0;
}


cJSON *getJsonDataAction(struct mg_connection *conn, cJSON *jsonData)
{
   cJSON *action=cJSON_GetObjectItem(jsonData,"action");
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
   cJSON *parameters=cJSON_GetObjectItem(jsonData, "parameters");
   
   if(!parameters) {
      returnResponseAndDeleteJsonData(conn, 400, 1, "no parameters", jsonData);
      return NULL;
   }
   if(parameters->type!=cJSON_Object) {
      returnResponseAndDeleteJsonData(conn, 400, 1, "parameters not an object", jsonData);
      return NULL;
   }
   return parameters;
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
         cJSON *_jsonUser=cJSON_Duplicate(jsonUser, 1);
         cJSON_DeleteItemFromObject(_jsonUser,"password");
         char *s=cJSON_Print(_jsonUser);
         httpResponse(conn, 200, NULL, s);
         free(s);
         cJSON_Delete(_jsonUser);
         return 0;
      }
   }
   return returnResponse(conn, 404, 1, NULL);
}


int mea_rest_api_user_POST(struct mg_connection *conn, int method, char *tokens[], int l_tokens)
{
   cJSON *action=NULL;
   cJSON *parameters=NULL;
   
   if(l_tokens==0) {
      cJSON *jsonData=getData_alloc(conn);
      if(jsonData) {
         action=getJsonDataAction(conn, jsonData);
         if(!action) {
            return 1;
         }

         if(mea_strcmplower(action->valuestring,"create")==0) {
            parameters=getJsonDataParameters(conn, jsonData);
            if(!parameters) {
               return 1;
            }
            int ret=_userCreate(parameters);
            if(ret==0)
               return returnResponseAndDeleteJsonData(conn, 200, 0, SUCCESS, jsonData);
            else
               return returnResponseAndDeleteJsonData(conn, 200, ret, "user not created", jsonData);
         }
         else if(mea_strcmplower(action->valuestring,"commit")==0) {
            int ret=_usersCommit();
            if(ret==0)
               return returnResponseAndDeleteJsonData(conn, 200, 0, SUCCESS, jsonData);
            else
               return returnResponseAndDeleteJsonData(conn, 200, ret, "commit not done", jsonData);
         }
         else if(mea_strcmplower(action->valuestring,"rollback")==0) {
            int ret=_usersRollback();
            if(ret==0)
               return returnResponseAndDeleteJsonData(conn, 200, 0, SUCCESS, jsonData);
            else
               return returnResponseAndDeleteJsonData(conn, 200, ret, "rollback not done", jsonData);
         }
      }
      else {
         return returnResponse(conn, 200, 1, NO_VALID_JSON_DATA);
      }
   }
   return returnResponse(conn, 404, 1, NULL);
}


int mea_rest_api_user_PUT(struct mg_connection *conn, int method, char *tokens[], int l_tokens)
{
   const char *meaSessionId=mg_get_header(conn, "Mea-Session");
   cJSON *action=NULL;
   cJSON *parameters=NULL;
   
   if(checkSession((char *)meaSessionId)!=0) {
      return returnResponse(conn, 401, 99, NOT_AUTHORIZED);
   }

   cJSON *jsonData=getData_alloc(conn);
   if(jsonData) {
      parameters=getJsonDataParameters(conn, jsonData);
      if(!parameters) {
         return 1;
      }
      action=getJsonDataAction(conn, jsonData);
      if(!action) {
         return 1;
      }
   }
   else {
      return returnResponse(conn, 400, 1, NO_VALID_JSON_DATA);
   }

   int profile=getSessionProfile((char *)meaSessionId);
   if(profile<0) {
      returnResponseAndDeleteJsonData(conn, 500, 1, "profile not defined",jsonData);
   }

   char *user=getSessionUser_alloc((char *)meaSessionId);
   if(!user) {
      return returnResponseAndDeleteJsonData(conn, 500, 1, "user not defined", jsonData);
   }

   if(l_tokens==0) {
      if(mea_strcmplower(action->valuestring, "password")==0) {
         int ret=_userUpdatePassword(user, parameters);
         free(user);
         if(ret==0) {
            return returnResponseAndDeleteJsonData(conn, 200, 0, SUCCESS, jsonData);
         }
         else {
            return returnResponseAndDeleteJsonData(conn, 400, ret, "password not updated", jsonData);
         }
      }
      else {
         return returnResponseAndDeleteJsonData(conn, 404, 1, "unknown action", jsonData);
      }
   }
   else if(l_tokens==1) {
      if(profile>0) {
         if(mea_strcmplower(action->valuestring, "update")==0) {
            int ret=_userUpdate(tokens[0], parameters);
            if(ret==0) {
               return returnResponseAndDeleteJsonData(conn, 200, 0, SUCCESS, jsonData);
            }
            else {
               return returnResponseAndDeleteJsonData(conn, 400, ret, "user not updated", jsonData);
            }
         }
         else {
            return returnResponseAndDeleteJsonData(conn, 404, 1, "unknown action", jsonData);
         }
      }
      else {
         return returnResponseAndDeleteJsonData(conn, 401, 99, NOT_AUTHORIZED, jsonData);
      }
   }
   return returnResponseAndDeleteJsonData(conn, 404, 1, NULL, jsonData);
}


int mea_rest_api_user_DELETE(struct mg_connection *conn, int method, char *tokens[], int l_tokens)
{
   int counter=0;
   
   if(l_tokens==1) {
      if(users2) {
         cJSON *user = users2->child;
         while(user) {
            cJSON *next = user->next;
            if(mea_strcmplower(user->string,tokens[0])==0) {
               cJSON_DeleteItemFromObject(users2, user->string);
               counter++;
               break;
            }
            user = next;
         }
         if(counter>0) {
            purgeSessionsByUsername(tokens[0]);  
            return returnResponse(conn, 200, 0, SUCCESS);
         }
         else {
            return returnResponse(conn, 400, 1, "user not found");
         }
      }
   }
   return returnResponse(conn, 404, 1, NULL);
}


int mea_rest_api_user(struct mg_connection *conn, int method, char *tokens[], int l_tokens)
{
   const char *meaSessionId=mg_get_header(conn, "Mea-Session");

   if(checkSession((char *)meaSessionId)!=0) {
      return returnResponse(conn, 401, 99, NOT_AUTHORIZED);
   }

   int profile=getSessionProfile((char *)meaSessionId);
   if(profile<0) {
      return returnResponse(conn, 500, 1, "no profile defined");
   }

   switch(method) {

      case HTTP_GET_ID:
         return mea_rest_api_user_GET(conn, method, tokens, l_tokens);

      case HTTP_POST_ID:
         if(profile>0) {
            return mea_rest_api_user_POST(conn, method, tokens, l_tokens);
         }
         else {
            return returnResponse(conn, 401, 98, NOT_AUTHORIZED);
         }
      case HTTP_PUT_ID:
         return mea_rest_api_user_PUT(conn, method, tokens, l_tokens);

      case HTTP_DELETE_ID:
         if(profile>0) {
            return mea_rest_api_user_DELETE(conn, method, tokens, l_tokens);
         }
         else {
            return returnResponse(conn, 401, 98, NOT_AUTHORIZED);
         }

      default:
         return returnResponse(conn, 405, 1, BAD_METHOD);
   }
}