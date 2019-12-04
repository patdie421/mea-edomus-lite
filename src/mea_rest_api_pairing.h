//
//  mea_rest_api_pairing.h
//  mea-edomus-lite
//
//  Created by Patrice Dietsch on 03/12/2019.
//  Copyright © 2019 Patrice Dietsch. All rights reserved.
//

#ifndef mea_rest_api_pairing_h
#define mea_rest_api_pairing_h

#include "mongoose.h"

int mea_rest_api_pairing(struct mg_connection *conn, int method, char *tokens[], int l_tokens);

#endif /* mea_rest_api_pairing_h */
