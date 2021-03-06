//
//  interface_type_010.c
//  mea-edomus
//
//  Created by Patrice Dietsch on 21/02/2015.
//
//
#include "interface_type_010.h"

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
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>

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


char *interface_type_010_xplin_str="XPLIN";
char *interface_type_010_senttoplugin_str="SENTTOPLUGIN";

char *valid_interface_010_params[]={"S:FSTARTSTR","S:FENDSTR","I:FSIZE","I:FDURATION", "S:PLUGIN", "S:PLUGIN_PARAMETERS", "S:DIRECTION", "S:IN-EXT", "S:OUT-EXT", NULL};
#define INTERFACE_PARAMS_FSTARTSTR         0
#define INTERFACE_PARAMS_FENDSTR           1
#define INTERFACE_PARAMS_FSIZE             2
#define INTERFACE_PARAMS_FDURATION         3
#define INTERFACE_PARAMS_PLUGIN            4
#define INTERFACE_PARAMS_PLUGIN_PARAMETERS 5
#define INTERFACE_PARAMS_DIRECTION         6
#define INTERFACE_PARAMS_IN_EXT            7
#define INTERFACE_PARAMS_OUT_EXT           8

char *valid_plugin_010_params[]={"S:PLUGIN","S:PLUGIN_PARAMETERS", NULL};
#define PLUGIN_PARAMS_PLUGIN       0
#define PLUGIN_PARAMS_PARAMETERS   1


struct thread_params_s
{
   interface_type_010_t *i010;
};


struct callback_xpl_data_s 
{
};

typedef void (*thread_f)(void *);


int start_interface_type_010(int my_id, void *data, char *errmsg, int l_errmsg);
int stop_interface_type_010(int my_id, void *data, char *errmsg, int l_errmsg);
int restart_interface_type_010(int my_id, void *data, char *errmsg, int l_errmsg);


void set_interface_type_010_isnt_running(void *data)
{
   interface_type_010_t *i010 = (interface_type_010_t *)data;

   i010->thread_is_running=0;
}


int16_t _interface_type_010_xPL_callback2(cJSON *xplMsgJson, struct device_info_s *device_info, void *userValue)
{
   int err = 0;

   interface_type_010_t *i010=(interface_type_010_t *)userValue;
   i010->indicators.xplin++;

   parsed_parameters_t *plugin_params=NULL;
   int nb_plugin_params;

   plugin_params=alloc_parsed_parameters(device_info->parameters, valid_plugin_010_params, &nb_plugin_params, &err, 0);
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
   cJSON_AddNumberToObject(data, API_KEY_STR_C, (double)i010->id_interface);
   if(plugin_params->parameters[PLUGIN_PARAMS_PARAMETERS].value.s) {
      cJSON_AddStringToObject(data, DEVICE_PARAMETERS_STR_C, plugin_params->parameters[PLUGIN_PARAMS_PARAMETERS].value.s);
   }
//   python_cmd_json(plugin_params->parameters[PLUGIN_PARAMS_PLUGIN].value.s, XPLMSG_JSON, data);
   plugin_fireandforget_function_json(plugin_params->parameters[PLUGIN_PARAMS_PLUGIN].value.s, XPLMSG_JSON, data);

   release_parsed_parameters(&plugin_params);
   plugin_params=NULL;

   return 0;
}


static int init_interface_type_010_data_source_pipe(interface_type_010_t *i010)
{
   int ret = 0;

   i010->file_desc_in  = -1;
   i010->file_desc_out = -1;
   
   if(i010->direction == DIR_IN || i010->direction == DIR_BOTH) {
      char *fname = alloca(strlen(i010->file_name)+strlen(i010->in_ext)+1);
      sprintf(fname,"%s%s", i010->file_name, i010->in_ext);

      unlink(fname);

      ret = mkfifo(fname, 0666);
      if(ret != 0) {
         VERBOSE(5) {
            char err_str[256];
            strerror_r(errno, err_str, sizeof(err_str)-1);
            mea_log_printf("%s (%s) : mkfifo in - %s\n", ERROR_STR, __func__,err_str);
         }
      }
      else {
         int p=open(fname, O_RDWR | O_NONBLOCK);
         if(p==-1) {
            VERBOSE(5) {
               char err_str[256];
               strerror_r(errno, err_str, sizeof(err_str)-1);
               mea_log_printf("%s (%s) : open in - %s\n", ERROR_STR, __func__,err_str);
            }

            free(i010->line_buffer);
            i010->line_buffer = NULL;
            i010->line_buffer_l=0;

            return -1;
         }
         else {
            i010->file_desc_in=p; 
         }
      }
   }


   if(i010->direction == DIR_OUT || i010->direction == DIR_BOTH) {
      char *fname = alloca(strlen(i010->file_name)+strlen(i010->out_ext)+1);
      sprintf(fname,"%s%s", i010->file_name, i010->out_ext);

      unlink(fname);

      ret = mkfifo(fname, 0666);
      if(ret != 0) {
         VERBOSE(5) {
            char err_str[256];
            strerror_r(errno, err_str, sizeof(err_str)-1);
            mea_log_printf("%s (%s) : mkfifo out - %s\n", ERROR_STR, __func__,err_str);
         }
      }
      else {
         int p=open(fname, O_RDWR | O_NONBLOCK);
         if(p==-1) {
            VERBOSE(5) {
               char err_str[256];
               strerror_r(errno, err_str, sizeof(err_str)-1);
               mea_log_printf("%s (%s) : open out - %s\n", ERROR_STR, __func__,err_str);
            }

            if(i010->file_desc_in != -1) {
               close(i010->file_desc_in);
               i010->file_desc_in = -1;
            }

            free(i010->line_buffer);
            i010->line_buffer = NULL;
            i010->line_buffer_l=0;

            return -1;
         }
         else {
            i010->file_desc_out=p;
         }
      }
   }

   if(i010->file_desc_out == -1 && i010->file_desc_in == -1) {
      VERBOSE(5) {
         mea_log_printf("%s (%s) : no fifo opend", ERROR_STR, __func__);
      }
      return -1;
   }

   return 0;
}


