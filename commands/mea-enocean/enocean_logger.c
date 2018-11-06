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

#include "mea_verbose.h"

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
#define ENOCEAN_RET_NOT_SUPPORTED  0x02
#define ENOCEAN_RET_WRONG_PARAM    0x03

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
//#define ENOCEAN_TIMEOUT_DELAY2_MS 500
#define ENOCEAN_NB_RETRY              5

//#ifdef __linux__
int verbose_level = 10;
//int verbose_level = 2;
#define VERBOSE(v) if(v <= verbose_level)
//#endif

uint8_t u8CRC8Table[256] = {
0x00, 0x07, 0x0e, 0x09, 0x1c, 0x1b, 0x12, 0x15,
0x38, 0x3f, 0x36, 0x31, 0x24, 0x23, 0x2a, 0x2d,
0x70, 0x77, 0x7e, 0x79, 0x6c, 0x6b, 0x62, 0x65,
0x48, 0x4f, 0x46, 0x41, 0x54, 0x53, 0x5a, 0x5d,
0xe0, 0xe7, 0xee, 0xe9, 0xfc, 0xfb, 0xf2, 0xf5,
0xd8, 0xdf, 0xd6, 0xd1, 0xc4, 0xc3, 0xca, 0xcd,
0x90, 0x97, 0x9e, 0x99, 0x8c, 0x8b, 0x82, 0x85,
0xa8, 0xaf, 0xa6, 0xa1, 0xb4, 0xb3, 0xba, 0xbd,
0xc7, 0xc0, 0xc9, 0xce, 0xdb, 0xdc, 0xd5, 0xd2,
0xff, 0xf8, 0xf1, 0xf6, 0xe3, 0xe4, 0xed, 0xea,
0xb7, 0xb0, 0xb9, 0xbe, 0xab, 0xac, 0xa5, 0xa2,
0x8f, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9d, 0x9a,
0x27, 0x20, 0x29, 0x2e, 0x3b, 0x3c, 0x35, 0x32,
0x1f, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0d, 0x0a,
0x57, 0x50, 0x59, 0x5e, 0x4b, 0x4c, 0x45, 0x42,
0x6f, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7d, 0x7a,
0x89, 0x8e, 0x87, 0x80, 0x95, 0x92, 0x9b, 0x9c,
0xb1, 0xb6, 0xbf, 0xb8, 0xad, 0xaa, 0xa3, 0xa4,
0xf9, 0xfe, 0xf7, 0xf0, 0xe5, 0xe2, 0xeb, 0xec,
0xc1, 0xc6, 0xcf, 0xc8, 0xdd, 0xda, 0xd3, 0xd4,
0x69, 0x6e, 0x67, 0x60, 0x75, 0x72, 0x7b, 0x7c,
0x51, 0x56, 0x5f, 0x58, 0x4d, 0x4a, 0x43, 0x44,
0x19, 0x1e, 0x17, 0x10, 0x05, 0x02, 0x0b, 0x0c,
0x21, 0x26, 0x2f, 0x28, 0x3d, 0x3a, 0x33, 0x34,
0x4e, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5c, 0x5b,
0x76, 0x71, 0x78, 0x7f, 0x6A, 0x6d, 0x64, 0x63,
0x3e, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2c, 0x2b,
0x06, 0x01, 0x08, 0x0f, 0x1a, 0x1d, 0x14, 0x13,
0xae, 0xa9, 0xa0, 0xa7, 0xb2, 0xb5, 0xbc, 0xbb,
0x96, 0x91, 0x98, 0x9f, 0x8a, 0x8D, 0x84, 0x83,
0xde, 0xd9, 0xd0, 0xd7, 0xc2, 0xc5, 0xcc, 0xcb,
0xe6, 0xe1, 0xe8, 0xef, 0xfa, 0xfd, 0xf4, 0xf3
};

#define proc_crc8(u8CRC, u8Data) (u8CRC8Table[u8CRC ^ u8Data])

