/**
 * \file tokens.c
 * \brief gestion de token (association identifiant/chaine de charactères).
 * \author Patrice DIETSCH
 * \date 29/10/12
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include "mea_string_utils.h"
#include "mea_verbose.h"
#include "uthash.h"

#include "tokens.h"

struct token_s
/**
 * \brief     représentation "interne" d'un token.
 */
{
   const char *str;          /*!< pointeur sur la chaine du token */
   const enum token_id_e id; /*!< identifiant du token   */
};
#define TOKEN_S_INIT  { .str="", .id=_UNKNOWN }

static struct token_s tokens_list[]={ /// liste de tous les tokens connus. Le dernier élément doit être {NULL, 0}

   {"action",                     ACTION_ID},
   {"ADDR_H",                     ADDR_H_ID},
   {"ADDR_L",                     ADDR_L_ID},
   {"algo",                       XPL_ALGO_ID},
   {"all",                        ALL_ID},
   {"analog",                     ANALOG_ID},
   {"analog_in",                  ANALOG_IN_ID},
   {"analog_out",                 ANALOG_OUT_ID},
   {"api_key",                    API_KEY_ID},
   {"aref11",                     XPL_AREF11_ID},
   {"aref5",                      XPL_AREF5_ID},
   {"basic",                      XPL_BASIC_ID},
   {"color",                      COLOR_ID},
   {"command",                    XPL_COMMAND_ID},
   {"commit",                     COMMIT_ID},
   {"CONFIGURATION",              API_CONFIGURATION_ID},
   {"control",                    XPL_CONTROL_ID},
   {"control.basic",              XPL_CONTROLBASIC_ID},
   {"create",                     CREATE_ID},
   {"current",                    XPL_CURRENT_ID},
   {"data",                       DATA_ID},
   {"data1",                      XPL_DATA1_ID},
   {"dec",                        DEC_ID},
   {"DELETE",                     HTTP_DELETE_ID},
   {"detail",                     DETAIL_ID},
   {"device",                     XPL_DEVICE_ID},
   {"device_id",                  DEVICE_ID_ID},
   {"device_interface_name",      DEVICE_INTERFACE_NAME_ID},
   {"device_interface_type_name", DEVICE_INTERFACE_TYPE_NAME_ID},
   {"device_location_id",         DEVICE_LOCATION_ID_ID},
   {"device_name",                DEVICE_NAME_ID},
   {"device_parameters",          DEVICE_PARAMETERS_ID},
   {"device_state",               DEVICE_STATE_ID},
   {"device_type_id",             DEVICE_TYPE_ID_ID},
   {"device_type_parameters",     DEVICE_TYPE_PARAMETERS_ID},
   {"devices",                    DEVICES_ID},
   {"digital",                    DIGITAL_ID},
   {"digital_in",                 DIGITAL_IN_ID},
   {"digital_out",                DIGITAL_OUT_ID},
   {"energy",                     XPL_ENERGY_ID},
   {"ENOCEAN_ADDR",               ENOCEAN_ADDR_ID},
   {"false",                      FALSE_ID},
   {"generic",                    GENERIC_ID},
   {"GET",                        HTTP_GET_ID},
   {"high",                       HIGH_ID},
   {"ID_ENOCEAN",                 ID_ENOCEAN_ID},
   {"ID_XBEE",                    ID_XBEE_ID},
   {"inc",                        INC_ID},
   {"input",                      XPL_INPUT_ID},
   {"INTERFACE",                  API_INTERFACE_ID},
   {"interface_id",               INTERFACE_ID_ID},
   {"interface_location_id",      INTERFACE_LOCATION_ID_ID},
   {"interface_name",             INTERFACE_NAME_ID},
   {"interface_parameters",       INTERFACE_PARAMETERS_ID},
   {"interface_state",            INTERFACE_STATE_ID},
   {"interface_type_id",          INTERFACE_TYPE_ID_ID},
   {"internal",                   INTERNAL_ID},
   {"l_data",                     L_DATA_ID},
   {"last",                       XPL_LAST_ID},
   {"LOCAL_XBEE_ADDR_H",          LOCAL_XBEE_ADDR_H_ID},
   {"LOCAL_XBEE_ADDR_L",          LOCAL_XBEE_ADDR_L_ID},
   {"low",                        LOW_ID},
   {"Mea-Session",                MEA_SESSION_ID},
   {"Mea-SessionId",              MEA_SESSIONID_ID},
   {"msgtype",                    XPLMSGTYPE_ID},
   {"name",                       NAME_ID},
   {"on",                         ON_ID},
   {"onoff",                      ONOFF_ID},
   {"output",                     XPL_OUTPUT_ID},
   {"PAIRING",                    API_PAIRING_ID},
   {"password",                   PASSWORD_ID},
   {"POST",                       HTTP_POST_ID},
   {"power",                      XPL_POWER_ID},
   {"profile",                    PROFILE_ID},
   {"pulse",                      XPL_PULSE_ID},
   {"PUT",                        HTTP_PUT_ID},
   {"pwm",                        PWM_ID},
   {"raw",                        RAW_ID},
   {"reachable",                  REACHABLE_ID},
   {"request",                    XPL_REQUEST_ID},
   {"restart",                    RESTART_ID},
   {"rollback",                   ROLLBACK_ID},
   {"fullname",                   FULLNAME_ID},
   {"username",                   USERNAME_ID},
   {"parameters",                 PARAMETERS_ID},
   {"update",                     UPDATE_ID},
   {"schema",                     XPLSCHEMA_ID},
   {"sensor",                     XPL_SENSOR_ID},
   {"sensor.request",             XPL_SENSORREQUEST_ID},
   {"SERVICE",                    API_SERVICE_ID},
   {"SESSION",                    API_SESSION_ID},
   {"source",                     XPLSOURCE_ID},
   {"start",                      START_ID},
   {"state",                      STATE_ID},
   {"stop",                       STOP_ID},
   {"target",                     XPLTARGET_ID},
   {"temp",                       XPL_TEMP_ID},
   {"time",                       TIME_ID},
   {"tmp36",                      XPL_TMP36_ID},
   {"todbflag",                   TODBFLAG_ID},
   {"true",                       TRUE_ID},
   {"type",                       XPL_TYPE_ID},
   {"typeoftype",                 TYPEOFTYPE_ID},
   {"unit",                       UNIT_ID},
   {"USER",                       API_USER_ID},
//   {"user",                       USER_ID},
   {"variable",                   VARIABLE_ID},
   {"voltage",                    XPL_VOLTAGE_ID},
   {"watchdog",                   XPL_WATCHDOG_ID},
   {"xpl-cmnd",                   XPL_CMND_ID},
   {"xplmsg",                     XPLMSG_ID},
   {"xpl-stat",                   XPL_STAT_ID},
   {"xpl-trig",                   XPL_TRIG_ID},
   {"dev",                        DEV_ID},
   {"id_type",                    ID_TYPE_ID},
   {"id_interface",               ID_INTERFACE_ID},
   {"description",                DESCRIPTION_ID},
   {"id_sensor_actuator",         ID_SENSOR_ACTUATOR_ID},
   {"METRIC",                     API_METRIC_ID},
   {"plugin_paramters",           PLUGIN_PARAMETERS_ID},
   {"MEAPATH",                    MEAPATH_ID},

