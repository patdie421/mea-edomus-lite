#include <stdio.h>
#include <signal.h>
#include <stdint.h>

#include "interface_type_003_pairing.h"

#include "interface_type_003.h"

#include "mea_verbose.h"
#include "mea_timer.h"
#include "mea_verbose.h"
#include "mea_plugins_utils.h"
#include "parameters_utils.h"
#include "tokens.h"
#include "tokens_da.h"

#include "enocean.h"
#include "cJSON.h"

#include "interfacesServer.h"

int16_t enocean_teachinout(enocean_ed_t *ed, int16_t addr_dec, uint8_t *data, uint16_t l_data, uint8_t *eep, int16_t teach) /* TO TEST */
{
   char *operationStr = "???";
   char *responseStr  = "???";
   char *requestStr   = "???";
   uint8_t operation  = (data[7] & 0b10000000) >> 7;
   uint8_t response   = (data[7] & 0b01000000) >> 6;
   uint8_t request    = (data[7] & 0b00110000) >> 4;
   uint8_t cmnd       = (data[7] & 0b00001111);
   uint32_t device_addr = enocean_calc_addr(data[14], data[15], data[16], data[17]);
//   uint32_t addr = ed->id;

   uint8_t resp_request   = 0;
   uint8_t resp_operation = 0;

   switch(operation) {
      case 0:
        operationStr = "Unidirectional";
        break;
      case 1:
        operationStr = "Bidirectional";
        break;
   }
   resp_operation = operation << 7;

   switch(response) {
      case 0:
        responseStr = "";
        break;
      case 1:
        responseStr = "No";
        break;
   }

   switch(request) {
      case 0:
        requestStr = "Teach-in request";
        resp_request = 0b01;
        break;
      case 1:
        requestStr = "Teach-in deletion request";
        resp_request = 0b10;
        break;
      case 2:
        requestStr = "Teach-in or deletion of teach-in, not specified";
        if(teach==0) {
           resp_request = 0b01;
         }
        else {
           resp_request = 0b10;
         }
        break;
      case 3:
        requestStr = "Not used";
        break;
   }
   resp_request = resp_request << 4;

   mea_log_printf("%s (%s) : EEP        : %02x-%02x-%02x\n", ERROR_STR, __func__, data[13], data[12], data[11]);
   mea_log_printf("%s (%s) : nb channels: %d\n", ERROR_STR, __func__, data[8]);
   mea_log_printf("%s (%s) : op         : %u (%s communication)\n", ERROR_STR, __func__, operation, operationStr);
   mea_log_printf("%s (%s) : rs         : %u (%s EEP Teach-In-Response message expected)\n", ERROR_STR, __func__, response, responseStr);
   mea_log_printf("%s (%s) : rq         : %u (%s)\n", ERROR_STR, __func__, request, requestStr);
   mea_log_printf("%s (%s) : cmnd       : %u\n", ERROR_STR, __func__, cmnd);

   if((request  != 3) &&
      (response == 0) &&
      (cmnd     == 0)) {
      int16_t nerr = -1;

      uint8_t resp[7];

      resp[0]=resp_operation+resp_request+1; // DB_6
      resp[1]=data[8];  // DB_5 (nb channel)
      resp[2]=data[9];  // DB_4 (manufacturer-ID LSB)
      resp[3]=data[10]; // DB_3 (manufacturer-ID MSB)
      resp[4]=data[11]; // DB_2 (TYPE)
      resp[5]=data[12]; // DB_1 (FUNC)
      resp[6]=data[13]; // DB_0 (RORG)

      usleep(5000); // 0,5s
      int ret=enocean_send_radio_erp1_packet(ed, ENOCEAN_UTE_TELEGRAM, ed->id, 0, device_addr, resp, 7, 0, &nerr);

      mea_log_printf("%s (%s) : enocean_teachinout - RESPONSE = %d\n", ERROR_STR, __func__, ret);

      eep[0]=data[13]; // (RORG)
      eep[1]=data[12]; // (FUNC)
      eep[2]=data[11]; // (TYPE)
      
      return ret;
   }
   else {
      return -1;
   }
}


