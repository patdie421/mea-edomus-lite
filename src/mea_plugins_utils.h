//
//  python_utils.h
//  mea-eDomus
//
//  Created by Patrice Dietsch on 04/06/13.
//
//
#ifndef mea_plugins_utils_h
#define mea_plugins_utils_h

#include "cJSON.h"

int    plugin_fireandforget_function_json(char *module, int type, cJSON *data);
cJSON *plugin_call_function_json_alloc(char *module, char *function, cJSON *data);

// int       python_cmd_json(char *module, int type, cJSON *data);
// cJSON    *python_call_function_json_alloc(char *module, char *function, cJSON *data);

#endif
