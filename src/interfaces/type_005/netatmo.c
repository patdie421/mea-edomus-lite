#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "netatmo.h"
#include <curl/curl.h>
#include "cJSON.h"
#include "curl_adds.h"
#include "mea_verbose.h"
#include "mea_string_utils.h"


char *netatmo_thermostat_mode[]={"program", "away", "hg", "manual", "off", "max", NULL};


static int _netatmo_parse_return_json(char *response, char *err, int l_err)
{
   int ret_code=-1;

   cJSON *response_cjson=cJSON_Parse(response);
   if(response_cjson==NULL)
   {
       DEBUG_SECTION mea_log_printf("%s (%s) : JSON error\n", DEBUG_STR, __func__);
       goto _netatmo_parse_return_json_clean_exit;
   }

   cJSON *e=cJSON_GetObjectItem(response_cjson, "status");
   if(e && e->type==cJSON_String)
   {
      if(strcmp(e->valuestring, "ok")==0)
         ret_code=0;
      else
      {
         if(err)
            strncpy(err, e->valuestring, l_err);
         ret_code=1;
      }
   } 

_netatmo_parse_return_json_clean_exit:
   if(response_cjson)
   {
      cJSON_Delete(response_cjson);
      response_cjson=NULL;
   }
   return ret_code;
}


static int _netatmo_check_error_data_json(cJSON *j,  int *err_code, char *err, int l_err)
{
   cJSON *e = NULL;

   *err_code=0;
 
   e=cJSON_GetObjectItem(j, "error");
   if(e)
   {
      cJSON *a=cJSON_GetObjectItem(e, "message");
      if(a)
      {
         if(err)
            strncpy(err, a->valuestring, l_err);
      }
      else
      {
         if(err)
            strncpy(err,"unknown error", l_err);
      }
      a=cJSON_GetObjectItem(e, "code");
      if(a)
         *err_code=a->valueint;
      else
         *err_code=9999;
      return -1;
   }
   else
      return 0;

}


static int _netatmo_parse_thermostat_data_json(char *response, char *thermostat_id, struct netatmo_thermostat_data_s *thermostat_data,  char *err, int l_err)
{
   int ret_code=-1;

   cJSON *response_cjson=cJSON_Parse(response);
   if(response_cjson==NULL)
   {
       DEBUG_SECTION mea_log_printf("%s (%s) : JSON error\n", DEBUG_STR, __func__);
       goto _netatmo_parse_data_json_clean_exit;
   }

   if(_netatmo_check_error_data_json(response_cjson,  &ret_code, err, l_err)<0)
      goto _netatmo_parse_data_json_clean_exit;

   cJSON *e = NULL;

   ret_code=-2;
   e=cJSON_GetObjectItem(response_cjson, "body"); 
   if(e==NULL)
      goto _netatmo_parse_data_json_clean_exit;

   ret_code=-3;
   e=cJSON_GetObjectItem(e, "devices");
   if(e==NULL || e->type!=cJSON_Array)
       goto _netatmo_parse_data_json_clean_exit;

   ret_code=-4;
   e=cJSON_GetArrayItem(e, 0);
   if(e==NULL)
       goto _netatmo_parse_data_json_clean_exit;

   ret_code=-5;
   e=cJSON_GetObjectItem(e, "modules");
   if(e==NULL || e->type!=cJSON_Array)
       goto _netatmo_parse_data_json_clean_exit;
 
   int i=0;
   cJSON *a;
   do
   {
      ret_code=-6;
      a=cJSON_GetArrayItem(e, i); 
      if(a)
      {
         cJSON *e;
         ret_code=-7;
         e=cJSON_GetObjectItem(a, "_id");
         if(e==NULL)
            goto _netatmo_parse_data_json_clean_exit;
         if(strcmp(e->valuestring, thermostat_id)==0)
         {
            ret_code=-8;
            e=cJSON_GetObjectItem(a, "battery_vp");
            if(e==NULL)
               goto _netatmo_parse_data_json_clean_exit;
            thermostat_data->battery_vp=e->valueint;

            ret_code=-9;
            e=cJSON_GetObjectItem(a, "therm_relay_cmd");
            if(e==NULL)
               goto _netatmo_parse_data_json_clean_exit;
            thermostat_data->therm_relay_cmd=e->valueint;

            ret_code=-10;
            e=cJSON_GetObjectItem(a, "setpoint");
            if(e==NULL)
               goto _netatmo_parse_data_json_clean_exit;
            {
               ret_code=-11;
               char *s=cJSON_GetObjectItem(e, "setpoint_mode")->valuestring;
               if(s==NULL)
                  goto _netatmo_parse_data_json_clean_exit;
               if(strcmp(s,"program")==0)
                  thermostat_data->setpoint=PROGRAM;
               else if(strcmp(s, "away")==0)
                  thermostat_data->setpoint=AWAY;
               else if(strcmp(s,"hg")==0)
                  thermostat_data->setpoint=HG;
               else if(strcmp(s,"manual")==0)
                  thermostat_data->setpoint=MANUAL;
               else if(strcmp(s,"off")==0)
                  thermostat_data->setpoint=OFF;
               else if(strcmp(s,"max")==0)
                  thermostat_data->setpoint=MAX;
               else
                  thermostat_data->setpoint=-1; 
            }

            ret_code=-12;
            cJSON *mesured=cJSON_GetObjectItem(a, "measured");
            if(mesured==NULL)
               goto _netatmo_parse_data_json_clean_exit;
            else
            {
               ret_code=-13;
               e=cJSON_GetObjectItem(mesured, "temperature");
               if(e==NULL)
                  goto _netatmo_parse_data_json_clean_exit;
               thermostat_data->temperature=e->valuedouble;

               ret_code=-14;
               e=cJSON_GetObjectItem(mesured, "setpoint_temp");
               if(e==NULL)
                  goto _netatmo_parse_data_json_clean_exit;
               thermostat_data->setpoint_temp=e->valuedouble;
               ret_code=0;
               break;
            }
         }
         i++;
      }
   }
   while(a);

_netatmo_parse_data_json_clean_exit:
   if(ret_code<0)
   {
      if(err)
         strcpy(err,"parsing data error");
   }

   if(response_cjson)
   {
      cJSON_Delete(response_cjson);
      response_cjson=NULL;
   }

   return ret_code;
}



