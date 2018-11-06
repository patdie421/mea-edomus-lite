#ifndef __philipshue_h
#define __philipshue_h

#include "cJSON.h"

extern char *_true;
extern char *_false;

extern char *apiStateTemplate;
extern char *apiLightsTemplate;


//cJSON *getAllScenes(char *server, int port, char *user);
cJSON *getAllGroups(char *server, int port, char *user);
cJSON *getAllLights(char *server, int port, char *user);

cJSON *my_cJSON_GetItemByName(cJSON *object, const char *name);
cJSON *getLightIdByName(cJSON *allLights, char *name, int16_t *id);
cJSON *getLightStateByName(cJSON *allLights, char *name, int16_t *on, int16_t *reachable);
cJSON *getLightStateById(cJSON *allLights, uint16_t id, int16_t *on, int16_t *reachable);
cJSON *getLightNameById(cJSON *allLights, uint16_t id, char *name, uint16_t l_name);

int16_t setLightStateByName(cJSON *allLights, char *name, int16_t state, char *server, int port, char *user);
int16_t setLightColorByName(cJSON *allLights, char *name, uint32_t color, char *server, int port, char *user);

int16_t setGroupStateByName(cJSON *allGroups, char *name, int16_t state, char *server, int port, char *user);
int16_t setGroupColorByName(cJSON *allGroups, char *name, uint32_t color, char *server, int port, char *user);

#endif
