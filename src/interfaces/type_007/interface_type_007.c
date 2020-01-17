//
//  interface_type_007.c
//  mea-edomus
//
//  Created by Patrice Dietsch on 21/02/2015.
//
//
#include "interface_type_007.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "macros.h"
#include "globals.h"

#include "serial.h"
#include "tokens.h"
#include "tokens_da.h"
#include "mea_verbose.h"
#include "mea_string_utils.h"
#include "parameters_utils.h"
#include "cJSON.h"
#include "mea_timer.h"
#include "mea_xpl.h"

#include "processManager.h"
#include "mea_plugins_utils.h"
#include "interfacesServer.h"


char *interface_type_007_xplin_str="XPLIN";
char *interface_type_007_senttoplugin_str="SENTTOPLUGIN";
char *interface_type_007_serialin_str="SERIALIN";
char *interface_type_007_serialout_str="SERIALOUT";


char *valid_interface_007_params[]={"S:FSTARTSTR","S:FENDSTR","I:FSIZE","I:FDURATION", "S:PLUGIN", "S:PLUGIN_PARAMETERS", NULL};
#define INTERFACE_PARAMS_FSTARTSTR         0
#define INTERFACE_PARAMS_FENDSTR           1
#define INTERFACE_PARAMS_FSIZE             2
#define INTERFACE_PARAMS_FDURATION         3
#define INTERFACE_PARAMS_PLUGIN            4
#define INTERFACE_PARAMS_PLUGIN_PARAMETERS 5

char *valid_plugin_007_params[]={"S:PLUGIN","S:PLUGIN_PARAMETERS", NULL};
#define PLUGIN_PARAMS_PLUGIN       0
#define PLUGIN_PARAMS_PARAMETERS   1


struct thread_params_s
{
   interface_type_007_t *i007;
};


struct callback_xpl_data_s 
{
};

typedef void (*thread_f)(void *);


int start_interface_type_007(int my_id, void *data, char *errmsg, int l_errmsg);
int stop_interface_type_007(int my_id, void *data, char *errmsg, int l_errmsg);
int restart_interface_type_007(int my_id, void *data, char *errmsg, int l_errmsg);


void set_interface_type_007_isnt_running(void *data)
{
   interface_type_007_t *i007 = (interface_type_007_t *)data;

   i007->thread_is_running=0;
}


int16_t _interface_type_007_xPL_callback2(cJSON *xplMsgJson, struct device_info_s *device_info, void *userValue)
{
   int err = 0;
   interface_type_007_t *i007=(interface_type_007_t *)userValue;

   i007->indicators.xplin++;

   parsed_parameters_t *plugin_params=NULL;
   int nb_plugin_params;

   plugin_params=alloc_parsed_parameters(device_info->parameters, valid_plugin_007_params, &nb_plugin_params, &err, 0);
   if(!plugin_params || !plugin_params->parameters[PLUGIN_PARAMS_PLUGIN].value.s) {
      if(plugin_params) {
         release_parsed_parameters(&plugin_params);
      }
      return -1;
   }

   cJSON *data=NULL;
   data=device_info_to_json_alloc(device_info);
   cJSON *msg=mea_xplMsgToJson_alloc(xplMsgJson);
   cJSON_AddItemToObject(data, XPLMSG_STR_C, msg);
   cJSON_AddNumberToObject(data, API_KEY_STR_C, (double)i007->id_interface);
   if(plugin_params->parameters[PLUGIN_PARAMS_PARAMETERS].value.s) {
      cJSON_AddStringToObject(data, DEVICE_PARAMETERS_STR_C, plugin_params->parameters[PLUGIN_PARAMS_PARAMETERS].value.s);
   }
   plugin_fireandforget_function_json(plugin_params->parameters[PLUGIN_PARAMS_PLUGIN].value.s, XPLMSG_JSON, data);

   release_parsed_parameters(&plugin_params);
   plugin_params=NULL;

   return 0;
}


int init_interface_type_007_data_preprocessor(interface_type_007_t *i007, char *plugin_name, char *plugin_parameters)
{
   if(!plugin_name) {
      i007->interface_plugin_name=NULL;
      return -1;
   }
      
   i007->interface_plugin_name=malloc(strlen(plugin_name)+1);
   if(i007->interface_plugin_name) {
      strcpy(i007->interface_plugin_name, plugin_name);
   }
   
   if(plugin_parameters) {
      i007->interface_plugin_parameters=malloc(strlen(plugin_parameters)+1);
      if(i007->interface_plugin_parameters) {
         strcpy(i007->interface_plugin_parameters, plugin_parameters);
      }
   }
   else {
      i007->interface_plugin_parameters=NULL;
   }

   return 0;
}


