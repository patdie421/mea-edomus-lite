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

#define HTTPREQUEST_ERR_NOERR    0
#define HTTPREQUEST_ERR_MALLOC   1
#define HTTPREQUEST_ERR_CONNECT  2
#define HTTPREQUEST_ERR_METHODE  3
#define HTTPREQUEST_ERR_SEND     4
#define HTTPREQUEST_ERR_TIMEOUT  5
#define HTTPREQUEST_ERR_SELECT   6
#define HTTPREQUEST_ERR_OVERFLOW 7

#define HTTP_GET    0
#define HTTP_POST   1
#define HTTP_PUT    2
#define HTTP_DELETE 3

int mea_socket_connect(int *s, char *hostname, int port);
int mea_socket_send(int *s, char *message, int l_message);
int mea_socket_read(int *s, char *message, int l_message, int t);
int mea_notify(char *hostname, int port, char *notif_str, char notif_type);

char *httpRequest(uint8_t type, char *server, int port, char *url, char *data, uint32_t l_data, char *response, uint32_t *l_response, int16_t *nerr);
int16_t httpGetResponseStatusCode(char *response, uint32_t l_response);

#endif
