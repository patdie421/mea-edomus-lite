/*
 *  xbee.h
 *
 *  Created by Patrice Dietsch on 18/08/12.
 *  Copyright 2012 -. All rights reserved.
 *
 */
#ifndef __xbee_h
#define __xbee_h

#include <inttypes.h>
#include <pthread.h>
#include <semaphore.h>

#include "mea_queue.h"

#define XBEE_MAX_CALLBACK 16

#define XBEE_ERR_NOERR		     0
#define XBEE_ERR_TIMEOUT        1 // sortie en timeout du sémaphore de synchro
#define XBEE_ERR_SELECT         2
#define XBEE_ERR_READ           3
#define XBEE_ERR_TRAMETYPE      4
#define XBEE_ERR_UNKNOWN        5
#define XBEE_ERR_STARTTRAME     6
#define XBEE_ERR_CHECKSUM       7
#define XBEE_ERR_BADRESP        8
#define XBEE_ERR_SYS            9 // erreur systeme
#define XBEE_ERR_HOSTTABLEFULL 10
#define XBEE_ERR_HOSTNOTFUND   11
#define XBEE_ERR_IN_CALLBACK   12
#define XBEE_ERR_DOWN          13
#define XBEE_MAX_NB_ERROR      13

#define XBEE_DIGITAL_OUT_HIGH 0x05
#define XBEE_DIGITAL_OUT_LOW  0x04

#define XBEE_S2_IO_D0  "D0"
#define XBEE_S2_IO_D1  "D1"
#define XBEE_S2_IO_D2  "D2"
#define XBEE_S2_IO_D3  "D3"
#define XBEE_S2_IO_D4  "D4"
#define XBEE_S2_IO_D5  "D5"
#define XBEE_S2_IO_D6  "D6"
#define XBEE_S2_IO_D7  "D7"
#define XBEE_S2_IO_D8  "P0"
#define XBEE_S2_IO_D9  "P1"
#define XBEE_S2_IO_D10 "P2"

#define XBEE_S2_I_A0   "D0"
#define XBEE_S2_I_A1   "D1"
#define XBEE_S2_I_A2   "D2"
#define XBEE_S2_I_A3   "D3"


#define XBEE_DIO0_AD0  0x0001
#define XBEE_DIO0      0x0001
#define XBEE_AD0       0x0001
#define XBEE_DIO1_AD1  0x0002
#define XBEE_DIO1      0x0002
#define XBEE_AD1       0x0002
#define XBEE_DIO2_AD2  0x0004
#define XBEE_DIO2      0x0004
#define XBEE_AD2       0x0004
#define XBEE_DIO3_AD3  0x0008
#define XBEE_DIO3      0x0008
#define XBEE_AD3       0x0008
#define XBEE_DIO4      0x0010
#define XBEE_DIO5_ASS  0x0020
#define XBEE_DIO5      0x0020
#define XBEE_DIO6_RTS  0x0040
#define XBEE_DIO6      0x0040
#define XBEE_DIO7_CTS  0x0080
#define XBEE_DIO7      0x0080
#define XBEE_DIO8      0x0100 // xbee S2 not use 
#define XBEE_DIO9      0X0200 // xbee S2 not use
#define XBEE_DIO10_PWM 0x0400
#define XBEE_DIO10     0x0400
#define XBEE_DIO11_RSS 0x0800
#define XBEE_DIO11     0x0800
#define XBEE_DIO12_CD  0x1000
#define XBEE_DIO12     0x1000

#define XBEE_NB_ID_PIN 14
extern char *xbee_at_io_id_list[XBEE_NB_ID_PIN];

#define XBEE_MAX_USER_FRAME_ID 200

#define XBEE_ND_FRAME_ID       201

#define XBEE_MAX_HOSTS 255

#define XBEE_MAX_FRAME_SIZE 80


typedef int (*callback_f)(int id, unsigned char *cmd, uint16_t l_cmd, void *data, char *addr_h, char *addr_l);


typedef struct xbee_host_s
{
   char name[21];
   //	int64_t l_addr_64;
   
   int32_t l_addr_64_h;
   int32_t l_addr_64_l;
   
   int64_t l_addr_16; 
   
   char addr_64_h[4];
   char addr_64_l[4];
   
   char addr_16[2];
} xbee_host_t;


typedef struct xbee_hosts_table_s
{
   uint16_t nb_hosts;
   uint16_t max_hosts;
   xbee_host_t **hosts_table;
} xbee_hosts_table_t;


