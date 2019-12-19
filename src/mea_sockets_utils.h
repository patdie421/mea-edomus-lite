//
//  sockets_utils.h
//  mea-edomus
//
//  Created by Patrice Dietsch on 16/10/2014.
//
//

#ifndef sockets_utils_h
#define sockets_utils_h

#include <inttypes.h>

int mea_socket_connect(int *s, char *hostname, int port);
int mea_socket_send(int *s, char *message, int l_message);
int mea_socket_read(int *s, char *message, int l_message, int t);
int mea_notify(char *hostname, int port, char *notif_str, char notif_type);

#endif