char *types_str[]={ "Temperature", "CO2", "Humidity", "Noise", "Pressure", "Rain", "WindAngle", "WindStrength", NULL };

static int _data_type_from_json_to_flags(cJSON *data_type)
{
   int flag=0;

   if(data_type->type==cJSON_Array)
   {
      int i=0;
      cJSON *type=NULL;
      while((type=cJSON_GetArrayItem(data_type, i))!=NULL)
      {
         if(strcmp(type->valuestring, types_str[TEMPERATURE])==0)
            flag=flag | TEMPERATURE_BIT;
         if(strcmp(type->valuestring, types_str[1])==0)
            flag=flag | CO2_BIT;
         if(strcmp(type->valuestring, types_str[2])==0)
            flag=flag | HUMIDITY_BIT;
         if(strcmp(type->valuestring, types_str[3])==0)
            flag=flag | NOISE_BIT;
         if(strcmp(type->valuestring, types_str[4])==0)
            flag=flag | PRESSURE_BIT;
         if(strcmp(type->valuestring, types_str[RAIN])==0)
            flag=flag | RAIN_BIT;
         if(strcmp(type->valuestring, "Wind")==0)
            flag=flag | WINDANGLE_BIT | WINDSTRENGTH_BIT;
         i++;
      }
      return flag;
   }
   else
      return -1;
}


static void print_netatmo_data(struct netatmo_data_s *data)
{
   int flag=1;
   int i=TEMPERATURE;
   for(;i<NETATMO_MAX_DATA;i++)
   {
      if(data->dataTypeFlags & flag)
      {
         fprintf(stderr,"%s = %f\n", types_str[i], data->data[i]);
      }
      flag=flag << 1;
   }
}


static int _netatmo_get_data_from_dashboard_json(cJSON *dashboard, int dataTypeFlags, struct netatmo_data_s *data)
{
   if(dashboard==NULL || dashboard->type!=cJSON_Object)
      return -1;
   data->dataTypeFlags=dataTypeFlags;
   int retour=-1;
   int flag=1;
   int i=TEMPERATURE;
   for(;i<NETATMO_MAX_DATA;i++)
   {
      if(dataTypeFlags & flag)
      {
         cJSON *mesure=cJSON_GetObjectItem(dashboard, types_str[i]);
         if(mesure)
         {
            data->data[i]=mesure->valuedouble;
            retour=0;
         }
      }
      flag=flag << 1;
   }

   return retour;
}