static int init_interface_type_010_data_source(interface_type_010_t *i010)
{
   if(!i010->file_name)
      return -1;

   if(i010->line_buffer) {
      free(i010->line_buffer);
   }
   i010->line_buffer_l = INIT_LINE_BUFFER_SIZE;
   i010->line_buffer_ptr = 0;
   i010->line_buffer = (char *)malloc(i010->line_buffer_l);
   if(!i010->line_buffer) {
      return -1;
   }

   i010->file_desc_in = -1;
   i010->file_desc_out = -1;

   switch(i010->file_type) {
      case FT_PIPE:
      {
         int p=init_interface_type_010_data_source_pipe(i010);
         if(p>=0) {
            return 0;
         }
         else {
            return -1;
         }
      }
      default:
         return -1;
   }
}


int init_interface_type_010_data_preprocessor(interface_type_010_t *i010, char *plugin_name, char *plugin_parameters)
{
   if(!plugin_name) {
      i010->interface_plugin_name=NULL;
      return -1;
   }
      
   i010->interface_plugin_name=malloc(strlen(plugin_name)+1);
   if(i010->interface_plugin_name) {
      strcpy(i010->interface_plugin_name, plugin_name);
   }
   
   if(plugin_parameters) {
      i010->interface_plugin_parameters=malloc(strlen(plugin_parameters)+1);
      if(i010->interface_plugin_parameters) {
         strcpy(i010->interface_plugin_parameters, plugin_parameters);
      }
   }
   else {
      i010->interface_plugin_parameters=NULL;
   }

   return 0;
}


int interface_type_010_data_preprocessor(interface_type_010_t *i010)
{
   if(i010->interface_plugin_name) {
      cJSON *data=cJSON_CreateObject();
      if(data) {
         cJSON_AddItemToObject(data, DATA_STR_C, cJSON_CreateByteArray((char *)i010->line_buffer, i010->line_buffer_ptr));
         cJSON_AddNumberToObject(data, L_DATA_STR_C, (double)i010->line_buffer_ptr);
         cJSON_AddNumberToObject(data, INTERFACE_ID_STR_C, (double)i010->id_interface);
         cJSON_AddNumberToObject(data, API_KEY_STR_C, (double)i010->id_interface);
         if(i010->interface_plugin_parameters) {
            cJSON_AddStringToObject(data, PLUGIN_PARAMETERS_STR_C, i010->interface_plugin_parameters);
         }
      }
//      cJSON *result=python_call_function_json_alloc(i010->interface_plugin_name, "mea_dataPreprocessor", data);
      cJSON *result=plugin_call_function_json_alloc(i010->interface_plugin_name, "mea_dataPreprocessor", data);
      
      if(result) {
         cJSON_Delete(result);
         result=NULL;
      }
   }

   return 0;
}


int interface_type_010_pairing(interface_type_010_t *i010)
{
   int ret=-1;
   char *data=NULL;
   cJSON *j=NULL, *_j=NULL;

   VERBOSE(5) {
      mea_log_printf("%s (%s) : ***** pairing data (%s) ? *****\n", INFO_STR, __func__, data);
   }
   
   char name[256];
   if(parsed_parameters_get_param_string(i010->parameters, valid_interface_010_params, INTERFACE_PARAMS_PLUGIN, name, sizeof(name)-1)==0) {
      name[sizeof(name)-1]=0;
      j=cJSON_CreateObject();
      cJSON_AddItemToObject(j, DATA_STR_C, cJSON_CreateByteArray((char *)i010->line_buffer, i010->line_buffer_ptr));
      cJSON_AddNumberToObject(j, L_DATA_STR_C, (double)i010->line_buffer_ptr);
//      _j=python_call_function_json_alloc(name, "pairing_get_devices", j);
      _j=plugin_call_function_json_alloc(name, "pairing_get_devices", j);

      VERBOSE(5) {
         char *s=cJSON_Print(_j);
         mea_log_printf("%s (%s) : pairing result=%s\n", ERROR_STR, __func__, s);
         free(s);
      }
      
      if(_j && _j->type!=cJSON_NULL) {
         int _ret=addDevice(i010->name, _j);
         if(_ret==0) {
            VERBOSE(5) mea_log_printf("%s (%s) : pairing done\n", ERROR_STR, __func__);
         }
         else {
            VERBOSE(5) mea_log_printf("%s (%s) : pairing error (%d)\n", ERROR_STR, __func__, _ret);
         }
         i010->pairing_state=0;
         i010->pairing_startms=-1.0;
      }
      else {
         VERBOSE(5) mea_log_printf("%s (%s) : no pairing data\n", ERROR_STR, __func__);
      }

      if(_j) {
         cJSON_Delete(_j);
      }

      return 0;
   }

   i010->pairing_state=0;
   i010->pairing_startms=-1.0;

   return ret;
}