int16_t enocean_open(char *dev)
{
   struct termios options, options_old;
   int fd;
   
   // ouverture du port
   int flags;
   
   flags=O_RDWR | O_NOCTTY | O_NDELAY | O_EXCL;
#ifdef O_CLOEXEC
   flags |= O_CLOEXEC;
#endif
   
   fd = open(dev, flags);
   if (fd == -1)
   {
      // ouverture du port serie impossible
      return -1;
   }
   
   // sauvegarde des caractéristiques du port serie
   tcgetattr(fd, &options_old);
   
   // initialisation à 0 de la structure des options (termios)
   memset(&options, 0, sizeof(struct termios));
   
   // paramétrage du débit
   if(cfsetispeed(&options, B57600)<0)
   {
      // modification du debit d'entrée impossible
      return -1;
   }
   if(cfsetospeed(&options, B57600)<0)
   {
      // modification du debit de sortie impossible
      return -1;
   }
   
   // ???
   options.c_cflag |= (CLOCAL | CREAD); // mise à 1 du CLOCAL et CREAD
   
   // 8 bits de données, pas de parité, 1 bit de stop (8N1):
   options.c_cflag &= ~PARENB; // pas de parité (N)
   options.c_cflag &= ~CSTOPB; // 1 bit de stop seulement (1)
   options.c_cflag &= ~CSIZE;
   options.c_cflag |= CS8; // 8 bits (8)
   
   // bit ICANON = 0 => port en RAW (au lieu de canonical)
   // bit ECHO =   0 => pas d'écho des caractères
   // bit ECHOE =  0 => pas de substitution du caractère d'"erase"
   // bit ISIG =   0 => interruption non autorisées
   options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
   
   // pas de contrôle de parité
   options.c_iflag &= ~INPCK;
   
   // pas de contrôle de flux
   options.c_iflag &= ~(IXON | IXOFF | IXANY);
   
   // parce qu'on est en raw
   options.c_oflag &=~ OPOST;
   
   // VMIN : Nombre minimum de caractère à lire
   // VTIME : temps d'attentes de données (en 10eme de secondes)
   // à 0 car O_NDELAY utilisé
   options.c_cc[VMIN] = 0;
   options.c_cc[VTIME] = 0;
   
   // réécriture des options
   tcsetattr(fd, TCSANOW, &options);

   return fd;
}


int16_t enocean_open_simulator(char *dev)
{
   return 1;
}


uint16_t uart_read_simulator(int16_t fd, int16_t *nerr)
{
   static int framePtr = 0;

   int16_t frame[] = { 0x01, 0x02, 0x55, 0x00, 0x55, 0x00, 0x0F, 0x07, 0x01, 0x2B, 0xD2, 0xDD, 0xDD, 0xDD, 0xDD, 0xDD, 0xDD, 0xDD, 0xDD, 0xDD, 0x00, 0x80, 0x35, 0xC4, 0x00, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0x4D, 0x00, 0x36, -1 };
//   int16_t frame[] = { 0x55, 0x00, 0x0F, 0x07, 0x01, 0x2B, 0xD2, 0xDD, 0xDD, 0xDD, 0xDD, 0xDD, 0xDD, 0xDD, 0xDD, 0xDD, 0x00, 0x80, 0x35, 0xC4, 0x00, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0x4D, 0x00, 0x36, -1 };
//   int16_t frame[] = { 0x55,0x00, 0x05, 0x00, 0x05, 0xDB, 0x01, 0x00, 0x00, 0x00, 0x0A, 0x54, -1 };

   if(frame[framePtr]==-1)
   {
      printf("No more data\n");
      exit(0);
   }
   return frame[framePtr++];
}


int16_t uart_read(int16_t fd, int16_t *nerr)
{
   fd_set input_set;
   struct timeval timeout;
   
   FD_ZERO(&input_set);
   FD_SET(fd, &input_set);
   
   timeout.tv_sec = 0;
   timeout.tv_usec = ENOCEAN_TIMEOUT_DELAY1_MS * 1000;
   int ret;
   uint8_t c;
   
   ret = select(fd+1, &input_set, NULL, NULL, &timeout);
   if(ret<=0)
   {
      if(ret == 0)
      {
         *nerr = ENOCEAN_ERR_TIMEOUT;
      }
      else
      {
         *nerr = ENOCEAN_ERR_SELECT;
      }
      goto on_error_exit_uart_read;
   }
   
   ret = (int)read(fd,&c,1);
   if(ret!=1)
   {
      VERBOSE(1) perror("READ: ");
      return -1;
   }
   else
   {
      *nerr = ENOCEAN_ERR_NOERR;
      return c;
   }

on_error_exit_uart_read:
   return -1;
}


