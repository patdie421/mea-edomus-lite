//
//  xPLServer.h
//
//  Created by Patrice DIETSCH on 17/10/12.
//
//

#ifndef __xPLServer_h
#define __xPLServer_h

#include <inttypes.h>
// #include <sqlite3.h>
#include <setjmp.h>
#include <pthread.h>

#include "cJSON.h"

extern char *xpl_vendorID;
extern char *xpl_deviceID;
extern char *xpl_instanceID;
extern char xpl_source[];

extern char *xpl_server_name_str;
extern char *xpl_server_xplin_str;
extern char *xpl_server_xplout_str;
extern char *xpl_server_senderr_str;
extern char *xpl_server_readerr_str;

extern pthread_t *_xPLServer_thread_id;
extern pthread_mutex_t _xPLServer_xPL_lock;

extern jmp_buf xPLServer_JumpBuffer;
   
typedef struct xplRespQueue_elem_s
{
   int id;
   cJSON *msgjson;
   uint32_t tsp;
} xplRespQueue_elem_t;


struct xplServer_start_stop_params_s
{
   cJSON *params_list;
//   sqlite3 *sqlite3_param_db;
};


// typedef int16_t (*xpl2_f)(cJSON *xplMsgJson, void *userdata);


//xPL_ServicePtr mea_getXPLServicePtr();

char          *mea_setXPLVendorID(char *value);
char          *mea_setXPLDeviceID(char *value);
char          *mea_setXPLInstanceID(char *value);

char          *mea_getXPLInstanceID(void);
char          *mea_getXPLDeviceID(void);
char          *mea_getXPLVendorID(void);

//char          *mea_getXPLSource();
char          *mea_getMyXPLAddr(void);

uint16_t       mea_sendXPLMessage2(cJSON *xplMsgJson);
cJSON         *mea_readXPLResponse(int id);
uint32_t       mea_getXplRequestId(void);
//int16_t        mea_xPLServerIsActive();

/*
extern xPL_MessagePtr xPL_AllocMessage();
extern xPL_NameValueListPtr xPL_AllocNVList();
*/

int           start_xPLServer(int my_id, void *data, char *errmsg, int l_errmsg);
int           stop_xPLServer(int my_id, void *data, char *errmsg, int l_errmsg);
int           restart_xPLServer(int my_id, void *data, char *errmsg, int l_errmsg);

//int16_t       displayXPLMsg(xPL_MessagePtr theMessage);

//cJSON        *mea_xPL2JSON(xPL_MessagePtr msg);
int           mea_sendXplMsgJson(cJSON *xplMsgJson);

int16_t       mea_xPLSendMessage2(char *data, int l_data);
#endif
