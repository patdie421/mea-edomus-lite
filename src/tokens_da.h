/**
 * \file tokens_da.h
 * \brief acces direct aux chaines des tokens.
 * \author Patrice DIETSCH
 * \date 02/09/15
 */

#ifndef __tokens_da_h
#define __tokens_da_h

#include <inttypes.h>

#include "tokens.h"

// Activation du mode "direct access" (DA).
#define TOKENS_STRING_DA 1

#if TOKENS_STRING_DA == 1
struct tokens_strings_da_s
{
   char *device_parameters_str_c;
   char *device_type_id_str_c;
   char *enocean_addr_str_c;
   char *false_str_c;
   char *id_enocean_str_c;
   char *interface_id_str_c;
   char *interface_parameters_str_c;
   char *low_str_c;
   char *name_str_c;
   char *on_str_c;
   char *recheable_str_c;
   char *true_str_c;
   char *xpl_basic_str_c;
   char *xpl_current_str_c;
   char *xpl_control_str_c;
   char *xpl_last_str_c;
   char *xpl_output_str_c;
   char *xpl_request_str_c;
   char *xpl_sensor_str_c;
   char *xpl_type_str_c;
   char *xpl_device_str_c;
   char *high_str_c;
   char *device_id_str_c;
   char *device_name_str_c;
   char *device_localtion_id_str_c;
   char *device_interface_name_str_c;
   char *device_interface_type_name_str_c;
   char *device_state_str_c;
   char *device_type_parameters_str_c;
   char *devices_str_c;
   char *todbflag_str_c;
   char *addr_l_str_c;
   char *addr_h_str_c;
   char *interface_type_id_str_c;
   char *interface_name_str_c;
   char *interface_state_str_c;
   char *local_xbee_addr_h_str_c;
   char *local_xbee_addr_l_str_c;
   char *reachable_str_c;
   char *state_str_c;
   char *internal_str_c;
   char *xplmsg_str_c;
   char *xplsource_str_c;
   char *xpltarget_str_c;
   char *xplschema_str_c;
   char *xplmsgtype_str_c;
   char *xpl_trig_str_c;
   char *xpl_stat_str_c;
   char *xpl_cmnd_str_c;
   char *xpl_pulse_str_c;
   char *xpl_controlbasic_str_c;
   char *xpl_sensorrequest_str_c;
   char *http_get_str_c;
   char *http_post_str_c;
   char *http_delete_str_c;
   char *http_put_str_c;
   char *api_session_str_c;
   char *api_interface_str_c;
   char *api_device_str_c;
   char *api_key_str_c;
   char *data_str_c;
   char *l_data_str_c;
   char *api_service_str_c;
   char *api_type_str_c;
   char *api_configuration_str_c;
   char *api_user_str_c;
   char *api_pairing_str_c;
   char *time_str_c;
   char *user_str_c;
   char *password_str_c;
   char *profile_str_c;
   char *mea_sessionid_str_c;
   char *mea_session_str_c;
   char *start_str_c;
   char *stop_str_c;
   char *restart_str_c;
   char *detail_str_c;
   char *all_str_c;
   char *action_str_c;
   char *create_str_c;
   char *commit_str_c;
   char *rollback_str_c;
};

extern struct tokens_strings_da_s *tokens_string_da;

