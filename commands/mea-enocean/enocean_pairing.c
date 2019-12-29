#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <sys/time.h>
#include <limits.h>
#include <fcntl.h>
#include <termios.h>

#include "enocean.h"
#include "mea_verbose.h"

int16_t learning_state = 0;

int16_t device_found=-1;
uint8_t device_data[256];
uint16_t device_l_data=0;

int16_t pairing_as_f60201(enocean_ed_t *ed, uint32_t addr_dec, uint32_t device_addr);


int16_t enocean_f6_02_01(enocean_ed_t *ed, uint32_t addr_dec, uint32_t device_addr, uint16_t channel, int16_t value)
{
   uint8_t data[1];
   uint8_t status;
   int16_t nerr;
   int ret;

   if(channel==0) {
      if(value!=0)
         data[0]=0b00010000; // AI appuyé
      else
         data[0]=0b00110000; // A0 appuyé
   }
   else {
      if(value!=0)
         data[0]=0b01010000; // BI appuyé
      else
         data[0]=0b01110000; // B0 appuyé
   }
   status=0b0110000; // status: T21=1, NU=1

   ret=enocean_send_radio_erp1_packet(ed, ENOCEAN_RPS_TELEGRAM, ed->id+addr_dec, 0, device_addr, data, 1, status, &nerr);
//      usleep(1000000/20);

   data[0]=0b00000000;
   status=0b00100000; // status: T21=1, NU=0
   ret=enocean_send_radio_erp1_packet(ed, ENOCEAN_RPS_TELEGRAM, ed->id+addr_dec, 0, device_addr, data, 1, status, &nerr);

   return ret;
}


int16_t enocean_d2(enocean_ed_t *ed, uint32_t addr_dec, uint32_t device_addr, uint16_t channel, int16_t value, int16_t reverse)
{
   uint8_t data[3];
   int16_t nerr;

   if(reverse == 0) {
      data[0]=0x01;
      data[1]=channel & 0b11111;
      data[2]=value & 0b1111111;
   }
   else {
      data[0]=value & 0b1111111;
      data[1]=channel & 0b11111;
      data[2]=0x01;
   }
   return enocean_send_radio_erp1_packet(ed, ENOCEAN_VLD_TELEGRAM, ed->id, 0, device_addr, data, 3, 0, &nerr);
}


int16_t teachinout(enocean_ed_t *ed, int16_t addr_dec, uint8_t *data, uint16_t l_data, int16_t teach, int16_t asf60201)
{
   char *operationStr = "???";
   char *responseStr  = "???";
   char *requestStr   = "???";
   uint8_t operation  = (data[7] & 0b10000000) >> 7;
   uint8_t response   = (data[7] & 0b01000000) >> 6;
   uint8_t request    = (data[7] & 0b00110000) >> 4;
   uint8_t cmnd       = (data[7] & 0b00001111);
   uint32_t device_addr = enocean_calc_addr(data[14], data[15], data[16], data[17]);
   uint32_t addr = ed->id;

   uint8_t resp_request   = 0;
   uint8_t resp_operation = 0;

   switch(operation)
   {
      case 0:
        operationStr = "Unidirectional";
        break;
      case 1:
        operationStr = "Bidirectional";
        break;
   }
   resp_operation = operation << 7;

   switch(response)
   {
      case 0:
        responseStr = "";
        break;
      case 1:
        responseStr = "No";
        break;
   }

   switch(request)
   {
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
        if(teach==0)
           resp_request = 0b01;
        else
           resp_request = 0b10;
        break;
      case 3:
        requestStr = "Not used";
        break;
   }
   resp_request = resp_request << 4;

   fprintf(stderr, "EEP        : %02x-%02x-%02x\n", data[13], data[12], data[11]);
   fprintf(stderr, "nb channels: %d\n", data[8]);
   fprintf(stderr, "op         : %u (%s communication)\n", operation, operationStr);
   fprintf(stderr, "rs         : %u (%s EEP Teach-In-Response message expected)\n", response, responseStr);
   fprintf(stderr, "rq         : %u (%s)\n", request, requestStr);
   fprintf(stderr, "cmnd       : %u\n", cmnd);

   if(asf60201==1)    {
      enocean_f6_02_01(ed, addr_dec, device_addr, 0, 1);
   }
   else {
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

         int ret=enocean_send_radio_erp1_packet(ed, ENOCEAN_UTE_TELEGRAM, ed->id, 0, device_addr, resp, 7, 0, &nerr);
         fprintf(stderr,"RESPONSE = %d\n", ret);

         learning_state = 0;
      }
   }

   return 0;
}


