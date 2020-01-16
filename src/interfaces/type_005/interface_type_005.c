#ifdef __APPLE__
#include <Python/Python.h>
#else
#include <Python.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>

#include "interface_type_005.h"
#include "interfacesServer.h"

#include "mea_verbose.h"
#include "mea_timer.h"
#include "mea_string_utils.h"
#include "mea_queue.h"

#include "cJSON.h"
#include "uthash.h"

#include "globals.h"
#include "consts.h"

#include "tokens.h"
#include "tokens_da.h"

#include "macros.h"
#include "parameters_utils.h"
#include "processManager.h"


#include "netatmo.h"


enum device_type_e { THERMOSTAT, STATION };

struct type005_device_queue_elem_s
{
   enum device_type_e device_type;
   char device_id[256];

   mea_queue_t modules_list;
};

union module_data_u
{
   struct netatmo_thermostat_data_s thermostat;
   struct netatmo_data_s station;
};

struct type005_module_queue_elem_s
{
   char module_id[256];
   
   union module_data_u d1, d2;
   union module_data_u *current, *last;

   mea_queue_t sensors_actuators_list;
   struct type005_device_queue_elem_s *device; // chainage vers pere
};

struct type005_sensor_actuator_queue_elem_s
{
   int id;
   int type;
   char name[256];
   int todbflag;
   int sensor;
   int actuator;
   
   struct type005_module_queue_elem_s *module; // chainage vers pere
};


char *valid_netatmo_sa_params[]={"S:DEVICE_ID","S:MODULE_ID", "S:SENSOR", "S:ACTUATOR", "S:DEVICE_TYPE", NULL};
#define PARAMS_DEVICE_ID   0
#define PARAMS_MODULE_ID   1
#define PARAMS_SENSOR      2
#define PARAMS_ACTUATOR    3
#define PARAMS_DEVICE_TYPE 4

enum netatmo_sensor_e { _TEMP, _RELAY_CMD, _BATTERY, _SETPOINT, _MODE, _TEMPERATURE, _CO2, _HUMIDITY, _NOISE, _PRESSURE, _RAIN, _WINDANGLE, _WINDSTRENGTH };

// enum netatmo_sensor_e { TEMPERATURE, RELAY_CMD, BATTERY, SETPOINT, MODE };

char *interface_type_005_xplin_str="XPLIN";
char *interface_type_005_xplout_str="XPLOUT";
char *interface_type_005_nbreaderror_str="NBREADERROR";
char *interface_type_005_nbread_str="NBREAD";


int start_interface_type_005(int my_id, void *data, char *errmsg, int l_errmsg);
int stop_interface_type_005(int my_id, void *data, char *errmsg, int l_errmsg);
int restart_interface_type_005(int my_id, void *data, char *errmsg, int l_errmsg);
int16_t check_status_interface_type_005(interface_type_005_t *i005);


static int16_t _interface_type_005_send_xPL_sensor_basic2(interface_type_005_t *i005, char *xplmsgtype, char *name, char *type, char *str_value, char *str_last)
{
   char str[36];
   cJSON *xplMsgJson = cJSON_CreateObject();
   cJSON_AddItemToObject(xplMsgJson, XPLMSGTYPE_STR_C, cJSON_CreateString(xplmsgtype));
   sprintf(str,"%s.%s", get_token_string_by_id(XPL_SENSOR_ID), get_token_string_by_id(XPL_BASIC_ID));
   cJSON_AddItemToObject(xplMsgJson, XPLSCHEMA_STR_C, cJSON_CreateString(str));
   cJSON_AddItemToObject(xplMsgJson, get_token_string_by_id(XPL_DEVICE_ID), cJSON_CreateString(name));
   cJSON_AddItemToObject(xplMsgJson, get_token_string_by_id(XPL_TYPE_ID), cJSON_CreateString(type));
   cJSON_AddItemToObject(xplMsgJson, get_token_string_by_id(XPL_CURRENT_ID), cJSON_CreateString(str_value));
   if(str_last && str_last[0]!=0)
      cJSON_AddItemToObject(xplMsgJson, get_token_string_by_id(XPL_LAST_ID), cJSON_CreateString(str_last));

   mea_sendXPLMessage2(xplMsgJson);

   (i005->indicators.xplout)++;

   cJSON_Delete(xplMsgJson);
   
   return 0;
}


static struct type005_sensor_actuator_queue_elem_s *_find_sensor_actuator(mea_queue_t *q, char *device)
{
   struct type005_device_queue_elem_s *d;
   struct type005_module_queue_elem_s *m;
   struct type005_sensor_actuator_queue_elem_s *sa;

   mea_queue_first(q);
   while(mea_queue_current(q,(void **)&d)==0) {
      mea_queue_first(&(d->modules_list));
      while(mea_queue_current(&(d->modules_list),(void **)&m)==0) {
         mea_queue_first(&(m->sensors_actuators_list));
         while(mea_queue_current(&(m->sensors_actuators_list),(void **)&sa)==0) {
            if(mea_strcmplower(sa->name, device)==0)
               return sa;
 
            mea_queue_next(&(m->sensors_actuators_list));
         }
         mea_queue_next(&(d->modules_list));
      }
      mea_queue_next(q);
   }

   return NULL;
}


int16_t format_float_current_last(char *str_current, char *str_last, char *format, double current, double last, double *d1)
{
   sprintf(str_current, format, current);
   if(last>=0.0)
      sprintf(str_last, format, last);
   if(d1) *d1=(double)current;
   
   return 0;
}


