#ifndef __configuration_h
#define __configuration_h
#include "cJSON.h"

cJSON *getAppParameters(void);
cJSON *appParameters_create(void);
void   appParameters_init(void);
void   appParameters_clean(void);
int    appParameters_set(char *k, char *v, cJSON *d);
char  *appParameters_get(char *k, cJSON *d);
void   appParameters_print(cJSON *p);
void   appParameters_display(void);
char  *getAppParametersAsString_alloc(void);
char  *getAppParameterAsString_alloc(char *name);
int    updateAppParameters(cJSON *jsonData);
int    updateAppParameter(char *name, cJSON *jsonData);

#endif
