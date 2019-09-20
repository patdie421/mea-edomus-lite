#ifndef __enocean_h
#define __enocean_h
 
#include <termios.h>
#include <unistd.h>
#include <inttypes.h>
#include <pthread.h>
#include <semaphore.h>
 
//#include "error.h"
#include "mea_queue.h"

// valeurs max
#define ENOCEAN_MAX_FRAME_SIZE   0xFFFF // taille maximum d'une trame

// numéros d'erreur 
// defined datagram type
#define ENOCEAN_RADIO_ERP1         0x01 // Radio telegram
#define ENOCEAN_RESPONSE           0x02 // Response to any packet 3
#define ENOCEAN_RADIO_SUB_TEL      0x03 // Radio subtelegram
#define ENOCEAN_EVENT              0x04 // Event message
#define ENOCEAN_COMMON_COMMAND     0x05 // Common command
#define ENOCEAN_SMART_ACK_COMMAND  0x06 // Smart Ack command
#define ENOCEAN_REMOTE_MAN_COMMAND 0x07 // Remote management command
#define ENOCEAN_RADIO_MESSAGE      0x09 // Radio message
#define ENOCEAN_RADIO_ERP2         0x0A // ERP2 protocol radio telegram

#define ENOCEAN_RPS_TELEGRAM       0xF6
#define ENOCEAN_1BS_TELEGRAM       0xD5
#define ENOCEAN_4BS_TELEGRAM       0xA5
#define ENOCEAN_VLD_TELEGRAM       0xD2

#define ENOCEAN_UTE_TELEGRAM       0xD4

// Response
#define ENOCEAN_RET_OK             0x00
#define ENOCEAN_RET_ERROR          0x01
#define ENOCEAN_RET_NOT_SUPPORTED  0x02
#define ENOCEAN_RET_WRONG_PARAM    0x03
#define ENOCEAN_RET_OPERATION_DENIED 0x04


#define ENOCEAN_ERR_NOERR             0
#define ENOCEAN_ERR_TIMEOUT           1
#define ENOCEAN_ERR_SELECT            2
#define ENOCEAN_ERR_READ              3
#define ENOCEAN_ERR_UNKNOWN           4
#define ENOCEAN_ERR_STARTFRAME        5
#define ENOCEAN_ERR_CHECKSUM          6
#define ENOCEAN_ERR_BADRESP           7
#define ENOCEAN_ERR_SYS               8 // erreur systeme
#define ENOCEAN_ERR_CRC8H             9
#define ENOCEAN_ERR_CRC8D            10
#define ENOCEAN_ERR_CHARTIMEOUT      11
#define ENOCEAN_ERR_OUTOFRAGE        12
#define ENOCEAN_ERR_DATALENGTH       13
#define ENOCEAN_MAX_NB_ERROR         13

// gestion des reprises
#define ENOCEAN_TIMEOUT_DELAY1_MS   500
#define ENOCEAN_NB_RETRY              5

extern uint8_t u8CRC8Table[];


#define proc_crc8(u8CRC, u8Data) (u8CRC8Table[u8CRC ^ u8Data]);

typedef int16_t (*enocean_callback_f)(uint8_t *data, uint16_t l_data, uint32_t addr, void *callbackdata);

struct enocean_in_buffer_s
{
   uint16_t      packet_l;
   uint8_t       packet[256];
   unsigned long timestamp;
   uint8_t       err;
};

typedef struct enocean_ed_s
{
   int             fd;
   pthread_t       read_thread;
   pthread_cond_t  sync_cond;
   pthread_mutex_t sync_lock;
   pthread_mutex_t write_lock;
   pthread_mutex_t ed_lock;
   char            serial_dev_name[255];
   mea_queue_t    *queue;
   uint16_t        signal_flag;
   enocean_callback_f enocean_callback;
   void            *enocean_callback_data;
   uint32_t        id;
   struct enocean_in_buffer_s in_buffer;
} enocean_ed_t;
 
 
int16_t       enocean_init(enocean_ed_t *ed, char *dev);
void          enocean_close(enocean_ed_t *ed);
void          enocean_clean_ed(enocean_ed_t *ed);
void          enocean_free_ed(enocean_ed_t *ed);
enocean_ed_t *enocean_new_ed(void);

uint32_t enocean_calc_addr(uint8_t a, uint8_t b, uint8_t c, uint8_t d);

int16_t enocean_get_chipid(enocean_ed_t *ed, uint32_t *chipid, int16_t *nerr);
int16_t enocean_get_baseid(enocean_ed_t *ed, uint32_t *baseid, int16_t *nerr);
int16_t  enocean_set_data_callback(enocean_ed_t *ed, enocean_callback_f f);
int16_t  enocean_set_data_callback2(enocean_ed_t *ed, enocean_callback_f f, void *data);
int16_t  enocean_remove_data_callback(enocean_ed_t *ed);
int16_t  enocean_send_packet(enocean_ed_t *ed, uint8_t *packet, uint16_t l_packet, uint8_t *response, uint16_t *l_response, int16_t *nerr);

int16_t enocean_send_radio_erp1_packet(enocean_ed_t *ed, uint8_t rorg, uint32_t source, uint32_t dec_id, uint32_t dest, uint8_t *data, uint16_t l_data, uint8_t status, int16_t *nerr);

int16_t enocean_learning_onoff(enocean_ed_t *ed, int onoff, int16_t *nerr);
int16_t enocean_sa_learning_onoff(enocean_ed_t *ed, int onoff, int16_t *nerr);
int16_t enocean_sa_confirm_learn_response(enocean_ed_t *ed, uint16_t response_time, uint16_t confirm, int16_t *nerr);

#endif
