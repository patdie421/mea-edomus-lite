//
// mea_plugins_utils.c
//  mea-eDomus
//
//  Created by Patrice Dietsch on 04/06/13.
//
//
#include "pythonPluginServer.h"
#include "cJSON.h"
#include "mea_plugins_utils.h"


int plugin_fireandforget_function_json(char *module, int type, cJSON *data)
{
   plugin_queue_elem_t *plugin_elem = (plugin_queue_elem_t *)malloc(sizeof(plugin_queue_elem_t));
   if(plugin_elem) {
      plugin_elem->type_elem=type;
      plugin_elem->aJsonDict = data;
      plugin_elem->aDict = NULL;
      pythonPluginServer_add_cmd(module, (void *)plugin_elem, sizeof(plugin_queue_elem_t));
      free(plugin_elem);
      plugin_elem=NULL;
   }
   return 0;
}


cJSON *plugin_call_function_json_alloc(char *module, char *function, cJSON *data)
{
   cJSON *result=NULL;
   
   plugin_queue_elem_t *plugin_elem = (plugin_queue_elem_t *)malloc(sizeof(plugin_queue_elem_t));
   if(plugin_elem) {
      plugin_elem->type_elem=CUSTOM_JSON;
      plugin_elem->aJsonDict=data;
      plugin_elem->aDict=NULL;
      result=pythonPluginServer_exec_cmd(module, function, (void *)plugin_elem, sizeof(plugin_queue_elem_t), 0);
      free(plugin_elem);
      plugin_elem=NULL;
   }
   
   return result;
}
