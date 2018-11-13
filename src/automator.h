#ifndef __automator_h
#define __automator_h

#include "cJSON.h"

#define VALUE_MAX_STR_SIZE 41

extern char *inputs_rules;
extern char *outputs_rules;

extern cJSON *_inputs_rules;
extern cJSON *_outputs_rules;

int automator_sendxpl2(cJSON *parameters);

int automator_init(char *rulesfile);
cJSON *automator_load_rules(char *rules);
int automator_reload_rules_from_file(char *rulesfile);

int automator_matchInputsRules(cJSON *rules, cJSON *message_json);
int automator_playOutputRules(cJSON *rules);
int automator_reset_inputs_change_flags(void);
int automator_send_all_inputs(void);
char *automator_inputs_table_to_json_string_alloc(void);
#endif