int16_t enocean_teachinout_reversed(enocean_ed_t *ed, int16_t addr_dec, uint8_t *data, uint16_t l_data, uint8_t *eep, int16_t teach) /* TO TEST */
{
   char *operationStr = "???";
   char *responseStr  = "???";
   char *requestStr   = "???";
   uint8_t operation  = (data[13] & 0b10000000) >> 7;
   uint8_t response   = (data[13] & 0b01000000) >> 6;
   uint8_t request    = (data[13] & 0b00110000) >> 4;
   uint8_t cmnd       = (data[13] & 0b00001111);
   uint32_t device_addr = enocean_calc_addr(data[14], data[15], data[16], data[17]);
//   uint32_t addr = ed->id;

   uint8_t resp_request   = 0;
   uint8_t resp_operation = 0;

   switch(operation) {
      case 0:
        operationStr = "Unidirectional";
        break;
      case 1:
        operationStr = "Bidirectional";
        break;
   }
   resp_operation = operation << 7;

   switch(response) {
      case 0:
        responseStr = "";
        break;
      case 1:
        responseStr = "No";
        break;
   }

   switch(request) {
      case 0:
        requestStr = "Teach-in request";
        resp_request = 0b01;
        break;
      case 1:
        requestStr = "Teach-in deletion request";
        resp_request = 0b10;
        break;
      case 2:
        requestStr = "Teach-in or deletion of teach-in, not specified";
        if(teach==0) {
           resp_request = 0b01;
         }
        else {
           resp_request = 0b10;
         }
        break;
      case 3:
        requestStr = "Not used";
        break;
   }
   resp_request = resp_request << 4;

   printf("%s (%s) : EEP        : %02x-%02x-%02x\n", ERROR_STR, __func__, data[7], data[8], data[9]);
   printf("%s (%s) : nb channels: %d\n", ERROR_STR, __func__, data[8]);
   printf("%s (%s) : op         : %u (%s communication)\n", ERROR_STR, __func__, operation, operationStr);
   printf("%s (%s) : rs         : %u (%s EEP Teach-In-Response message expected)\n", ERROR_STR, __func__, response, responseStr);
   printf("%s (%s) : rq         : %u (%s)\n", ERROR_STR, __func__, request, requestStr);
   printf("%s (%s) : cmnd       : %u\n", ERROR_STR, __func__, cmnd);

   if((request  != 3) &&
      (response == 0) &&
      (cmnd     == 0)) {
      int16_t nerr = -1;

      uint8_t resp[7];

      resp[0]=data[7];   // (RORG)
      resp[1]=data[8];   // (FUNC)
      resp[2]=data[9];   // (TYPE)
      resp[3]=data[10];  // (manufacturer-ID 3MSB)
      resp[4]=data[11];  // (manufacturer-ID 8LSB)
      resp[5]=data[12];  // (nb channel)
      resp[6]=resp_operation+resp_request+1 /* cmnd */; // DB_6

      usleep(5000); // 0,5s
      int ret=enocean_send_radio_erp1_packet(ed, ENOCEAN_UTE_TELEGRAM, ed->id, addr_dec, device_addr, resp, 7, 0, &nerr);

      mea_log_printf("%s (%s) : enocean_teachinout_reversed - RESPONSE = %d\n", ERROR_STR, __func__, ret);

      eep[0]=data[7]; // (RORG)
      eep[1]=data[8]; // (FUNC)
      eep[2]=data[9]; // (TYPE)

      return ret;
   }
   else {
      return -1;
   }
}