int interface_type_007_data_preprocessor(interface_type_007_t *i007)
{
   if(i007->interface_plugin_name) {
      cJSON *data=cJSON_CreateObject();
      if(data) {
         cJSON_AddItemToObject(data, DATA_STR_C, cJSON_CreateByteArray((char *)i007->line_buffer, i007->line_buffer_ptr));
         cJSON_AddNumberToObject(data, L_DATA_STR_C, (double)i007->line_buffer_ptr);
         cJSON_AddNumberToObject(data, INTERFACE_ID_STR_C, (double)i007->id_interface);
         cJSON_AddNumberToObject(data, API_KEY_STR_C, (double)i007->id_interface);
         if(i007->interface_plugin_parameters) {
            cJSON_AddStringToObject(data, PLUGIN_PARAMETERS_STR_C, i007->interface_plugin_parameters);
         }
      }
      cJSON *result=plugin_call_function_json_alloc(i007->interface_plugin_name, "mea_dataPreprocessor", data);
      
      if(result) {
         cJSON_Delete(result);
         result=NULL;
      }
   }

   return 0;
}


int interface_type_007_pairing(interface_type_007_t *i007)
{
   int ret=-1;
   char *data=NULL;
   cJSON *j=NULL, *_j=NULL;

   VERBOSE(5) {
      mea_log_printf("%s (%s) : ***** pairing data (%s) ? *****\n", INFO_STR, __func__, data);
   }
   
   char name[256];
   if(parsed_parameters_get_param_string(i007->parameters, valid_interface_007_params, INTERFACE_PARAMS_PLUGIN, name, sizeof(name)-1)==0) {
      name[sizeof(name)-1]=0;
      j=cJSON_CreateObject();
      cJSON_AddItemToObject(j, DATA_STR_C, cJSON_CreateByteArray((char *)i007->line_buffer, i007->line_buffer_ptr));
      cJSON_AddNumberToObject(j, L_DATA_STR_C, (double)i007->line_buffer_ptr);
      _j=plugin_call_function_json_alloc(name, "pairing_get_devices", j);

      VERBOSE(5) {
         if(_j) {
            char *s=cJSON_Print(_j);
            mea_log_printf("%s (%s) : pairing result=%s\n", ERROR_STR, __func__, s);
            free(s);
         }
      }
      
      if(_j && _j->type!=cJSON_NULL) {
         int _ret=addDevice(i007->name, _j);
         if(_ret==0) {
            VERBOSE(5) mea_log_printf("%s (%s) : pairing done\n", ERROR_STR, __func__);
         }
         else {
            VERBOSE(5) mea_log_printf("%s (%s) : pairing error (%d)\n", ERROR_STR, __func__, _ret);
         }
         i007->pairing_state=0;
         i007->pairing_startms=-1.0;
      }
      else {
         VERBOSE(5) mea_log_printf("%s (%s) : no pairing data\n", ERROR_STR, __func__);
      }

      if(_j) {
         cJSON_Delete(_j);
      }

      return 0;
   }

   i007->pairing_state=0;
   i007->pairing_startms=-1.0;

   return ret;
}


static int _interface_type_007_data_to_plugin(interface_type_007_t *i007,  struct device_info_s *device_info)
{
   int err = 0;

   parsed_parameters_t *plugin_params=NULL;
   int nb_plugin_params;

   plugin_params=alloc_parsed_parameters((char *)device_info->parameters, valid_plugin_007_params, &nb_plugin_params, &err, 0);
   if(!plugin_params || !plugin_params->parameters[PLUGIN_PARAMS_PLUGIN].value.s) {
      if(plugin_params) {
         release_parsed_parameters(&plugin_params);
      }
      return -1;
   }

   cJSON *data = device_info_to_json_alloc(device_info);
   cJSON_AddNumberToObject(data,API_KEY_STR_C,(double)i007->id_interface);
   cJSON_AddNumberToObject(data,DEVICE_TYPE_ID_STR_C,(double)device_info->id);

   if(plugin_params->parameters[PLUGIN_PARAMS_PARAMETERS].value.s) {
      cJSON_AddStringToObject(data, DEVICE_PARAMETERS_STR_C, plugin_params->parameters[PLUGIN_PARAMS_PARAMETERS].value.s);
   }
   cJSON_AddItemToObject(data,DATA_STR_C,cJSON_CreateByteArray(i007->line_buffer, i007->line_buffer_ptr));
   cJSON_AddNumberToObject(data,L_DATA_STR_C, (double)i007->line_buffer_ptr);

   plugin_fireandforget_function_json(plugin_params->parameters[PLUGIN_PARAMS_PLUGIN].value.s, DATAFROMSENSOR_JSON, data);
   
   i007->indicators.senttoplugin++;

   release_parsed_parameters(&plugin_params);
   
   return 0;
}


