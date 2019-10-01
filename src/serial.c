//
//  serial.c
//  mea-edomus
//
//  Created by Patrice Dietsch on 16/11/2015.
//
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>

#include "mea_string_utils.h"

int serial_open(char *dev, int speed)
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
   if (fd == -1) {
      // ouverture du port serie impossible
      return -1;
   }

   // sauvegarde des caractéristiques du port serie
   tcgetattr(fd, &options_old);

   // initialisation à 0 de la structure des options (termios)
   memset(&options, 0, sizeof(struct termios));

   // paramétrage du débit
   if(cfsetispeed(&options, speed)<0) {
      // modification du debit d'entrée impossible
      return -1;
   }
   if(cfsetospeed(&options, speed)<0) {
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

   // préparation du descripteur

   return fd;
}


uint32_t speeds[][3]={
   {   300,    B300},
   {  1200,   B1200},
   {  4800,   B4800},
   {  9600,   B9600},
   { 19200,  B19200},
   { 38400,  B38400},
   { 57600,  B57600},
   {115200, B115200},
   {0,0}
};


int32_t get_speed_from_speed_t(speed_t speed)
{
   for(int16_t i=0;speeds[i][0];i++) {
      if(speeds[i][1]==speed)
         return speeds[i][0];
   }
   return -1;
}


int16_t get_dev_and_speed(char *device, char *dev, int16_t dev_l, speed_t *speed)
{
   *speed=0;
   char _dev[41];
   char reste[41];
   char vitesse[41];

   char *_dev_ptr;
   char *reste_ptr;
   char *vitesse_ptr;
   char *end=NULL;

   int16_t n=sscanf(device,"SERIAL://%40[^:]%40[^/r/n]",_dev,reste);
   if(n<=0)
      return -1;

   _dev_ptr=mea_strtrim(_dev);

   if(n==1) {
      *speed=B9600;
   }
   else {
      uint32_t v;

      reste_ptr=mea_strtrim(reste);
      n=sscanf(reste_ptr,":%40[^/n/r]",vitesse);
      if(n!=1)
         return -1;

      vitesse_ptr=mea_strtrim(vitesse);
      v=(uint32_t)strtol(vitesse_ptr,&end,10);
      if(end==vitesse || *end!=0 || errno==ERANGE)
         return -1;

      *speed=0;
      int i;
      for(i=0;speeds[i][0];i++) {
         if(speeds[i][0]==v) {
            *speed=speeds[i][1];
            break;
         }
      }
      if(*speed==0)
         return -1;
   }

   strncpy(dev,_dev_ptr,dev_l-1);

   return 0;
}