int enocean_pairing(interface_type_003_t *i003, enocean_data_queue_elem_t *e, uint32_t addr, uint8_t *eep) /* TO TEST */
{
   uint8_t *data=e->data;
   int16_t l_data=e->l_data;
   int16_t nerr=0;
   int ret=-1;
      
   if(data[4]==ENOCEAN_EVENT) {
      
      mea_log_printf("ENOCEAN_EVENT ...\n");
      if(data[6]==2) { // SA_CONFIRM_LEARN
         mea_log_printf("   SA_CONFIRM_LEARN ...\n");
         mea_log_printf("      PM Priority       : %02x", data[7]);
         mea_log_printf("      manufacturer ID   : %02x...%02x", data[8] & 0b00000111, data[9]);
         mea_log_printf("      EEP               : %02x-%02x-%02x\n", data[10], data[11], data[12]);
         mea_log_printf("      RSSI              : %d", data[13]);
         mea_log_printf("      PM Candidat ID    : %02x-%02x-%02x-%02x", data[14],data[15],data[16],data[17]);
         mea_log_printf("      Smart Ack ClientID: %02x-%02x-%02x-%02x", data[18],data[19],data[20],data[21]);

         enocean_sa_confirm_learn_response(i003->ed, 1000 /* ms */, 0, &nerr);

         eep[0]=data[10]; // (RORG)
         eep[1]=data[11]; // (FUNC)
         eep[2]=data[12]; // (TYPE)

         ret=0;
      }
   }
   else if(data[4]==ENOCEAN_RADIO_ERP1) {
      mea_log_printf("ENOCEAN_RADIO_ERP1 (%08x ...\n", addr);
      switch((unsigned char)data[6]) {
         case ENOCEAN_RPS_TELEGRAM: {
         /* Exemple "Trame d'un bouton F6"
            00 0x55  85 Synchro
            01 0x00   0 Header1 Data Lenght
            02 0x07   7 Header2 Data Lenght
            03 0x07   7 Header3 Optionnal lenght
            04 0x01   1 Header4 Packet type
            05 0x7A 122 CRC8H
            06 0xF6 246 Data1 RORG
            07 0x30  48 Data2 = B00110000
            08 0x00   0 Data3 ID1
            09 0x25  37 Data4 ID2
            10 0x86 134 Data5 ID3
            11 0x71 113 Data6 ID4
            12 0x30  48 Data7 Status
            13 0x02   2 Optionnal1 Number sub telegram
            14 0xFF 255 Optionnal2 Destination ID
            15 0xFF 255 Optionnal3 Destination ID
            16 0xFF 255 Optionnal4 Destination ID
            17 0xFF 255 Optionnal5 Destination ID
            18 0x2D  45 Optionnal6 Dbm
            19 0x00   0 Optionnal7 Security level
            20 0x38  56 CRC8D
         */
            mea_log_printf("=== RPS Telegram (%x) ===\n", data[6]);
            mea_log_printf("Adresse    : %02x-%02x-%02x-%02x\n", data[8], data[9], data[10], data[11]);

            eep[0]=data[6]; // (RORG)
            eep[1]=0;       // (FUNC)
            eep[2]=0;       // (TYPE)

            ret=0;
         }
         break;
 
         case ENOCEAN_UTE_TELEGRAM: {
         /*
            Trame normale ... profil v2.5
            00 0x55  85 Synchro
            01 0x00   0 Header1 Data Lenght 
            02 0x0d  13 Header2 Data Lenght
            03 0x07   7 Header3 Optionnal lenght
            04 0x01   1 Header4 Packet type
            05 0xfd 253 CRC8H
            06 0xd4 212 RORG : UTE
            07 0xa0 160 (0b10100000)
            08 0x02   2 Number of indifidual channel to be taught in
            09 0x46  70 Manufacturer-ID (8LSB)
            10 0x00   0 Manufacturer-ID (3MSB)
            11 0x12  18 TYPE
            12 0x01   1 FUNC
            13 0xd2 210 RORG
            14 0x01   1 ID1
            15 0x94 148 ID2
            16 0xc9 201 ID3
            17 0x40  64 ID4
            18 0x00   0 (status)
            19 0x01   1 (number sub telegram)
            20 0xFF 255 Optionnal2 Destination ID
            21 0xFF 255 Optionnal3 Destination ID
            22 0xFF 255 Optionnal4 Destination ID
            23 0xFF 255 Optionnal5 Destination ID
            24 0x3d  61 Optionnal6 Dbm
            25 0x00   0 Optionnal7 Security level
            26 0xde 222 CRC8D
         */
         /*
            Trame inversée ... profil v2.5
            data[000] = 0x55 (085)
            data[001] = 0x00 (000)
            data[002] = 0x0d (013)
            data[003] = 0x07 (007)
            data[004] = 0x01 (001)
            data[005] = 0xfd (253)
            data[006] = 0xd4 (212)
            data[007] = 0xd2 (210) -+ RORG
            data[008] = 0x01 (001)  | FUNC 
            data[009] = 0x01 (001)  | TYPE
            data[010] = 0x00 (000)  | // b00000000 : MID MSB3
            data[011] = 0x3e (062)  | // b00111110 : MID LSB8
            data[012] = 0xff (255)  | // b11111111 All channel supported by the device
            data[013] = 0xa0 (160) -+ // b10100000 1=bidirectional, 0=response expected, 10=teach-in or delete, 0000=command:teach-in request
            data[014] = 0x01 (001)
            data[015] = 0x84 (132)
            data[016] = 0x5c (092)
            data[017] = 0x80 (128)
            data[018] = 0x00 (000)
            data[019] = 0x01 (001)
            data[020] = 0xff (255)
            data[021] = 0xff (255)
            data[022] = 0xff (255)
            data[023] = 0xff (255)
            data[024] = 0x44 (068)
            data[025] = 0x00 (000)
            data[026] = 0x5a (090)
         */
            mea_log_printf("=== UTE Telegram (D2) ===\n");
            mea_log_printf("Adresse    : %02x-%02x-%02x-%02x\n", data[14], data[15], data[16], data[17]);

            if(data[7]!=0xd2)
               ret=enocean_teachinout(i003->ed, 0, data, l_data, eep, 1);
            else
               ret=enocean_teachinout_reversed(i003->ed, 0, data, l_data, eep, 1);

            ret=0;
         }
         break;
      } 
   }
   else {  // quand on ne sais pas analyser le packet, on le dump
      for(int i=0; i<l_data; i++) {
         if(i && (i % 10) == 0) {
            fprintf(MEA_STDERR, "\n");
         }
         mea_log_printf("0x%02x ", data[i]);
      }
      fprintf(MEA_STDERR, "\n");
      
      eep[0]=0; // (RORG)
      eep[1]=0; // (FUNC)
      eep[2]=0; // (TYPE)

      ret=-1;
   }

   return ret;
}


