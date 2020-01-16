#include <ctype.h>

#include "mea_rest_api_metric.h"
#include "mea_rest_api.h"
#include "interfacesServer.h"
#include "httpServer.h"
#include "mea_http_utils.h"
#include "mongoose.h"
#include "cJSON.h"
#include "tokens.h"
#include "tokens_da.h"
#include "processManager.h"


int mea_rest_api_metric(struct mg_connection *conn, int method, char *tokens[], int l_tokens)
{
   const char *meaSessionId=mg_get_header(conn, MEA_SESSION_STR_C);

   if(checkSession((char *)meaSessionId)!=0) {
      return returnResponse(conn, 401, 99, NOT_AUTHORIZED);
   }

   switch(method) {
      case HTTP_GET_ID:
         if(l_tokens==0) {
            char s[4096]="";
            int ret=managed_processes_processes_to_json(s, sizeof(s), -1);
            if(ret==0) {
               httpResponse(conn, 200, NULL, s);
               return 1;
            }
            else {
               return returnResponse(conn, 400, 1, NULL);
            }
         }
         else if(l_tokens==1) {
            int id=0;
            for (int i=0;i<strlen(tokens[0]); i++) {
               if ( isdigit(tokens[0][i]) ) {
                  id=id*(10*i)+(tokens[0][i]-'0');
               } 
               else {
                  return returnResponse(conn, 400, 1, "id not an integer");
               }
            }

            char s[4096]="";
            int ret=managed_processes_process_to_json(id, s, sizeof(s));
            if(ret==0) {
               httpResponse(conn, 200, NULL, s);
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