static int interface_type_007_data_to_plugin(interface_type_007_t *i007)
{
   int ret=0;
   
   if(i007->pairing_state==1) {
      int ret=interface_type_007_pairing(i007);
      if(ret>0) {
         return 0;
      }
   }

   ret=interface_type_007_data_preprocessor(i007);
   if(ret==-1) {
      return -1;
   }
   
   if(ret==1) {
   }

   cJSON *jsonInterface = getInterfaceById_alloc(i007->id_interface);
   if(jsonInterface) {
      cJSON *jsonDevices = cJSON_GetObjectItem(jsonInterface, DEVICES_STR_C);
      if(jsonDevices) {
         cJSON *jsonDevice = jsonDevices->child;
         while(jsonDevice) {
            struct device_info_s device_info;
            ret=device_info_from_json(&device_info, jsonDevice, jsonInterface, NULL);
            if(ret==0) {
               _interface_type_007_data_to_plugin(i007, &device_info);
            }

            jsonDevice=jsonDevice->next;
         }
      }
      cJSON_Delete(jsonInterface);
   } 

   return 0;
}


int haveFrameStartStr(interface_type_007_t *i007)
{
   if(!i007->fstartstr || !i007->fstartstr[0]) {
      return -1;
   }
   int l=(int)strlen(i007->fstartstr);
   char *s=&(i007->line_buffer[i007->line_buffer_ptr-l]);
   if(strncmp(s, i007->fstartstr, l) == 0) {
      return 0;
   }
   else {
      return -1;
   }
}


int haveFrameEndStr(interface_type_007_t *i007)
{
   if(!i007->fendstr || !i007->fendstr[0]) {
      return -1;
   }

   int l=(int)strlen(i007->fendstr);
   char *s=&(i007->line_buffer[i007->line_buffer_ptr-l]);
   if(strncmp(s, i007->fendstr, l) == 0) {
      return l;
   }
   else {
      return -1;
   }
}


static int process_interface_type_007_data(interface_type_007_t *i007)
{
   if(i007->fd == -1) {
      return -1;
   }

   fd_set set;
   struct timeval timeout;

   mea_timer_t timer;
   mea_init_timer(&timer, 10, 1);
   mea_start_timer(&timer);

   FD_ZERO(&set);
   FD_SET(i007->fd, &set);
   int ret = 0;
   do {
      if(mea_test_timer(&timer)==0) {
         break;
      }

      if(i007->pairing_state==1) {
         if(mea_now()-i007->pairing_startms>60.0*1000.0) {
            i007->pairing_state=0;
            i007->pairing_startms=-1.0;
         }
      }

      /* Initialize the timeout data structure. */
      if(i007->fduration <= 0) {
         timeout.tv_sec = 5;
         timeout.tv_usec = 0;
      }
      else {
         timeout.tv_sec = i007->fduration / 1000;
         timeout.tv_usec = (i007->fduration % 1000)*1000;
      }

      ret = select(FD_SETSIZE, &set, NULL, NULL, &timeout);
      
      if(ret < 0) {
         // erreur de select
         VERBOSE(5) {
            char err_str[256];
            strerror_r(errno, err_str, sizeof(err_str)-1);
            mea_log_printf("%s (%s) : select error - %s\n", ERROR_STR, __func__, err_str);
         }
         return -1;
      }
      else if(ret == 0) {
         if((i007->fduration > 0) && (i007->line_buffer_ptr)) {
            i007->line_buffer[i007->line_buffer_ptr]=0;
            interface_type_007_data_to_plugin(i007);
            i007->line_buffer_ptr=0;
         }
         break;
      }
      else {
         char c;
         ret=(int)read(i007->fd, &c, 1);
         if(ret<0) {
            VERBOSE(5) {
               char err_str[256];
               strerror_r(errno, err_str, sizeof(err_str)-1);
               mea_log_printf("%s (%s) : read error - %s\n", ERROR_STR, __func__,err_str);
            }
            close(i007->fd);
            i007->fd=-1;
            return -1;
         }
         break;

         i007->line_buffer[i007->line_buffer_ptr++]=c;
         i007->indicators.serialin++;

         if(i007->line_buffer_ptr>=i007->line_buffer_l) {
            char *tmp = realloc(i007->line_buffer, i007->line_buffer_l + INC_LINE_BUFFER_SIZE);
            if(!tmp) {
               i007->line_buffer_ptr = 0;
               VERBOSE(5) {
                  char err_str[256];
                  strerror_r(errno, err_str, sizeof(err_str)-1);
                  mea_log_printf("%s (%s) : realloc error - %s\n", ERROR_STR, __func__,err_str);
               }
               return -1;
            }
            else {
               i007->line_buffer = tmp;
               i007->line_buffer_l+=INC_LINE_BUFFER_SIZE;
            }
         }

         if(i007->fsize>0 && i007->line_buffer_ptr==i007->fsize) {
            i007->line_buffer[i007->line_buffer_ptr]=0;
            
            int b = haveFrameEndStr(i007);
            if(b>0) {
               i007->line_buffer_ptr-=b;
               i007->line_buffer[i007->line_buffer_ptr]=0;
            }

            interface_type_007_data_to_plugin(i007);
            i007->line_buffer_ptr=0;
            break;
         }

         if(i007->fendstr && i007->fendstr[0]) {
            int b = 0;
            b = haveFrameEndStr(i007);
            if(b>0) {
               i007->line_buffer_ptr-=b;
               i007->line_buffer[i007->line_buffer_ptr]=0;
               interface_type_007_data_to_plugin(i007);
               i007->line_buffer_ptr=0;
               break;
            }
         }

         if(i007->fstartstr && i007->fstartstr[0] && haveFrameStartStr(i007)==0) {
            i007->line_buffer_ptr=0;
            break;
         }
      }
   }
   while(ret);
   
   return 0;
}