static int _netatmo_parse_station_data_json(char *response, char *station_id, struct netatmo_station_data_s *station_data,  char *err, int l_err)
{
   int ret_code=-1;

   cJSON *response_cjson=cJSON_Parse(response);

   if(response_cjson==NULL)
   {
      DEBUG_SECTION mea_log_printf("%s (%s) : JSON error\n", DEBUG_STR, __func__);
      goto _netatmo_parse_station_data_json_clean_exit;
   }

   ret_code=-2;
   if(_netatmo_check_error_data_json(response_cjson,  &ret_code, err, l_err)<0)
      goto _netatmo_parse_station_data_json_clean_exit;

   cJSON *e=cJSON_GetObjectItem(response_cjson, "body"); 
   if(e!=NULL)
   {
      e=cJSON_GetObjectItem(e, "devices");
      cJSON *device=e->child;
      e = NULL;
      while(device)
      {
         cJSON *_id=cJSON_GetObjectItem(device, "_id");
         if(_id && strcmp(_id->valuestring, station_id)==0)
         {
            cJSON *station_name=cJSON_GetObjectItem(device, "station_name");
            if(station_name)
            {
               strcpy(station_data->id, _id->valuestring);
               strcpy(station_data->name, station_name->valuestring);
//               fprintf(stderr,"id:   %s\n", _id->valuestring);
//               fprintf(stderr,"name: %s\n", station_name->valuestring);
               e = device;
               break;
            }
         }
         device=device->next;
      }
   }

   if(e==NULL)
      goto _netatmo_parse_station_data_json_clean_exit;

   cJSON *data_type=cJSON_GetObjectItem(e, "data_type");
   cJSON *dashboard_data=cJSON_GetObjectItem(e, "dashboard_data");
   if(data_type && dashboard_data)
   {
      int data_type_flags=_data_type_from_json_to_flags(data_type);
      if(data_type_flags!=-1)
      {
         if(!dashboard_data || _netatmo_get_data_from_dashboard_json(dashboard_data, data_type_flags, &(station_data->data))!=0)
            goto _netatmo_parse_station_data_json_clean_exit;
      }
   }
   else
      goto _netatmo_parse_station_data_json_clean_exit;

   cJSON *modules=cJSON_GetObjectItem(e, "modules");
   cJSON *m=modules->child;
   int num_module=0;
   while(m)
   {
      cJSON *_id=cJSON_GetObjectItem(m, "_id");
      cJSON *module_name=cJSON_GetObjectItem(m, "module_name");
      cJSON *type=cJSON_GetObjectItem(m, "type");

//      fprintf(stderr,"id:   %s\n", _id->valuestring);
//      fprintf(stderr,"name: %s\n", module_name->valuestring);
//      fprintf(stderr,"type: %s\n", type->valuestring);

      cJSON *data_type=cJSON_GetObjectItem(m, "data_type");
      cJSON *battery_vp = cJSON_GetObjectItem(m, "battery_vp");
      cJSON *dashboard_data = cJSON_GetObjectItem(m, "dashboard_data");

      if(_id && data_type && module_name && dashboard_data && battery_vp)
      {
         strncpy(station_data->modules_data[num_module].id, _id->valuestring, sizeof(station_data->modules_data[num_module].id));
         strncpy(station_data->modules_data[num_module].name, module_name->valuestring, sizeof(station_data->modules_data[num_module].name));
         strncpy(station_data->modules_data[num_module].type, type->valuestring, sizeof(station_data->modules_data[num_module].type));
         station_data->modules_data[num_module].battery=battery_vp->valueint;
         int data_type_flags=_data_type_from_json_to_flags(data_type);
         if(data_type_flags!=-1)
         {
            if(!dashboard_data || _netatmo_get_data_from_dashboard_json(dashboard_data, data_type_flags, &(station_data->modules_data[num_module].data))!=0)
               goto _netatmo_parse_station_data_json_clean_exit;
         }
      }
      m=m->next;
      num_module++;
   }
   station_data->nb_modules=num_module;

   // donnees de la station

_netatmo_parse_station_data_json_clean_exit:
   if(ret_code<0)
   {
      if(err)
         strcpy(err,"parsing data error");
   }

   if(response_cjson)
   {
      cJSON_Delete(response_cjson);
      response_cjson=NULL;
   }

   return ret_code;
}