   {NULL,0}
};


#if TOKEN_BY_SEQUENTIAL_READ == 1
void init_tokens()
{
}


void release_tokens()
{
}


char *get_token_string_by_id(enum token_id_e id)
/**
 * \brief     recherche un token par son id
 * \param     id  identifiant du token.
 * \return    pointeur sur la chaine du token ou NULL s'il n'existe pas.
 */
{
   for(int i=0;tokens_list[i].str;i++) {
      if(tokens_list[i].id == id)
         return (char *)tokens_list[i].str;
   }
   return NULL;
}


enum token_id_e get_token_id_by_string(char *str)
/**
 * \brief     recherche l'id un token par la valeur de sa chaine
 * \details   les chaines sont comparées "en minuscule", c'est à dire qu'on ne tient pas compte de la "case" pour trouver la chaine. Attention cependant aux caractères accentués qui ne sont certainement pas correctement traité.
 * \param     token (chaine) à trouver.
 * \return    id du token ou _UNKNOWN (-1) sinon
 */
{
   if(!str)
      return _UNKNOWN;
   
   for(int i=0;tokens_list[i].str;i++) {
      if(mea_strcmplower((char *)tokens_list[i].str, str) == 0)
         return tokens_list[i].id;
   }
   return _UNKNOWN;
}


#elif TOKEN_BY_INDEX == 1
struct tokens_index_s
{
   int16_t nb_tokens;
   int16_t *index_by_string;
   int16_t *index_by_id;
};
#define TOKEN_INDEX_S_INIT { .nb_tokens=0, .index_by_string=NULL, .index_by_id=NULL }

struct tokens_index_s *tokens_index=NULL;

int qsort_compare_ids(const void * a, const void * b)
{
   return (int)(tokens_list[*(int16_t *)a].id) - (int)(tokens_list[*(int16_t *)b].id);
}


int qsort_compare_strings(const void *a, const void *b)
{
   return mea_strcmplower2((char *)tokens_list[*(int16_t *)a].str, (char *)tokens_list[*(int16_t *)b].str);
}