int update_devices_type_007(void *context)
{
   interface_type_007_t *i007=(interface_type_007_t *)context;
   int ret=-1;

   if(i007->interface_plugin_name) {
      cJSON *data=cJSON_CreateObject();
      cJSON_AddNumberToObject(data, INTERFACE_ID_STR_C, i007->id_interface);
      cJSON_AddNumberToObject(data, API_KEY_STR_C, i007->id_interface);
      if(i007->interface_plugin_parameters) {
         cJSON_AddStringToObject(data, PLUGIN_PARAMETERS_STR_C, i007->interface_plugin_parameters);
      }
      cJSON *result=plugin_call_function_json_alloc(i007->interface_plugin_name, "mea_updateDevices", data);
      if(result) {
         if(result->type!=cJSON_False && result->type!=cJSON_NULL) {
            ret=0;
         }
         cJSON_Delete(result);
         result=NULL;
      }
   }

   return ret;
}


static int clean_interface_type_007_data_source(interface_type_007_t *i007)
{
   i007->line_buffer_l = 0;
   i007->line_buffer_ptr = 0;
   if(i007->line_buffer) {
      free(i007->line_buffer);
   }
   i007->line_buffer = NULL;

   return 0;
}


int clean_interface_type_007(void *ixxx)
{
   interface_type_007_t *i007 = (interface_type_007_t *)ixxx;

   if(i007->parameters) {
      free(i007->parameters);
      i007->parameters=NULL;
   }

   if(i007->xPL_callback2) {
      i007->xPL_callback2=NULL;
   }

   if(i007->thread) {
      free(i007->thread);
      i007->thread=NULL;
   }

   if(i007->fstartstr) {
      free(i007->fstartstr);
      i007->fstartstr=NULL;
   }

   if(i007->fendstr) {
      free(i007->fendstr);
      i007->fendstr=NULL;
   }

   if(i007->interface_plugin_name) {
      free(i007->interface_plugin_name);
      i007->interface_plugin_name=NULL;
   }

   if(i007->interface_plugin_parameters) {
      free(i007->interface_plugin_parameters);
      i007->interface_plugin_parameters=NULL;
   }

   return 0;
}


xpl2_f get_xPLCallback_interface_type_007(void *ixxx)
{
   interface_type_007_t *i007 = (interface_type_007_t *)ixxx;

   if(i007 == NULL) {
      return NULL;
   }
   else {
      return i007->xPL_callback2;
   }
}


int get_monitoring_id_interface_type_007(void *ixxx)
{
   interface_type_007_t *i007 = (interface_type_007_t *)ixxx;

   if(i007 == NULL) {
      return -1;
   }
   else {
      return i007->monitoring_id;
   }
}


int set_xPLCallback_interface_type_007(void *ixxx, xpl2_f cb)
{
   interface_type_007_t *i007 = (interface_type_007_t *)ixxx;

   if(i007 == NULL) {
      return -1;
   }
   else {
      i007->xPL_callback2 = cb;
      return 0;
   }
}


int set_monitoring_id_interface_type_007(void *ixxx, int id)
{
   interface_type_007_t *i007 = (interface_type_007_t *)ixxx;

   if(i007 == NULL) {
      return -1;
   }
   else {
      i007->monitoring_id = id;
      return 0;
   }
}


int get_type_interface_type_007()
{
   return INTERFACE_TYPE_007;
}


int get_interface_id_interface_type_007(void *ixxx)
{
   interface_type_007_t *i007 = (interface_type_007_t *)ixxx;

   if(i007 == NULL) {
      return -1;
   }
   else {
      return i007->id_interface;
   }
}


