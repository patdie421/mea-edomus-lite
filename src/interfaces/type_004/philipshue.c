#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

//#include "debug.h"
//#include "error.h"
#include "mea_verbose.h"

#include "mea_sockets_utils.h"
#include "cJSON.h" // lib JSON voir http://sourceforge.net/projects/cjson/
#include "philipshue_color.h"

char *_true = "true";
char *_false = "false";
char *_nameStr = "name";
char *_stateStr = "state";
char *_reachableStr = "reachable";
char *_onStr = "on";
char *_successStr = "success";

char *apiLightStateTemplate = "/api/%s/lights/%d/state";
char *apiGroupStateTemplate = "/api/%s/groups/%d/action";
char *apiLightsListTemplate = "/api/%s/lights";
char *apiGroupsListTemplate = "/api/%s/groups";
//char *apiscenesListTemplate = "/api/%s/scenes"; // à vérifier ...

char *apiOnxyBriColorTemplate = "{\"on\": true, \"xy\" : [%f, %f], \"bri\" : %d}";
char *apiLightStateOnTemplate = "/lights/%s/state/on";


cJSON *my_cJSON_GetItemByName(cJSON *object, const char *name)
{
   if(object == NULL)
      return NULL;
   if(object->type != cJSON_Object)
      return NULL;
   cJSON *c=object->child;
   while (c && strcmp(c->string, name))
      c=c->next;
   return c;
}


cJSON *getLightIdByName(cJSON *allLights, char *name, int16_t *id)
{
   if(allLights == NULL)
      return NULL;
   cJSON *light=allLights->child;
   while(light)
   {
      cJSON *_name = my_cJSON_GetItemByName(light,_nameStr);
      if(_name == NULL)
         return NULL;
      if(strcmp(_name->valuestring, name)==0)
      {
         *id = atoi(light->string);
         return light;
      }
      light=light->next;
   }
   return NULL;
}


cJSON *getGroupIdByName(cJSON *allGroups, char *name, int16_t *id)
{
   if(allGroups == NULL)
      return NULL;
   cJSON *group=allGroups->child;
   while(group)
   {
      cJSON *_name = my_cJSON_GetItemByName(group, _nameStr);
      if(_name == NULL)
         return NULL;
      if(strcmp(_name->valuestring, name)==0)
      {
         *id = atoi(group->string);
         return group;
      }
      group=group->next;
   }
   return NULL;
}


cJSON *getLightStateByName(cJSON *allLights, char *name, int16_t *on, int16_t *reachable)
{
   if(allLights == NULL)
      return NULL;
   cJSON *light=allLights->child;
   while(light)
   {
      cJSON *_name = my_cJSON_GetItemByName(light,_nameStr);
      if(_name == NULL)
         return NULL;
      
      if(strcmp(_name->valuestring,name)==0)
      {
         cJSON *state = cJSON_GetObjectItem(light, _stateStr);
         if(state)
         {
            cJSON *_on = NULL;
            cJSON *_reachable = NULL;

            cJSON *c=state->child;
            while (c && (!_on || !_reachable))
            {
               if(strcmp(c->string, _onStr)==0)
                  _on = c;
               if(strcmp(c->string, _reachableStr)==0)
                  _reachable = c;
               c=c->next;
            }

            if(_on && _reachable)
            {
               *on = _on->valueint;
               *reachable = _reachable->valueint;
               return state;
            }
         }
      }
      light=light->next;
   }
   return NULL;
}


cJSON *getLightStateById(cJSON *allLights, uint16_t id, int16_t *on, int16_t *reachable)
{
   if(allLights == NULL)
      return NULL;
   cJSON *light=allLights->child;
   while(light)
   {
      int _id = atoi(light->string);

      if(_id == id)
      {
         cJSON *state = cJSON_GetObjectItem(light, _stateStr);
         if(state)
         {
            cJSON *_on = NULL;
            cJSON *_reachable = NULL;

            cJSON *c=state->child;
            while (c && (!_on || !_reachable))
            {
               if(strcmp(c->string, _onStr)==0)
                  _on = c;
               if(strcmp(c->string, _reachableStr)==0)
                  _reachable = c;
               c=c->next;
            }

            if(_on && _reachable)
            {
               if(on)
                  *on = _on->valueint;
               if(reachable)
                  *reachable = _reachable->valueint;
               return state;
            }
         }
      }
      light=light->next;
   }
   return NULL;
}


