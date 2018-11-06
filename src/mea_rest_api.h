#ifndef mea_rest_api_h
#define mea_rest_api_h

#include "mongoose.h"

int mea_rest_api(struct mg_connection *conn,int method,char *tokens[], int l_tokens);

#endif