int16_t interface_type_005_current_last(struct type005_sensor_actuator_queue_elem_s *e,
                                        char *type,
                                        char *str_current,
                                        char *str_last,
                                        double *d1,
                                        double *d2,
                                        char *comp)
{
   if(!type || !str_current || !str_last)
      return -1;

   if(d1) *d1=0.0;
   if(d2) *d2=0.0;
   if(comp) comp[0]=0;

   str_current[0]=0;
   str_last[0]=0;

   switch(e->sensor)
   {
      case _TEMP:
         sprintf(str_current, "%4.1f", e->module->current->thermostat.temperature);
         if(e->module->last->thermostat.temperature>0.0)
            sprintf(str_last,    "%4.1f", e->module->last->thermostat.temperature);
         strcpy(type, "temp");
         if(d1) *d1=(double)e->module->current->thermostat.temperature;
         break;

      case _RELAY_CMD:
         sprintf(str_current, "%d", e->module->current->thermostat.therm_relay_cmd);
         if(e->module->last->thermostat.therm_relay_cmd>=0)
            sprintf(str_last,    "%d", e->module->last->thermostat.therm_relay_cmd);
         strcpy(type, "output");
         if(d1) *d1=(double)e->module->current->thermostat.therm_relay_cmd;
         break;

      case _BATTERY:
      {
         float v2;
         float v1=(e->module->current->thermostat.battery_vp-3000.0)/1500.0*100.0;

         if(v1>100.0) v1=100.0;
         if(v1<0.0) v1=0.0;
         sprintf(str_current, "%3.0f", v1);
         if(e->module->last->thermostat.battery_vp>0) {
            v2=(e->module->last->thermostat.battery_vp-3000.0)/1500.0*100.0;
            if(v2>100.0) v2=100.0;
            if(v2<0.0) v2=0.0;
            sprintf(str_last, "%3.0f", v2);
         }
         strcpy(type, "battery");
         if(d1) *d1=(double)e->module->current->thermostat.battery_vp;
         if(d2) *d2=(double)v1;
         if(e->module->current->thermostat.battery_vp != e->module->last->thermostat.battery_vp)
            return 1;
         break;
      }

      case _MODE:
         sprintf(str_current, "%d", e->module->current->thermostat.setpoint);
         if(e->module->last->thermostat.setpoint >= 0)
            sprintf(str_last, "%d", e->module->last->thermostat.setpoint);
         strcpy(type,"generic");
         if(d1) *d1=(double)e->module->current->thermostat.setpoint;
         if(comp) strcpy(comp, netatmo_thermostat_mode[e->module->current->thermostat.setpoint]);
         break;

     case _SETPOINT:
         sprintf(str_current, "%4.1f", e->module->current->thermostat.setpoint_temp);
         if(e->module->last->thermostat.setpoint_temp > 0.0)
            sprintf(str_last, "%4.1f", e->module->last->thermostat.setpoint_temp);
         strcpy(type,"setpoint");
         if(d1) *d1=(double)e->module->current->thermostat.setpoint_temp;
         break;

      case _TEMPERATURE:
         if(e->module->current->station.dataTypeFlags & TEMPERATURE_BIT) {
            sprintf(str_current, "%4.1f", e->module->current->station.data[TEMPERATURE]);
            if(e->module->last->station.data[TEMPERATURE]>0.0)
               sprintf(str_last, "%4.1f", e->module->last->station.data[TEMPERATURE]);
            strcpy(type, "temp");
            if(d1) *d1=(double)e->module->current->station.data[TEMPERATURE];
         }
         else
            return -1;
         break;

      case _CO2:
         if(e->module->current->station.dataTypeFlags & CO2_BIT) {
            sprintf(str_current, "%4.0f", e->module->current->station.data[CO2]);
            if(e->module->last->station.data[CO2]>0.0)
               sprintf(str_last, "%4.0f", e->module->last->station.data[CO2]);
            strcpy(type, "co2");
            if(d1) *d1=(double)e->module->current->station.data[CO2];
         }
         else
            return -1;
         break;

      case _HUMIDITY:
         if(e->module->current->station.dataTypeFlags & HUMIDITY_BIT) {
            sprintf(str_current, "%4.2f", e->module->current->station.data[HUMIDITY]);
            if(e->module->last->station.data[HUMIDITY]>0.0)
               sprintf(str_last, "%4.2f", e->module->last->station.data[HUMIDITY]);
            strcpy(type, "humidity");
            if(d1) *d1=(double)e->module->current->station.data[HUMIDITY];
         }
         else
            return -1;
         break;

      case _NOISE:
         if(e->module->current->station.dataTypeFlags & NOISE_BIT) {
            sprintf(str_current, "%4.0f", e->module->current->station.data[NOISE]);
            if(e->module->last->station.data[NOISE]>0.0)
               sprintf(str_last, "%4.0f", e->module->last->station.data[NOISE]);
            strcpy(type, "noise");
            if(d1) *d1=(double)e->module->current->station.data[NOISE];
         }
         else
            return -1;
         break;

      case _PRESSURE:
         if(e->module->current->station.dataTypeFlags & PRESSURE_BIT) {
            sprintf(str_current, "%6.2f", e->module->current->station.data[PRESSURE]);
            if(e->module->last->station.data[PRESSURE]>0.0)
               sprintf(str_last, "%6.2f", e->module->last->station.data[PRESSURE]);
            strcpy(type, "noise");
            if(d1) *d1=(double)e->module->current->station.data[PRESSURE];
         }
         else
            return -1;
         break;

      case _RAIN:
         if(e->module->current->station.dataTypeFlags & RAIN_BIT) {
            format_float_current_last(str_current, str_last, "%4.0f", e->module->current->station.data[RAIN], e->module->last->station.data[RAIN], d1);
            strcpy(type, "rain");
         }
         else
            return -1;

      case _WINDANGLE:
         if(e->module->current->station.dataTypeFlags & WINDANGLE_BIT) {
            format_float_current_last(str_current, str_last, "%4.0f", e->module->current->station.data[WINDANGLE], e->module->last->station.data[WINDANGLE], d1);
            strcpy(type, "angle");
         }
         else
            return -1;

      case _WINDSTRENGTH:
         if(e->module->current->station.dataTypeFlags & WINDSTRENGTH_BIT) {
            format_float_current_last(str_current, str_last, "%4.0f", e->module->current->station.data[WINDSTRENGTH], e->module->last->station.data[WINDSTRENGTH], d1);
            strcpy(type, "speed");
         }
         else
            return -1;
     default:
         return -1;
   }

   if(strcmp(str_current, str_last)==0)
      return 0;
   else
      return 1;
}


