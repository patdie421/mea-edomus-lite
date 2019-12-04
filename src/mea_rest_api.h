#ifndef mea_rest_api_h
#define mea_rest_api_h

#include "mongoose.h"
#include "cJSON.h"

extern char *no_valid_json_data_str;
extern char *bad_method_str;
extern char *not_authorized_str;

#define NO_VALID_JSON_DATA no_valid_json_data_str
#define BAD_METHOD bad_method_str
#define NOT_AUTHORIZED not_authorized_str

cJSON *getData_alloc(struct mg_connection *conn);
int checkSession(char *sessionId);
int returnResponse(struct mg_connection *conn, int httperr, int errnum, char *msg);
int mea_rest_api(struct mg_connection *conn,int method,char *tokens[], int l_tokens);

#endif