static int api_write_data_json(interface_type_007_t *i007, cJSON *args, cJSON **res, int16_t *nerr, char *err, int l_err)
{
   int16_t ret=0;
   
   if(args->type!=cJSON_Array || cJSON_GetArraySize(args)!=3) {
      return -255;
   }

   cJSON *arg=cJSON_GetArrayItem(args, 2);

   if(arg->type==cJSON_String || arg->type==cJSON_ByteArray) {
      int l=0;
      if(arg->type==cJSON_String) {
         l=(int)strlen(arg->valuestring);
      }
      else {
         l=arg->valueint;
      }
      if(i007->fd >=0) {
         ret=write(i007->fd, arg->valuestring, l);
         if(ret<0) {
            VERBOSE(5) {
               char err_str[256];
               strerror_r(errno, err_str, sizeof(err_str)-1);
               mea_log_printf("%s (%s) : write error - %s\n", ERROR_STR, __func__, err_str);
            }
            close(i007->fd);
            i007->fd=-1;
            return -253;
         }
         else {
            i007->indicators.serialout+=ret;
         }
      }
      else {
         return -254;
      }
   }
   else {
      return -255;
   }
   return 0;
}


void *pairing_interface_type_007(enum pairing_cmd_e cmd, void *context, void *parameters)
{
   interface_type_007_t *i007=(interface_type_007_t *)context;
   
   switch(cmd) {
      case PAIRING_CMD_ON:
         i007->pairing_state=1;
         i007->pairing_startms=mea_now();
         return cJSON_CreateTrue();

      case PAIRING_CMD_OFF:
         i007->pairing_state=0;
         i007->pairing_startms=-1.0;
         return cJSON_CreateTrue();

      case PAIRING_CMD_GETSTATE:
         return cJSON_CreateNumber((double)i007->pairing_state);

      default:
         return cJSON_CreateNull();
   }
   
   return NULL;
}


int16_t api_interface_type_007_json(void *ixxx, char *cmnd, void *args, int nb_args, void **res, int16_t *nerr, char *err, int l_err)
{
//   interface_type_007_t *i007 = (interface_type_007_t *)ixxx;
   cJSON *jsonArgs = (cJSON *)args;
   cJSON **jsonRes = (cJSON **)res;
   
   if(strcmp(cmnd, "mea_writeData") == 0) {
      int ret=api_write_data_json(ixxx, jsonArgs, jsonRes, nerr, err, l_err);
      if(ret<0) {
         strncpy(err, "error", l_err);
         return -1;
      }
      else {
         strncpy(err, "no error", l_err);
         *nerr=0;
         return 0;
      }
   }
   else if(strcmp(cmnd, "test") == 0)
   {
      *jsonRes = cJSON_CreateString("New style Api call OK !!!");
      *nerr=0;
      strncpy(err, "no error", l_err);

      return 0;
   }
   else {
      strncpy(err, "unknown function", l_err);

      return -254;
   }
}