static int _netatmo_parse_token_json(char *response, struct netatmo_token_s *token, char *err, int l_err)
{
   int ret_code=-1;
   cJSON *response_cjson=cJSON_Parse(response);

   if(response_cjson==NULL)
   {
       DEBUG_SECTION mea_log_printf("%s (%s) : JSON error\n", DEBUG_STR, __func__);
       goto netatmo_parse_token_json_clean_exit;
   }

   cJSON *e = NULL;

    e=cJSON_GetObjectItem(response_cjson, "error");
    if(e)
    {
       if(e->valuestring)
       {
          VERBOSE(2) mea_log_printf("%s (%s) : NETATMO error : %s\n", DEBUG_STR, __func__, e->valuestring);
       }
       else
       {
          VERBOSE(2) mea_log_printf("%s (%s) : NETATMO error : non message\n", DEBUG_STR, __func__);
       }
       if(err)
       {
          if(e->valuestring)
          {
             strncpy(err, e->valuestring, l_err);
          }
          else
          {
             strncpy(err, "non message", l_err);
          }
       }
       ret_code=1;
       goto netatmo_parse_token_json_clean_exit;
    }

    e=cJSON_GetObjectItem(response_cjson,"access_token");
   if(e && e->type==cJSON_String)
      strcpy(token->access, e->valuestring);
   else goto netatmo_parse_token_json_clean_exit;

   e=cJSON_GetObjectItem(response_cjson,"refresh_token");
   if(e && e->type==cJSON_String)
      strcpy(token->refresh, e->valuestring);
   else goto netatmo_parse_token_json_clean_exit;

   e=cJSON_GetObjectItem(response_cjson,"expire_in");
   if(e && e->type==cJSON_Number)
      token->expire_in=e->valueint;
   else goto netatmo_parse_token_json_clean_exit;

   ret_code=0;

netatmo_parse_token_json_clean_exit:
   if(ret_code==-1)
   {
      token->access[0]=0;
      token->refresh[0]=0;
      token->expire_in=0;
   }

   if(response_cjson)
   {
      cJSON_Delete(response_cjson);
      response_cjson=NULL;
   }

   return ret_code;
}


int netatmo_refresh_token(char *client_id, char *client_secret, struct netatmo_token_s *netatmo_token, char *err, int l_err)
{
   CURL *curl;
   char *url="https://api.netatmo.net/oauth2/token";
   char post_data[1024];

   if(netatmo_token->access[0]==0 || netatmo_token->refresh[0]==0)
   {
      DEBUG_SECTION mea_log_printf("%s (%s) : token not initialized (%d)\n", DEBUG_STR, __func__);
      return -1;
   }

   sprintf(post_data, "grant_type=refresh_token&client_id=%s&client_secret=%s&refresh_token=%s", client_id, client_secret, netatmo_token->refresh);

   struct curl_result_s cr;
   int ret=-1;

   ret=curl_result_init(&cr);
   if(ret<0)
      return -1;

   curl = curl_easy_init();
   if(curl)
   {
      curl_easy_setopt(curl, CURLOPT_URL, url);

//      DEBUG_SECTION curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
      curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
      curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
      curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, (curl_write_callback)curl_result_get);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &cr);

      CURLcode res = curl_easy_perform(curl);
      if(res != CURLE_OK)
      {
         DEBUG_SECTION mea_log_printf("%s (%s) : curl_easy_perform() failed (%d) - %s\n", DEBUG_STR, __func__, time(NULL), curl_easy_strerror(res));
         ret=-1;
      }
      else
         ret=_netatmo_parse_token_json(cr.p, netatmo_token, err, l_err);

      curl_easy_cleanup(curl);
   }
   else
      ret=-1;

   curl_result_release(&cr);

   return ret;
}


int netatmo_get_token(char *client_id, char *client_secret, char *username, char *password, char *scope, struct netatmo_token_s *netatmo_token, char *err, int l_err)
{
   CURL *curl;
   char *url="https://api.netatmo.net/oauth2/token";
   char post_data[1024];

   sprintf(post_data, "grant_type=password&client_id=%s&client_secret=%s&username=%s&password=%s&scope=%s", client_id,client_secret,username,password,scope);
     
   struct curl_result_s cr;
   int ret=-1;

   ret=curl_result_init(&cr);
   if(ret<0)
      return -1;

   curl = curl_easy_init();
   if(curl)
   {
      curl_easy_setopt(curl, CURLOPT_URL, url);

//      DEBUG_SECTION curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
      curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
      curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
      curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, (curl_write_callback)curl_result_get);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &cr);

      CURLcode res = curl_easy_perform(curl);
      if(res != CURLE_OK)
      {
         DEBUG_SECTION mea_log_printf("%s (%s) : curl_easy_perform() failed (%d) - %s\n", DEBUG_STR, __func__, time(NULL), curl_easy_strerror(res));
         ret=-1;
      }
      else
         ret=_netatmo_parse_token_json(cr.p, netatmo_token, err, l_err); 

      curl_easy_cleanup(curl);
   }
   else
      ret=-1;

   curl_result_release(&cr);

   return ret;
}


