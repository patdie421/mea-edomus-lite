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


int usage(char *name)
{
   fprintf(stderr, "usage : %s /dev/<device> addr type channel value [reverse]\n", name);
   exit(1);
}


int16_t enocean_f6_02_01(enocean_ed_t *ed, uint32_t addr_dec, uint32_t device_addr, uint16_t channel, int16_t value)
{
      uint8_t data[1];
      uint8_t status;
      int16_t nerr;
      int ret;

      if(channel==0)
      {
         if(value!=0)
            data[0]=0b00010000; // AI appuyé
         else
            data[0]=0b00110000; // A0 appuyé
      }
      else
      {
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

   if(reverse == 0)
   {
      data[0]=0x01;
      data[1]=channel & 0b11111;
      data[2]=value & 0b1111111;
   }
   else
   {
      data[0]=value & 0b1111111;
      data[1]=channel & 0b11111;
      data[2]=0x01;
   }
/*
   fprintf(stderr,"ed->id = %x\n", ed->id);
   fprintf(stderr,"_addr  = %x\n", _addr);
   fprintf(stderr,"data[0]= %x\n", data[0]);
   fprintf(stderr,"data[1]= %x\n", data[1]);
   fprintf(stderr,"data[2]= %x\n", data[2]);
*/
   return enocean_send_radio_erp1_packet(ed, ENOCEAN_VLD_TELEGRAM, ed->id, 0, device_addr, data, 3, 0, &nerr);
}


int main(int argc, char *argv[])
{
   char *dev; // ="/dev/ttyS0";
   int16_t enocean_fd = 0;
   enocean_ed_t *ed=NULL;

   char *addr;
   char *type;
   int  channel;
   int  value;
   int  reverse=0;
   char data[3];
   int _type=-1;
 
   int16_t ret;

   verbose_level = 2;

   if(argc!=6 && argc!=7)
      usage(argv[0]);
 
   if(argc==7)
   {
      if(strcmp(argv[6],"reverse")==0)
         reverse=1;
      else
         usage(argv[0]);
   }

   dev     = argv[1];
   addr    = argv[2];
   type    = argv[3];

   int a,b,c,d, n;

   ret=sscanf(addr,"%2x-%2x-%2x-%2x%n", &a, &b, &c, &d, &n);
   if(ret!=4 || n!=strlen(addr))
   {
      fprintf(stderr,"Addess error (%s)\n", addr);
      exit(1);
   }
   uint32_t _addr = enocean_calc_addr(a, b, c, d);

   if( strcmp(type,"d2")!=0 &&
       strcmp(type, "f6-02-01")!=0 )
   {
      fprintf(stderr,"unknown type (%s)\n", type);
      exit(1);
   }

   if(strcmp(type,"d2")==0)
      _type=0;
   else
      _type=-1;

   ret=sscanf(argv[4],"%d%n\n",&channel, &n);
   if(ret!=1 || n!=strlen(argv[4]) || channel < 0)
   {
      fprintf(stderr,"channel error (%s)\n", argv[4]);
      exit(1);
   }

   ret=sscanf(argv[5],"%d%n\n",&value, &n);
   if(ret!=1 || n!=strlen(argv[5]) || value < 0 || value > 100)
   {
      fprintf(stderr,"value error (%s)\n", argv[5]);
      exit(1);
   }

   int16_t nerr = 0;
   ed = enocean_new_ed();
  
   enocean_fd = enocean_init(ed, dev);

   if(enocean_fd<0)
   {
      fprintf(stderr,"enocean_init error: ");
      perror("");
      exit(1);
   }

   ret=0;
   if(_type==0)
   {
      ret=enocean_d2(ed, 0, _addr, channel, value, reverse);
   }
   else
   {
      if(channel < 2)
         ret=enocean_f6_02_01(ed, 0, _addr, channel, value);
      else
         ret=-1;
   }

   enocean_close(ed);
   enocean_free_ed(ed);

   exit(ret);
}