static int _interface_type_010_data_to_plugin(interface_type_010_t *i010,  struct device_info_s *device_info)
{
   int err = 0;

/* TO TEST
   cJSON *plugin_params=parsed_parameters_json_alloc((char *)device_info->parameters, valid_plugin_010_params, &nb_plugin_params, &err, 0);
   if(!plugin_params) {
      return -1;
   }
   
   cJSON *_plugin=cJSON_GetObjectItem(plugin_params, "PLUGIN");
   if(!plugin || plugin->type!=cJSON_String) {
      return -1;
   }
   
   char *plugin=_plugin->valuestring;
   char *plugin_parameters=NULL;
   
   cJSON *_parameters=cJSON_GetObjectItem(plugin_params, "PLUGIN_PARAMETERS");
   if(_parameters && _parameters->type==cJSON_String) {
      plugin_parameters=_parameters->valuestring;
   }
*/
   parsed_parameters_t *plugin_params=NULL;
   int nb_plugin_params = -1;

   plugin_params=alloc_parsed_parameters((char *)device_info->parameters, valid_plugin_010_params, &nb_plugin_params, &err, 0);
   if(!plugin_params || !plugin_params->parameters[PLUGIN_PARAMS_PLUGIN].value.s) {
      if(plugin_params) {
         release_parsed_parameters(&plugin_params);
      }
      return -1;
   }

   cJSON *data = device_info_to_json_alloc(device_info);
   cJSON_AddNumberToObject(data,API_KEY_STR_C,(double)i010->id_interface);
   cJSON_AddNumberToObject(data,DEVICE_TYPE_ID_STR_C,(double)device_info->id);
/* TO TEST
   if(_parameters) {
      cJSON_AddStringToObject(data, DEVICE_PARAMETERS_STR_C, _parameters);
   }
*/
   if(plugin_params->parameters[PLUGIN_PARAMS_PARAMETERS].value.s) {
      cJSON_AddStringToObject(data, DEVICE_PARAMETERS_STR_C, plugin_params->parameters[PLUGIN_PARAMS_PARAMETERS].value.s);
   }
   cJSON_AddItemToObject(data,DATA_STR_C,cJSON_CreateByteArray(i010->line_buffer, i010->line_buffer_ptr));
   cJSON_AddNumberToObject(data,L_DATA_STR_C, (double)i010->line_buffer_ptr);
/* TO TEST
   plugin_fireandforget_function_json(plugin, DATAFROMSENSOR_JSON, data);
*/
   plugin_fireandforget_function_json(plugin_params->parameters[PLUGIN_PARAMS_PLUGIN].value.s, DATAFROMSENSOR_JSON, data);
   
   i010->indicators.senttoplugin++;

/* TO TEST
   cJSON_Delete(plugin_params);
*/
   release_parsed_parameters(&plugin_params);
   
   return 0;
}


static int interface_type_010_data_to_plugin(interface_type_010_t *i010)
{
   int ret=0;
   
   if(i010->pairing_state==1) {
      int ret=interface_type_010_pairing(i010);
      if(ret>0) {
         return 0;
      }
   }

   ret=interface_type_010_data_preprocessor(i010);
   if(ret==-1) {
      return -1;
   }
   
   if(ret==1) {
   }

   cJSON *jsonInterface = getInterfaceById_alloc(i010->id_interface);
   if(jsonInterface) {
      cJSON *jsonDevices = cJSON_GetObjectItem(jsonInterface, DEVICES_STR_C);
      if(jsonDevices) {
         cJSON *jsonDevice = jsonDevices->child;
         while(jsonDevice) {
            struct device_info_s device_info;
            ret=device_info_from_json(&device_info, jsonDevice, jsonInterface, NULL);
            if(ret==0) {
               _interface_type_010_data_to_plugin(i010, &device_info);
            }

            jsonDevice=jsonDevice->next;
         }
      }
      cJSON_Delete(jsonInterface);
   } 

   return 0;
}


static int haveFrameStartStr(interface_type_010_t *i010)
{
   if(!i010->fstartstr || !i010->fstartstr[0]) {
      return -1;
   }
   int l=(int)strlen(i010->fstartstr);
   char *s=&(i010->line_buffer[i010->line_buffer_ptr-l]);
   if(strncmp(s, i010->fstartstr, l) == 0) {
      return 0;
   }
   else {
      return -1;
   }
}