interface_type_007_t *malloc_and_init2_interface_type_007(int id_driver, cJSON *jsonInterface)
{
   interface_type_007_t *i007;
                  
   i007=(interface_type_007_t *)malloc(sizeof(interface_type_007_t));
   if(!i007) {
      VERBOSE(2) {
        char err_str[256];
        strerror_r(errno, err_str, sizeof(err_str)-1);
        mea_log_printf("%s (%s) : %s - %s\n", ERROR_STR, __func__, MALLOC_ERROR_STR,err_str);
      }
      return NULL;
   }
   i007->thread_is_running=0;

   struct interface_type_007_data_s *i007_start_stop_params=(struct interface_type_007_data_s *)malloc(sizeof(struct interface_type_007_data_s));
   if(!i007_start_stop_params) {
      free(i007);
      i007=NULL;

      VERBOSE(2) {
         char err_str[256];
         strerror_r(errno, err_str, sizeof(err_str)-1);
         mea_log_printf("%s (%s) : %s - %s\n",ERROR_STR,__func__,MALLOC_ERROR_STR,err_str);
      }  
      return NULL;
   }

   int id_interface=(int)cJSON_GetObjectItem(jsonInterface,ID_INTERFACE_STR_C)->valuedouble;
   char *name=jsonInterface->string;
   char *dev=cJSON_GetObjectItem(jsonInterface,DEV_STR_C)->valuestring;
   char *parameters=cJSON_GetObjectItem(jsonInterface,PARAMETERS_STR_C)->valuestring;
   char *description=cJSON_GetObjectItem(jsonInterface,DESCRIPTION_STR_C)->valuestring;

   strncpy(i007->dev, (char *)dev, sizeof(i007->dev)-1);
   strncpy(i007->name, (char *)name, sizeof(i007->name)-1);
   i007->real_dev[0]=0;
   i007->real_speed=0;
   i007->id_interface=id_interface;
   i007->id_driver=id_driver;
   i007->parameters=(char *)malloc(strlen((char *)parameters)+1);
   if(i007->parameters) {
      strcpy(i007->parameters,(char *)parameters);
   }
   else {
      VERBOSE(2) {
         char err_str[256];
         strerror_r(errno, err_str, sizeof(err_str)-1);
         mea_log_printf("%s (%s) : %s - %s\n",ERROR_STR,__func__,MALLOC_ERROR_STR,err_str);
      }  
      goto clean_exit;
   }
   i007->indicators.xplin=0;
   i007->indicators.senttoplugin=0;
   i007->indicators.serialin=0;
   i007->indicators.serialout=0;
   
   i007->line_buffer = NULL;
   i007->line_buffer_l=0;
   i007->line_buffer_ptr=0;
   i007->fduration = 2000;
   i007->fendstr = NULL;
   i007->fstartstr = NULL;
   i007->fsize = 0;
   i007->pairing_state=0;
   i007->pairing_startms=-1;

   parsed_parameters_t *interface_params=NULL;
   int nb_interface_params=0;
   char buff[256];
   speed_t speed;
   int err=0;
   char err_str[128];

   int ret=get_dev_and_speed((char *)i007->dev, buff, sizeof(buff)-1, &speed);
   if(!ret) {
      i007->real_speed=(int)speed;
      int n=snprintf(i007->real_dev,sizeof(i007->real_dev)-1,"/dev/%s",buff);
      if(n<0 || n==(sizeof(i007->real_dev)-1)) {
         strerror_r(errno, err_str, sizeof(err_str));
         VERBOSE(2) {
            mea_log_printf("%s (%s) : snprintf - %s\n", ERROR_STR, __func__, err_str);
         }
         goto clean_exit;
      }
   }
   else {
      VERBOSE(2) {
         mea_log_printf("%s (%s) : incorrect device/speed interface - %s\n", ERROR_STR, __func__, i007_start_stop_params->i007->dev);
      }
      goto clean_exit;
   }

   interface_params=alloc_parsed_parameters(parameters, valid_interface_007_params, &nb_interface_params, &err, 0);
   if(!interface_params) {
      VERBOSE(5) {
         mea_log_printf("%s (%s) : alloc_parsed_parameters error\n", ERROR_STR, __func__);
      }
   }
   else {
      if(interface_params->parameters[INTERFACE_PARAMS_FDURATION].label) {
         if(interface_params->parameters[INTERFACE_PARAMS_FDURATION].value.i>0) {
            i007->fduration=interface_params->parameters[INTERFACE_PARAMS_FDURATION].value.i;
            if(i007->fduration<0)
               i007->fduration=0;
            if(i007->fduration>5000)
               i007->fduration=5000; // 5 secondes max
         }
      }

      if(interface_params->parameters[INTERFACE_PARAMS_FSTARTSTR].label) {
         int l=(int)strlen(interface_params->parameters[INTERFACE_PARAMS_FSTARTSTR].value.s);
         if(l>0) {
            i007->fstartstr=malloc(l+1);
            if(i007->fstartstr) {
               mea_unescape(i007->fstartstr, interface_params->parameters[INTERFACE_PARAMS_FSTARTSTR].value.s);
               i007->fstartstr = realloc(i007->fstartstr, strlen(i007->fstartstr)+1);
            }
         }
      }

      if(interface_params->parameters[INTERFACE_PARAMS_FENDSTR].label) {
         int l=(int)strlen(interface_params->parameters[INTERFACE_PARAMS_FENDSTR].value.s);
         if(l>0) {
            i007->fendstr=malloc(l+1);
            if(i007->fendstr) {
               mea_unescape(i007->fendstr, interface_params->parameters[INTERFACE_PARAMS_FENDSTR].value.s);
               i007->fendstr = realloc(i007->fendstr, strlen(i007->fendstr)+1);
            }
         }
      }

      if(interface_params->parameters[INTERFACE_PARAMS_PLUGIN].label) {
         init_interface_type_007_data_preprocessor(i007, interface_params->parameters[INTERFACE_PARAMS_PLUGIN].value.s, interface_params->parameters[INTERFACE_PARAMS_PLUGIN_PARAMETERS].value.s);
      }
      release_parsed_parameters(&interface_params);
   }

   DEBUG_SECTION {
      fprintf(stderr,"DEV       =%s\n", i007->dev);
      fprintf(stderr,"FSIZE     =%d\n", i007->fsize);
      fprintf(stderr,"FDURATION =%d\n", i007->fduration);
      fprintf(stderr,"FSTARTSTR =%s\n", i007->fstartstr);
      fprintf(stderr,"FENDSTR   =%s\n", i007->fendstr);
   }

   i007->thread=NULL;
   i007->xPL_callback2=NULL;
   i007->xPL_callback_data=NULL;
   i007->monitoring_id=process_register((char *)name);

   i007_start_stop_params->i007=i007;
                  
   process_set_group(i007->monitoring_id, 1);
   process_set_start_stop(i007->monitoring_id, start_interface_type_007, stop_interface_type_007, (void *)i007_start_stop_params, 1);
   process_set_watchdog_recovery(i007->monitoring_id, restart_interface_type_007, (void *)i007_start_stop_params);
   process_set_description(i007->monitoring_id, (char *)description);
   process_set_heartbeat_interval(i007->monitoring_id, 60); // chien de garde au bout de 60 secondes sans heartbeat

   process_add_indicator(i007->monitoring_id, interface_type_007_xplin_str, 0);
   process_add_indicator(i007->monitoring_id, interface_type_007_senttoplugin_str, 0);
   process_add_indicator(i007->monitoring_id, interface_type_007_serialin_str, 0);
   process_add_indicator(i007->monitoring_id, interface_type_007_serialout_str, 0);
   
   return i007;
   
clean_exit:
   if(i007) {
      if(i007->fstartstr) {
         free(i007->fstartstr);
         i007->fstartstr=NULL;
      }
      
      if(i007->fendstr) {
         free(i007->fendstr);
         i007->fendstr=NULL;
      }
      
      if(i007->parameters) {
         free(i007->parameters);
         i007->parameters=NULL;
      }
      
      if(i007_start_stop_params) {
         free(i007_start_stop_params);
         i007_start_stop_params=NULL;
      }
      free(i007);
      i007=NULL;
   }
   return NULL;
}


