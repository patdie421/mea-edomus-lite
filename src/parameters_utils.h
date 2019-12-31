//
//  parameters_mgr.h
//
//  Created by Patrice DIETSCH on 24/10/12.
//
//

#ifndef __parameters_utils_h
#define __parameters_utils_h

#include <time.h>

#include "uthash.h"
#include "cJSON.h"

#define PARAM_SYSTEM_ERR 1
#define PARAM_SYSTAX_ERR 2
#define PARAM_UNKNOW_ERR 3
#define PARAM_NOVAL_ERR  4

#define PARSED_PARAMETERS_CACHING_ENABLED    1
#define PARSED_PARAMETERS_CACHING_AUTOINIT   1
#define PARSED_PARAMETERS_CACHING_MAX      200


struct assoc_s
{
   int16_t val1;
   int16_t val2;
};


typedef union
{
   int i;
   long l;
   float f;
   char *s;
} value_token_t;


typedef struct parsed_parameter_s
{
   char *label;
   enum { INT=1, LONG=2, FLOAT=3, STRING=4 } type;
   value_token_t value;
} parsed_parameter_t;


typedef struct parsed_parameters_s
{
   parsed_parameter_t *parameters;
   uint16_t nb;
#ifdef PARSED_PARAMETERS_CACHING_ENABLED
   int8_t from_cache;
   uint16_t in_use;
#endif
} parsed_parameters_t;


int16_t parsed_parameters_init(void);

parsed_parameters_t *alloc_parsed_parameters(char *parameters_string, char *parameters_to_find[], int *nb_params, int *err, int value_to_upper_flag);
void release_parsed_parameters(parsed_parameters_t **params);
void display_parsed_parameters(parsed_parameters_t *params);
int parsed_parameters_get_param_string(char *params, char *valid_params[], int param_id, char *param, int param_l);
cJSON *parsed_parameters_to_json_alloc(parsed_parameters_t *mpp);
cJSON *parsed_parameters_json_alloc(char *parameters_string, char *parameters_to_find[], int *nb_params, int *err, int value_to_upper);
int16_t is_in_assocs_list(struct assoc_s *assocs_list, int val1, int val2);

int16_t parsed_parameters_clean_older_than(time_t t);
int16_t parsed_parameters_clean_all(int force);
#endif
