#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <ctype.h>

#include "cJSON.h"
#include "mea_verbose.h"
#include "mea_string_utils.h"
#include "tokens.h"
#include "tokens_da.h"
#include "mongoose.h"
#include "httpServer.h"
#include "mea_http_utils.h"

#include "mea_rest_api.h"
#include "mea_rest_api_session.h"
#include "mea_rest_api_pairing.h"
#include "mea_rest_api_user.h"
#include "mea_rest_api_service.h"
#include "mea_rest_api_configuration.h"
#include "mea_rest_api_interface.h"
#include "mea_rest_api_type.h"



// #include "configuration.h"


char *no_valid_json_data_str="no valid json data";
char *bad_method_str="bad method";
char *not_authorized_str="not authorized";
char *success_str="success";


int mea_rest_api_init()
{
   srand((unsigned int)time(NULL));
   sessions=cJSON_CreateObject();

   return 1;
}


int mea_rest_api_exit()
{
   cJSON_Delete(sessions);
   sessions=NULL;
   cJSON_Delete(users2);
   users2=NULL;

   return 1;
}


int mea_rest_api(struct mg_connection *conn, int method, char *tokens[], int l_tokens)
{
   if(sessions==NULL)
      mea_rest_api_init();

   purgeSessions();

   int ressource=get_token_id_by_string((char *)tokens[0]);

   char **_tokens;
   int _l_tokens=l_tokens-1;

   if(_l_tokens<=0) {
      _l_tokens=0;
      _tokens=NULL;
   }
   else {
      _tokens=&tokens[1];
   }

   switch(ressource) {
      case API_SESSION_ID:
         return mea_rest_api_session(conn, method, _tokens, _l_tokens);
      case API_INTERFACE_ID:
         return mea_rest_api_interface(conn, method, _tokens, _l_tokens);
      case API_SERVICE_ID:
         return mea_rest_api_service(conn, method, _tokens, _l_tokens);
      case API_TYPE_ID:
         return mea_rest_api_type(conn, method, _tokens, _l_tokens);
      case API_CONFIGURATION_ID:
         return mea_rest_api_configuration(conn, method, _tokens, _l_tokens);
      case API_USER_ID:
         return mea_rest_api_user(conn, method, _tokens, _l_tokens);
      case API_PAIRING_ID:
         return mea_rest_api_pairing(conn, method, _tokens, _l_tokens);
      default:
         return returnResponse(conn, 404, 1, NULL);
   } 
}