unsigned long millis()
{
  struct timeval tv;
  gettimeofday(&tv,NULL);

  return 1000 * tv.tv_sec + tv.tv_usec/1000;
}


unsigned long diffMillis(unsigned long chrono, unsigned long now)
{
   return now >= chrono ? now - chrono : ULONG_MAX - chrono + now;
}


int16_t enocean_process_data(uint16_t *step, uint8_t c, uint8_t *data, uint16_t *l_data, int16_t *nerr)
{
 static uint16_t i=0;
 static uint8_t  enocean_data[0xFFFF];
 static uint16_t enocean_dataPtr = 0;
 static uint16_t enocean_data_l=0,enocean_optional_l=0;
 static uint8_t  crc8h = 0, crc8d = 0;
 static uint8_t  enocean_packet_type = 0;

   *nerr=ENOCEAN_ERR_NOERR;
   switch(*step)
   {
      case 0:
         if(c==0x55)
         {
            VERBOSE(5) fprintf(stderr, "STEP %d : 0x%02x\n", *step, c);
            crc8h = 0;
            crc8d = 0;
            enocean_data_l=0;
            enocean_optional_l=0;
            enocean_dataPtr=0;
            enocean_data[enocean_dataPtr++]=c;
            *step=*step+1;
            break;
         }
         *nerr=ENOCEAN_ERR_STARTFRAME;
         goto on_error_exit_enocean_process_data;

      case 1:
         VERBOSE(5) fprintf(stderr, "STEP %d : 0x%02x\n", *step, c);
         enocean_data_l=c;
         crc8h = proc_crc8(crc8h,c);
         enocean_data[enocean_dataPtr++]=c;
         *step=*step+1;
         break;

      case 2:
         VERBOSE(5) fprintf(stderr, "STEP %d : 0x%02x\n", *step, c);
         enocean_data_l=(enocean_data_l << 8) + c;
         crc8h = proc_crc8(crc8h,c);
         VERBOSE(6) fprintf(stderr, "   enocean_data_l = %d\n", enocean_data_l);
         if(enocean_data_l == 0)
         {
            *step = 0;
            *nerr = ENOCEAN_ERR_DATALENGTH;
            goto on_error_exit_enocean_process_data;
         }
         enocean_data[enocean_dataPtr++]=c;
         *step=*step+1;
         break;

      case 3:
         VERBOSE(5) fprintf(stderr, "STEP %d : 0x%02x\n", *step, c);
         enocean_optional_l=c;
         crc8h = proc_crc8(crc8h,c);
         VERBOSE(6) printf("   enocean_optional_l = %d\n",enocean_optional_l);
         enocean_data[enocean_dataPtr++]=c;
         *step=*step+1;
         break;

      case 4:
         VERBOSE(5) fprintf(stderr, "STEP %d : 0x%02x\n", *step, c);
         enocean_packet_type = c;
         crc8h = proc_crc8(crc8h,c);
         enocean_data[enocean_dataPtr++]=c;
         *step=*step+1;
         break;

      case 5:
         VERBOSE(5) fprintf(stderr, "STEP %d : 0x%02x\n", *step, c);
         VERBOSE(6) fprintf(stderr, "   crc8h = %d vs %d\n", crc8h, c);
         if(c != crc8h)
         {
            *nerr = ENOCEAN_ERR_CRC8H;
            *step = 0;
            goto on_error_exit_enocean_process_data;
         }
         crc8d=0;
         enocean_data[enocean_dataPtr++]=c;
         if(enocean_data_l)
            *step=*step+1;
         else
            *step=*step+2;
         i=0;
         break;

      case 6:
         VERBOSE(5) fprintf(stderr, "STEP %d : 0x%02x\n", *step, c);
         i++;
         crc8d = proc_crc8(crc8d,c);
         enocean_data[enocean_dataPtr++]=c;
         if(i>=enocean_data_l)
         {
            if(enocean_optional_l)
               *step=*step+1;
            else
               *step=*step+2;
            i=0;
         }
         break;

      case 7:
         VERBOSE(5) fprintf(stderr, "STEP %d : 0x%02x\n", *step, c);
         crc8d = proc_crc8(crc8d,c);
         i++;
         enocean_data[enocean_dataPtr++]=c;
         if(i>=enocean_optional_l)
            *step=*step+1; // read checksum
         break;

      case 8:
         VERBOSE(5) fprintf(stderr, "STEP %d : 0x%02x\n", *step, c);
         VERBOSE(6) fprintf(stderr, "   crc8d = %d vs %d\n", crc8d, c);
         if(c != crc8d)
         {
            *nerr = ENOCEAN_ERR_CRC8D;
            *step = 0;
            goto on_error_exit_enocean_process_data;
         }
         enocean_data[enocean_dataPtr++]=c;
         *step=*step+1;

         VERBOSE(6) fprintf(stderr, "   enocean_packet_type = 0x%02x (%03d)\n", enocean_packet_type, enocean_packet_type);
         VERBOSE(6) fprintf(stderr, "   enocean_data_length = %d\n", enocean_data_l);
         VERBOSE(6) fprintf(stderr, "   enocean_optional_length = %d\n", enocean_optional_l);
         
         VERBOSE(6) {
            for(int i=0;i<enocean_dataPtr;i++)
            {
               fprintf(stderr,"   data[%03d] = 0x%02x (%03d)\n", i, enocean_data[i], enocean_data[i]);
            }
         }
         
         if(enocean_dataPtr <= *l_data)
         {
            *l_data = enocean_dataPtr;
            memcpy(data, enocean_data, enocean_dataPtr);
            *nerr = ENOCEAN_ERR_NOERR;
         }
         else
         {
            *l_data = 0;
            *nerr = ENOCEAN_ERR_OUTOFRAGE;
            *step = 0;
         }

         return 1;
         break;
   }
   return 0;

on_error_exit_enocean_process_data:   
   return -1;   
}