int16_t teachinout_reversed(enocean_ed_t *ed, int16_t addr_dec, uint8_t *data, uint16_t l_data, int16_t teach, int16_t asf60201)
{
   char *operationStr = "???";
   char *responseStr  = "???";
   char *requestStr   = "???";
   uint8_t operation  = (data[13] & 0b10000000) >> 7;
   uint8_t response   = (data[13] & 0b01000000) >> 6;
   uint8_t request    = (data[13] & 0b00110000) >> 4;
   uint8_t cmnd       = (data[13] & 0b00001111);
   uint32_t device_addr = enocean_calc_addr(data[14], data[15], data[16], data[17]);
   uint32_t addr = ed->id;

   uint8_t resp_request   = 0;
   uint8_t resp_operation = 0;

   switch(operation)
   {
      case 0:
        operationStr = "Unidirectional";
        break;
      case 1:
        operationStr = "Bidirectional";
        break;
   }
   resp_operation = operation << 7;

   switch(response)
   {
      case 0:
        responseStr = "";
        break;
      case 1:
        responseStr = "No";
        break;
   }

   switch(request)
   {
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
        if(teach==0)
           resp_request = 0b01;
        else
           resp_request = 0b10;
        break;
      case 3:
        requestStr = "Not used";
        break;
   }
   resp_request = resp_request << 4;

   fprintf(stderr, "reversed !\n");
   fprintf(stderr, "EEP        : %02x-%02x-%02x\n", data[7], data[8], data[9]);
   fprintf(stderr, "nb channels: %d\n", data[12]);
   fprintf(stderr, "op         : %u (%s communication)\n", operation, operationStr);
   fprintf(stderr, "rs         : %u (%s EEP Teach-In-Response message expected)\n", response, responseStr);
   fprintf(stderr, "rq         : %u (%s)\n", request, requestStr);
   fprintf(stderr, "cmnd       : %u\n", cmnd);

   if(asf60201==1) {
      return enocean_f6_02_01(ed, addr_dec, device_addr, 0, 1);
   }
   else {
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

         int ret=enocean_send_radio_erp1_packet(ed, ENOCEAN_UTE_TELEGRAM, ed->id, addr_dec, device_addr, resp, 7, 0, &nerr);
         fprintf(stderr,"RESPONSE = %d\n", ret);

         learning_state = 0;

         return ret;
      }
      else {
         return -1;
      }
   }
}