// Macro pour les accès direct à la chaine d'un token. Tous les tokens ne sont pas repris pour l'instant
#define DEVICE_PARAMETERS_STR_C          tokens_string_da->device_parameters_str_c
#define DEVICE_TYPE_ID_STR_C             tokens_string_da->device_type_id_str_c
#define FALSE_STR_C                      tokens_string_da->false_str_c
#define HIGH_STR_C                       tokens_string_da->high_str_c
#define ID_ENOCEAN_STR_C                 tokens_string_da->id_enocean_str_c
#define LOW_STR_C                        tokens_string_da->low_str_c
#define NAME_STR_C                       tokens_string_da->name_str_c
#define ON_STR_C                         tokens_string_da->on_str_c
#define REACHABLE_STR_C                  tokens_string_da->reachable_str_c
#define STATE_STR_C                      tokens_string_da->state_str_c
#define TRUE_STR_C                       tokens_string_da->true_str_c
#define XPL_BASIC_STR_C                  tokens_string_da->xpl_basic_str_c
#define XPL_CURRENT_STR_C                tokens_string_da->xpl_current_str_c
#define XPL_CONTROL_STR_C                tokens_string_da->xpl_control_str_c
#define XPL_DEVICE_STR_C                 tokens_string_da->xpl_device_str_c
#define XPL_ENOCEAN_ADDR_STR_C           tokens_string_da->enocean_addr_str_c
#define XPL_LAST_STR_C                   tokens_string_da->xpl_last_str_c
#define XPL_OUTPUT_STR_C                 tokens_string_da->xpl_output_str_c
#define XPL_REQUEST_STR_C                tokens_string_da->xpl_request_str_c
#define XPL_SENSOR_STR_C                 tokens_string_da->xpl_sensor_str_c
#define XPL_TYPE_STR_C                   tokens_string_da->xpl_type_str_c
#define DEVICE_ID_STR_C                  tokens_string_da->device_id_str_c
#define DEVICE_NAME_STR_C                tokens_string_da->device_name_str_c
#define DEVICE_TYPE_ID_STR_C             tokens_string_da->device_type_id_str_c
#define DEVICE_LOCATION_ID_STR_C         tokens_string_da->device_localtion_id_str_c
#define DEVICE_INTERFACE_NAME_STR_C      tokens_string_da->device_interface_name_str_c
#define DEVICE_INTERFACE_TYPE_NAME_STR_C tokens_string_da->device_interface_type_name_str_c
#define INTERFACE_PARAMETERS_STR_C       tokens_string_da->interface_parameters_str_c
#define DEVICE_STATE_STR_C               tokens_string_da->device_state_str_c
#define DEVICE_TYPE_PARAMETERS_STR_C     tokens_string_da->device_type_parameters_str_c
#define DEVICE_TODBFLAG_STR_C            tokens_string_da->todbflag_str_c
#define DEVICE_ADDR_H_STR_C              tokens_string_da->addr_h_str_c
#define DEVICE_ADDR_L_STR_C              tokens_string_da->addr_l_str_c
#define DEVICES_STR_C                    tokens_string_da->devices_str_c
#define INTERFACE_ID_STR_C               tokens_string_da->interface_id_str_c
#define INTERFACE_TYPE_ID_STR_C          tokens_string_da->interface_type_id_str_c
#define INTERFACE_NAME_STR_C             tokens_string_da->interface_name_str_c
#define INTERFACE_STATE_STR_C            tokens_string_da->interface_state_str_c
#define LOCAL_XBEE_ADDR_H_STR_C          tokens_string_da->local_xbee_addr_h_str_c
#define LOCAL_XBEE_ADDR_L_STR_C          tokens_string_da->local_xbee_addr_l_str_c
#define INTERNAL_STR_C                   tokens_string_da->internal_str_c
#define XPLMSG_STR_C                     tokens_string_da->xplmsg_str_c
#define XPLSOURCE_STR_C                  tokens_string_da->xplsource_str_c
#define XPLTARGET_STR_C                  tokens_string_da->xpltarget_str_c
#define XPLMSGTYPE_STR_C                 tokens_string_da->xplmsgtype_str_c
#define XPLSCHEMA_STR_C                  tokens_string_da->xplschema_str_c
#define XPL_TRIG_STR_C                   tokens_string_da->xpl_trig_str_c
#define XPL_STAT_STR_C                   tokens_string_da->xpl_stat_str_c
#define XPL_CMND_STR_C                   tokens_string_da->xpl_cmnd_str_c
#define XPL_PULSE_STR_C                  tokens_string_da->xpl_pulse_str_c
#define XPL_CONTROLBASIC_STR_C           tokens_string_da->xpl_controlbasic_str_c
#define XPL_SENSORREQUEST_STR_C          tokens_string_da->xpl_sensorrequest_str_c
#define HTTP_GET_STR_C                   tokens_string_da->http_get_str_c
#define HTTP_DELETE_STR_C                tokens_string_da->http_delete_str_c
#define HTTP_POST_STR_C                  tokens_string_da->http_post_str_c
#define HTTP_PUT_STR_C                   tokens_string_da->http_put_str_c
#define API_SESSION_STR_C                tokens_string_da->api_session_str_c
#define API_INTERFACE_STR_C              tokens_string_da->api_interface_str_c
#define API_DEVICE_STR_C                 tokens_string_da->api_interface_str_c
#define API_TYPE_STR_C                   tokens_string_da->api_type_str_c
#define API_KEY_STR_C                    tokens_string_da->api_key_str_c
#define DATA_STR_C                       tokens_string_da->data_str_c
#define L_DATA_STR_C                     tokens_string_da->l_data_str_c
#define API_SERVICE_STR_C                tokens_string_da->api_service_str_c
#define API_CONFIGURATION_STR_C          tokens_string_da->api_configuration_str_c
#define API_USER_STR_C                   tokens_string_da->api_user_str_c
#define API_PAIRING_STR_C                tokens_string_da->api_pairing_str_c
#define TIME_STR_C                       tokens_string_da->time_str_c
#define USER_STR_C                       tokens_string_da->user_str_c
#define PASSWORD_STR_C                   tokens_string_da->password_str_c
#define PROFILE_STR_C                    tokens_string_da->profile_str_c
#define MEA_SESSIONID_STR_C              tokens_string_da->mea_sessionid_str_c
#define MEA_SESSION_STR_C                tokens_string_da->mea_session_str_c
#define START_STR_C                      tokens_string_da->start_str_c
#define STOP_STR_C                       tokens_string_da->stop_str_c
#define RESTART_STR_C                    tokens_string_da->restart_str_c
#define DETAIL_STR_C                     tokens_string_da->detail_str_c
#define ALL_STR_C                        tokens_string_da->all_str_c
#define ACTION_STR_C                     tokens_string_da->action_str_c
#define CREATE_STR_C                     tokens_string_da->create_str_c
#define COMMIT_STR_C                     tokens_string_da->commit_str_c
#define ROLLBACK_STR_C                   tokens_string_da->rollback_str_c