typedef struct xbee_xd_s
{
   int             fd;
   pthread_t       read_thread;
   pthread_cond_t  sync_cond;
   pthread_mutex_t sync_lock;
   pthread_mutex_t xd_lock;
   char		       serial_dev_name[255];
   int             speed;
   mea_queue_t		   *queue;
   callback_f      io_callback;
   void           *io_callback_data;
   callback_f      commissionning_callback;
   void           *commissionning_callback_data;
   callback_f      dataflow_callback;
   void           *dataflow_callback_data;
   xbee_hosts_table_t
                  *hosts;
   uint16_t        frame_id;
   int16_t         signal_flag;
} xbee_xd_t;


struct xbee_cmd_response_s
{
   unsigned char frame_type;
   unsigned char frame_id;
   unsigned char at_cmd[2];
   unsigned char cmd_status;
   unsigned char at_cmd_data[];
} __attribute__((packed));


struct xbee_map_io_data_s
{
   unsigned char frame_type;
   unsigned char addr_64_h[4];
   unsigned char addr_64_l[4];
   unsigned char addr_16[2];
   unsigned char receive_options;
   unsigned char wire_sensors;
   unsigned char io_values[];
} __attribute__((packed));


struct xbee_remote_cmd_response_s
{
   unsigned char frame_type;
   unsigned char frame_id;
   unsigned char addr_64_h[4];
   unsigned char addr_64_l[4];
   unsigned char addr_16[2];
   unsigned char at_cmd[2];
   unsigned char cmd_status;
   unsigned char at_cmd_data[];
} __attribute__((packed));


struct xbee_receive_packet_s
{
   unsigned char frame_type;
   unsigned char addr_64_h[4];
   unsigned char addr_64_l[4];
   unsigned char addr_16[2];
   unsigned char receive_options;
   unsigned char data[];
} __attribute__((packed));


struct xbee_node_identification_response_s
{
   unsigned char frame_type;
   unsigned char addr_64_h[4];
   unsigned char addr_64_l[4];
   unsigned char addr_16[2];
   unsigned char receive_options;
   unsigned char remote_addr_16[2];
   unsigned char remote_addr_64_h[4];
   unsigned char remote_addr_64_l[4];
   unsigned char nd_data[]; // cf 
} __attribute__((packed));


struct xbee_node_identification_nd_data_s // to shift and map after NI
{
   unsigned char parent_16bits[2];
   unsigned char device_type;
   unsigned char source_event;
   unsigned char digi_profile_id[2];
   unsigned char manufacturer_id[2];
} __attribute__((packed));


int xbee_init(xbee_xd_t *xd, char *dev, int speed);
int xbee_reinit(xbee_xd_t *xd);
void xbee_close(xbee_xd_t *xd);

int16_t xbee_atCmdSendAndWaitResp(xbee_xd_t *xd,
                                  xbee_host_t *destination,
                                  unsigned char *frame_data, // zone donnee d'une trame
                                  uint16_t l_frame_data, // longueur zone donnee
                                  unsigned char *resp,
                                  uint16_t *l_resp,
                                  int16_t *xbee_err);

int16_t xbee_atCmdSend(xbee_xd_t *xd,
                       xbee_host_t *destination,
                       unsigned char *frame_data,
                       uint16_t l_frame_data,
                       int16_t *xbee_err);

mea_error_t xbee_get_host_by_name(xbee_xd_t *xd, xbee_host_t *host, char *name, int16_t *nerr);
mea_error_t xbee_get_host_by_addr_64(xbee_xd_t *xd, xbee_host_t *host, uint32_t addr_64_h, uint32_t addr_64_l, int16_t *nerr);

mea_error_t xbee_set_iodata_callback(xbee_xd_t *xd, callback_f f);
mea_error_t xbee_set_iodata_callback2(xbee_xd_t *xd, callback_f f, void *data);
mea_error_t xbee_remove_iodata_callback(xbee_xd_t *xd);

mea_error_t xbee_set_commissionning_callback(xbee_xd_t *xd, callback_f f);
mea_error_t xbee_set_commissionning_callback2(xbee_xd_t *xd, callback_f f, void *data);
mea_error_t xbee_remove_commissionning_callback(xbee_xd_t *xd);

mea_error_t xbee_set_dataflow_callback(xbee_xd_t *xd, callback_f f);
mea_error_t xbee_set_dataflow_callback2(xbee_xd_t *xd, callback_f f, void *data);
mea_error_t xbee_remove_dataflow_callback(xbee_xd_t *xd);

int16_t xbee_start_network_discovery(xbee_xd_t *xd, int16_t *nerr);

void xbee_perror(int16_t nerr);
void xbee_clean_xd(xbee_xd_t *xd);

int hosts_table_display(xbee_hosts_table_t *table);

#endif
