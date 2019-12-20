//
//  mea_xpl.h
//
//  Created by Patrice DIETSCH on 12/31/16.
//  Copyright (c) 2012 -. All rights reserved.
//

#ifndef __mea_xpl_h
#define __mea_xpl_h

#include <netinet/in.h>

#include "cJSON.h"

#define XPLPORT 3865

#define XPL_VERSION "0.1a2"

int mea_xPLConnectBroadcast(char *xPLInterfaceName, char *ip, int l_ip, struct sockaddr_in *xPLBroadcastAddr);
int mea_xPLConnectHub(int *xPLPort);
int16_t mea_xPLSendMessage(int fd, struct sockaddr_in *xPLBroadcastAddr, char *data, int l_data);
int16_t mea_xPLReadMessage(int fd, int32_t timeoutms, char *data, int *l_data, int16_t *nerr);
void mea_xPLSendHbeat(int fd, struct sockaddr_in *xPLBroadcastAddr, char *remoteip, int port, char *version, char *source, char *type, int interval);

cJSON *mea_xplMsgToJson_alloc(cJSON *xplMsgJson);

#endif
