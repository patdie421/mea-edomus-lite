#ifndef mea_http_utils_h
#define mea_http_utils_h

#include "mongoose.h"
#include "cJSON.h"

cJSON *getData_alloc(struct mg_connection *conn);
int returnResponseAndDeleteJsonData(struct mg_connection *conn, int httperr, int errnum, char *msg, cJSON *jsonData);
int returnResponse(struct mg_connection *conn, int httperr, int errnum, char *msg);

#endif