int16_t interface_type_005_xPL_actuator2(interface_type_005_t *i005,
                                         cJSON *xplMsgJson,
                                         char *device,
                                         struct type005_sensor_actuator_queue_elem_s *e)
{
// xPLSend -m cmnd -c control.basic device=therm01 mode=manual setpoint=25.5 expire=3600
// xPLSend -m cmnd -c control.basic device=therm01 mode=away expire=3600
// xPLSend -m cmnd -c control.basic device=therm01 mode=away
// xPLSend -m cmnd -c control.basic device=therm01 mode=program
   int16_t ret=-1;
   char err[256];
   if(e->type==501)
   {
      char *mode     = NULL;
      char *setpoint = NULL;
      char *expire   = NULL;
      cJSON *j;

      j=cJSON_GetObjectItem(xplMsgJson, "mode");
      if(j)
         mode=j->valuestring;

      j=cJSON_GetObjectItem(xplMsgJson, "setpoint");
      if(j)
         setpoint=j->valuestring;

      j=cJSON_GetObjectItem(xplMsgJson, "expire");
      if(j)
         expire=j->valuestring;

      char *pEnd;
  
      long int _mode=-1L;
      long int _expire=0L;
      double _setpoint=0.0;
      
      if(mode) {
         if(expire) {
            _expire=strtol(expire, &pEnd, 10);
            if(*pEnd>0)
               return -1;
         }
      
         if(setpoint) {
            _setpoint=strtod(setpoint, &pEnd);
            if(*pEnd>0)
               return -1;
         }

         for(int i=0;netatmo_thermostat_mode[i];i++) {
            if(strcmp(netatmo_thermostat_mode[i],mode)==0) {
               _mode=i;
               break;
            }
         }
         
         switch(_mode)
         {
            case OFF:
            case HG:
            case MAX:
            case PROGRAM:
               _setpoint=0.0;
            case MANUAL:
            case AWAY:
               netatmo_set_thermostat_setpoint(i005->token.access, e->module->device->device_id, e->module->module_id, (enum netatmo_setpoint_mode_e)_mode, (uint32_t)_expire, _setpoint, err, sizeof(err)-1);
               break;
               
            default:
               return -1;
         }
      }
      else
         ret=-1;
   }
   return ret;
}


int16_t interface_type_005_xPL_sensor2(interface_type_005_t *i005,
                                       cJSON *xplMsgJson,
                                       char *device,
                                       struct type005_sensor_actuator_queue_elem_s *e)
{
// xPLSend -m cmnd -c sensor.request device=temp02 request=current
   if(e->type==500)
   {
      char ntype[256], str_value[256], str_last[256];

      if(interface_type_005_current_last(e, ntype, str_value, str_last, NULL, NULL, NULL)<0)
         return -1;
      else {
         _interface_type_005_send_xPL_sensor_basic2(i005, XPL_STAT_STR_C, e->name, ntype, str_value, str_last);
         return 0;
      }
   }
   else
      return -1;
}


int16_t interface_type_005_xPL_callback2(cJSON *xplMsgJson, struct device_info_s *device_info, void *userValue)
{
   char *schema = NULL, *device = NULL; //, *type = NULL;
   cJSON *j;

   interface_type_005_t *i005=(interface_type_005_t *)userValue;

   j=cJSON_GetObjectItem(xplMsgJson, get_token_string_by_id(XPL_DEVICE_ID));
   if(j)
      device=j->valuestring;
   if(!device) {
      VERBOSE(5) mea_log_printf("%s (%s) : xPL message no device\n", INFO_STR, __func__);
      return -1;
   }
   mea_strtolower(device);

/*
   j=cJSON_GetObjectItem(xplMsgJson, "type");
   if(j)
      type=j->valuestring;
   if(!type)
   {
      VERBOSE(5) mea_log_printf("%s (%s) : xPL message no type\n", INFO_STR, __func__);
      return -1;
   }
*/

   j=cJSON_GetObjectItem(xplMsgJson, XPLSCHEMA_STR_C);
   if(j)
      schema=j->valuestring;
   if(!schema) {
      VERBOSE(5) mea_log_printf("%s (%s) : xPL message no schema\n", INFO_STR, __func__);
      return -1;
   }
   mea_strtolower(schema);

   struct type005_sensor_actuator_queue_elem_s *elem=_find_sensor_actuator(&(i005->devices_list), device);
   if(elem==NULL) {
      DEBUG_SECTION mea_log_printf("%s (%s) : %s not for me\n", INFO_STR, __func__, device);
      return -1;
   } 
   (i005->indicators.xplin)++;
   
   VERBOSE(9) mea_log_printf("%s (%s) : xPL Message to process : %s\n", INFO_STR, __func__, schema);
   
   if(mea_strcmplower(schema, XPL_CONTROLBASIC_STR_C) == 0) {
      return interface_type_005_xPL_actuator2(i005, xplMsgJson, device, elem);
   }
   else if(mea_strcmplower(schema, XPL_SENSORREQUEST_STR_C) == 0) {
      char *request = NULL;
      j = cJSON_GetObjectItem(xplMsgJson, get_token_string_by_id(XPL_REQUEST_ID));
      if(j)
         request = j->valuestring;
      if(!request) {
         VERBOSE(5) mea_log_printf("%s (%s) : xPL message no request\n", INFO_STR, __func__);
         return 0;
      }

      if(mea_strcmplower(request,get_token_string_by_id(XPL_CURRENT_ID))!=0 && mea_strcmplower(request,get_token_string_by_id(XPL_LAST_ID))!=0) {
         VERBOSE(5) mea_log_printf("%s (%s) : xPL message request!=current or request!=last\n", INFO_STR, __func__);
         return -1;
      }

      return interface_type_005_xPL_sensor2(i005, xplMsgJson, device, elem);
   }
   
   return 0;
}


int clean_queues(mea_queue_t *q)
{
   struct type005_device_queue_elem_s *d;
   struct type005_module_queue_elem_s *m;
   struct type005_sensor_actuator_queue_elem_s *sa;
  
   mea_queue_first(q);
   while(mea_queue_current(q,(void **)&d)==0) {
      mea_queue_first(&(d->modules_list));
      while(mea_queue_current(&(d->modules_list),(void **)&m)==0) {
         mea_queue_first(&(m->sensors_actuators_list));
         while(mea_queue_current(&(m->sensors_actuators_list),(void **)&sa)==0) {
            free(sa);
            sa=NULL;
            mea_queue_remove_current(&(m->sensors_actuators_list));
         }
         free(m);
         m=NULL;
         mea_queue_remove_current(&(d->modules_list));
      }
      free(d);
      d=NULL;
      mea_queue_remove_current(q);
   }
   return 0;
}