void init_tokens()
{
   if(tokens_index!=NULL)
      free(tokens_index);
   tokens_index=(struct tokens_index_s *)malloc(sizeof(struct tokens_index_s));
   if(tokens_index==NULL) {
      VERBOSE(2) {
         char err_str[256];
         strerror_r(errno, err_str, sizeof(err_str)-1);
         mea_log_printf("%s (%s) : %s - %s\n", ERROR_STR, __func__, MALLOC_ERROR_STR, err_str);
      }
      return;
   }
   tokens_index->nb_tokens=0;
   tokens_index->index_by_id=NULL;
   tokens_index->index_by_string=NULL;
   
   int i=0;
   for(;tokens_list[i].str;i++); // comptage du nombre de tokens
   if(i>0) {
      tokens_index->nb_tokens=i;
      tokens_index->index_by_string = (int16_t *)malloc(tokens_index->nb_tokens*sizeof(int16_t));
      if(tokens_index->index_by_string == NULL) {
         tokens_index->nb_tokens=0;
         DEBUG_SECTION PRINT_MALLOC_ERROR;
         return;
      }
      tokens_index->index_by_id = (int16_t *)malloc(tokens_index->nb_tokens*sizeof(int16_t));
      if(tokens_index->index_by_id  == NULL) {
         tokens_index->nb_tokens=0;
         DEBUG_SECTION PRINT_MALLOC_ERROR;
         free(tokens_index->index_by_string);
         tokens_index->index_by_string=NULL;
         return;
      }

      for(int i=0;i<tokens_index->nb_tokens;i++) {
         tokens_index->index_by_string[i]=i;
         tokens_index->index_by_id[i]=i;
      }

      qsort (tokens_index->index_by_id, tokens_index->nb_tokens, sizeof (int16_t), qsort_compare_ids);
      qsort (tokens_index->index_by_string, tokens_index->nb_tokens, sizeof (int16_t), qsort_compare_strings);
   }
}


void release_tokens()
{
   if(tokens_index) {
      if(tokens_index->index_by_id) {
         free(tokens_index->index_by_id);
         tokens_index->index_by_id=NULL;
      }

      if(tokens_index->index_by_string) {
         free(tokens_index->index_by_string);
         tokens_index->index_by_string=NULL;
      }
      free(tokens_index);
   }
}


char *get_token_string_by_id(enum token_id_e id)
{
   if(tokens_index==NULL) {
#if TOKENS_AUTOINIT == 1
      init_tokens();
      if(tokens_index==NULL)
         return NULL;
#else
      return NULL;
#endif
   }
   
   int16_t start = 0;
   int16_t end = tokens_index->nb_tokens - 1;
   int16_t _cmpres = 0;
   do {
      int16_t middle=(end + start) / 2;
      if(middle<0)
         return NULL;

      _cmpres=(int)(tokens_list[tokens_index->index_by_id[middle]].id) - (int)id;

      if(_cmpres==0)
         return (char *)tokens_list[tokens_index->index_by_id[middle]].str;
      if(_cmpres<0)
         start=middle+1;
      if(_cmpres>0)
         end=middle-1;
   }
   while(start<=end);
   
   return NULL;
}


enum token_id_e get_token_id_by_string(char *str)
{
   if(!str) {
      return _UNKNOWN;
   }

   if(tokens_index==NULL) {
#if TOKENS_AUTOINIT == 1
      init_tokens();
      if(tokens_index==NULL) {
         return _UNKNOWN;
      }
#else
      return _UNKNOWN;
#endif
   }
   if(!tokens_index->index_by_string || tokens_index->nb_tokens<1)
      return _UNKNOWN;
   
   int16_t start = 0;
   
   int16_t end = tokens_index->nb_tokens - 1;
   int _cmpres = 0;
  
   do {
      int16_t middle = (end + start) / 2;
      
      if(middle<0) {
         return -1;
      }
      
      _cmpres=mea_strcmplower2((char *)tokens_list[tokens_index->index_by_string[middle]].str, str);
      if(_cmpres==0) {
         return tokens_list[tokens_index->index_by_string[middle]].id;
      }
      if(_cmpres<0) {
         start=middle+1;
      }
      if(_cmpres>0) {
         end=middle-1;
      }
   }
   while(start<=end);
   
   return -1;
}

#elif TOKEN_BY_HASH_TABLE == 1
#include "uthash.h"

struct tokens_hash_s
{
   struct token_s *token;
   char *str;
   UT_hash_handle hh_token_by_string;
   UT_hash_handle hh_token_by_id;
};


struct tokens_hash_s *tokens_hash_by_string = NULL;
struct tokens_hash_s *tokens_hash_by_id = NULL;
static int _token_max_string_size=0;
static char * _token_string_buf=NULL;

