#ifndef __mea_json_utils_h
#define __mea_json_utils_h

#include "cJSON.h"

cJSON *loadJson_alloc(char *file);
int writeJson(char *file, cJSON *j);

#endif