int load_interface_type_005(interface_type_005_t *i005, cJSON *jsonInterface)
{
   parsed_parameters_t *netatmo_sa_params=NULL;
   int nb_netatmo_sa_params=0;
   int nerr=0;
//   int ret;

   if(mea_queue_nb_elem(&(i005->devices_list))!=0) {
      clean_queues(&(i005->devices_list));
   }

   struct type005_device_queue_elem_s *d_elem=NULL;
   struct type005_module_queue_elem_s *m_elem=NULL;
   struct type005_sensor_actuator_queue_elem_s *sa_elem=NULL;
  
   // récupération des parametrages des capteurs dans la base
   cJSON *jsonDevices=cJSON_GetObjectItem(jsonInterface, "devices");

   if(jsonDevices==NULL)
      goto load_interface_type_005_clean_exit;

   cJSON *jsonDevice=jsonDevices->child;
   while (jsonDevice) {

      int new_d_elem_flag=0;
      int new_m_elem_flag=0;

      // les valeurs dont on a besoin
      char *name=jsonDevice->string;
      int id_sensor_actuator=(int)cJSON_GetObjectItem(jsonDevice, "id_sensor_actuator")->valuedouble;
      int id_type=(int)cJSON_GetObjectItem(jsonDevice, "id_type")->valuedouble;
      char *parameters=(char*)cJSON_GetObjectItem(jsonDevice, "parameters")->valuestring;
      int todbflag=0;

      netatmo_sa_params=alloc_parsed_parameters((char *)parameters, valid_netatmo_sa_params, &nb_netatmo_sa_params, &nerr, 0);
      if(netatmo_sa_params &&
         netatmo_sa_params->parameters[PARAMS_DEVICE_ID].value.s &&
         netatmo_sa_params->parameters[PARAMS_MODULE_ID].value.s &&
         netatmo_sa_params->parameters[PARAMS_DEVICE_TYPE].value.s)
      { // les trois parametres sont obligatoires
         char *device_id=netatmo_sa_params->parameters[PARAMS_DEVICE_ID].value.s;
         char *module_id=netatmo_sa_params->parameters[PARAMS_MODULE_ID].value.s;
         char *devicetype = netatmo_sa_params->parameters[PARAMS_DEVICE_TYPE].value.s;
         enum device_type_e device_type = -1;

         if(mea_strcmplower(devicetype, "thermostat") == 0)
            device_type = THERMOSTAT;
         else if(mea_strcmplower(devicetype, "station") == 0)
            device_type = STATION;
         else {
            VERBOSE(2) mea_log_printf("%s (%s) : %s, configuration error - DEVICE_TYPE is not THERMOSTAT or STATION\n", ERROR_STR, __func__, name);
            release_parsed_parameters(&netatmo_sa_params);
            continue;
         }

         d_elem=NULL;
         mea_queue_first(&(i005->devices_list));
         for(int i=0; i<i005->devices_list.nb_elem; i++) { // on commence par chercher si le device existe déjà
            mea_queue_current(&(i005->devices_list), (void **)&d_elem);
            if(strcmp(d_elem->device_id, device_id)==0)
               break;
            else
               d_elem=NULL;
            mea_queue_next(&(i005->devices_list));
         }
         if(d_elem==NULL) { // il n'existe pas, on va le créer
            d_elem=(struct type005_device_queue_elem_s *)malloc(sizeof(struct type005_device_queue_elem_s));
            if(d_elem==NULL) {
               VERBOSE(2) {
                  char err_str[256];
                  strerror_r(errno, err_str, sizeof(err_str)-1);
                  mea_log_printf("%s (%s) : %s - %s\n", ERROR_STR, __func__, MALLOC_ERROR_STR,err_str);
               }
               release_parsed_parameters(&netatmo_sa_params);
               continue; // on à pas encore fait d'autre malloc on boucle direct
            }
            strncpy(d_elem->device_id, device_id, sizeof(d_elem->device_id)-1);
            d_elem->device_type = device_type;
            mea_queue_init(&(d_elem->modules_list));
            new_d_elem_flag=1; // on signale qu'on a créé un device
         }

         m_elem=NULL;
         mea_queue_first(&(d_elem->modules_list));
         for(int i=0; i<d_elem->modules_list.nb_elem; i++) { // on cherche si le module est déjà associé au device
            mea_queue_current(&(d_elem->modules_list), (void **)&m_elem);
            if(strcmp(m_elem->module_id, module_id)==0)
               break;
            else
               m_elem=NULL;
            mea_queue_next(&(d_elem->modules_list));
         }
         if(m_elem==NULL) { // pas encore associé on le créé
            m_elem=(struct type005_module_queue_elem_s *)malloc(sizeof(struct type005_module_queue_elem_s));
            if(m_elem==NULL) {
               VERBOSE(2) {
                  char err_str[256];
                  strerror_r(errno, err_str, sizeof(err_str)-1);
                  mea_log_printf("%s (%s) : %s - %s\n", ERROR_STR, __func__, MALLOC_ERROR_STR,err_str);
               }
               goto load_interface_type_005_clean_queues; // cette fois on a peut-être déjà fait un malloc, on branche sur la section de clean
            }
            strncpy(m_elem->module_id, module_id, sizeof(m_elem->module_id)-1);
            mea_queue_init(&(m_elem->sensors_actuators_list));
           
            // initialisation des données associées au module 
            switch(device_type)
            {
               case THERMOSTAT:
                  m_elem->d1.thermostat.battery_vp=-1;
                  m_elem->d1.thermostat.setpoint=-1;
                  m_elem->d1.thermostat.setpoint_temp=0.0;
                  m_elem->d1.thermostat.temperature=0.0;
                  m_elem->d1.thermostat.therm_relay_cmd=-1;

                  m_elem->d2.thermostat.battery_vp=-1;
                  m_elem->d2.thermostat.setpoint=-1;
                  m_elem->d2.thermostat.setpoint_temp=0.0;
                  m_elem->d2.thermostat.temperature=0.0;
                  m_elem->d2.thermostat.therm_relay_cmd=-1;
                  break;
               case STATION:
                  memset(&(m_elem->d1.station), 0, sizeof(struct netatmo_data_s));
                  memset(&(m_elem->d2.station), 0, sizeof(struct netatmo_data_s));
                  break;
            }

            m_elem->last=&(m_elem->d1);
            m_elem->current=&(m_elem->d2);

            m_elem->device=d_elem;

            new_m_elem_flag=1; // on signale qu'on a créé un module
         }

         sa_elem=(struct type005_sensor_actuator_queue_elem_s *)malloc(sizeof(struct type005_sensor_actuator_queue_elem_s));
         if(sa_elem==NULL) {
            VERBOSE(2) {
               char err_str[256];
               strerror_r(errno, err_str, sizeof(err_str)-1);
               mea_log_printf("%s (%s) : %s - ", ERROR_STR, __func__, MALLOC_ERROR_STR,err_str);
            }
            goto load_interface_type_005_clean_queues;
         }

         char *sensor=netatmo_sa_params->parameters[PARAMS_SENSOR].value.s;
         char *actuator=netatmo_sa_params->parameters[PARAMS_ACTUATOR].value.s;

         if(sensor && actuator) {
            VERBOSE(2) mea_log_printf("%s (%s) : parameter errors - SENSOR and ACUTATOR are incompatible\n", ERROR_STR,__func__,sensor);
            goto load_interface_type_005_clean_queues;
         }

         sa_elem->sensor=-1;
         sa_elem->actuator=-1;
         if(id_type==500) {
            if(sensor) {
               mea_strtolower(sensor);
               switch(device_type)
               {
                  case THERMOSTAT:
                     if(strcmp(sensor, "temperature")==0)
                        sa_elem->sensor=_TEMP;
                     else if(strcmp(sensor, "relay_cmd")==0)
                        sa_elem->sensor=_RELAY_CMD;
                     else if(strcmp(sensor, "battery")==0)
                        sa_elem->sensor=_BATTERY;
                     else if(strcmp(sensor, "setpoint")==0)
                        sa_elem->sensor=_SETPOINT;
                     else if(strcmp(sensor, "mode")==0)
                        sa_elem->sensor=_MODE;
                     else {
                        VERBOSE(2) mea_log_printf("%s (%s) : %s, incorrect sensor type error - %s\n", ERROR_STR, __func__, name, sensor);
                        goto load_interface_type_005_clean_queues;
                     }
                     break;
                   
                  case STATION:
                     if(strcmp(sensor, "temperature")==0)
                        sa_elem->sensor=_TEMPERATURE;
                     else if(strcmp(sensor, "co2")==0)
                        sa_elem->sensor=_CO2;
                     else if(strcmp(sensor, "humidity")==0)
                        sa_elem->sensor=_HUMIDITY;
                     else if(strcmp(sensor, "noise")==0)
                        sa_elem->sensor=_NOISE;
                     else if(strcmp(sensor, "pressure")==0)
                        sa_elem->sensor=_PRESSURE;
                     else if(strcmp(sensor, "rain")==0)
                        sa_elem->sensor=_RAIN;
                     else if(strcmp(sensor, "windangle")==0)
                        sa_elem->sensor=_WINDANGLE;
                     else if(strcmp(sensor, "windstrength")==0)
                        sa_elem->sensor=_WINDSTRENGTH;
                     else {
                        VERBOSE(2) mea_log_printf("%s (%s) : %s, incorrect sensor type error - %s\n", ERROR_STR, __func__, name, sensor);
                        goto load_interface_type_005_clean_queues;
                     }
                     break;
               }
            }
            else {
               VERBOSE(2) mea_log_printf("%s (%s) : %s, INPUT type need SENSOR parameter - %s\n", ERROR_STR, __func__, name, sensor);
               goto load_interface_type_005_clean_queues;
            }
         }
         else if(id_type==501) {
            if(device_type == STATION) {
                VERBOSE(2) mea_log_printf("%s (%s) : %s, no actuator available for station - %s\n", ERROR_STR, __func__, name, actuator);
               goto load_interface_type_005_clean_queues;
            }
            if(actuator) {
               mea_strtolower(actuator);
               if(strcmp(actuator, "setpoint")==0)
                  sa_elem->actuator=1;
               else {
                  VERBOSE(2) mea_log_printf("%s (%s) : %s, incorrect actuator type error - %s\n", ERROR_STR, __func__, name, actuator);
                  goto load_interface_type_005_clean_queues;
               }
            }
            else {
               VERBOSE(2) mea_log_printf("%s (%s) : %s, OUTPUT type need ACTUATOR parameter - %s\n", ERROR_STR, __func__, name, sensor);
               goto load_interface_type_005_clean_queues;
            }
         }
         else {
            VERBOSE(2) mea_log_printf("%s (%s) : %s, configuration error - incorrect (INPUT or OUPUT only allowed) type\n", ERROR_STR,__func__, name);
            goto load_interface_type_005_clean_queues;
         }

         sa_elem->id=id_sensor_actuator;
         sa_elem->type=id_type;
         strncpy(sa_elem->name, (const char *)name, sizeof(sa_elem->name)-1);
         mea_strtolower(sa_elem->name);
         sa_elem->todbflag=todbflag;
         sa_elem->module=m_elem;

         mea_queue_in_elem(&(m_elem->sensors_actuators_list), (void *)sa_elem);
         if(new_m_elem_flag==1) {
            mea_queue_in_elem(&(d_elem->modules_list), (void *)m_elem);
         }
         if(new_d_elem_flag==1) {
            mea_queue_in_elem(&(i005->devices_list), (void *)d_elem);
         }

         release_parsed_parameters(&netatmo_sa_params);
         continue;

load_interface_type_005_clean_queues:
         if(netatmo_sa_params)
            release_parsed_parameters(&netatmo_sa_params);
         if(sa_elem) {
            free(sa_elem);
            sa_elem=NULL;
         }
         if(new_d_elem_flag==1) {
            if(d_elem) {
               free(d_elem);
               d_elem=NULL;
            }
         }
         if(new_m_elem_flag==1) {
            if(m_elem) {
               free(m_elem);
               m_elem=NULL;
            }
         }
      }
      else {
         VERBOSE(2) mea_log_printf("%s (%s) : %s, configuration error - DEVICE_ID, MODULE_ID and DEVICE_TYPE parameters are mandatory\n", ERROR_STR, __func__, name);
         release_parsed_parameters(&netatmo_sa_params);
      }
   }

   return 0;

load_interface_type_005_clean_exit:
   clean_queues(&(i005->devices_list));
   return -1;
}


