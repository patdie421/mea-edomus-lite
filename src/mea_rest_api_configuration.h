#ifndef mea_rest_api_configuration_h
#define mea_rest_api_configuration_h

int mea_rest_api_configuration(struct mg_connection *conn, int method, char *tokens[], int l_tokens);

#endif