/*
 *  globals.h
 *
 *  Created by Patrice Dietsch on 25/08/12.
 *  Copyright 2012 -. All rights reserved.
 *
 */

#ifndef __globals_h
#define __globals_h

#include "mea_queue.h"

#define __MEA_EDOMUS_VERSION__ "0.0-lite"

// base de paramétrage

#define MEA_PATH               1
#define CONFIG_FILE            3
#define HTML_PATH              6
#define LOG_PATH               7
#define PLUGINS_PATH           8
#define XPL_VENDORID           9 
#define XPL_DEVICEID          10
#define XPL_INSTANCEID        11
#define VERBOSELEVEL          12
#define HTTP_PORT             13
#define INTERFACE             16
#define RULES_FILE            17
#define RULES_FILES_PATH      18
#define COLLECTOR_ID          19
#define DRIVERS_PATH          20 
#define MAX_LIST_SIZE         21

#define CURRENT_PARAMS_DB_VERSION 0

// voir ou mettre
#define UNIT_WH 1 // Watt/Heure
#define UNIT_W  2 // Watt
#define UNIT_C  3 // degré C
#define UNIT_V  4 // Volt
#define UNIT_H  5 // pourcentage humidité

#define NOPTHREADJOIN 1

#endif