void set_interface_type_005_isnt_running(void *data)
{
   interface_type_005_t *i005 = (interface_type_005_t *)data;
   i005->thread_is_running=0;
}


static int interface_type_005_sendData(interface_type_005_t *i005, struct type005_sensor_actuator_queue_elem_s *sa_elem)
{
   int ret=-1;
   char str_value[20], str_last[20], type[20];
   double data4db=0.0;
   double data4db2=0.0;
   char data4dbComp[20]="";
                     
//   VERBOSE(9) mea_log_printf("%s (%s) : start sendData\n", ERROR_STR, __func__);
   ret=interface_type_005_current_last(sa_elem, type, str_value, str_last, &data4db, &data4db2, data4dbComp);
   if(ret==1)
   {
      _interface_type_005_send_xPL_sensor_basic2(i005, XPL_TRIG_STR_C, sa_elem->name, type, str_value, str_last);
      return 0;
   }
   else
   {
//      VERBOSE(9) mea_log_printf("%s (%s) : sendData error\n", ERROR_STR, __func__);
      return -1;
   }
}


static int interface_type_005_updateModuleData(interface_type_005_t *i005,
                                              struct type005_module_queue_elem_s *m_elem,
                                              void *data, int l_data)
{
   union module_data_u *tmp;
   struct type005_sensor_actuator_queue_elem_s *sa_elem;

//   VERBOSE(9) mea_log_printf("%s (%s) : start updateModuleData %x\n", DEBUG_STR, __func__, m_elem);
   tmp=m_elem->last;
   m_elem->last=m_elem->current;
   m_elem->current=tmp;
   memcpy(m_elem->current, data, l_data);
   
   mea_queue_first(&(m_elem->sensors_actuators_list));
   for(int k=0;k<m_elem->sensors_actuators_list.nb_elem;k++)
   {
      mea_queue_current(&(m_elem->sensors_actuators_list), (void **)&sa_elem);

      if(sa_elem->type==500)
         interface_type_005_sendData(i005, sa_elem);

      mea_queue_next(&(m_elem->sensors_actuators_list));
   }
//   VERBOSE(9) mea_log_printf("%s (%s) : updateModuleData done\n", DEBUG_STR, __func__);

   return 0;
}