static int haveFrameEndStr(interface_type_010_t *i010)
{
   if(!i010->fendstr || !i010->fendstr[0]) {
      return -1;
   }

   int l=(int)strlen(i010->fendstr);
   char *s=&(i010->line_buffer[i010->line_buffer_ptr-l]);
   if(strncmp(s, i010->fendstr, l) == 0) {
      return l;
   }
   else {
      return -1;
   }
}


static int process_interface_type_010_data(interface_type_010_t *i010)
{
   if(i010->file_desc_in == -1) {
      return -1;
   }

   fd_set set;
   struct timeval timeout={0,0};

   mea_timer_t timer = MEA_TIMER_S_INIT;
   mea_init_timer(&timer, 10, 1);
   mea_start_timer(&timer);

   /* Initialize the file descriptor set. */
   FD_ZERO(&set);
   FD_SET(i010->file_desc_in, &set);
   int ret = 0;
   do {
      if(mea_test_timer(&timer)==0) {
         break;
      }

      if(i010->pairing_state==1) {
         if(mea_now()-i010->pairing_startms>60.0*1000.0) {
            i010->pairing_state=0;
            i010->pairing_startms=-1.0;
         }
      }

      /* Initialize the timeout data structure. */
      if(i010->fduration <= 0) {
         timeout.tv_sec = 5;
         timeout.tv_usec = 0;
      }
      else {
         timeout.tv_sec = i010->fduration / 1000;
         timeout.tv_usec = (i010->fduration % 1000)*1000;
      }

      ret = select(FD_SETSIZE, &set, NULL, NULL, &timeout);
      
      if(ret == 0) {
         if((i010->fduration > 0) && (i010->line_buffer_ptr)) {
            i010->line_buffer[i010->line_buffer_ptr]=0;
            interface_type_010_data_to_plugin(i010);
            i010->line_buffer_ptr=0;
         }
         
         break;
      }
      else if(ret < 0) {
         // erreur de select
         VERBOSE(5) {
            char err_str[256];
            strerror_r(errno, err_str, sizeof(err_str)-1);
            mea_log_printf("%s (%s) : select error - %s\n", ERROR_STR, __func__,err_str);
         }
         break;
      }
      else {
         char c=0;
         int r=(int)read(i010->file_desc_in, &c, 1);
         if(r<1) {
            if(r<0) {
               VERBOSE(5) {
                  char err_str[256];
                  strerror_r(errno, err_str, sizeof(err_str)-1);
                  mea_log_printf("%s (%s) : select error - %s\n", ERROR_STR, __func__,err_str);
               }
            }
            break;
         }

         i010->line_buffer[i010->line_buffer_ptr++]=c;
         if(i010->line_buffer_ptr>=i010->line_buffer_l) {
            char *tmp = realloc(i010->line_buffer, i010->line_buffer_l + INC_LINE_BUFFER_SIZE);
            if(!tmp) {
               i010->line_buffer_ptr = 0;
               VERBOSE(5) {
                  char err_str[256];
                  strerror_r(errno, err_str, sizeof(err_str)-1);
                  mea_log_printf("%s (%s) : realloc error - %s\n", ERROR_STR, __func__,err_str);
               }
               break;
            }
            else {
               i010->line_buffer = tmp;
               i010->line_buffer_l+=INC_LINE_BUFFER_SIZE;
            }
         }

         if(i010->fsize>0 && i010->line_buffer_ptr==i010->fsize) {
            i010->line_buffer[i010->line_buffer_ptr]=0;
            
            int b = haveFrameEndStr(i010);
            if(b>0) {
               i010->line_buffer_ptr-=b;
               i010->line_buffer[i010->line_buffer_ptr]=0;
            }

            interface_type_010_data_to_plugin(i010);
            i010->line_buffer_ptr=0;
            break;
         }

         if(i010->fendstr && i010->fendstr[0]) {
            int b = 0;
            b = haveFrameEndStr(i010);
            if(b>0) {
               i010->line_buffer_ptr-=b;
               i010->line_buffer[i010->line_buffer_ptr]=0;
               interface_type_010_data_to_plugin(i010);
               i010->line_buffer_ptr=0;
               break;
            }
         }

         if(i010->fstartstr && i010->fstartstr[0] && haveFrameStartStr(i010)==0) {
            i010->line_buffer_ptr=0;
            break;
         }
      }
   }
   while(ret);
   
   return 0;
}


int update_devices_type_010(void *ixxx)
{
   mea_log_printf("%s (%s) : update devices type 010: nothing to do\n", ERROR_STR, __func__);

   return 0;
}


static int clean_interface_type_010_data_source(interface_type_010_t *i010)
{
   if(i010->file_desc_in >= 0) {
      close(i010->file_desc_in);
   }
   char *fname = alloca(strlen(i010->file_name)+strlen(i010->in_ext)+1);
   sprintf(fname,"%s%s", i010->file_name, i010->in_ext);
   unlink(fname);

   if(i010->file_desc_out >= 0) {
      close(i010->file_desc_out);
   }
   fname = alloca(strlen(i010->file_name)+strlen(i010->out_ext)+1);
   sprintf(fname,"%s%s", i010->file_name, i010->out_ext);
   unlink(fname);

   i010->line_buffer_l = 0;
   i010->line_buffer_ptr = 0;
   if(i010->line_buffer) {
      free(i010->line_buffer);
   }
   i010->line_buffer = NULL;

   return 0;
}


