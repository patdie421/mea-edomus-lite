#ifndef mea_http_utils_h
#define mea_http_utils_h

#include <inttypes.h>

#include "mongoose.h"
#include "cJSON.h"

#define HTTP_GET    0
#define HTTP_POST   1
#define HTTP_PUT    2
#define HTTP_DELETE 3

#define HTTPREQUEST_ERR_NOERR    0
#define HTTPREQUEST_ERR_MALLOC   1
#define HTTPREQUEST_ERR_CONNECT  2
#define HTTPREQUEST_ERR_METHODE  3
#define HTTPREQUEST_ERR_SEND     4
#define HTTPREQUEST_ERR_TIMEOUT  5
#define HTTPREQUEST_ERR_SELECT   6
#define HTTPREQUEST_ERR_OVERFLOW 7


cJSON *getData_alloc(struct mg_connection *conn);
int returnResponseAndDeleteJsonData(struct mg_connection *conn, int httperr, int errnum, char *msg, cJSON *jsonData);
int returnResponse(struct mg_connection *conn, int httperr, int errnum, char *msg);

int16_t httpGetResponseStatusCode(char *response, uint32_t l_response);
char *httpRequest(uint8_t type, char *server, int port, char *url, char *data, uint32_t l_data, char *response, uint32_t *l_response, int16_t *nerr);

#endif