int16_t enocean_read_packet(int16_t fd, uint8_t *data, uint16_t *l_data, int16_t *nerr)
{
   int ret=0;
   uint16_t step=0;
   
   int16_t rc=0;
   uint8_t c=0;
   
   uint8_t resyncBuffer[6]; // buffer taillé pour stocker les données d'un header complet
   uint16_t resyncBufferPtr=0;

   *nerr = 0;

   while(1)
   {
      *nerr=ENOCEAN_ERR_NOERR;
      
      rc=uart_read(fd,nerr);
      if(rc<0)
      {
         return -1;
      }
      else
         c = (uint8_t)(rc & 0xFF);
      
      /* note :
         La synchronisation se fait sur 6 octets qui correspondent au header d'un packet ESP3.
         Si lors du calcul du CRC8H on est pas synchro, il faut envisager qu'un 0x55 de début de trame
         est déjà passé. On va donc prévoir de rejouer les caractères déjà lus, on les stocke donc dans
         resyncBuffer tant qu'on a pas passé la validation de CRC8H (step 6). */
         
      if(step < 6) // on est dans la lecture du header
         resyncBuffer[resyncBufferPtr++]=c; // on garde trace des données pour pouvoir les rejouer en cas d'erreur de CRC

      ret=enocean_process_data(&step, c, data, l_data, nerr);
      if(ret<0) //  && step < 6
      {
         if(*nerr == ENOCEAN_ERR_CRC8H) // erreur de CRC, problème de synchro de début de trame ?
         {
            for(int i=1;i<resyncBufferPtr;i++) // on rejoue les données déjà reçues qu'on a pris pour le contenu d'un header
            {
               step=0; // réinitialisation de l'automate
               for(int j=i;j<resyncBufferPtr;j++) // on rejout tout sans ce poser de question
               {
                  enocean_process_data(&step, resyncBuffer[j], data, l_data, nerr);
               }
               resyncBufferPtr=0; // on vide le buffer
            }
         }
         else if(*nerr == ENOCEAN_ERR_STARTFRAME)
         {
//            VERBOSE(5) fprintf(stderr,"0x55 not found\n");
            resyncBufferPtr=0;
            
            // envisager ici de mettre un timeout ...
            
            continue; // tant qu'on a pas eu de start, on reboucle
         }
         else
         {
            // pour toutes autres erreurs on s'arrête
            return -1;
         }
      }
      else if(ret == 1)
      {
         VERBOSE(5) fprintf(stderr, "datagram found\n");
         return 0;
      }
      else
      {
         // une erreur à traiter
      }
   }
   *nerr=ENOCEAN_ERR_UNKNOWN;

   return -1;
}