int clean_interface_type_010(void *ixxx)
{
   interface_type_010_t *i010 = (interface_type_010_t *)ixxx;

   if(i010->parameters) {
      free(i010->parameters);
      i010->parameters=NULL;
   }

   if(i010->xPL_callback2) {
      i010->xPL_callback2=NULL;
   }

   if(i010->thread) {
      free(i010->thread);
      i010->thread=NULL;
   }

   if(i010->file_name) {
      free(i010->file_name);
      i010->file_name=NULL;
   }

   if(i010->in_ext) {
      free(i010->in_ext);
      i010->in_ext=NULL;
   }

   if(i010->out_ext) {
      free(i010->out_ext);
      i010->out_ext=NULL;
   }

   if(i010->fstartstr) {
      free(i010->fstartstr);
      i010->fstartstr=NULL;
   }

   if(i010->fendstr) {
      free(i010->fendstr);
      i010->fendstr=NULL;
   }

   if(i010->interface_plugin_name) {
      free(i010->interface_plugin_name);
      i010->interface_plugin_name=NULL;
   }

   if(i010->interface_plugin_parameters) {
      free(i010->interface_plugin_parameters);
      i010->interface_plugin_parameters=NULL;
   }

   return 0;
}


xpl2_f get_xPLCallback_interface_type_010(void *ixxx)
{
   interface_type_010_t *i010 = (interface_type_010_t *)ixxx;

   if(i010 == NULL) {
      return NULL;
   }
   else {
      return i010->xPL_callback2;
   }
}


int get_monitoring_id_interface_type_010(void *ixxx)
{
   interface_type_010_t *i010 = (interface_type_010_t *)ixxx;

   if(i010 == NULL) {
      return -1;
   }
   else {
      return i010->monitoring_id;
   }
}


int set_xPLCallback_interface_type_010(void *ixxx, xpl2_f cb)
{
   interface_type_010_t *i010 = (interface_type_010_t *)ixxx;

   if(i010 == NULL) {
      return -1;
   }
   else {
      i010->xPL_callback2 = cb;
      return 0;
   }
}


int set_monitoring_id_interface_type_010(void *ixxx, int id)
{
   interface_type_010_t *i010 = (interface_type_010_t *)ixxx;

   if(i010 == NULL) {
      return -1;
   }
   else {
      i010->monitoring_id = id;
      return 0;
   }
}


int get_type_interface_type_010()
{
   return INTERFACE_TYPE_010;
}


int get_interface_id_interface_type_010(void *ixxx)
{
   interface_type_010_t *i010 = (interface_type_010_t *)ixxx;

   if(i010 == NULL) {
      return -1;
   }
   else {
      return i010->id_interface;
   }
}


static int api_write_data_json(interface_type_010_t *ixxx, cJSON *args, cJSON **res, int16_t *nerr, char *err, int l_err)
{
   int16_t ret=0;
   
   if(ixxx->file_desc_out == -1) {
      *nerr=253;
      return -253;
   }

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
      ret=write(ixxx->file_desc_out, arg->valuestring, l);
   }
   else {
      return -255;
   }
   return 0;
}


void *pairing_interface_type_010(enum pairing_cmd_e cmd, void *context, void *parameters)
{
   interface_type_010_t *i010=(interface_type_010_t *)context;
   
   switch(cmd) {
      case PAIRING_CMD_ON:
         i010->pairing_state=1;
         i010->pairing_startms=mea_now();
         return cJSON_CreateTrue();

      case PAIRING_CMD_OFF:
         i010->pairing_state=0;
         i010->pairing_startms=-1.0;
         return cJSON_CreateTrue();

      case PAIRING_CMD_GETSTATE:
         return cJSON_CreateNumber((double)i010->pairing_state);

      default:
         return cJSON_CreateNull();
   }
   
   return NULL;
}