void *_thread_interface_type_007_cleanup(void *args)
{
   struct thread_params_s *params=(struct thread_params_s *)args;

   if(params) {
      free(params);
      params=NULL;
   }
   return NULL;
}


void *_thread_interface_type_007(void *args)
{
   int err_counter=0;
   struct thread_params_s *params=(struct thread_params_s *)args;
   
   pthread_cleanup_push( (void *)_thread_interface_type_007_cleanup, (void *)params );
   pthread_cleanup_push( (void *)set_interface_type_007_isnt_running, (void *)params->i007 );

   interface_type_007_t *i007=params->i007;
   i007->thread_is_running=1;
   process_heartbeat(i007->monitoring_id);

   cJSON *result=NULL;
   cJSON *jsonData=cJSON_CreateObject();
   if(jsonData) {
      cJSON_AddNumberToObject(jsonData, INTERFACE_ID_STR_C, i007->id_interface);
      if(i007->interface_plugin_parameters) {
         cJSON_AddStringToObject(jsonData, INTERFACE_PARAMETERS_STR_C, i007->interface_plugin_parameters);
      }
      result=plugin_call_function_json_alloc(i007->interface_plugin_name, "mea_init", jsonData);
      if(result) {
         cJSON_Delete(result);
      }
   }

   params->i007->fd=-1;
   while(1) {
      process_heartbeat(i007->monitoring_id);
      process_update_indicator(i007->monitoring_id, interface_type_007_xplin_str, i007->indicators.xplin);
      process_update_indicator(i007->monitoring_id, interface_type_007_senttoplugin_str, i007->indicators.senttoplugin);
      process_update_indicator(i007->monitoring_id, interface_type_007_serialin_str, i007->indicators.serialin);
      process_update_indicator(i007->monitoring_id, interface_type_007_serialout_str, i007->indicators.serialout);

      if(i007->fd<0) { // pas ou plus de communication avec le périphérique série
         i007->fd=serial_open(i007->real_dev, i007->real_speed);
      }

      if(i007->fd>=0) {
         while(1) {
            // traiter les données en provenance des périphériques
            if(process_interface_type_007_data(i007)<0) {
               break;
            }
            pthread_testcancel();
         }
      }
      else {
         VERBOSE(5) {
            char err_str[256];
            strerror_r(errno, err_str, sizeof(err_str)-1);
            mea_log_printf("%s (%s) : can't open %s - %s\n", ERROR_STR, __func__, i007->real_dev,err_str);
         }

         err_counter++;
         if(err_counter<5) {
            sleep(5);
         }
         else {
            goto _thread_interface_type_007_clean_exit;
         }
      }
   }

_thread_interface_type_007_clean_exit:   
   pthread_cleanup_pop(1);
   pthread_cleanup_pop(1);
}


