//
//  httpServer.h
//  mea-eDomus
//
//  Created by Patrice Dietsch on 04/06/13.
//
//
#ifndef __httpServer_h
#define __httpServer_h

#include "mongoose.h"
#include "cJSON.h"

struct httpServerData_s
{
   cJSON *params_list;
};

void httpResponse(struct mg_connection *conn, int httpCode, char *httpMessage, char *response);

int start_guiServer(int my_id, void *data, char *errmsg, int l_errmsg);
int stop_guiServer(int my_id, void *data, char *errmsg, int l_errmsg);
int gethttp(char *server, int port, char *url, char *response, int l_response);

int restart_guiServer(int my_id, void *data, char *errmsg, int l_errmsg);

#endif
