#include "mea_rest_api_type.h"
#include "mea_rest_api.h"
#include "interfacesServer.h"
#include "httpServer.h"
#include "mea_http_utils.h"
#include "mongoose.h"
#include "cJSON.h"
#include "tokens.h"
#include "tokens_da.h"


int mea_rest_api_type(struct mg_connection *conn, int method, char *tokens[], int l_tokens)
{
   const char *meaSessionId=mg_get_header(conn, MEA_SESSION_STR_C);

   if(checkSession((char *)meaSessionId)!=0) {
      return returnResponse(conn, 401, 99, NOT_AUTHORIZED);
   }

   switch(method) {
      case HTTP_GET_ID:
         if(l_tokens==0) {
            char *s=getTypesAsString_alloc();
            if(s) {
               httpResponse(conn, 200, NULL, s);
               free(s);
               return 1;
            }
            else {
               return returnResponse(conn, 404, 1, NULL);
            }
         }
         else if(l_tokens==1) {
            char *s=getTypeAsStringByName_alloc(tokens[0]);
            if(s) {
               httpResponse(conn, 200, "type doesn't exist", s);
               free(s);
               return 1;
            }
            else {
               return returnResponse(conn, 400, 1, NULL);
            }
         }
         else {
            return returnResponse(conn, 404, 1, NULL);
         }

      default:
         return returnResponse(conn, 405, 1, BAD_METHOD);
   }
}
