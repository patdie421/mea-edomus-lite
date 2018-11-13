//
//  init.h
//  mea-edomus
//
//  Created by Patrice Dietsch on 18/08/13.
//
//

#ifndef __init_h
#define __init_h

#include <inttypes.h>
#include "cJSON.h"

int16_t create_queries_db(char *queries_db_path);
int16_t checkInstallationPaths(char *base_path, int16_t try_to_create_flag);
int16_t create_configs_php(char *base_path, char *gui_home, char *params_db_fullname, char *php_log_fullname, char *php_sessions_fullname);
int16_t initMeaEdomus(int16_t mode, cJSON *params_list);
int16_t updateMeaEdomus(cJSON *params_list);
#endif
