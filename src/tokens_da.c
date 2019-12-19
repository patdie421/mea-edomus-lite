/**
 * \file tokens_da.c
 * \brief acces direct aux chaines des tokens
 * \author Patrice DIETSCH
 * \date 29/10/12
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <inttypes.h>

#include "mea_verbose.h"

#include "tokens_da.h"
#include "tokens.h"


struct tokens_strings_da_s *tokens_string_da = NULL;

int16_t init_strings_da()
{
#if TOKENS_STRING_DA == 1
   if(tokens_string_da == NULL) {
      tokens_string_da = (struct tokens_strings_da_s *)malloc(sizeof(struct tokens_strings_da_s));
      if(tokens_string_da == NULL) {
         DEBUG_SECTION PRINT_MALLOC_ERROR;
         return -1;
      }
   }
   
   tokens_string_da->high_str_c                       = get_token_string_by_id(HIGH_ID);
   tokens_string_da->low_str_c                        = get_token_string_by_id(LOW_ID);
   tokens_string_da->true_str_c                       = get_token_string_by_id(TRUE_ID);
   tokens_string_da->false_str_c                      = get_token_string_by_id(FALSE_ID);
   tokens_string_da->xpl_sensor_str_c                 = get_token_string_by_id(XPL_SENSOR_ID);
   tokens_string_da->xpl_basic_str_c                  = get_token_string_by_id(XPL_BASIC_ID);
   tokens_string_da->xpl_device_str_c                 = get_token_string_by_id(XPL_DEVICE_ID);
   tokens_string_da->xpl_type_str_c                   = get_token_string_by_id(XPL_TYPE_ID);
   tokens_string_da->xpl_output_str_c                 = get_token_string_by_id(XPL_OUTPUT_ID);
   tokens_string_da->xpl_current_str_c                = get_token_string_by_id(XPL_CURRENT_ID);
   tokens_string_da->xpl_control_str_c                = get_token_string_by_id(XPL_CONTROL_ID);
   tokens_string_da->xpl_last_str_c                   = get_token_string_by_id(XPL_LAST_ID);
   tokens_string_da->xpl_request_str_c                = get_token_string_by_id(XPL_REQUEST_ID);
   tokens_string_da->reachable_str_c                  = get_token_string_by_id(REACHABLE_ID);
   tokens_string_da->on_str_c                         = get_token_string_by_id(ON_ID);
   tokens_string_da->name_str_c                       = get_token_string_by_id(NAME_ID);
   tokens_string_da->state_str_c                      = get_token_string_by_id(STATE_ID);
   tokens_string_da->device_parameters_str_c          = get_token_string_by_id(DEVICE_PARAMETERS_ID);
   tokens_string_da->id_enocean_str_c                 = get_token_string_by_id(ID_ENOCEAN_ID);
   tokens_string_da->enocean_addr_str_c               = get_token_string_by_id(ENOCEAN_ADDR_ID);
   tokens_string_da->device_type_id_str_c             = get_token_string_by_id(DEVICE_TYPE_ID_ID);
   tokens_string_da->interface_id_str_c               = get_token_string_by_id(INTERFACE_ID_ID);
   tokens_string_da->interface_parameters_str_c       = get_token_string_by_id(INTERFACE_PARAMETERS_ID);
   tokens_string_da->device_id_str_c                  = get_token_string_by_id(DEVICE_ID_ID);
   tokens_string_da->device_name_str_c                = get_token_string_by_id(DEVICE_NAME_ID);
   tokens_string_da->device_type_id_str_c             = get_token_string_by_id(DEVICE_TYPE_ID_ID);
   tokens_string_da->device_localtion_id_str_c        = get_token_string_by_id(DEVICE_LOCATION_ID_ID);
   tokens_string_da->device_interface_name_str_c      = get_token_string_by_id(DEVICE_INTERFACE_NAME_ID);
   tokens_string_da->device_interface_type_name_str_c = get_token_string_by_id(DEVICE_INTERFACE_TYPE_NAME_ID);
   tokens_string_da->device_state_str_c               = get_token_string_by_id(DEVICE_STATE_ID);
   tokens_string_da->device_type_parameters_str_c     = get_token_string_by_id(DEVICE_TYPE_PARAMETERS_ID);
   tokens_string_da->devices_str_c                    = get_token_string_by_id(DEVICES_ID);
   tokens_string_da->todbflag_str_c                   = get_token_string_by_id(TODBFLAG_ID);
   tokens_string_da->addr_h_str_c                     = get_token_string_by_id(ADDR_H_ID);
   tokens_string_da->addr_l_str_c                     = get_token_string_by_id(ADDR_L_ID);
   tokens_string_da->interface_id_str_c               = get_token_string_by_id(INTERFACE_ID_ID);
   tokens_string_da->interface_type_id_str_c          = get_token_string_by_id(INTERFACE_TYPE_ID_ID);
   tokens_string_da->interface_name_str_c             = get_token_string_by_id(INTERFACE_NAME_ID);
   tokens_string_da->interface_state_str_c            = get_token_string_by_id(INTERFACE_STATE_ID);
   tokens_string_da->local_xbee_addr_h_str_c          = get_token_string_by_id(LOCAL_XBEE_ADDR_H_ID);
   tokens_string_da->local_xbee_addr_l_str_c          = get_token_string_by_id(LOCAL_XBEE_ADDR_L_ID);
   tokens_string_da->internal_str_c                   = get_token_string_by_id(INTERNAL_ID);
   tokens_string_da->xplmsg_str_c                     = get_token_string_by_id(XPLMSG_ID);
   tokens_string_da->xplsource_str_c                  = get_token_string_by_id(XPLSOURCE_ID);
   tokens_string_da->xpltarget_str_c                  = get_token_string_by_id(XPLTARGET_ID);
   tokens_string_da->xplschema_str_c                  = get_token_string_by_id(XPLSCHEMA_ID);
   tokens_string_da->xplmsgtype_str_c                 = get_token_string_by_id(XPLMSGTYPE_ID);
   tokens_string_da->xpl_trig_str_c                   = get_token_string_by_id(XPL_TRIG_ID);
   tokens_string_da->xpl_stat_str_c                   = get_token_string_by_id(XPL_STAT_ID);
   tokens_string_da->xpl_cmnd_str_c                   = get_token_string_by_id(XPL_CMND_ID);
   tokens_string_da->xpl_pulse_str_c                  = get_token_string_by_id(XPL_PULSE_ID);
   tokens_string_da->xpl_sensorrequest_str_c          = get_token_string_by_id(XPL_SENSORREQUEST_ID);
   tokens_string_da->xpl_controlbasic_str_c           = get_token_string_by_id(XPL_CONTROLBASIC_ID);
   tokens_string_da->http_get_str_c                   = get_token_string_by_id(HTTP_GET_ID);
   tokens_string_da->http_post_str_c                  = get_token_string_by_id(HTTP_POST_ID);
   tokens_string_da->http_delete_str_c                = get_token_string_by_id(HTTP_DELETE_ID);
   tokens_string_da->http_put_str_c                   = get_token_string_by_id(HTTP_PUT_ID);
   tokens_string_da->api_session_str_c                = get_token_string_by_id(API_SESSION_ID);
   tokens_string_da->api_interface_str_c              = get_token_string_by_id(API_INTERFACE_ID);
   tokens_string_da->api_device_str_c                 = get_token_string_by_id(API_DEVICE_ID);
   tokens_string_da->api_service_str_c                = get_token_string_by_id(API_SERVICE_ID);
   tokens_string_da->api_key_str_c                    = get_token_string_by_id(API_KEY_ID);
   tokens_string_da->data_str_c                       = get_token_string_by_id(DATA_ID);
   tokens_string_da->l_data_str_c                     = get_token_string_by_id(L_DATA_ID);
   tokens_string_da->api_type_str_c                   = get_token_string_by_id(API_TYPE_ID);
   tokens_string_da->api_configuration_str_c          = get_token_string_by_id(API_CONFIGURATION_ID);
   tokens_string_da->api_user_str_c                   = get_token_string_by_id(API_USER_ID);
   tokens_string_da->api_pairing_str_c                = get_token_string_by_id(API_PAIRING_ID);
   tokens_string_da->time_str_c                       = get_token_string_by_id(TIME_ID);
   tokens_string_da->user_str_c                       = get_token_string_by_id(USER_ID);
   tokens_string_da->password_str_c                   = get_token_string_by_id(PASSWORD_ID);
   tokens_string_da->profile_str_c                    = get_token_string_by_id(PROFILE_ID);
   tokens_string_da->mea_sessionid_str_c              = get_token_string_by_id(MEA_SESSIONID_ID);
   tokens_string_da->mea_session_str_c                = get_token_string_by_id(MEA_SESSION_ID);
   tokens_string_da->start_str_c                      = get_token_string_by_id(START_ID);
   tokens_string_da->stop_str_c                       = get_token_string_by_id(STOP_ID);
   tokens_string_da->restart_str_c                    = get_token_string_by_id(RESTART_ID);
   tokens_string_da->detail_str_c                     = get_token_string_by_id(DETAIL_ID);
   tokens_string_da->all_str_c                        = get_token_string_by_id(ALL_ID);
   tokens_string_da->action_str_c                     = get_token_string_by_id(ACTION_ID);
   tokens_string_da->create_str_c                     = get_token_string_by_id(CREATE_ID);
   tokens_string_da->commit_str_c                     = get_token_string_by_id(COMMIT_ID);
   tokens_string_da->rollback_str_c                   = get_token_string_by_id(ROLLBACK_ID);
   tokens_string_da->fullname_str_c                   = get_token_string_by_id(FULLNAME_ID);
   tokens_string_da->username_str_c                   = get_token_string_by_id(USERNAME_ID);
   tokens_string_da->parameters_str_c                 = get_token_string_by_id(PARAMETERS_ID);
   tokens_string_da->update_str_c                     = get_token_string_by_id(UPDATE_ID);

#endif
   return 0;
}

void release_strings_da()
{
#if TOKENS_STRING_DA == 1
   if(tokens_string_da) {
      free(tokens_string_da);
      tokens_string_da=NULL;
   }
#endif
}


#ifdef TOKENS_DA_MODULE_TEST
// gcc -std=c99 -std=gnu99 tokens_da.c tokens.c mea_verbose.c mea_string_utils.c -DTOKENS_DA_MODULE_TEST
#include <sys/time.h>

double millis()
{
   struct timeval te;
   gettimeofday(&te, NULL);

   double milliseconds = (double)te.tv_sec*1000.0 + (double)te.tv_usec/1000.0;

   return milliseconds;
}


int main(int argc, char *argv[])
{
   double t0=0.0;

   init_tokens();
   init_strings_da();

   t0=millis();
   for(int i=0;i<10000;i++) {
      for(int j=1;j<=_END;j++) {
         char *str=get_token_string_by_id(j);
         int id=(int)get_token_id_by_string(str);
         if(j!=id) {
            printf("Error %s %d %d\n", str, j, id);
            goto exit;
         }
      }
   }
   
   printf("%5.2f ms\n",millis()-t0);
   
   printf("USER %d\n", get_token_id_by_string("USER"));
   printf("user %d\n", get_token_id_by_string("user"));
   
   printf("USER_ID: %s\n", get_token_string_by_id(USER_ID));
   printf("API_USER_ID: %s\n", get_token_string_by_id(API_USER_ID));
   
exit:
   release_strings_da();
   release_tokens();
}
#endif