int netatmo_set_thermostat_setpoint(char *access_token, char *relay_id, char *thermostat_id, enum netatmo_setpoint_mode_e mode, uint32_t delay, double temp, char *err, int l_err)
{
   CURL *curl;
   char *api="https://api.netatmo.net/api/setthermpoint";
   char *_mode=NULL;
   char post_data[1024];
   time_t endtime=0;
   double _temp=0.0;

   if(delay!=0)
      endtime=time(NULL)+delay;

   switch(mode)
   {
      case PROGRAM:
         _mode="program";
         break;
      case AWAY:
         _mode="away";
         break;
      case HG:
         _mode="hg";
         break;
      case MANUAL:
         _mode="manual";
         if(temp<7.0)
         {
            if(err)
               snprintf(err,l_err,"temp (%3.0f °C) too low !\n", temp);
            return 1;
         }
         _temp=temp;
         break;
      case OFF:
         _mode="off";
         break;
      case MAX:
         _mode="max";
         if(delay==0)
            delay=3600;
         endtime=time(NULL)+delay;
         break;
      default:
         return -1;
   }

   sprintf(post_data, "access_token=%s&device_id=%s&module_id=%s&setpoint_mode=%s", access_token, relay_id, thermostat_id, _mode);
   if(_temp!=0.0)
   {
      char spt[5];
      sprintf(spt, "%04.1f", _temp);
      mea_snprintfcat(post_data, sizeof(post_data), "&setpoint_temp=%s", spt); 
   }
   if(endtime!=0)
      mea_snprintfcat(post_data, sizeof(post_data), "&setpoint_endtime=%d", endtime);

     struct curl_result_s cr;
   int ret=-1;

   ret=curl_result_init(&cr);
   if(ret<0)
      return -1;

   curl = curl_easy_init();
   if(curl)
   {
      curl_easy_setopt(curl, CURLOPT_URL, api);

//      DEBUG_SECTION curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
      curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
      curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
      curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, (curl_write_callback)curl_result_get);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &cr);

      CURLcode res = curl_easy_perform(curl);
      if(res != CURLE_OK)
      {
         DEBUG_SECTION mea_log_printf("%s (%s) : curl_easy_perform() failed (%d) - %s\n", DEBUG_STR, __func__, time(NULL), curl_easy_strerror(res));
         ret=-1;
      }
      else
      {
         ret=_netatmo_parse_return_json(cr.p, err, l_err);
      }

      curl_easy_cleanup(curl);
   }
   else
      ret=-1;

   curl_result_release(&cr);

   return ret;
}


int netatmo_get_thermostat_data(char *access_token, char *relay_id, char *thermostat_id, struct netatmo_thermostat_data_s *thermostat_data, char *err, int l_err)
{
   CURL *curl;
   char *api="https://api.netatmo.net/api/getthermostatsdata";
   //char *api="https://api.netatmo.net/api/homesdata";

   char url[1024];

   sprintf(url, "%s?access_token=%s&device_id=%s", api, access_token, relay_id);
     
   struct curl_result_s cr;
   int ret=-1;

   ret=curl_result_init(&cr);
   if(ret<0)
   {
      if(err)
         strncpy(err, "curl_result_init error", l_err);
      return -1;
   }

   curl = curl_easy_init();
   if(curl)
   {
      curl_easy_setopt(curl, CURLOPT_URL, url);

 //     DEBUG_SECTION curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
      curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
      curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
      curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, (curl_write_callback)curl_result_get);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &cr);

      CURLcode res = curl_easy_perform(curl);
      if(res != CURLE_OK)
      {
         DEBUG_SECTION mea_log_printf("%s (%s) : curl_easy_perform() failed (%d) - %s\n", DEBUG_STR, __func__, time(NULL), curl_easy_strerror(res));
         if(err)
            strncpy(err, curl_easy_strerror(res), l_err);
         ret=-1;
      }
      else
      {
         printf("ICI1: %s\n",cr.p);
         ret=_netatmo_parse_thermostat_data_json(cr.p, thermostat_id, thermostat_data, err, l_err);
         if(ret<0)
         {
            DEBUG_SECTION mea_log_printf("%s\n", cr.p);
         }
      }

      curl_easy_cleanup(curl);
   }
   else
   {
      if(err)
         strncpy(err, "curl_easy_init error", l_err);
      ret=-1;
   }

   curl_result_release(&cr);
   return ret;
}