cJSON *getLightNameById(cJSON *allLights, uint16_t id, char *name, uint16_t l_name)
{
   if(allLights == NULL)
      return NULL;
   cJSON *light=allLights->child;
   while(light)
   {
      int _id = atoi(light->string);
      if(_id == id)
      {
         cJSON *_name = cJSON_GetObjectItem(light, _nameStr);
         if(_name)
         {
            strncpy(name,_name->valuestring,l_name-1);
            return light;
         }
      }
      light=light->next;
   }
   return NULL;
}


int16_t _updateLightStateIfSuccess(cJSON *light, cJSON *httpResponseJSON)
{
   int16_t ret=-1;
   char lightStateOn[21];
   
   if(httpResponseJSON)
   {
      cJSON *_on = my_cJSON_GetItemByName( my_cJSON_GetItemByName(light,_stateStr) ,_onStr);
      
      snprintf(lightStateOn, sizeof(lightStateOn)-1, apiLightStateOnTemplate, light->string);
      
      cJSON *_resp = cJSON_GetArrayItem(httpResponseJSON, 0); // la réponse est un tableau, on pointe sur le premier element
      while(_resp)
      {
         cJSON *_success = my_cJSON_GetItemByName( cJSON_GetArrayItem(httpResponseJSON, 0), _successStr);
         if(_success)
         {
            int16_t old=-1;
            cJSON *_lightStateOn=my_cJSON_GetItemByName(_success, lightStateOn);
            if(_lightStateOn)
            {
               old=_on->valueint;
               if(_lightStateOn->valueint == 1)
               {
                  _on->type = cJSON_True;
                  _on->valueint = 1;
                  _on->valuedouble = 1.0;
               }
               else
               {
                  _on->type = cJSON_False;
                  _on->valueint = 0;
                  _on->valuedouble = 0.0;
               }
               ret=old;
               break;
            }
         }
         _resp = _resp->next;
      }
   }  
   return ret;
}


int16_t _setOnOffState(int id, char *template, int16_t state, char *server, int port, char *user)
{
   char *_bool;
   int16_t nerr=-1;
   int16_t ret=-1;
   char request[256];
   char response[2048];
   uint32_t l_response=0;
   char url[256];

   if(state)
      _bool = _true;
    else
      _bool = _false;

   snprintf(url, sizeof(url) - 1, template, user, id);
   snprintf(request, sizeof(request) - 1, "{\"on\":%s}", _bool);


   l_response=sizeof(response);
   char *httpResponseDataPtr = httpRequest(HTTP_PUT, server, port, url, request, (int)(strlen(request)), response, &l_response, &nerr);
   if(!httpResponseDataPtr)
   {
      VERBOSE(5) mea_log_printf("%s : httpRequest() error (%d)\n", __func__, nerr);
      return -1;
   }

   int httpResponseStatus = httpGetResponseStatusCode(response, l_response);
   if(httpResponseStatus != 200)
   {
      VERBOSE(5) mea_log_printf("%s : http error = %d\n", __func__, httpResponseStatus);
      return -1;
   }

   cJSON *httpResponseJSON = cJSON_Parse(httpResponseDataPtr);
   if(httpResponseJSON)
   {
      ret=0;
      cJSON *_resp = cJSON_GetArrayItem(httpResponseJSON, 0); // la réponse est un tableau, on pointe sur le premier element
      while(_resp)
      {
         cJSON *_success = my_cJSON_GetItemByName( _resp, _successStr);
         if(!_success)
         {
            ret=-1;
         }
         _resp = _resp->next;
      }
      cJSON_Delete(httpResponseJSON);
   }

   return ret;
}


int16_t setGroupStateByName(cJSON *allGroups, char *name, int16_t state, char *server, int port, char *user)
{
   int16_t id;

   if(allGroups == NULL)
      return 0;
   cJSON *group = getGroupIdByName(allGroups, name, &id);
   if(group)
   {
      return _setOnOffState(id, apiGroupStateTemplate, state, server, port, user);
   }
   return -1;
}


int16_t setLightStateByName(cJSON *allLights, char *name, int16_t state, char *server, int port, char *user)
{
   int16_t id;
   if(allLights == NULL)
      return 0;
   cJSON *light = getLightIdByName(allLights, name, &id);
   if(light)
   {
      if(_setOnOffState(id, apiLightStateTemplate, state, server, port, user)==0)
      {
         cJSON *_on = my_cJSON_GetItemByName( my_cJSON_GetItemByName(light,_stateStr) ,_onStr);
         int16_t oldState = _on->valueint;
         if(state == 1)
            _on->type = cJSON_True;
         else
            _on->type = cJSON_False;
         _on->valueint = (int)state;
         _on->valuedouble = (double)state;
         
         return oldState;
      }
   }
   return -1;
}


