#ifndef __users_h
#define __users_h

#include <stdio.h>
#include <stdlib.h>

#include "cJSON.h"

extern cJSON *users2;

char *user_getFileFullPath(void);
int initUsers(void);

#endif