int netatmo_get_station_data(char *access_token, char *station_id, struct netatmo_station_data_s *station_data, char *err, int l_err)
{
   CURL *curl;
   char *api="https://api.netatmo.net/api/getstationsdata";

   char url[1024];

   sprintf(url, "%s?access_token=%s&device_id=%s", api, access_token, station_id);
//   sprintf(url, "%s?access_token=%s", api, access_token);
     
   struct curl_result_s cr;
   int ret=-1;

   ret=curl_result_init(&cr);
   if(ret<0)
   {
      if(err)
         strncpy(err, "curl_result_init error", l_err);
      return -1;
   }

   curl = curl_easy_init();
   if(curl)
   {
      curl_easy_setopt(curl, CURLOPT_URL, url);

      DEBUG_SECTION curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
      curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
      curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
      curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, (curl_write_callback)curl_result_get);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &cr);

      CURLcode res = curl_easy_perform(curl);
      if(res != CURLE_OK)
      {
         DEBUG_SECTION mea_log_printf("%s (%s) : curl_easy_perform() failed (%d) - %s\n", DEBUG_STR, __func__, time(NULL), curl_easy_strerror(res));
         if(err)
            strncpy(err, curl_easy_strerror(res), l_err);
         ret=-1;
      }
      else
      {
         ret=_netatmo_parse_station_data_json(cr.p, station_id, station_data, err, l_err);
         if(ret<0)
         {
            DEBUG_SECTION mea_log_printf("%s\n", cr.p);
         }
      }

      curl_easy_cleanup(curl);
   }
   else
   {
      if(err)
         strncpy(err, "curl_easy_init error", l_err);
      ret=-1;
   }

   curl_result_release(&cr);
   return ret;
}


#ifdef NETATMO_MODULE_TEST
char *_data=" \
{ \
   \"body\":\
   {\
      \"devices\": [ \
         { \
            \"_id\": \"70:ee:50:12:5b:c2\", \
            \"alarm_config\": \
            { \
               \"default_alarm\":[{\"db_alarm_number\":0},{\"db_alarm_number\":1},{\"db_alarm_number\":2},{\"db_alarm_number\":6},{\"db_alarm_number\":4},{\"db_alarm_number\":5},{\"db_alarm_number\":7}], \
               \"personnalized\":[{\"threshold\":1,\"data_type\":0,\"direction\":1,\"db_alarm_number\":19}] \
            }, \
            \"cipher_id\": \"enc:16:bZV9YUcYITcw2z3GIdM8K0OVEZ2dqVgMBNcbd1jpWTeFClCSotzCr+45x68b0MF\", \
            \"co2_calibrating\": false, \
            \"firmware\":111, \
            \"last_status_store\": 1448978187, \
            \"last_upgrade\": 1440372667, \
            \"module_name\": \"Station\", \
            \"modules\": [{ \
               \"_id\":\"02:00:00:12:79:9a\", \
               \"module_name\":\"Ext\u00e9rieur\", \
               \"type\":\"NAModule1\", \
               \"firmware\":43, \
               \"last_message\":1448978181, \
               \"last_seen\":1448978136, \
               \"rf_status\":89, \
               \"battery_vp\":6242, \
               \"dashboard_data\": \
               { \
                  \"time_utc\":1448978136, \
                  \"Temperature\":12.9, \
                  \"temp_trend\":\"stable\", \
                  \"Humidity\":73, \
                  \"date_max_temp\":1448976958, \
                  \"date_min_temp\":1448924920, \
                  \"min_temp\":11.4, \
                  \"max_temp\":12.9 \
               }, \
               \"data_type\": [\"Temperature\",\"Humidity\"] \
            }], \
            \"place\":{\"altitude\":110,\"city\":\"Fontenay-sous-Bois\",\"country\":\"FR\",\"improveLocProposed\":true,\"location\":[2.4953483110451,48.8745462],\"timezone\":\"EuropeParis\"}, \
            \"station_name\":\"Maison\", \
            \"type\":\"NAMain\", \
            \"wifi_status\":41, \
            \"dashboard_data\": \
            { \
               \"AbsolutePressure\":1014.9, \
               \"time_utc\":1448978172, \
               \"Noise\":47, \
               \"Temperature\":21.5, \
               \"temp_trend\":\"stable\", \
               \"Humidity\":58, \
               \"Pressure\":1028.2, \
               \"pressure_trend\":\"up\", \
               \"CO2\":0, \
               \"date_max_temp\":1448975149, \
               \"date_min_temp\":1448938249, \
               \"min_temp\":19.3, \
               \"max_temp\":21.6 \
            }, \
            \"data_type\": [\"Temperature\",\"CO2\",\"Humidity\",\"Noise\",\"Pressure\"] \
         } \
      ], \
      \"user\": {\"mail\":\"patrice.dietsch@gmail.com\",\"administrative\":{\"reg_locale\":\"fr-FR\",\"lang\":\"fr-FR\",\"unit\":0,\"windunit\":0,\"pressureunit\":0,\"feel_like_algo\":0}} \
   }, \
   \"status\": \"ok\", \
   \"time_exec\":2.2652621269226, \
   \"time_server\":1448978661 \
}";