static int interface_type_005_getData(interface_type_005_t *i005)
{
   struct type005_device_queue_elem_s *d_elem;
   int ret = -1;
   char err[256];

   mea_queue_first(&(i005->devices_list));
   for(int i=0;i<i005->devices_list.nb_elem;i++)
   {
      mea_queue_current(&(i005->devices_list), (void **)&d_elem);

      struct type005_module_queue_elem_s *m_elem = NULL;

      if(d_elem->device_type==STATION)
      {
         struct netatmo_station_data_s station_data;

         ret=netatmo_get_station_data(i005->token.access, d_elem->device_id, &station_data, err,  sizeof(err)-1);
         if(ret==0)
         {
            mea_queue_first(&(d_elem->modules_list));
            for(int j=0;j<d_elem->modules_list.nb_elem;j++)
            {
               struct netatmo_data_s *data = NULL;

               mea_queue_current(&(d_elem->modules_list), (void **)&m_elem);

               if(mea_strcmplower(m_elem->module_id, "station")==0)
                  data = &(station_data.data);
               else
               {
                  for(int i=0; i<station_data.nb_modules; i++)
                  {
                     if(mea_strcmplower(m_elem->module_id, station_data.modules_data[i].id)==0)
                     {
                        data = &(station_data.modules_data[i].data);
                        break;
                     }
                  }
               }
               if(data!=NULL)
               {
                  (i005->indicators.nbread)++;

//                  VERBOSE(9) mea_log_printf("%s (%s) : call updateModuleData for %s (station)\n", DEBUG_STR, __func__, m_elem->module_id);
                  interface_type_005_updateModuleData(i005, m_elem, data,  sizeof(struct netatmo_data_s));
               }

               mea_queue_next(&(d_elem->modules_list));
            }
         }
         else
         {
            (i005->indicators.nbreaderror)++;
//            DEBUG_SECTION mea_log_printf("%s (%s) : can't get station data (%d: %s)\n", ERROR_STR, __func__, ret, err);
         }
      }
      else if(d_elem->device_type==THERMOSTAT)
      {
         mea_queue_first(&(d_elem->modules_list));
         for(int j=0;j<d_elem->modules_list.nb_elem;j++)
         {
            mea_queue_current(&(d_elem->modules_list), (void **)&m_elem);

            struct netatmo_thermostat_data_s data;
            ret=netatmo_get_thermostat_data(i005->token.access, d_elem->device_id, m_elem->module_id, &data, err, sizeof(err)-1);
            if(ret==0)
            {
               (i005->indicators.nbread)++;

//               VERBOSE(9) mea_log_printf("%s (%s) : call updateModuleData for %s (thermostat)\n", DEBUG_STR, __func__, m_elem->module_id);
               interface_type_005_updateModuleData(i005, m_elem, &data,  sizeof(data));
            }
            else
            {
               (i005->indicators.nbreaderror)++;
//               DEBUG_SECTION mea_log_printf("%s (%s) : can't get thermostat data (%d: %s)\n", ERROR_STR, __func__, ret, err);
            }
            mea_queue_next(&(d_elem->modules_list));
         }
      }
      mea_queue_next(&(i005->devices_list));
   }

   return 0;
}


void *_thread_interface_type_005(void *thread_data)
{
   struct thread_interface_type_005_args_s *args = (struct thread_interface_type_005_args_s *)thread_data;
   interface_type_005_t *i005=NULL;

   if(thread_data) {   
      i005=args->i005;
      free(thread_data);
   } 
   else
      return NULL; 
 
   
   pthread_cleanup_push( (void *)set_interface_type_005_isnt_running, (void *)i005 );
   i005->thread_is_running=1;

   // à récupérer dans un fichier de config (dans etc de l'appli)
   char *client_id="563e5ce3cce37c07407522f2";
   char *client_secret="lE1CUF1k3TxxSceiPpmIGY8QXJWIeXJv0tjbTRproMy4";

   int auth_flag=0;
   mea_timer_t refresh_timer;
   mea_timer_t getdata_timer;
   char err[256];
   int ret;

   mea_init_timer(&refresh_timer,0,0);
   mea_init_timer(&getdata_timer,60,1);
//   mea_init_timer(&getdata_timer,5,1); // pour debug
   mea_start_timer(&getdata_timer); 

   while(1) {
      pthread_testcancel();
     
      process_heartbeat(i005->monitoring_id);
      process_update_indicator(i005->monitoring_id, I005_XPLIN,  i005->indicators.xplin);
      process_update_indicator(i005->monitoring_id, I005_XPLOUT, i005->indicators.xplout);
      process_update_indicator(i005->monitoring_id, I005_NBREADERROR, i005->indicators.nbreaderror);
      process_update_indicator(i005->monitoring_id, I005_NBREAD, i005->indicators.nbread);

      if(auth_flag==0) {
         VERBOSE(9) mea_log_printf("%s (%s) : start authentification\n", ERROR_STR, __func__);
         ret=netatmo_get_token(client_id, client_secret, i005->user, i005->password, "read_thermostat write_thermostat read_station", &(i005->token), err, sizeof(err)-1);
         if(ret!=0) {
            VERBOSE(5) {
               mea_log_printf("%s (%s) : authentification error - ", ERROR_STR, __func__);
               if(ret<0)
                  fprintf(MEA_STDERR, "%d\n", ret);
               else
                  fprintf(MEA_STDERR, "%s (%d)\n", err, ret);
            }
         }
         else {
            VERBOSE(9) {
               mea_log_printf("%s (%s) : authentification done - %s\n", INFO_STR, __func__, i005->token.access);
            }
          
            auth_flag=1;
            mea_init_timer(&refresh_timer,(int)((float)i005->token.expire_in*0.95), 0);
            mea_start_timer(&refresh_timer);
         }
      }

      if(mea_test_timer(&refresh_timer)==0) {
         VERBOSE(9) mea_log_printf("%s (%s) : start refresh", ERROR_STR, __func__);
         ret=netatmo_refresh_token(client_id, client_secret, &(i005->token), err, sizeof(err)-1);
         if(ret!=0) {
            VERBOSE(9) {
               mea_log_printf("%s (%s) : authentification error\n", INFO_STR, __func__);
            }
            auth_flag=0;
            continue;
         }
         else {
            mea_init_timer(&refresh_timer,(int)((float)i005->token.expire_in*0.95), 0);
            mea_start_timer(&refresh_timer);
         }
      }

      if(auth_flag!=0 && mea_test_timer(&getdata_timer)==0) {
         interface_type_005_getData(i005);
      }

      sleep(5);
   }
   
_thread_interface_type_005_clean_exit:
   pthread_cleanup_pop(1);

   pthread_exit(NULL);
}


