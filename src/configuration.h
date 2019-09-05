#ifndef __configuration_h
#define __configuration_h
#include "cJSON.h"

cJSON *getAppParameters();
cJSON *appParameters_create(void);
void appParameters_init();
void appParameters_clean();
int appParameters_set(char *k, char *v, cJSON *d);
char *appParameters_get(char *k, cJSON *d);
void appParameters_print(cJSON *p);
void appParameters_display();

char *getAppParametersAsString_alloc();
char *getAppParameterAsString_alloc(char *name);

int updateAppParameters(cJSON *jsonData);
int updateAppParameter(char *name, cJSON *jsonData);

#endif