int16_t learning_callback(uint8_t *data, uint16_t l_data, uint32_t addr, void *callbackdata)
{
   int16_t nerr = 0;
   int16_t ret = 0;

   enocean_ed_t *ed=(enocean_ed_t *)callbackdata;

   if(learning_state == 0) {
      return -1;
   }

   if(data[4]==ENOCEAN_EVENT) {
      fprintf(stderr,"ENOCEAN_EVENT ...\n");
      if(data[6]==2) { // SA_CONFIRM_LEARN
         fprintf(stderr,"   SA_CONFIRM_LEARN ...\n");
         fprintf(stderr,"      PM Priority       : %02x", data[7]);
         fprintf(stderr,"      manufacturer ID   : %02x...%02x", data[8] & 0b00000111, data[9]);
         fprintf(stderr,"      EEP               : %02x-%02x-%02x\n", data[10], data[11], data[12]);
         fprintf(stderr,"      RSSI              : %d", data[13]);
         fprintf(stderr,"      PM Candidat ID    : %02x-%02x-%02x-%02x", data[14],data[15],data[16],data[17]);
         fprintf(stderr,"      Smart Ack ClientID: %02x-%02x-%02x-%02x", data[18],data[19],data[20],data[21]);

         device_found=1;
      }
   }
   else if(data[4]==ENOCEAN_RADIO_ERP1) {
      fprintf(stderr,"ENOCEAN_RADIO_ERP1 (%08x ...\n", addr);
      switch(data[6]) {
         case ENOCEAN_RPS_TELEGRAM: {
         /* Exemple "Trame d'un bouton"
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
            fprintf(stderr, "=== RPS Telegram (F6) ===\n");
            fprintf(stderr, "Adresse    : %02x-%02x-%02x-%02x\n", data[8], data[9], data[10], data[11]);

            device_found=2;
         }
         break;
 
         case ENOCEAN_UTE_TELEGRAM: {
         /*
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
            fprintf(stderr, "=== UTE Telegram (D4) ===\n");
            fprintf(stderr, "Adresse    : %02x-%02x-%02x-%02x\n", data[14], data[15], data[16], data[17]);

            for(int i=0;i<l_data;i++)
               device_data[i]=data[i];

            device_l_data=l_data;

            device_found=0;
         }
         break;
      } 
   }
   else {  // quand on ne sais pas analyser le packet, on le dump
      for(int i=0; i<l_data; i++)
      {
         if(i && (i % 10) == 0)
            fprintf(stderr, "\n");
         fprintf(stderr, "0x%02x ", data[i]);
      }
      fprintf(stderr, "\n");
   }

   return ret;
}


enocean_ed_t *ed=NULL;

int usage(char *name)
{
   fprintf(stderr, "usage : %s /dev/<device> teachin|teachout [asf60201]\n", name);
   exit(1);
}


int main(int argc, char *argv[])
{
   char *dev; // ="/dev/ttyS0";
   int16_t enocean_fd = 0;
   int16_t ret;
   int16_t teach=-1;
   int16_t asf60201=0;
   verbose_level=1;
 
   if(argc!=3 && argc!=4) {
      usage(argv[0]);
      exit(1);
   }

   dev = argv[1];
   
   if(strcmp(argv[2],"teachin")==0)
      teach=0;
   else if(strcmp(argv[2],"teachout")==0)
      teach=1;
   else {
      usage(argv[0]);
   }

   if(argc==4) {
      if(strcmp(argv[3], "asf60201")==0)
         asf60201 = 1;
      else
         usage(argv[0]);
   }
 
   uint8_t data[0xFFFF]; // taille maximum d'un packet ESP3 = 65536
   uint16_t l_data = 0;
   int16_t nerr = 0;

   ed = enocean_new_ed();
   enocean_fd = enocean_init(ed, dev);

   if(enocean_fd<0) {
      fprintf(stderr,"enocean_init error: ");
      perror("");
      exit(1);
   }

   enocean_set_data_callback2(ed, learning_callback, ed);

//   fprintf(stderr,"asf60201 = %d\n", asf60201);

   learning_state = 1;
   int16_t teach_delay = 60;

   ret=enocean_sa_learning_onoff(ed, 1, &nerr);

   if(ret==0) {   
      int i=0; 
      for(i=0;i<(teach_delay*10) && learning_state == 1;i++) {
         if(device_found==0) {
            if(device_data[7]!=0xd2)
               teachinout(ed, 0, device_data, device_l_data, teach, asf60201);
            else
               teachinout_reversed(ed, 0, device_data, device_l_data, teach, asf60201);

            learning_state = 0;
         }
         else if(device_found==1) {
            enocean_sa_confirm_learn_response(ed, 1000 /* ms */, 0, &nerr);
            learning_state = 0;
         }
         else if(device_found==2) {
            learning_state = 0;
         }
         else {
            if(i%10==0) {
               fprintf(stderr,"\rTEACHING REMAINING TIME: %3d s", teach_delay - i/10); 
               fflush(stderr);
            }
            usleep(100000);
         }
      }

      learning_state = 0;
      enocean_sa_learning_onoff(ed, 0, &nerr);
   }
   else
      fprintf(stderr,"CAN'T LEARN\n");

   enocean_close(ed);
   enocean_free_ed(ed);
}

