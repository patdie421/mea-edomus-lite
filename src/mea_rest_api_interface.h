#ifndef mea_rest_api_interface_h
#define mea_rest_api_interface_h

#include "mongoose.h"

int mea_rest_api_interface(struct mg_connection *conn, int method, char *tokens[], int l_tokens);

#endif