int16_t api_interface_type_010_json(void *ixxx, char *cmnd, void *args, int nb_args, void **res, int16_t *nerr, char *err, int l_err)
{
//   interface_type_010_t *i010 = (interface_type_010_t *)ixxx;
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


interface_type_010_t *malloc_and_init2_interface_type_010(int id_driver, cJSON *jsonInterface)
{
   interface_type_010_t *i010;
                  
   i010=(interface_type_010_t *)malloc(sizeof(interface_type_010_t));
   if(!i010) {
      VERBOSE(2) {
         char err_str[256];
         strerror_r(errno, err_str, sizeof(err_str)-1);
         mea_log_printf("%s (%s) : %s - %s\n", ERROR_STR, __func__, MALLOC_ERROR_STR,err_str);
      }
      return NULL;
   }
   i010->thread_is_running=0;
   i010->file_name = NULL;

                  
   struct interface_type_010_data_s *i010_start_stop_params=(struct interface_type_010_data_s *)malloc(sizeof(struct interface_type_010_data_s));
   if(!i010_start_stop_params) {
      free(i010);
      i010=NULL;

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

   strncpy(i010->dev, (char *)dev, sizeof(i010->dev)-1);
   strncpy(i010->name, (char *)name, sizeof(i010->name)-1);
   i010->id_interface=id_interface;
   i010->id_driver=id_driver;
   i010->parameters=(char *)malloc(strlen((char *)parameters)+1);
   if(i010->parameters) {
      strcpy(i010->parameters,(char *)parameters);
   }
   i010->indicators.xplin=0;
   i010->indicators.senttoplugin=0;

   i010->line_buffer = NULL;
   i010->line_buffer_l=0;
   i010->line_buffer_ptr=0;
   i010->fduration = 2000;
   i010->fendstr = NULL;
   i010->fstartstr = NULL;
   i010->fsize = 0;
   i010->direction = DIR_IN;
   i010->in_ext = malloc(4);
   if(i010->in_ext) {
      strcpy(i010->in_ext,"-in");
   }
   i010->out_ext = malloc(5);
   if(i010->out_ext)
      strcpy(i010->out_ext,"-out");
   i010->pairing_state=0;
   i010->pairing_startms=-1;

   parsed_parameters_t *interface_params=NULL;
   int nb_interface_params;
   int err;
   interface_params=alloc_parsed_parameters(parameters, valid_interface_010_params, &nb_interface_params, &err, 0);
   if(!interface_params) {
      VERBOSE(5) {
         mea_log_printf("%s (%s) : alloc_parsed_parameters error\n", ERROR_STR, __func__);
      }
   }
   else {
      if(interface_params->parameters[INTERFACE_PARAMS_IN_EXT].label) {
         char *tmp = realloc(i010->in_ext, strlen(interface_params->parameters[INTERFACE_PARAMS_IN_EXT].value.s)+1);
         if(tmp) {
            i010->in_ext=tmp;
            strcpy(i010->in_ext, interface_params->parameters[INTERFACE_PARAMS_IN_EXT].value.s);
         } 
      }

      if(interface_params->parameters[INTERFACE_PARAMS_OUT_EXT].label) {
         char *tmp = realloc(i010->out_ext, strlen(interface_params->parameters[INTERFACE_PARAMS_OUT_EXT].value.s)+1);
         if(tmp) {
            i010->out_ext=tmp;
            strcpy(i010->out_ext, interface_params->parameters[INTERFACE_PARAMS_OUT_EXT].value.s);
         } 
      }

      if(interface_params->parameters[INTERFACE_PARAMS_FSIZE].label) {
         if(interface_params->parameters[INTERFACE_PARAMS_FSIZE].value.i>0) {
            i010->fsize=interface_params->parameters[INTERFACE_PARAMS_FSIZE].value.i;
         }
      }

      if(interface_params->parameters[INTERFACE_PARAMS_DIRECTION].label) {
         if(mea_strcmplower(interface_params->parameters[INTERFACE_PARAMS_DIRECTION].value.s, "IN")==0) {
            i010->direction=DIR_IN;
         }
         else if(mea_strcmplower(interface_params->parameters[INTERFACE_PARAMS_DIRECTION].value.s, "OUT")==0) {
            i010->direction=DIR_OUT;
         }
         else if(mea_strcmplower(interface_params->parameters[INTERFACE_PARAMS_DIRECTION].value.s, "BOTH")==0) {
            i010->direction=DIR_BOTH;
         }
         else {
            VERBOSE(5) {
               mea_log_printf("%s (%s) : direction parameter error - \"%s\" unknown and not used\n", ERROR_STR, __func__, interface_params->parameters[INTERFACE_PARAMS_DIRECTION].value.s);
            }
         }
      }

      if(interface_params->parameters[INTERFACE_PARAMS_FDURATION].label) {
         if(interface_params->parameters[INTERFACE_PARAMS_FDURATION].value.i>0) {
            i010->fduration=interface_params->parameters[INTERFACE_PARAMS_FDURATION].value.i;
            if(i010->fduration<0) {
               i010->fduration=0;
            }
            if(i010->fduration>5000) {
               i010->fduration=5000; // 5 secondes max
            }
         }
      }

      if(interface_params->parameters[INTERFACE_PARAMS_FSTARTSTR].label) {
         int l=(int)strlen(interface_params->parameters[INTERFACE_PARAMS_FSTARTSTR].value.s);
         if(l>0) {
            i010->fstartstr=malloc(l+1);
            if(i010->fstartstr) {
               mea_unescape(i010->fstartstr, interface_params->parameters[INTERFACE_PARAMS_FSTARTSTR].value.s);
               i010->fstartstr = realloc(i010->fstartstr, strlen(i010->fstartstr)+1);
            }
         }
      }

      if(interface_params->parameters[INTERFACE_PARAMS_FENDSTR].label) {
         int l=(int)strlen(interface_params->parameters[INTERFACE_PARAMS_FENDSTR].value.s);
         if(l>0) {
            i010->fendstr=malloc(l+1);
            if(i010->fendstr) {
               mea_unescape(i010->fendstr, interface_params->parameters[INTERFACE_PARAMS_FENDSTR].value.s);
               i010->fendstr = realloc(i010->fendstr, strlen(i010->fendstr)+1);
            }
         }
      }


      if(interface_params->parameters[INTERFACE_PARAMS_PLUGIN].label) {
         init_interface_type_010_data_preprocessor(i010, interface_params->parameters[INTERFACE_PARAMS_PLUGIN].value.s, interface_params->parameters[INTERFACE_PARAMS_PLUGIN_PARAMETERS].value.s);
      }
      release_parsed_parameters(&interface_params);
   }


   char *tmpstr1=alloca(strlen(dev)+1);
   char *tmpstr2=alloca(strlen(dev)+1);
   i010->file_type=0;
   int n=sscanf(i010->dev, "%[^:]://%[^\n]", tmpstr1, tmpstr2);
   if(n==2) {
      i010->file_name=(char *)malloc(strlen(tmpstr2)+1+1);
      if(i010->file_name) {
         if(strcmp(tmpstr1,"PIPE")==0) {
            sprintf(i010->file_name, "/%s", tmpstr2);
            i010->file_type=FT_PIPE;
         }
      }
   }
/*
   DEBUG_SECTION {
      fprintf(stderr,"DEV       =%s\n", i010->dev);
      fprintf(stderr,"FSIZE     =%d\n", i010->fsize);
      fprintf(stderr,"FDURATION =%d\n", i010->fduration);
      fprintf(stderr,"FSTARTSTR =%s\n", i010->fstartstr);
      fprintf(stderr,"FENDSTR   =%s\n", i010->fendstr);
      fprintf(stderr,"FILE_NAME =%s (%d)\n", i010->file_name, n);
      fprintf(stderr,"FILE_TYPE =%d\n", i010->file_type);
   }
*/
   i010->thread=NULL;
   i010->xPL_callback2=NULL;
   i010->xPL_callback_data=NULL;
   i010->monitoring_id=process_register((char *)name);

   i010_start_stop_params->i010=i010;
                  
   process_set_group(i010->monitoring_id, 1);
   process_set_start_stop(i010->monitoring_id, start_interface_type_010, stop_interface_type_010, (void *)i010_start_stop_params, 1);
   process_set_watchdog_recovery(i010->monitoring_id, restart_interface_type_010, (void *)i010_start_stop_params);
   process_set_description(i010->monitoring_id, (char *)description);
   process_set_heartbeat_interval(i010->monitoring_id, 60); // chien de garde au bout de 60 secondes sans heartbeat

   process_add_indicator(i010->monitoring_id, interface_type_010_xplin_str, 0);
   process_add_indicator(i010->monitoring_id, interface_type_010_senttoplugin_str, 0);

   return i010;
}


void *_thread_interface_type_010_cleanup(void *args)
{
   struct thread_params_s *params=(struct thread_params_s *)args;

   if(params) {
      free(params);
      params=NULL;
   }
   return NULL;
}


void *_thread_interface_type_010(void *args)
{
   struct thread_params_s *params=(struct thread_params_s *)args;

   pthread_cleanup_push( (void *)_thread_interface_type_010_cleanup, (void *)params );
   pthread_cleanup_push( (void *)set_interface_type_010_isnt_running, (void *)params->i010 );

   params->i010->thread_is_running=1;
   process_heartbeat(params->i010->monitoring_id);

   cJSON *result=NULL;
   cJSON *jsonData=cJSON_CreateObject();
   if(jsonData) {
      cJSON_AddNumberToObject(jsonData, INTERFACE_ID_STR_C, params->i010->id_interface);
      if(params->i010->interface_plugin_parameters) {
         cJSON_AddStringToObject(jsonData, INTERFACE_PARAMETERS_STR_C, params->i010->interface_plugin_parameters);
      }
      result=plugin_call_function_json_alloc(params->i010->interface_plugin_name, "mea_init", jsonData);
      if(result) {
         cJSON_Delete(result);
      }
   }

   while(1)
   {
      process_heartbeat(params->i010->monitoring_id);
      process_update_indicator(params->i010->monitoring_id, interface_type_010_xplin_str, params->i010->indicators.xplin);
      process_update_indicator(params->i010->monitoring_id, interface_type_010_senttoplugin_str, params->i010->indicators.senttoplugin);

      // traiter les données en provenance des périphériques
      if(params->i010->file_desc_in != -1) {
         process_interface_type_010_data(params->i010);
      }
      else {
         sleep(1);
      }

      pthread_testcancel();
   }

   pthread_cleanup_pop(1);
   pthread_cleanup_pop(1);
   
   return NULL;
}


pthread_t *start_interface_type_010_thread(interface_type_010_t *i010, void *fd, thread_f thread_function)
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

   thread_params->i010=(void *)i010;

   thread=(pthread_t *)malloc(sizeof(pthread_t));
   if(!thread) {
      VERBOSE(2) mea_log_printf("%s (%s) : %s\n", ERROR_STR, __func__, MALLOC_ERROR_STR);
      goto clean_exit;
   }

   if(pthread_create (thread, NULL, (void *)thread_function, (void *)thread_params)) {
      goto clean_exit;
   }

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


int stop_interface_type_010(int my_id, void *data, char *errmsg, int l_errmsg)
{
   if(!data) {
      return -1;
   }

   struct interface_type_010_data_s *start_stop_params=(struct interface_type_010_data_s *)data;
   interface_type_010_t *i010=start_stop_params->i010;

   VERBOSE(1) mea_log_printf("%s (%s) : %s shutdown thread ... ", INFO_STR, __func__, start_stop_params->i010->name);

   if(i010->xPL_callback2)
      i010->xPL_callback2=NULL;
      
   if(i010->xPL_callback_data) {
      free(i010->xPL_callback_data);
      i010->xPL_callback_data=NULL;
   }

   clean_interface_type_010_data_source(i010);

   int ret=-1;
   if(i010->thread) {
      pthread_cancel(*(i010->thread));

      int counter=100;
      while(counter--) {
         if(i010->thread_is_running) {
            usleep(100);
         }
         else {
            ret=0;
            break;
         }
      }
      DEBUG_SECTION mea_log_printf("%s (%s) : %s, end after %d loop(s)\n", DEBUG_STR, __func__, i010->name, 100-counter);

      free(i010->thread);
      i010->thread=NULL;
   }

   if(ret==0) {
      VERBOSE(2) mea_log_printf("%s (%s) : %s %s.\n", INFO_STR, __func__, i010->name, stopped_successfully_str);
   }
   else {
      VERBOSE(2) mea_log_printf("%s (%s) : %s can't cancel thread.\n", INFO_STR, __func__, i010->name);
   }
   return ret;
}


int restart_interface_type_010(int my_id, void *data, char *errmsg, int l_errmsg)
{
   process_stop(my_id, NULL, 0);
   sleep(5);
   return process_start(my_id, NULL, 0);
}


int start_interface_type_010(int my_id, void *data, char *errmsg, int l_errmsg)
{
   char err_str[128];
   struct callback_xpl_data_s *xpl_callback_params=NULL;
   struct interface_type_010_data_s *start_stop_params=(struct interface_type_010_data_s *)data;

   if(init_interface_type_010_data_source(start_stop_params->i010)<0) {
      VERBOSE(1) mea_log_printf("%s (%s) : cant'open start interface %s, initialisation error", INFO_STR, __func__, start_stop_params->i010->name);
      return -1;
   }

   xpl_callback_params=(struct callback_xpl_data_s *)malloc(sizeof(struct callback_xpl_data_s));
   if(!xpl_callback_params) {
       strerror_r(errno, err_str, sizeof(err_str));
       VERBOSE(2) {
          mea_log_printf("%s (%s) : %s - %s\n", ERROR_STR, __func__, MALLOC_ERROR_STR, err_str);
       }
       goto clean_exit;
   }

   start_stop_params->i010->thread=start_interface_type_010_thread(start_stop_params->i010, NULL, (thread_f)_thread_interface_type_010);

   start_stop_params->i010->xPL_callback_data=xpl_callback_params;
   start_stop_params->i010->xPL_callback2=_interface_type_010_xPL_callback2;

   VERBOSE(2) mea_log_printf("%s (%s) : %s %s.\n", INFO_STR, __func__, start_stop_params->i010->name, launched_successfully_str);

   return 0;

clean_exit:
   if(start_stop_params->i010->xPL_callback_data) {
      free(start_stop_params->i010->xPL_callback_data);
      start_stop_params->i010->xPL_callback_data=NULL;
   }

   clean_interface_type_010_data_source(start_stop_params->i010);

   return -1; 
}


#ifndef ASPLUGIN
int get_fns_interface_type_010(struct interfacesServer_interfaceFns_s *interfacesFns)
{
   interfacesFns->malloc_and_init2 = (malloc_and_init2_f)&malloc_and_init2_interface_type_010;
   interfacesFns->get_monitoring_id = (get_monitoring_id_f)&get_monitoring_id_interface_type_010;
   interfacesFns->get_xPLCallback = (get_xPLCallback_f)&get_xPLCallback_interface_type_010;
   interfacesFns->update_devices = (update_devices_f)&update_devices_type_010;
   interfacesFns->clean = (clean_f)&clean_interface_type_010;
   interfacesFns->set_monitoring_id = (set_monitoring_id_f)&set_monitoring_id_interface_type_010;
   interfacesFns->set_xPLCallback = (set_xPLCallback_f)&set_xPLCallback_interface_type_010;
   interfacesFns->get_type = (get_type_f)&get_type_interface_type_010;
   interfacesFns->api = (api_f)&api_interface_type_010_json
   ;
   interfacesFns->pairing = (pairing_f)&pairing_interface_type_010;
   interfacesFns->lib = NULL;
   interfacesFns->type = interfacesFns->get_type();
   interfacesFns->plugin_flag = 0;

   return 0;
}
#endif