#else

#define DEVICE_PARAMETERS_STR_C          get_token_string_by_id(DEVICE_PARAMETERS_ID)
#define DEVICE_TYPE_ID_STR_C             get_token_string_by_id(DEVICE_TYPE_ID_ID)
#define FALSE_STR_C                      get_token_string_by_id(FALSE_ID)
#define HIGH_STR_C                       get_token_string_by_id(HIGH_ID)
#define ID_ENOCEAN_STR_C                 get_token_string_by_id(ID_ENOCEAN_ID)
#define LOW_STR_C                        get_token_string_by_id(LOW_ID)
#define NAME_STR_C                       get_token_string_by_id(NAME_ID)
#define ON_STR_C                         get_token_string_by_id(ON_ID)
#define REACHABLE_STR_C                  get_token_string_by_id(REACHABLE_ID)
#define STATE_STR_C                      get_token_string_by_id(STATE_ID)
#define TRUE_STR_C                       get_token_string_by_id(TRUE_ID)
#define XPL_BASIC_STR_C                  get_token_string_by_id(XPL_BASIC_ID)
#define XPL_CURRENT_STR_C                get_token_string_by_id(XPL_CURRENT_ID)
#define XPL_CONTROL_STR_C                get_token_string_by_id(XPL_CONTROL_ID)
#define XPL_DEVICE_STR_C                 get_token_string_by_id(XPL_DEVICE_ID)
#define XPL_ENOCEAN_ADDR_STR_C           get_token_string_by_id(XPL_ENOCEAN_ADDR_ID)
#define XPL_LAST_STR_C                   get_token_string_by_id(XPL_LAST_ID)
#define XPL_OUTPUT_STR_C                 get_token_string_by_id(XPL_OUTPUT_STR_C)
#define XPL_REQUEST_STR_C                get_token_string_by_id(XPL_REQUEST_ID)
#define XPL_SENSOR_STR_C                 get_token_string_by_id(XPL_SENSOR_ID)
#define XPL_TYPE_STR_C                   get_token_string_by_id(XPL_TYPE_ID)
#define DEVICE_ID_STR_C                  get_token_string_by_id(DEVICE_ID_ID)
#define DEVICE_NAME_STR_C                get_token_string_by_id(DEVICE_NAME_ID)
#define DEVICE_TYPE_ID_STR_C             get_token_string_by_id(DEVICE_TYPE_ID_ID)
#define DEVICE_LOCATION_ID_STR_C         get_token_string_by_id(DEVICE_LOCATION_ID_ID)
#define DEVICE_INTERFACE_NAME_STR_C      get_token_string_by_id(DEVICE_INTERFACE_NAME_ID)
#define DEVICE_INTERFACE_TYPE_NAME_STR_C get_token_string_by_id(DEVICE_INTERFACE_TYPE_NAME_ID)
#define INTERFACE_PARAMETERS_STR_C       get_token_string_by_id(INTERFACE_PARAMETERS_ID)
#define DEVICE_STATE_STR_C               get_token_string_by_id(DEVICE_STATE_ID)
#define DEVICE_TYPE_PARAMETERS_STR_C     get_token_string_by_id(DEVICE_TYPE_PARAMETERS_ID)
#define DEVICE_TODBFLAG_STR_C            get_token_string_by_id(DEVICE_TODBFLAG_ID)
#define DEVICE_ADDR_H_STR_C              get_token_string_by_id(DEVICE_ADDR_H_ID)
#define DEVICE_ADDR_L_STR_C              get_token_string_by_id(DEVICE_ADDR_L_ID)
#define DEVICES_STR_C                    get_token_string_by_id(DEVICES_ID)
#define INTERFACE_ID_STR_C               get_token_string_by_id(INTERFACE_ID_ID)
#define INTERFACE_TYPE_ID_STR_C          get_token_string_by_id(INTERFACE_TYPE_ID_ID)
#define INTERFACE_NAME_STR_C             get_token_string_by_id(INTERFACE_NAME_ID)
#define INTERFACE_STATE_STR_C            get_token_string_by_id(INTERFACE_STATE_ID)
#define LOCAL_XBEE_ADDR_H_STR_C          get_token_string_by_id(LOCAL_XBEE_ADDR_H_ID)
#define LOCAL_XBEE_ADDR_L_STR_C          get_token_string_by_id(LOCAL_XBEE_ADDR_L_ID)
#define INTERNAL_STR_C                   get_token_string_by_id(INTERNAL_ID)
#define XPLMSG_STR_C                     get_token_string_by_id(XPLMSG_ID)
#define XPLSOURCE_STR_C                  get_token_string_by_id(XPLSOURCE_ID)
#define XPLTARGET_STR_C                  get_token_string_by_id(XPLTARGET_ID)
#define XPLMSGTYPE_STR_C                 get_token_string_by_id(XPLMSGTYPE_ID)
#define XPLSCHEMA_STR_C                  get_token_string_by_id(XPLSCHEMA_ID)
#define XPL_TRIG_STR_C                   get_token_string_by_id(XPL_TRIG_ID)
#define XPL_STAT_STR_C                   get_token_string_by_id(XPL_STAT_ID)
#define XPL_CMND_STR_C                   get_token_string_by_id(XPL_CMND_ID)
#define XPL_PULSE_STR_C                  get_token_string_by_id(XPL_PULSE_ID)
#define XPL_CONTROLBASIC_STR_C           get_token_string_by_id(XPL_CONTROLBASIC_ID)
#define XPL_SENSORREQUEST_STR_C          get_token_string_by_id(XPL_SENSORREQUEST_ID)
#define HTTP_GET_STR_C                   get_token_string_by_id(HTTP_GET_ID)
#define HTTP_PUT_STR_C                   get_token_string_by_id(HTTP_PUT_ID)
#define HTTP_DELETE_STR_C                get_token_string_by_id(HTTP_DELETE_ID)
#define HTTP_POST_STR_C                  get_token_string_by_id(HTTP_POST_ID)
#define API_SESSION_STR_C                get_token_string_by_id(API_SESSION_ID)
#define API_SESSIONS_STR_C               get_token_string_by_id(API_SESSIONS_ID)
#define API_INTERFACE_STR_C              get_token_string_by_id(API_INTERFACE_ID)
#define API_DEVICE_STR_C                 get_token_string_by_id(API_DEVICE_ID)
#define API_SERVICE_STR_C                get_token_string_by_id(API_SERVICE_ID)
#define API_KEY_STR_C                    get_token_string_by_id(API_KEY_ID)
#define DATA_STR_C                       get_token_string_by_id(DATA_ID)
#define L_DATA_STR_C                     get_token_string_by_id(L_DATA_ID)
#define API_TYPE_STR_C                   get_token_string_by_id(API_TYPE_ID)
#define API_CONFIGURATION_STR_C          get_token_string_by_id(API_CONFIGURATION_ID)
#define API_USER_STR_C                   get_token_string_by_id(API_USER_ID)
#define API_PAIRING_STR_C                get_token_string_by_id(API_PAIRING_ID)
#define TIME_STR_C                       get_token_string_by_id(TIME_ID)
#define USER_STR_C                       get_token_string_by_id(USER_ID)
#define PASSWORD_STR_C                   get_token_string_by_id(PASSWORD_ID)
#define PROFILE_STR_C                    get_token_string_by_id(PROFILE_ID)
#define MEA_SESSIONID_STR_C              get_token_string_by_id(MEA_SESSIONID_ID)
#define MEA_SESSION_STR_C                get_token_string_by_id(MEA_SESSION_ID)
#define START_STR_C                      get_token_string_by_id(START_ID)
#define STOP_STR_C                       get_token_string_by_id(STOP_ID)
#define RESTART_STR_C                    get_token_string_by_id(RESTART_ID)
#define DETAIL_STR_C                     get_token_string_by_id(DETAIL_ID)
#define ALL_STR_C                        get_token_string_by_id(ALL_ID)
#define ACTION_STR_C                     get_token_string_by_id(ACTION_ID)
#define CREATE_STR_C                     get_token_string_by_id(CREATE_ID)
#define COMMIT_STR_C                     get_token_string_by_id(COMMIT_ID)
#define ROLLBACK_STR_C                   get_token_string_by_id(ROLLBACK_ID)

#endif

int16_t init_strings_da(void);
void release_strings_da(void);

#endif