int16_t setRGBColor(int id, char *template, uint32_t color, char *server, int port, char *user)
{
   int16_t nerr;

   char request[256];
   char response[2048];
   uint32_t l_response;
   char url[256];
      
   float x,y, fbri;
   int16_t bri;
      
   snprintf(url, sizeof(url) - 1, template, user, id);
   
   int16_t gamutID = 0;

   X_Y_B_ForGamutIDFromRgbUint32(color, gamutID, &x, &y, &fbri);
   bri=(int16_t)(fbri * 255.0);
   sprintf(request, apiOnxyBriColorTemplate, x, y, bri);
   
   l_response=sizeof(response);
   char *httpResponseDataPtr = httpRequest(HTTP_PUT, server, port, url, request, (int)(strlen(request)), response, &l_response, &nerr);
   if(!httpResponseDataPtr)
   {
      VERBOSE(5) mea_log_printf("%s : httpRequest() error (%d)\n", __func__, nerr);
      return -1;
   }
   
   int httpResponseStatus = httpGetResponseStatusCode(response, l_response);
   if(httpResponseStatus != 200)
   {
      VERBOSE(5) mea_log_printf("%s : http error = %d\n", __func__, httpResponseStatus);
      return -1;
   }

   int ret = -1;
   cJSON *httpResponseJSON = cJSON_Parse(httpResponseDataPtr);
   if(httpResponseJSON)
   {
      ret=0;
      cJSON *_resp = cJSON_GetArrayItem(httpResponseJSON, 0); // la réponse est un tableau, on pointe sur le premier element
      while(_resp)
      {
         cJSON *_success = my_cJSON_GetItemByName( _resp, _successStr);
         if(!_success)
         {
            ret=-1;
         }
         _resp = _resp->next;
      }
      cJSON_Delete(httpResponseJSON);
   }

   return ret;
}


int16_t setGroupColorByName(cJSON *allGroups, char *name, uint32_t color, char *server, int port, char *user)
{
   int16_t id;
   if(allGroups == NULL)
      return 0;
   cJSON *group = getGroupIdByName(allGroups, name, &id);
   if(group)
   {
      return setRGBColor(id, apiGroupStateTemplate, color, server, port, user);
   }
   return -1;
}


int16_t setLightColorByName(cJSON *allLights, char *name, uint32_t color, char *server, int port, char *user)
{
   int16_t id;
   if(allLights == NULL)
      return 0;
   cJSON *light = getLightIdByName(allLights, name, &id);
   if(light)
   {
      return setRGBColor(id, apiLightStateTemplate, color, server, port, user);
   }
   return -1;
}


cJSON *_getAll(char *template, char *server, int port, char *user)
{
   char response[0xFFFF];
   char *httpResponseDataPtr;
   uint32_t l_response=0;
   int16_t nerr;
   
   char *query =(char *)malloc(strlen(template)-2 + strlen(user) + 1);
   if(query == NULL)
   {
      DEBUG_SECTION {
         mea_log_printf("%s (%s) : %s - ", ERROR_STR, __func__, MALLOC_ERROR_STR);
         perror("");
      }
      return NULL;
   }

   pthread_cleanup_push( (void *)free, query );
   sprintf(query,template,user);
   l_response=sizeof(response);
   httpResponseDataPtr = httpRequest(HTTP_GET, server, port, query, NULL, 0, response, &l_response, &nerr);
   pthread_cleanup_pop(1);
   
   if(!httpResponseDataPtr)
   {
      DEBUG_SECTION {
         mea_log_printf("%s (%s) : no data in response (error = %d)\n", ERROR_STR, __func__, nerr);
      }
      return NULL;
   }
  
   int httpResponseStatus = httpGetResponseStatusCode(response, l_response);
   if(httpResponseStatus != 200)
   {
      DEBUG_SECTION {
         mea_log_printf("%s (%s) : http error = %d\n", ERROR_STR, __func__, httpResponseStatus);
      }
   }
   else
   {
      cJSON *all;
      all=cJSON_Parse(httpResponseDataPtr);
      if (!all)
      {
         DEBUG_SECTION {
            mea_log_printf("%s (%s) : Error before:\n...[%80s]...\n", ERROR_STR, __func__, cJSON_GetErrorPtr());
         }
         return NULL;
      }
      else
      {
         return all;
      }
   }
   
   return NULL;
}

/*
cJSON *getAllScenes(char *server, int port, char *user)
{
   return _getAll(apiscenesListTemplate, server, port, user);
}
*/

cJSON *getAllGroups(char *server, int port, char *user)
{
   return _getAll(apiGroupsListTemplate, server, port, user);
}


cJSON *getAllLights(char *server, int port, char *user)
{
  return _getAll(apiLightsListTemplate, server, port, user);
}

