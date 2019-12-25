#ifndef mea_rest_api_h
#define mea_rest_api_h

#include "mongoose.h"
#include "cJSON.h"

extern char *no_valid_json_data_str;
extern char *bad_method_str;
extern char *not_authorized_str;
extern char *success_str;
extern char *forbidden_str;

#define NO_VALID_JSON_DATA no_valid_json_data_str
#define BAD_METHOD bad_method_str
#define NOT_AUTHORIZED not_authorized_str
#define FORBIDDEN forbidden_str
#define SUCCESS success_str

cJSON *getData_alloc(struct mg_connection *conn);
int getSessionProfile(char *sessionId);
char *getSessionUser_alloc(char *sessionId);
int checkSession(char *sessionId);
int purgeSessionsByUsername(char *username);
int returnResponse(struct mg_connection *conn, int httperr, int errnum, char *msg);
int returnResponseAndDeleteJsonData(struct mg_connection *conn, int httperr, int errnum, char *msg, cJSON *jsonData);
int mea_rest_api(struct mg_connection *conn,int method,char *tokens[], int l_tokens);

#endif