cJSON *enocean_pairing_get(void *context, void *parameters) /* TO TEST */
{
   interface_type_003_t *i003 = (interface_type_003_t *)context;

   return cJSON_CreateNumber((double)i003->pairing_state);
}


cJSON *enocean_pairing_start(void *context, void *parameters) /* TO TEST */
{
   interface_type_003_t *i003 = (interface_type_003_t *)context;
   int16_t nerr=0;
   cJSON *p=NULL;

   if(parameters) {
      p=(cJSON *)parameters;
   }

   enocean_sa_learning_onoff(i003->ed, 1, &nerr);
   
   i003->pairing_state=ENOCEAN_PAIRING_ON;
   i003->pairing_startms=mea_now();
   
   return cJSON_CreateTrue();
}


cJSON *enocean_pairing_end(void *context, void *parameters) /* TO TEST */
{
   interface_type_003_t *i003 = (interface_type_003_t *)context;
   int16_t nerr=0;
   cJSON *p=NULL;

   if(parameters) {
      p=(cJSON *)parameters;
   }

   enocean_sa_learning_onoff(i003->ed, 0, &nerr);

   i003->pairing_state=ENOCEAN_PAIRING_OFF;
   i003->pairing_startms=-1.0;
   
   return cJSON_CreateTrue();
}