int main(int argc, char *argv[])
{
 int16_t enocean_fd = 0;
 char *dev; // ="/dev/ttyS0";
 int16_t ret;
 
 if(argc!=2)
 {
    fprintf(stderr, "usage : %s /dev/<device>\n", argv[0]);
    exit(1);
 }
 
 dev = argv[1];
 
 uint8_t data[0xFFFF]; // taille maximum d'un packet ESP3 = 65536
 uint16_t l_data;
 int16_t nerr = 0;
  
   enocean_fd = enocean_open(dev);
   if(enocean_fd<0)
   {
      fprintf(stderr, "open error:");
      perror("");
      exit(1);
   }
   
   while(1)
   {
      l_data=sizeof(data);
      ret=enocean_read_packet(enocean_fd, data, &l_data, &nerr);
      if((ret == 0) & (l_data > 0))
      {
         if(data[4]==ENOCEAN_RADIO_ERP1)
         {
            switch(data[6])
            {
               case ENOCEAN_VLD_TELEGRAM:
                  /*
                     00 0x55  85 Synchro
                     01 0x00   0 Header1 Data Lenght 
                     02 0x09   9 Header2 Data Lenght
                     03 0x07   7 Header3 Optionnal lenght
                     04 0x01   1 Header4 Packet type
                     05 0x56  86 CRC8H
                     06 0xd2 210 RORG : D2
                     [
                     07 0x04   4 Power Failure(B7)/Power Detection(B6)/Command(B3-B0) : 4 = Actuator status
                     08 0x60  96 Over Current Switch off (B7)/Error lvl(B6-B5)/I/O Channel(B4-B0) : 0 = channel
                     09 0xe4 228 Local control(B7)/Output Value(B6-B0) : 1101100
                     ]
                     10 0x01   1 ID1
                     11 0x94 148 ID2
                     12 0xc9 201 ID3
                     13 0x40 064 ID4
                     14 0x00   0 (status)
                     15 0x01   1 (number sub telegram)
                     16 0xff 255 Optionnal2 Destination ID
                     17 0xff 255 Optionnal3 Destination ID
                     18 0xff 255 Optionnal4 Destination ID
                     19 0xff 255 Optionnal5 Destination ID
                     20 0x4d  77 Optionnal6 Dbm
                     21 0x00  0  Optionnal7 Security level
                     22 0x43  67 CRC8D
                  */
                  VERBOSE(1) {
                     fprintf(stderr, "=== VLD Telegram D2 ===\n");
                     fprintf(stderr, "Address   : %02x-%02x-%02x-%02x\n", data[10], data[11], data[12], data[13]);
                     fprintf(stderr, "Command   : %d\n", data[7] & 0b00001111);
                     fprintf(stderr, "channel   : %d\n", data[8] & 0b00011111);
                     fprintf(stderr, "state     : %d\n", data[9] & 0b01111111);
                     fprintf(stderr, "local ctrl: %d\n", (data[9] & 0b10000000) >> 7);
                  }
                  break;

               case ENOCEAN_UTE_TELEGRAM:
                  /*
                     Trame inversée ... profil v2.5 ?
                     data[000] = 0x55 (085)
                     data[001] = 0x00 (000)
                     data[002] = 0x0d (013)
                     data[003] = 0x07 (007)
                     data[004] = 0x01 (001)
                     data[005] = 0xfd (253)
                     data[006] = 0xd4 (212)
                     data[007] = 0xd2 (210) -+ RORG
                     data[008] = 0x01 (001)  | FUNC ?
                     data[009] = 0x01 (001)  | TYPE ?
                     data[010] = 0x00 (000)  | // b00000000 : MID LSB3 ?
                     data[011] = 0x3e (062)  | // b00111110 : MID MSB8 ?
                     data[012] = 0xff (255)  | // b11111111 All channel supported by the device
                     data[013] = 0xa0 (160) -+ // b10100000 1=bidirectional, 0=response expected, 10=teach-in or delete, 0000=command:teach-in request
                     data[014] = 0x01 (001) -+
                     data[015] = 0x84 (132)  | address :
                     data[016] = 0x5c (092)  | 01-84-5c-80
                     data[017] = 0x80 (128) -+
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
                  /*
                     Trame standard
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
                  if(data[7]!=0xD2)
                  {
                     VERBOSE(1) {
                        fprintf(stderr, "=== RPS Telegram D4 (UTE)  ===\n");
                        fprintf(stderr, "Adresse   : %02x-%02x-%02x-%02x\n", data[14], data[15], data[16], data[17]);
                        fprintf(stderr, "EEP       : %02x-%02x-%02x\n", data[13], data[12], data[11]);
                        fprintf(stderr, "num chnnl : %d\n", data[8]);
                        uint8_t operation = (data[7] & 0b10000000) >> 7;
                        uint8_t response  = (data[7] & 0b01000000) >> 6;
                        uint8_t request   = (data[7] & 0b00110000) >> 4;
                        uint8_t cmnd      = (data[7] & 0b00001111);
                        fprintf(stderr, "op: %u, rs: %u, rq: %u, cm: %u\n", operation, response, request, cmnd); 
                     }
                  }
                  else
                  {
                     VERBOSE(1) {
                        fprintf(stderr, "=== RPS Telegram D4 (UTE) inverted ===\n");
                        fprintf(stderr, "Adresse   : %02x-%02x-%02x-%02x\n", data[14], data[15], data[16], data[17]);
                        fprintf(stderr, "EEP       : %02x-%02x-%02x\n", data[7], data[8], data[9]);
                        fprintf(stderr, "num chnnl : %d\n", data[12]);
                        uint8_t operation = (data[13] & 0b10000000) >> 7;
                        uint8_t response  = (data[13] & 0b01000000) >> 6;
                        uint8_t request   = (data[13] & 0b00110000) >> 4;
                        uint8_t cmnd      = (data[13] & 0b00001111);
                        fprintf(stderr, "op: %u, rs: %u, rq: %u, cm: %u\n", operation, response, request, cmnd);
                     }
                  }
                  break;

               case ENOCEAN_RPS_TELEGRAM:
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

                     Avec B00110000
                        0011 :
                          001 => bouton A0
                            1 => bouton appuyé (0 = relaché)
                        0000 :
                  */
                  VERBOSE(1) {
                     fprintf(stderr, "=== RPS Telegram F6 ===\n");
                     fprintf(stderr, "Adresse   : %02x-%02x-%02x-%02x\n", data[8], data[9], data[10], data[11]);
                     int t21 = (data[12] & 0b00100000) >> 5;
                     fprintf(stderr,"T21       : %d\n", t21);
                     int nu = (data[12]  & 0b00010000) >> 4;
                     fprintf(stderr,"NU        : %d\n", nu);
               
                     if(t21 == 1)
                     {
                        if(nu == 1)
                        {
                           int butonNum = (data[7] & 0b11100000) >> 5;
                           fprintf(stderr,"Bouton    : %d\n", butonNum);
                           int action   = (data[7] & 0b00010000) >> 4;
                           if(data[7] & 0b00000001)
                           {
                              fprintf(stderr, "2d bouton : %d\n", (data[7] & 0b00001110) >> 1);
                           }
                           if(action == 1)
                              fprintf(stderr, "Action    : on\n");
                           else
                              fprintf(stderr, "Action    : off\n");
                        }
                        else
                        {
                           fprintf(stderr, "Nb boutons appuyes = %d\n", (data[7] & 0b11100000) >> 5);
                           fprintf(stderr, "Energy bow = %d\n", (data[7] & 0b00010000) >> 4);
                        }
                     }
               
                     fprintf(stderr, "Dbm = %d\n", data[18]);
                  }
                  break;
               default:
                  VERBOSE(1) {
                     for(int i=0; i<l_data; i++)
                     {
                        if(i && (i % 10) == 0)
                           fprintf(stderr, "\n");
                        fprintf(stderr, "0x%02x ", data[i]);
                     }
                  }
                  break;
            }
         }
         else // quand on ne sais pas analyser le packet, on le dump
         {
            for(int i=0; i<l_data; i++)
            {
               if(i && (i % 10) == 0)
                  fprintf(stderr, "\n");
               fprintf(stderr, "0x%02x ", data[i]);
            }
         }
      }
      else
      {
         // traiter ici les erreurs si nécessaire
      }
   }
}