int main(int argc, char *argv[])
{
   struct netatmo_token_s token;
   struct netatmo_thermostat_data_s data;
   char err[80];
   int ret;

   ret=netatmo_get_token("563e5ce3cce37c07407522f2","lE1CUF1k3TxxSceiPpmIGY8QXJWIeXJv0tjbTRproMy4","patrice.dietsch@gmail.com","WEBcdpii10", "read_thermostat write_thermostat read_station", &token, err, sizeof(err)-1);
   if(ret!=0)
   {
      if(ret<0)
         printf("Authentification error : %d\n", ret);
      else
         printf("Authentification error : %s (%d)\n", err, ret);
      exit(1);
   }


   ret=netatmo_refresh_token("563e5ce3cce37c07407522f2","lE1CUF1k3TxxSceiPpmIGY8QXJWIeXJv0tjbTRproMy4", &token, err, sizeof(err)-1);
   if(ret!=0)
   {
      if(ret<0)
         printf("Refresh error : %d\n", ret);
      else
         printf("Refresh error : %s (%d)\n", err, ret);
      exit(1);
   }


   ret=netatmo_get_thermostat_data(token.access, "70:ee:50:0a:34:e0", "04:00:00:0a:37:8c", &data, err, sizeof(err)-1);
   if(ret==0)
      printf("temperature = %f %d\n", data.temperature,data.setpoint);
   else if(ret>0)
      printf("error : %s (%d)\n", err, ret); 

//   struct netatmo_station_data_s station_data;
//   netatmo_get_station_data(token.access, "70:ee:50:12:5b:c2", &station_data, err,  sizeof(err)-1);
//   netatmo_set_thermostat_setpoint(token.access, "70:ee:50:0a:34:e0", "04:00:00:0a:37:8c", MANUAL, 3600, 19.5, err, sizeof(err)-1);
//   netatmo_set_thermostat_setpoint(token.access, "70:ee:50:0a:34:e0", "04:00:00:0a:37:8c", MAX, 3600, -1.0, err, sizeof(err)-1);
//   netatmo_set_thermostat_setpoint(token.access, "70:ee:50:0a:34:e0", "04:00:00:0a:37:8c", AWAY, 3600, -1.0, err, sizeof(err)-1);

   struct netatmo_station_data_s station;

   ret=netatmo_get_station_data(token.access, "70:ee:50:12:5b:c2", &station, err,  sizeof(err)-1);
   if(ret==0)
   {
      fprintf(stderr,"%s %s\n", station.id, station.name);
      print_netatmo_data(&(station.data));
      int i=0;
      for(;i<station.nb_modules;i++)
      {
         fprintf(stderr,"%s %s %s %d\n", station.modules_data[i].id, station.modules_data[i].name, station.modules_data[i].type, station.modules_data[i].battery);
         print_netatmo_data(&(station.modules_data[i].data));
      }
   }
}
#endif