xpl2_f get_xPLCallback_interface_type_005(void *ixxx)
{
   interface_type_005_t *i005 = (interface_type_005_t *)ixxx;

   if(i005 == NULL)
      return NULL;
   else
      return i005->xPL_callback2;
}


int get_monitoring_id_interface_type_005(void *ixxx)
{
   interface_type_005_t *i005 = (interface_type_005_t *)ixxx;

   if(i005 == NULL)
      return -1;
   else
      return i005->monitoring_id;
}


int set_xPLCallback_interface_type_005(void *ixxx, xpl2_f cb)
{
   interface_type_005_t *i005 = (interface_type_005_t *)ixxx;

   if(i005 == NULL)
      return -1;
   else {
      i005->xPL_callback2 = cb;
      return 0;
   }
}


int set_monitoring_id_interface_type_005(void *ixxx, int id)
{
   interface_type_005_t *i005 = (interface_type_005_t *)ixxx;

   if(i005 == NULL)
      return -1;
   else {
      i005->monitoring_id = id;
      return 0;
   }
}


int get_type_interface_type_005()
{
   return INTERFACE_TYPE_005;
}


interface_type_005_t *malloc_and_init2_interface_type_005(int id_driver, cJSON *jsonInterface)
{
   interface_type_005_t *i005=NULL;

   i005=(interface_type_005_t *)calloc(1, sizeof(interface_type_005_t));
   if(!i005) {
      VERBOSE(2) {
         char err_str[256];
         strerror_r(errno, err_str, sizeof(err_str)-1);
         mea_log_printf("%s (%s) : %s - %s\n",ERROR_STR,__func__,MALLOC_ERROR_STR,err_str);
      }
      return NULL;
   }
   i005->thread_is_running=0;
   
   struct interface_type_005_start_stop_params_s *i005_start_stop_params=(struct interface_type_005_start_stop_params_s *)malloc(sizeof(struct interface_type_005_start_stop_params_s));
   if(!i005_start_stop_params) {
      free(i005);
      i005=NULL;
      VERBOSE(2) {
         char err_str[256];
         strerror_r(errno, err_str, sizeof(err_str)-1);
         mea_log_printf("%s (%s) : %s - %s\n",ERROR_STR,__func__,MALLOC_ERROR_STR,err_str);
      }
      return NULL;
   }

   int id_interface=(int)cJSON_GetObjectItem(jsonInterface,"id_interface")->valuedouble;
   char *name=jsonInterface->string;
   char *dev=cJSON_GetObjectItem(jsonInterface,"dev")->valuestring;
   char *parameters=cJSON_GetObjectItem(jsonInterface,"parameters")->valuestring;
   char *description=cJSON_GetObjectItem(jsonInterface,"description")->valuestring;

   i005->parameters=(char *)malloc(strlen((char *)parameters)+1);
   if(i005->parameters==NULL) {
      free(i005);
      i005=NULL;
      free(i005_start_stop_params);
      i005_start_stop_params=NULL;

      VERBOSE(2) {
         char err_str[256];
         strerror_r(errno, err_str, sizeof(err_str)-1);
         mea_log_printf("%s (%s) : %s - %s\n",ERROR_STR,__func__,MALLOC_ERROR_STR,err_str);
      }
      return NULL;
   }
   strcpy(i005->parameters,(char *)parameters);
   strncpy(i005->name,(char *)name,sizeof(i005->name)-1);
   strncpy(i005->dev,(char *)dev,sizeof(i005->dev)-1);
   i005->id_interface=id_interface;
   mea_queue_init(&(i005->devices_list));

   i005->indicators.xplin=0;
   i005->indicators.xplout=0;
   i005->indicators.nbreaderror=0;
   i005->indicators.nbread=0;
   i005->xPL_callback2=NULL;
   i005->thread=NULL;
   
   i005->monitoring_id=process_register((char *)name);
   i005_start_stop_params->i005=i005;
   
   process_set_group(i005->monitoring_id, 1);
   process_set_start_stop(i005->monitoring_id, start_interface_type_005, stop_interface_type_005, (void *)i005_start_stop_params, 1);
   process_set_watchdog_recovery(i005->monitoring_id, restart_interface_type_005, (void *)i005_start_stop_params);
   process_set_description(i005->monitoring_id, (char *)description);
   process_set_heartbeat_interval(i005->monitoring_id, 60); // chien de garde au bout de 60 secondes sans heartbeat
   
   process_add_indicator(i005->monitoring_id, interface_type_005_xplout_str, 0);
   process_add_indicator(i005->monitoring_id, interface_type_005_xplin_str, 0);
   process_add_indicator(i005->monitoring_id, interface_type_005_nbreaderror_str, 0);
   process_add_indicator(i005->monitoring_id, interface_type_005_nbread_str, 0);
   
   return i005;
}


int update_devices_type_005(void *ixxx)
{
   printf("update devices type 005\n");

   return 0;
}


int clean_interface_type_005(interface_type_005_t *i005)
{
   clean_queues(&(i005->devices_list));

   if(i005->parameters) {
      free(i005->parameters);
      i005->parameters=NULL;
   }
   
   i005->xPL_callback2=NULL;
   
   if(i005->thread) {
      free(i005->thread);
      i005->thread=NULL;
   }

   return 0;
}


