#ifndef mea_rest_api_user_h
#define mea_rest_api_user_h

#include "cJSON.h"
#include "mongoose.h"

extern char *_users2;
extern cJSON *users2;

int mea_rest_api_user(struct mg_connection *conn, int method, char *tokens[], int l_tokens);

#endif