#ifndef __interface_type_003_pairing_h
#define __interface_type_003_pairing_h

#include "interface_type_003.h"

#include <stdint.h>

#include "enocean.h"
#include "cJSON.h"


enum pairing_cmd_e {
   PAIRING_GET,
   PAIRING_START,
   PAIRING_STOP
};

#define ENOCEAN_PAIRING_ON  1
#define ENOCEAN_PAIRING_OFF 0


int16_t enocean_teachinout(enocean_ed_t *ed, int16_t addr_dec, uint8_t *data, uint16_t l_data, uint8_t *eep, int16_t teach);
int16_t enocean_teachinout_reversed(enocean_ed_t *ed, int16_t addr_dec, uint8_t *data, uint16_t l_data, uint8_t *eep, int16_t teach);
int     enocean_pairing(interface_type_003_t *i003, enocean_data_queue_elem_t *e, uint32_t addr, uint8_t *eep);
cJSON  *enocean_pairing_get(void *context, void *parameters);
cJSON  *enocean_pairing_start(void *context, void *parameters);
cJSON  *enocean_pairing_end(void *context, void *parameters);
void   *enocean_pairingCtrl(int cmd, void *context, void *parameters);
int     enocean_update_interfaces(void *context, char *interfaceDevName, uint8_t *addr, uint8_t *eep);

#endif