void init_tokens()
/**
 * \brief     initialisation des tables de hashage
 */
{
   struct tokens_hash_s *s = NULL;
   
   tokens_hash_by_string = NULL;
   tokens_hash_by_id = NULL;
   int l=0;
 
   for(int i=0;tokens_list[i].str;i++) {
      HASH_FIND(hh_token_by_string, tokens_hash_by_string, tokens_list[i].str, strlen(tokens_list[i].str), s);
      if(s) {
         DEBUG_SECTION mea_log_printf("%s (%s) : string key duplicated (%s/%d)\n",DEBUG_STR, __func__, tokens_list[i].str, tokens_list[i].id);
         continue;
      }
      HASH_FIND(hh_token_by_id, tokens_hash_by_id, &(tokens_list[i].id), sizeof(tokens_list[i].id), s);
      if(s) {
         DEBUG_SECTION mea_log_printf("%s (%s) : id key duplicated (%s/%d)\n",DEBUG_STR, __func__, tokens_list[i].str, tokens_list[i].id);
         continue;
      }

      s = malloc(sizeof(struct tokens_hash_s));
      if(!s) {
         DEBUG_SECTION PRINT_MALLOC_ERROR;
         return;
      }
      s->token=&tokens_list[i];
      s->str = mea_string_alloc_and_copy((char *)(s->token->str));
      if(!s->str) {
         DEBUG_SECTION PRINT_MALLOC_ERROR;
         free(s);
         s=NULL;
         return;
      }
      mea_strtolower(s->str);
      l=(int)strlen(s->str);
      if(l>_token_max_string_size)
         _token_max_string_size=l;
      HASH_ADD_KEYPTR( hh_token_by_string, tokens_hash_by_string, s->str, strlen(s->str), s );
      HASH_ADD_KEYPTR( hh_token_by_id, tokens_hash_by_id, &(s->token->id), sizeof(s->token->id), s );
   }

   if(_token_max_string_size>0) {
      _token_string_buf=(char *)malloc(_token_max_string_size+1);
   }
}


void release_tokens()
{
   if(tokens_hash_by_id) {
      struct tokens_hash_s *s = NULL, *ts = NULL; 
      HASH_ITER(hh_token_by_string, tokens_hash_by_string, s, ts) { 
//         if(s->str) {
//            free(s->str);
//            s->str=NULL;
//         }
         HASH_DELETE(hh_token_by_string, tokens_hash_by_string, s);
//         free(s);
      } 
      tokens_hash_by_string=NULL;
   }
  
   if(tokens_hash_by_id) { 
      struct tokens_hash_s *s = NULL, *ts = NULL;
      HASH_ITER(hh_token_by_id, tokens_hash_by_id, s, ts) {
         HASH_DELETE(hh_token_by_id, tokens_hash_by_id, s);
         if(s->str) {
            free(s->str);
            s->str=NULL;
         }
         free(s);
      } 
      tokens_hash_by_id=NULL;
   }

   if(_token_string_buf) {
      free(_token_string_buf);
      _token_string_buf=NULL;
   }
}


char *get_token_string_by_id(enum token_id_e id)
/**
 * \brief     recherche un token par son id
 * \param     id  identifiant du token.
 * \return    pointeur sur la chaine du token ou NULL s'il n'existe pas.
 */
{
   struct tokens_hash_s *s = NULL;

   if(tokens_hash_by_string == NULL) {
#if TOKENS_AUTOINIT == 1
      init_tokens();
#else
      return NULL;
#endif
   }
      
   HASH_FIND(hh_token_by_id, tokens_hash_by_id, &id, sizeof(id), s);
   
   if(s) {
      return (char *)(s->token->str);
   }

   return NULL;
}


enum token_id_e get_token_id_by_string(char *str)
/**
 * \brief     recherche l'id un token
 * \param     token (chaine) à trouver.
 * \return    id du token
 */
{
   struct tokens_hash_s *s = NULL;

   if(!str) {
      return _UNKNOWN;
   }
      
   if(tokens_hash_by_id == NULL) {
#if TOKENS_AUTOINIT == 1
      init_tokens();
#else
      return _UNKNOWN;
#endif
   }

   if(_token_string_buf == NULL) {
      return _UNKNOWN;
   }

//   strncpy(_token_string_buf, str, _token_max_string_size-1);
   strncpy(_token_string_buf, str, _token_max_string_size);
   _token_string_buf[_token_max_string_size]=0;
   mea_strtolower(_token_string_buf); 

   HASH_FIND(hh_token_by_string, tokens_hash_by_string, _token_string_buf, strlen(_token_string_buf), s);

   if(s) {
      return s->token->id;
   }

   return _UNKNOWN;
}

#endif
