#ifndef mea_rest_api_service_h
#define mea_rest_api_service_h

int mea_rest_api_service(struct mg_connection *conn, int method, char *tokens[], int l_tokens);

#endif