pthread_t *start_interface_type_007_thread(interface_type_007_t *i007, void *fd, thread_f thread_function)
{
   pthread_t *thread=NULL;
   struct thread_params_s *thread_params=NULL;

   thread_params=malloc(sizeof(struct thread_params_s));
   if(!thread_params) {
      VERBOSE(2) {
         char err_str[256];
         strerror_r(errno, err_str, sizeof(err_str)-1);
         mea_log_printf("%s (%s) : %s - %s\n", ERROR_STR, __func__, MALLOC_ERROR_STR,err_str);
      }
      goto clean_exit;
   }

   thread_params->i007=(void *)i007;

   thread=(pthread_t *)malloc(sizeof(pthread_t));
   if(!thread) {
      VERBOSE(2) {
         mea_log_printf("%s (%s) : %s\n", ERROR_STR, __func__, MALLOC_ERROR_STR);
      }
      goto clean_exit;
   }

   if(pthread_create (thread, NULL, (void *)thread_function, (void *)thread_params))
      goto clean_exit;

   pthread_detach(*thread);

   return thread;

clean_exit:
   if(thread) {
      free(thread);
      thread=NULL;
   }

   if(thread_params) {
      free(thread_params);
      thread_params=NULL;
   }
   return NULL;
}


int stop_interface_type_007(int my_id, void *data, char *errmsg, int l_errmsg)
{
   if(!data)
      return -1;

   struct interface_type_007_data_s *start_stop_params=(struct interface_type_007_data_s *)data;
   interface_type_007_t *i007=start_stop_params->i007;

   VERBOSE(1) {
      mea_log_printf("%s (%s) : %s shutdown thread ... ", INFO_STR, __func__, i007->name);
   }

   if(i007->xPL_callback2) {
      i007->xPL_callback2=NULL;
   }
      
   if(i007->xPL_callback_data) {
      free(i007->xPL_callback_data);
      i007->xPL_callback_data=NULL;
   }

   clean_interface_type_007_data_source(i007);

   if(i007->thread) {
      pthread_cancel(*(i007->thread));

      int counter=100;
      while(counter--) {
         if(i007->thread_is_running) {
            usleep(100);
         }
         else
            break;
      }
      DEBUG_SECTION mea_log_printf("%s (%s) : %s, end after %d loop(s)\n", DEBUG_STR, __func__, i007->name, 100-counter);

      free(i007->thread);
      i007->thread=NULL;
   }

   return 0;
}


int restart_interface_type_007(int my_id, void *data, char *errmsg, int l_errmsg)
{
   process_stop(my_id, NULL, 0);
   sleep(5);
   return process_start(my_id, NULL, 0);
}


int start_interface_type_007(int my_id, void *data, char *errmsg, int l_errmsg)
{
   char err_str[256];
   struct callback_xpl_data_s *xpl_callback_params=NULL;
   struct interface_type_007_data_s *start_stop_params=(struct interface_type_007_data_s *)data;
   interface_type_007_t *i007=start_stop_params->i007;

   xpl_callback_params=(struct callback_xpl_data_s *)malloc(sizeof(struct callback_xpl_data_s));
   if(!xpl_callback_params) {
       strerror_r(errno, err_str, sizeof(err_str));
       VERBOSE(2) {
          mea_log_printf("%s (%s) : %s - %s\n", ERROR_STR, __func__, MALLOC_ERROR_STR, err_str);
       }
       goto clean_exit;
   }

   i007->thread=start_interface_type_007_thread(start_stop_params->i007, NULL, (thread_f)_thread_interface_type_007);

   i007->xPL_callback_data=xpl_callback_params;
   i007->xPL_callback2=_interface_type_007_xPL_callback2;

   VERBOSE(2) {
      mea_log_printf("%s (%s) : %s %s.\n", INFO_STR, __func__, i007->name, launched_successfully_str);
   }

   return 0;

clean_exit:
   if(i007->xPL_callback_data) {
      free(i007->xPL_callback_data);
      i007->xPL_callback_data=NULL;
   }

   clean_interface_type_007_data_source(i007);

   return -1; 
}


#ifndef ASPLUGIN
int get_fns_interface_type_007(struct interfacesServer_interfaceFns_s *interfacesFns)
{
   interfacesFns->malloc_and_init2 = (malloc_and_init2_f)&malloc_and_init2_interface_type_007;
   interfacesFns->get_monitoring_id = (get_monitoring_id_f)&get_monitoring_id_interface_type_007;
   interfacesFns->get_xPLCallback = (get_xPLCallback_f)&get_xPLCallback_interface_type_007;
   interfacesFns->update_devices = (update_devices_f)&update_devices_type_007;
   interfacesFns->clean = (clean_f)&clean_interface_type_007;
   interfacesFns->set_monitoring_id = (set_monitoring_id_f)&set_monitoring_id_interface_type_007;
   interfacesFns->set_xPLCallback = (set_xPLCallback_f)&set_xPLCallback_interface_type_007;
   interfacesFns->get_type = (get_type_f)&get_type_interface_type_007;
   interfacesFns->api = (api_f)&api_interface_type_007_json;
   interfacesFns->pairing = (pairing_f)&pairing_interface_type_007;
   interfacesFns->lib = NULL;
   interfacesFns->type = interfacesFns->get_type();
   interfacesFns->plugin_flag = 0;

   return 0;
}
#endif