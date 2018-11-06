/*
 *  globals.h
 *
 *  Created by Patrice Dietsch on 25/08/12.
 *  Copyright 2012 -. All rights reserved.
 *
 */

#ifndef __globals_h
#define __globals_h

#include <sqlite3.h>

#include "mea_queue.h"

#define __MEA_EDOMUS_VERSION__ "0.0-lite"

// base de paramétrage

#define MEA_PATH               1
#define SQLITE3_DB_PARAM_PATH  2
#define CONFIG_FILE            3
#define PHPCGI_PATH            4
#define PHPINI_PATH            5
#define GUI_PATH               6
#define LOG_PATH               7
#define PLUGINS_PATH           8
#define VENDOR_ID              9 
#define DEVICE_ID             10
#define INSTANCE_ID           11
#define VERBOSELEVEL          12
#define GUIPORT               13
#define PHPSESSIONS_PATH      14
#define PARAMSDBVERSION       15
#define INTERFACE             16
#define RULES_FILE            17
#define RULES_FILES_PATH      18
#define COLLECTOR_ID          19
#define DRIVERS_PATH          20 
#define MAX_LIST_SIZE         21

#define CURRENT_PARAMS_DB_VERSION 0
sqlite3 *get_sqlite3_param_db(void);

// voir ou mettre
#define UNIT_WH 1 // Watt/Heure
#define UNIT_W  2 // Watt
#define UNIT_C  3 // degré C
#define UNIT_V  4 // Volt
#define UNIT_H  5 // pourcentage humidité

#define NOPTHREADJOIN 1

// extern FILE *dbgfd;

#endif
