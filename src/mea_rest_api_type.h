#ifndef mea_rest_api_user_h
#define mea_rest_api_user_h

#include "mongoose.h"

int mea_rest_api_type(struct mg_connection *conn, int method, char *tokens[], int l_tokens);

#endif