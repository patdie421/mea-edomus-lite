#ifndef mea_rest_api_session_h
#define mea_rest_api_session_h

#include "mongoose.h"
#include "cJSON.h"

extern cJSON *sessions;

int   openSession(struct mg_connection *conn);
int   closeSession(struct mg_connection *conn, char *id);
int   purgeSessions(void);
int   purgeSessionsByUsername(char *username);
int   getSessionProfile(char *sessionId);
char *getSessionUser_alloc(char *sessionId);

int   checkSession(char *sessionId);
int   mea_rest_api_session(struct mg_connection *conn, int method, char *tokens[], int l_tokens);

#endif