void *enocean_pairingCtrl(int cmd, void *context, void *parameters)
{
   switch(cmd) {
      case PAIRING_CMD_GETSTATE:
         return (void *)enocean_pairing_get(context, parameters);

      case PAIRING_CMD_ON:
         return (void *)enocean_pairing_start(context, parameters);
         
      case PAIRING_CMD_OFF:
         return (void *)enocean_pairing_end(context, parameters);
   }
   return NULL;
}


int enocean_update_interfaces(void *context, char *interfaceDevName, uint8_t *addr, uint8_t *eep) /* TO TEST */
{
   interface_type_003_t *i003 = (interface_type_003_t *)context;

   cJSON *j=NULL, *_devices=NULL;
   cJSON *pairing_data=NULL;
   char name[256];
   char strAddr[256];

   snprintf(strAddr, sizeof(strAddr)-1, "%02x%02x%02x%02x", addr[0], addr[1], addr[2], addr[3]);
   snprintf(name, sizeof(name)-1, "%s_%s", i003->name, strAddr);

   pairing_data=cJSON_CreateObject();
   cJSON_AddNumberToObject(pairing_data, "RORG", (double)eep[0]);
   cJSON_AddNumberToObject(pairing_data, "FUNC", (double)eep[1]);
   cJSON_AddNumberToObject(pairing_data, "TYPE", (double)eep[2]);
   cJSON_AddStringToObject(pairing_data, "interface_name", i003->name);
   cJSON_AddStringToObject(pairing_data, "addr", strAddr);
      
   j=cJSON_CreateObject();
   cJSON_AddStringToObject(j, NAME_STR_C, name);
   cJSON_AddNumberToObject(j, ID_TYPE_STR_C, INTERFACE_TYPE_003);
   cJSON_AddStringToObject(j, DESCRIPTION_STR_C, "");
   cJSON_AddStringToObject(j, DEV_STR_C, interfaceDevName);
   cJSON_AddStringToObject(j, PARAMETERS_STR_C, "");
   cJSON_AddNumberToObject(j, STATE_STR_C, 2); // delegate

   cJSON *jj=cJSON_Duplicate(j, 1);
   cJSON_AddItemToObject(jj, "_pairing_data", pairing_data);

   // call plugin to get json devices
   int err=0;
   int nbPluginParams=0;
   parsed_parameters_t *pluginParams=NULL;

   pluginParams=alloc_parsed_parameters(i003->parameters, valid_enocean_plugin_params, &nbPluginParams, &err, 0);
   if(!pluginParams || !pluginParams->parameters[PLUGIN_PARAMS_PLUGIN].value.s) {
      if(pluginParams) {
         // pas de plugin spécifié
         release_parsed_parameters(&pluginParams);
         pluginParams=NULL;
         VERBOSE(9) mea_log_printf("%s (%s) : no python plugin specified\n", INFO_STR, __func__);
      }
      else {
         VERBOSE(5) mea_log_printf("%s (%s) : invalid or no python plugin parameters (%s)\n", ERROR_STR, __func__, i003->parameters);
      }
   }
   else {
      _devices=plugin_call_function_json_alloc(pluginParams->parameters[PLUGIN_PARAMS_PLUGIN].value.s, "mea_pairingDevices", jj);
   }

   addInterface(j);
   if(_devices->type==cJSON_Array) {
      cJSON *jsonDevice=_devices->child;
      while(jsonDevice) {
         addDevice(name, jsonDevice);
         jsonDevice=jsonDevice->next;
      }
   }
   cJSON_Delete(j);

   return 0;
}
