//
//  mea_rest_api_pairing.h
//  mea-edomus-lite
//
//  Created by Patrice Dietsch on 03/12/2019.
//  Copyright © 2019 Patrice Dietsch. All rights reserved.
//

#ifndef mea_rest_api_user_h
#define mea_rest_api_user_h

#include "cJSON.h"
#include "mongoose.h"

extern char *_users2;
extern cJSON *users2;

int mea_rest_api_user(struct mg_connection *conn, int method, char *tokens[], int l_tokens);

#endif