int stop_interface_type_005(int my_id, void *data, char *errmsg, int l_errmsg)
{
   if(!data)
      return -1;
   
   struct interface_type_005_start_stop_params_s *start_stop_params=(struct interface_type_005_start_stop_params_s *)data;
   
   VERBOSE(1) mea_log_printf("%s (%s) : %s shutdown thread ...\n", INFO_STR, __func__, start_stop_params->i005->name);
   
   start_stop_params->i005->xPL_callback2=NULL;
   
   if(start_stop_params->i005->thread) {
      pthread_cancel(*(start_stop_params->i005->thread));
      
      int counter=100;
//      int stopped=-1;
      while(counter--) {
         if(start_stop_params->i005->thread_is_running) {
            // pour éviter une attente "trop" active
            usleep(100); // will sleep for 10 ms
         }
         else {
//            stopped=0;
            break;
         }
      }
      DEBUG_SECTION mea_log_printf("%s (%s) : %s, fin après %d itération(s)\n", DEBUG_STR, __func__, start_stop_params->i005->name, 100-counter);

      clean_interface_type_005(start_stop_params->i005);
   }
   
   return 0;
}


int restart_interface_type_005(int my_id, void *data, char *errmsg, int l_errmsg)
{
   process_stop(my_id, NULL, 0);
   sleep(5);
   return process_start(my_id, NULL, 0);
}


int16_t check_status_interface_type_005(interface_type_005_t *it005)
/**
 * \brief     indique si une anomalie a généré l'emission d'un signal SIGHUP
 * \param     i005           descripteur de l'interface
 * \return    toujours 0, pas d'emission de signal
 **/
{
   return 0;
}


int start_interface_type_005(int driver_id, void *data, char *errmsg, int l_errmsg)
{
   pthread_t *interface_type_005_thread_id=NULL; // descripteur du thread
   struct thread_interface_type_005_args_s *interface_type_005_thread_args=NULL; // parametre à transmettre au thread
   
   struct interface_type_005_start_stop_params_s *start_stop_params=(struct interface_type_005_start_stop_params_s *)data; // données pour la gestion des arrêts/relances

   start_stop_params->i005->xPL_callback2=NULL;

   cJSON *jsonInterface = getInterfaceById_alloc(start_stop_params->i005->id_interface);

   int ret=load_interface_type_005(start_stop_params->i005, jsonInterface);

   cJSON_Delete(jsonInterface);

   if(ret<0) 
      return -1;

 
   interface_type_005_thread_args=malloc(sizeof(struct thread_interface_type_005_args_s));
   if(!interface_type_005_thread_args) {
      VERBOSE(2) {
         char err_str[256];
         strerror_r(errno, err_str, sizeof(err_str)-1);
         VERBOSE(2) mea_log_printf("%s (%s) : malloc - %s\n", ERROR_STR,__func__,err_str);
      }
      goto start_interface_type_005_clean_exit;
   }
   interface_type_005_thread_args->i005=start_stop_params->i005;

   // initialisation et préparation des données => on complète i005
   start_stop_params->i005->user[0]=0;
   start_stop_params->i005->password[0]=0;
   sscanf(start_stop_params->i005->dev, "NETATMO://%80[^/]/%80[^\n]", start_stop_params->i005->user, start_stop_params->i005->password);
   if(start_stop_params->i005->user[0]==0 || start_stop_params->i005->password[0]==0) {
      VERBOSE(2) mea_log_printf("%s (%s) : bad netatmo dev - incorrect user/password%s\n", ERROR_STR, __func__, start_stop_params->i005->dev);
      goto start_interface_type_005_clean_exit;
   }

   // préparation du lancement du thread   
   interface_type_005_thread_id=(pthread_t *)malloc(sizeof(pthread_t));
   if(!interface_type_005_thread_id) {
      VERBOSE(2) mea_log_printf("%s (%s) : malloc - %s\n", ERROR_STR,__func__);
      goto start_interface_type_005_clean_exit;
   }

   if(pthread_create (interface_type_005_thread_id, NULL, _thread_interface_type_005, (void *)interface_type_005_thread_args)) {
      VERBOSE(2) mea_log_printf("%s (%s) : pthread_create - can't start thread\n",ERROR_STR,__func__);
      goto start_interface_type_005_clean_exit;
   }
   fprintf(stderr, "INTERFACE_TYPE_005 : %x\n", (unsigned int)*interface_type_005_thread_id);

   start_stop_params->i005->xPL_callback2=interface_type_005_xPL_callback2;
   start_stop_params->i005->thread=interface_type_005_thread_id;

   pthread_detach(*interface_type_005_thread_id);
 
   VERBOSE(2) mea_log_printf("%s (%s) : %s %s.\n", INFO_STR, __func__, start_stop_params->i005->name, launched_successfully_str);
 
   return 0;

start_interface_type_005_clean_exit:
   if(interface_type_005_thread_id) {
      free(interface_type_005_thread_id);
      interface_type_005_thread_id=NULL;
   }
   if(interface_type_005_thread_args) {
      free(interface_type_005_thread_args);
      interface_type_005_thread_args=NULL;
   }
   
   return -1;
}


#ifdef ASPLUGIN
#else
int get_fns_interface_type_005(struct interfacesServer_interfaceFns_s *interfacesFns)
{
   interfacesFns->malloc_and_init2 = (malloc_and_init2_f)&malloc_and_init2_interface_type_005;
   interfacesFns->get_monitoring_id = (get_monitoring_id_f)&get_monitoring_id_interface_type_005;
   interfacesFns->get_xPLCallback = (get_xPLCallback_f)&get_xPLCallback_interface_type_005;
   interfacesFns->update_devices = (clean_f)&update_devices_type_005;
   interfacesFns->clean = (clean_f)&clean_interface_type_005;
   interfacesFns->set_monitoring_id = (set_monitoring_id_f)&set_monitoring_id_interface_type_005;
   interfacesFns->set_xPLCallback = (set_xPLCallback_f)&set_xPLCallback_interface_type_005;
   interfacesFns->get_type = (get_type_f)&get_type_interface_type_005;
   interfacesFns->api = NULL;
   interfacesFns->pairing = NULL;
   interfacesFns->lib = NULL;
   interfacesFns->type = interfacesFns->get_type();
   interfacesFns->plugin_flag = 0;

   return 0;
}
#endif

