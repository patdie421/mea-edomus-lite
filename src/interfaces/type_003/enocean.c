// 
//  Created by Patrice Dietsch on 15/12/2014.
//  Copyright (c) 2014 Patrice Dietsch. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <errno.h>
#include <sys/select.h>
#include <signal.h>
#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <limits.h>

#include "serial.h"
#include "enocean.h"
#include "mea_verbose.h"
#include "mea_queue.h"


typedef struct enocean_queue_elem_s
{
   char *frame;
   uint16_t l_frame;
   int16_t enocean_err;
   
} enocean_queue_elem_t;


void     enocean_close(enocean_ed_t *ed);
void     _enocean_free_queue_elem(void *d); // pour vider_file2
void    *_enocean_thread(void *args);
uint32_t enocean_calc_addr(uint8_t a, uint8_t b, uint8_t c, uint8_t d);


unsigned long _enocean_millis()
{
  struct timeval tv;
  gettimeofday(&tv,NULL);

  return 1000 * tv.tv_sec + tv.tv_usec/1000;
}


unsigned long _enocean_diffMillis(unsigned long chrono, unsigned long now)
{
   return now >= chrono ? now - chrono : ULONG_MAX - chrono + now;
}


int16_t _enocean_open(enocean_ed_t *ed, char *dev)
{
   int fd=serial_open(dev, B57600);
   if(fd != -1) {
      strncpy(ed->serial_dev_name,dev,sizeof(ed->serial_dev_name)-1);
      ed->serial_dev_name[sizeof(ed->serial_dev_name)-1]=0;
      ed->fd=fd;
   }

   return fd;
}


int16_t enocean_init(enocean_ed_t *ed, char *dev)
/**
 * \brief     Initialise les mécanismes de communication avec un periphérique serie ENOCEAN
 * \details   Cette fonction assure :
 *            - l'initialisation du "descripteur" (ensemble des éléments nécessaire à la gestion des échanges avec un périphérique ENOCEAN). Le descripteur sera utilisé par toutes les fonctions liées à la gestion ENOCEAN
 *            - le parametrage et l'ouverture du port serie (/dev/ttyxxx)
 * \param     ed     descripteur à initialiser. Il doit etre alloue par l'appelant.
 * \param     dev    le chemin "unix" (/dev/ttyxxx) de l'interface série (ou USB)
 * \return    -1 en cas d'erreur, descripteur du périphérique sinon
 */
{
   int16_t nerr;

   memset (ed,0,sizeof(enocean_ed_t));

   if(_enocean_open(ed, dev)<0)
     return -1;

   ed->enocean_callback=NULL;
   ed->enocean_callback_data=NULL;

   // préparation synchro consommateur / producteur
   pthread_cond_init(&ed->sync_cond, NULL);
   pthread_mutex_init(&ed->sync_lock, NULL);
   
   // verrou de mutex écriture vers mcu
   pthread_mutex_init(&ed->write_lock, NULL);

   // verrou de section critique interne
   pthread_mutex_init(&ed->ed_lock, NULL);
   
   ed->queue=(mea_queue_t *)malloc(sizeof(mea_queue_t));
   if(!ed->queue)
      return -1;
   
   mea_queue_init(ed->queue); // initialisation de la file
   ed->signal_flag=0;
   
   if(pthread_create (&(ed->read_thread), NULL, _enocean_thread, (void *)ed)) {
      enocean_close(ed);
      free(ed->queue);
      ed->queue=NULL;
      return -1;
   }

   VERBOSE(9) {
       mea_log_printf("%s (%s) : ENOCEAN thread_id = %x\n", INFO_STR, __func__,  (unsigned int)ed->read_thread);
   }

   ed->id=-1;
   ed->in_buffer.packet_l = 0;
   ed->in_buffer.timestamp = 0;
   ed->in_buffer.err = 0;

   if(enocean_get_baseid(ed, &ed->id,&nerr)<0) {  // pas de reponse correct
      enocean_close(ed);
      free(ed->queue);
      ed->queue=NULL;
      return -1;
   }

   return ed->fd;
}


enocean_ed_t *enocean_new_ed()
{
   return (enocean_ed_t *)malloc(sizeof(enocean_ed_t));
}


void enocean_clean_ed(enocean_ed_t *ed)
/**
 * \brief     Vide les "conteneurs" d'un descipteur ed
 * \details   Appeler cette fonction pour libérer la mémoire allouée pour un descipteur de communication enocean.
 * \param     ad   descripteur de communication comio
 */
{
   if(ed) {
      if(ed->queue) {
         free(ed->queue);
         ed->queue=NULL;
      }
   }
}


void enocean_free_ed(enocean_ed_t *ed)
/**
 * \brief     Vide les "conteneurs" d'un descipteur ad et libère la mémoire
 * \details   Appeler cette fonction pour libérer la mémoire allouée pour un descipteur de communication enocean.
 * \param     ad   descripteur de communication comio
 */
{
   if(ed) {
      enocean_clean_ed(ed);
      free(ed);
      ed=NULL;
   }
}


int16_t enocean_set_data_callback(enocean_ed_t *ed, enocean_callback_f f)
/**
 * \brief     met en place le callback qui sera déclenché à la réception de données.
 * \details   reférence le pointeur de la fonction à appeler dans le descripteur.
 * \param     ed     descripteur enocean.
 * \param     f      pointeur sur la fonction à appeler
 * \return    -1 si ed n'est pas initialisé (ie=NULL) 0 sinon
 */
{
   if(!ed)
      return -1;
   
   ed->enocean_callback=f;
   ed->enocean_callback_data=NULL;
   
   return 0;
}


int16_t enocean_set_data_callback2(enocean_ed_t *ed, enocean_callback_f f, void *data)
/**
 * \brief     met en place le callback qui sera declenché à la réception de données et stock un pointeur sur des données qui seront toujours tramisses au callback.
 * \details   référence le pointeur de la fonction à appeler et un pointeur sur des données (libre, a "caster" void *) dans le descripteur.
 * \param     ed     descripteur enocean.
 * \param     f      pointeur sur la fonction à appeler
 * \param     data   pointeur sur la zone de données
 * \return    -1 si ed n'est pas initialisé (ie=NULL) 0 sinon
 */
{
   if(!ed)
      return -1;

   ed->enocean_callback=f;
   ed->enocean_callback_data=data;
   
   return 0;
}


int16_t enocean_remove_data_callback(enocean_ed_t *ed)
/**
 * \brief     retiré le callback sur réception de data
 * \details   déréference la fonction et les donnees éventuellement associées.
 * \param     ed     descripteur enocean.
 * \return    -1 si ed n'est pas initialisé (ie=NULL) 0 sinon
 */
{
   if(!ed)
      return -1;

   ed->enocean_callback=NULL;
   ed->enocean_callback_data=NULL;

   return 0;
}


void enocean_close(enocean_ed_t *ed)
/**
 * \brief     fermeture d'une communication avec un contrôleur enocean
 * \details   arrête le thead de gestion de la communication, ménage dans ed et fermeture du "fichier".
 * \param     ad   descripteur de communication comio
 */
{
   pthread_cancel(ed->read_thread);
   pthread_join(ed->read_thread, NULL);
   
   close(ed->fd);
}


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

//#define proc_crc8(u8CRC, u8Data) (u8CRC8Table[u8CRC ^ u8Data])

//#define ENOCEAN_NB_RETRY 5
//#define ENOCEAN_ERR_READ 1
//#define ENOCEAN_ERR_TIMEOUT 2
//#define ENOCEAN_ERR_SELECT 3
//#define ENOCEAN_ERR_STARTFRAME 4
//#define ENOCEAN_ERR_UNKNOWN 5


int16_t _enocean_uart_read(int16_t fd, int32_t timeoutms, int16_t *nerr)
{
   fd_set input_set;
   struct timeval timeout;
   
   FD_ZERO(&input_set);
   FD_SET(fd, &input_set);
   
   timeout.tv_sec = 0;
   timeout.tv_usec = timeoutms * 1000;
   int ret;
   uint8_t c;
   
   ret = select(fd+1, &input_set, NULL, NULL, &timeout);
   if(ret<=0) {
      if(ret == 0) {
         *nerr = ENOCEAN_ERR_TIMEOUT;
      }
      else {
         *nerr = ENOCEAN_ERR_SELECT;
      }
      goto on_error_exit_uart_read;
   }
   
   ret = (int)read(fd,&c,1);
   if(ret!=1) {
      *nerr = ENOCEAN_ERR_SYS;
      return -1;
   }
   else {
      *nerr = ENOCEAN_ERR_NOERR;
      return c;
   }

on_error_exit_uart_read:
   return -1;
}


int16_t _enocean_process_data(uint16_t *step, uint8_t c, uint8_t *data, uint16_t *l_data, int16_t *nerr)
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
         if(c==0x55) {
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
         enocean_data_l=c;
         crc8h = proc_crc8(crc8h,c);
         enocean_data[enocean_dataPtr++]=c;
         *step=*step+1;
         break;

      case 2:
         enocean_data_l=(enocean_data_l << 8) + c;
         crc8h = proc_crc8(crc8h,c);
         if(enocean_data_l == 0) {
            *step = 0;
            *nerr = ENOCEAN_ERR_DATALENGTH;
            goto on_error_exit_enocean_process_data;
         }
         enocean_data[enocean_dataPtr++]=c;
         *step=*step+1;
         break;

      case 3:
         enocean_optional_l=c;
         crc8h = proc_crc8(crc8h,c);
         enocean_data[enocean_dataPtr++]=c;
         *step=*step+1;
         break;

      case 4:
         enocean_packet_type = c;
         crc8h = proc_crc8(crc8h,c);
         enocean_data[enocean_dataPtr++]=c;
         *step=*step+1;
         break;

      case 5:
         if(c != crc8h) {
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
         i++;
         crc8d = proc_crc8(crc8d,c);
         enocean_data[enocean_dataPtr++]=c;
         if(i>=enocean_data_l) {
            if(enocean_optional_l)
               *step=*step+1;
            else
               *step=*step+2;
            i=0;
         }
         break;

      case 7:
         crc8d = proc_crc8(crc8d,c);
         i++;
         enocean_data[enocean_dataPtr++]=c;
         if(i>=enocean_optional_l)
            *step=*step+1; // read checksum
         break;

      case 8:
         if(c != crc8d) {
            *nerr = ENOCEAN_ERR_CRC8D;
            *step = 0;
            goto on_error_exit_enocean_process_data;
         }
         enocean_data[enocean_dataPtr++]=c;
         *step=*step+1;
      
         if(enocean_dataPtr <= *l_data) {
            *l_data = enocean_dataPtr;
            memcpy(data, enocean_data, enocean_dataPtr);
            *nerr = ENOCEAN_ERR_NOERR;
         }
         else {
            *l_data = 0;
            *nerr = ENOCEAN_ERR_OUTOFRAGE;
            *step = 0;
         }

         return 1;
   }
   return 0;

on_error_exit_enocean_process_data:   
   return -1;   
}


int16_t _enocean_read_packet(int16_t fd, uint8_t *data, uint16_t *l_data, int16_t *nerr)
{ 
   int ret=0;
   uint16_t step=0;
   
   int16_t rc=0;
   uint8_t c=0;
   
   uint8_t resyncBuffer[6]; // buffer taillé pour stocker les données d'un header complet
   uint16_t resyncBufferPtr=0;

   *nerr = 0;

   while(1) {
      *nerr=ENOCEAN_ERR_NOERR;
      
      rc=_enocean_uart_read(fd, ENOCEAN_TIMEOUT_DELAY1_MS, nerr);
      if(rc<0) {
         *l_data=0;
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

      ret=_enocean_process_data(&step, c, data, l_data, nerr);
      if(ret<0) {  //  && step < 6
         if(*nerr == ENOCEAN_ERR_CRC8H) {  // erreur de CRC, problème de synchro de début de trame ?
            step=0; // réinitialisation de l'automate
            for(int j=1;j<resyncBufferPtr;j++) { // on rejoue tout (sauf le premier caractère) sans ce poser de question
               _enocean_process_data(&step, resyncBuffer[j], data, l_data, nerr);
            }
            resyncBufferPtr=0; // on vide le buffer
         }
         else if(*nerr == ENOCEAN_ERR_STARTFRAME) {
            resyncBufferPtr=0;
            
            // envisager ici de mettre un timeout ...
            
            continue; // tant qu'on a pas eu de start, on reboucle
         }
         else {
            // pour toutes autres erreurs on s'arrête
            *l_data=0;
            return -1;
         }
      }
      else if(ret == 1) {
         return 0;
      }
      else {
         // une erreur à traiter
      }
   }
   *nerr=ENOCEAN_ERR_UNKNOWN;
   *l_data=0;
   return -1;
}


int16_t _enocean_build_radio_erp1_packet(uint8_t rorg, uint32_t source, uint32_t dest, uint8_t *data, uint16_t l_data, uint8_t status, uint8_t *packet, uint16_t *l_packet)
{
   uint16_t ptr=0;
   uint8_t crc8h = 0, crc8d = 0;
   
   if(*l_packet < 20+l_data)
      return -1;

   switch(rorg)
   {
      case ENOCEAN_RPS_TELEGRAM:
      case ENOCEAN_1BS_TELEGRAM:
         if(l_data != 1)
            return -1;
         break;
         
      case ENOCEAN_4BS_TELEGRAM:
         if(l_data != 4)
            return -1;
         break;

      case ENOCEAN_UTE_TELEGRAM:
         if(l_data != 7)
            return -1;
         break;

      case ENOCEAN_VLD_TELEGRAM:
         if(!l_data || l_data > 14)
            return -1;
         break;
         
      default:
         return -1;
   }

   // Octet de synchro
   packet[ptr++] = 0x55;
   
   // longueur données
   packet[ptr++] = 0;
   crc8h = proc_crc8(crc8h, 0);
   packet[ptr++] = 6+l_data;
   crc8h = proc_crc8(crc8h, 6+l_data);
   
   // longueur données optionnelles  
   packet[ptr++] = 7;
   crc8h = proc_crc8(crc8h, 7);

   // type de packet
   packet[ptr++] = ENOCEAN_RADIO_ERP1;
   crc8h = proc_crc8(crc8h, ENOCEAN_RADIO_ERP1);

   // CRC8H
   packet[ptr++] = crc8h;
         
   // rorg
   packet[ptr++] = rorg;
   crc8d = proc_crc8(crc8d, rorg);
   
   // données
   for(int i=0;i<l_data;i++) {
      crc8d = proc_crc8(crc8d, data[i]);
      packet[ptr++] = data[i];
   }

   // ajout adresse source
   uint8_t a,b,c,d;
   uint32_t addr = source;
 
   d=addr & 0xFF;
   addr = addr >> 8;
   c=addr & 0xFF;
   addr = addr >> 8;
   b=addr & 0xFF;
   addr = addr >> 8;
   a=addr & 0xFF;
         
   packet[ptr++] = a;
   crc8d = proc_crc8(crc8d, a);
   packet[ptr++] = b;
   crc8d = proc_crc8(crc8d, b);
   packet[ptr++] = c;
   crc8d = proc_crc8(crc8d, c);
   packet[ptr++] = d;
   crc8d = proc_crc8(crc8d, d);

   // ajout status
   packet[ptr++] = status;
   crc8d = proc_crc8(crc8d, status);
   
   // données optionnelles
   // num subtelegram
   packet[ptr++] = 3;
   crc8d = proc_crc8(crc8d, 3);

   // ajout adresse destination
   addr = dest;
 
   d=addr & 0xFF;
   addr = addr >> 8;
   c=addr & 0xFF;
   addr = addr >> 8;
   b=addr & 0xFF;
   addr = addr >> 8;
   a=addr & 0xFF;
         
   packet[ptr++] = a;
   crc8d = proc_crc8(crc8d, a);
   packet[ptr++] = b;
   crc8d = proc_crc8(crc8d, b);
   packet[ptr++] = c;
   crc8d = proc_crc8(crc8d, c);
   packet[ptr++] = d;
   crc8d = proc_crc8(crc8d, d);

   // RSSI
   packet[ptr++] = 0xFF;
   crc8d = proc_crc8(crc8d, 0xFF);   
   
   // security level
   packet[ptr++] = 0;
   crc8d = proc_crc8(crc8d, 0);
   
   packet[ptr++] = crc8d;
   
   *l_packet=ptr;
   
   return 0;
}


int16_t enocean_send_packet(enocean_ed_t *ed, uint8_t *packet, uint16_t l_packet, uint8_t *response, uint16_t *l_response, int16_t *nerr)
{
   int16_t ret=-1;
   
   pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)&(ed->write_lock) );
   pthread_mutex_lock(&(ed->write_lock));

   struct timeval tv;
   struct timespec ts;
  
   int wr=(int)write(ed->fd, packet, l_packet);
   if(wr!=-1) {
      pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)&(ed->sync_lock) );
      pthread_mutex_lock(&(ed->sync_lock));

      // préparation du buffer pour la réponse
      ed->in_buffer.timestamp = _enocean_millis();
      ed->in_buffer.packet_l=0;
      ed->in_buffer.err=0;
     
      gettimeofday(&tv, NULL);
      ts.tv_sec = tv.tv_sec + 2;
      ts.tv_nsec = 0;
      int pr=pthread_cond_timedwait(&ed->sync_cond, &ed->sync_lock, &ts);
      if(pr) {
         if(pr!=ETIMEDOUT)
            *nerr=ENOCEAN_ERR_SYS;
         else
            *nerr=ENOCEAN_ERR_TIMEOUT;
         *l_response = 0;
         ret=-1;
      }
      else {
         if(ed->in_buffer.err == 0) {
            int16_t i = 0;
            for(i=0;i<*l_response && i< ed->in_buffer.packet_l;i++) {
               response[i]= ed->in_buffer.packet[i];
            }
            *l_response=i;
            ret = 0;
         }
         else {
            *nerr = ed->in_buffer.err;
            ret = -1;
         }
      }
      pthread_mutex_unlock(&(ed->sync_lock));
      pthread_cleanup_pop(0);
   }
   else {
      *l_response=0;
      *nerr=ENOCEAN_ERR_SYS;
      ret=-1;
   }
   
   pthread_mutex_unlock(&(ed->write_lock));
   pthread_cleanup_pop(0);

   return  ret;
}


int16_t enocean_get_chipid(enocean_ed_t *ed, uint32_t *chipid, int16_t *nerr)
{
   uint8_t request[8];
   uint8_t response[40];
   uint16_t l_response=sizeof(response);
   
   uint16_t ptr=0;
   uint8_t crc8h=0, crc8d=0;
   
   request[ptr++] = 0x55;
   
   // longueur données
   request[ptr++] = 0;
   crc8h = proc_crc8(crc8h, 0);
   request[ptr++] = 1;
   crc8h = proc_crc8(crc8h, 1);
   
   // longueur données optionnelles  
   request[ptr++] = 0;
   crc8h = proc_crc8(crc8h, 0);
   
   // type de packet
   request[ptr++] = ENOCEAN_COMMON_COMMAND;
   crc8h = proc_crc8(crc8h, ENOCEAN_COMMON_COMMAND);

   // CRC8H
   request[ptr++] = crc8h;
   
   // donnée
   request[ptr++] = 3; // CO_RD_VERSION
   crc8d = proc_crc8(crc8d, 3);
   
   // CRC8D
   request[ptr++] = crc8d;
   
   *nerr=0;
   int r = enocean_send_packet(ed, request, ptr, response, &l_response, nerr);
   if(r!=-1) {
      VERBOSE(5) {
         mea_log_printf("%s ENOCEAN_RD_VERSION : Description  = %s\n",INFO_STR,&response[23]);
         mea_log_printf("%s ENOCEAN_RD_VERSION : Version APP  = %02d, %02d, %02d, %02d\n",INFO_STR,response[7],response[8],response[9],response[10]);
         mea_log_printf("%s ENOCEAN_RD_VERSION : Version API  = %02d, %02d, %02d, %02d\n",INFO_STR,response[11],response[12],response[13],response[14]);
         mea_log_printf("%s ENOCEAN_RD_VERSION : Chip ID      = %02x-%02x-%02x-%02x\n",INFO_STR,response[15],response[16],response[17],response[18]);
         mea_log_printf("%s ENOCEAN_RD_VERSION : Chip Version = %02d, %02d, %02d, %02d\n",INFO_STR,response[19],response[20],response[21],response[22]);
      }

      *chipid = enocean_calc_addr(response[15],response[16],response[17],response[18]);
   }
   else {
      return -1;
   }

   return 0;
}


int16_t enocean_learning_onoff(enocean_ed_t *ed, int onoff, int16_t *nerr)
{
   uint8_t request[14];
   uint8_t reponse[40];
   uint16_t l_reponse=sizeof(reponse);

   uint16_t ptr=0;
   uint8_t crc8h=0, crc8d=0;

   request[ptr++] = 0x55;

   // longueur données
   request[ptr++] = 0;
   crc8h = proc_crc8(crc8h, 0);
   request[ptr++] = 6;
   crc8h = proc_crc8(crc8h, 6);

   // longueur données optionnelles
   request[ptr++] = 1;
   crc8h = proc_crc8(crc8h, 1);

   // type de packet
   request[ptr++] = ENOCEAN_COMMON_COMMAND;
   crc8h = proc_crc8(crc8h, ENOCEAN_COMMON_COMMAND);

   // CRC8H
   request[ptr++] = crc8h;

   // donnée
   request[ptr++] = 23; // CO_WR_LEARNMODE = 23
   crc8d = proc_crc8(crc8d, 23);
  
   if(onoff != 0) 
      request[ptr] = 1; // Enable : 1 = start Learn mode
   else
      request[ptr] = 0; // Enable : 0 = stop Learn mode

   crc8d = proc_crc8(crc8d, request[ptr]);
   ptr++;

   for(int i=0;i<4;i++) { // Timeout : 0 => valeur par defaut : 60 s (60000 ms)
      request[ptr++] = 0;
      crc8d = proc_crc8(crc8d, 0);
   }

   request[ptr++] = 0xFF; // channel : 0xFF (Next channel relative ??? cf https://github.com/aschwith/smarthome/blob/master/enocean/__init__.py) 
   crc8d = proc_crc8(crc8d, 0xFF);

   // CRC8D
   request[ptr++] = crc8d;

   *nerr=0;
   l_reponse=40;
   if(enocean_send_packet(ed, request, ptr, reponse, &l_reponse, nerr)!=-1) {
      VERBOSE(9) {
         // traiter ici la réponse
         mea_log_printf("%s RESPONSE = %d %x\n",INFO_STR, reponse[6], reponse[6]);
      }
   }
   else {
      return -1;
   }

   return 0;
}


int16_t enocean_sa_learning_onoff(enocean_ed_t *ed, int onoff, int16_t *nerr)
{
   int ret = -1;
   uint8_t request[40];
   uint8_t response[40];
   uint16_t l_response=sizeof(response);

   uint16_t ptr=0;
   uint8_t crc8h=0, crc8d=0;

   request[ptr++] = 0x55;

   // longueur données
   request[ptr++] = 0;
   crc8h = proc_crc8(crc8h, 0);
   request[ptr++] = 7;
   crc8h = proc_crc8(crc8h, 7);

   // longueur données optionnelles
   request[ptr++] = 0;
   crc8h = proc_crc8(crc8h, 0);

   // type de packet
   request[ptr++] = ENOCEAN_SMART_ACK_COMMAND;
   crc8h = proc_crc8(crc8h, ENOCEAN_SMART_ACK_COMMAND);

   // CRC8H
   request[ptr++] = crc8h;

   // donnée
   request[ptr++] = 1; // SA_WR_LEARNMODE = 1
   crc8d = proc_crc8(crc8d, 1);

   if(onoff != 0)
      request[ptr] = 1; // Enable : 1 = start Learn mode
   else
      request[ptr] = 0; // Enable : 0 = stop Learn mode
   crc8d = proc_crc8(crc8d, request[ptr]);
   ptr++;

   request[ptr++] = 0; // Simple learnmode
   crc8d = proc_crc8(crc8d, 0);

   for(int i=0;i<4;i++) { // Timeout : 0 => valeur par defaut : 60 s (60000 ms) 
      request[ptr++] = 0;
      crc8d = proc_crc8(crc8d, 0);
   }

   // CRC8D
   request[ptr++] = crc8d;

   *nerr=0;
   if(enocean_send_packet(ed, request, ptr, response, &l_response, nerr)!=-1) {
      VERBOSE(9) {
         mea_log_printf("%s RESPONSE = %d\n",INFO_STR, response[6]);
      }
      ret=response[6];
   }
   else {
      ret=-1;
   }

   return ret;
}


int16_t enocean_sa_confirm_learn_response(enocean_ed_t *ed, uint16_t response_time, uint16_t confirm, int16_t *nerr)
{
   uint8_t request[40];
   uint8_t response[40];
   uint16_t l_response=sizeof(response);

   uint16_t ptr=0;
   uint8_t crc8h=0, crc8d=0;

   request[ptr++] = 0x55;

   // longueur données
   request[ptr++] = 0;
   crc8h = proc_crc8(crc8h, 0);
   request[ptr++] = 4;
   crc8h = proc_crc8(crc8h, 4);

   // longueur données optionnelles
   request[ptr++] = 0;
   crc8h = proc_crc8(crc8h, 0);

   // packet response
   request[ptr++] = ENOCEAN_RESPONSE;
   crc8h = proc_crc8(crc8h, ENOCEAN_RESPONSE);

   // CRC8H
   request[ptr++] = crc8h;

   // donnée
   request[ptr++] = 0; // RET_OK = 0
   crc8d = proc_crc8(crc8d, 23);

   // response time
   uint8_t rt = (response_time >> 8) & 0xFF;
   request[ptr++] = rt;
   crc8d = proc_crc8(crc8d, rt);
   rt = response_time & 0xFF;
   request[ptr++] = rt;
   crc8d = proc_crc8(crc8d, rt);

   request[ptr++] = confirm & 0xFF;
   crc8d = proc_crc8(crc8d, confirm & 0xFF);

   for(int i=0;i<4;i++) { // Timeout : 0 => valeur par defaut : 60 s (60000 ms)
      request[ptr++] = 0;
      crc8d = proc_crc8(crc8d, 0);
   }

   // CRC8D
   request[ptr++] = crc8d;

   *nerr=0;
   if(enocean_send_packet(ed, request, ptr, response, &l_response, nerr)!=-1) {
      VERBOSE(9) {
         mea_log_printf("%s RESPONSE = %d\n",INFO_STR, response[6]);
      }
   }
   else {
      return -1;
   }
   return 0;
}


int16_t enocean_get_baseid(enocean_ed_t *ed, uint32_t *baseid, int16_t *nerr)
{
   uint8_t request[8];
   uint8_t response[40];
   uint16_t l_response=sizeof(response);

   uint16_t ptr=0;
   uint8_t crc8h=0, crc8d=0;

   request[ptr++] = 0x55;

   // longueur données
   request[ptr++] = 0;
   crc8h = proc_crc8(crc8h, 0);
   request[ptr++] = 1;
   crc8h = proc_crc8(crc8h, 1);

   // longueur données optionnelles
   request[ptr++] = 0;
   crc8h = proc_crc8(crc8h, 0);

   // type de packet
   request[ptr++] = ENOCEAN_COMMON_COMMAND;
   crc8h = proc_crc8(crc8h, ENOCEAN_COMMON_COMMAND);

   // CRC8H
   request[ptr++] = crc8h;

   // donnée
   request[ptr++] = 8; // CO_RD_IDBASE = 8 
   crc8d = proc_crc8(crc8d, 8);

   // CRC8D
   request[ptr++] = crc8d;

   *nerr=0;
   if(enocean_send_packet(ed, request, ptr, response, &l_response, nerr)!=-1) {
         if(response[6]==0) {
            VERBOSE(9) {
               mea_log_printf("%s CO_RD_IDBASE : BASEID    = %02x-%02x-%02x-%02x\n",INFO_STR,response[7],response[8],response[9],response[10]);
               mea_log_printf("%s CO_RD_IDBASE : REMAINING = %d\n",INFO_STR, response[8]);
            }
            *baseid = enocean_calc_addr(response[7],response[8],response[9],response[10]);
         }
         else {
            return -1;
         }
   }
   else {
      return -1;
   }

   return 0;
}


int16_t enocean_send_radio_erp1_packet(enocean_ed_t *ed, uint8_t rorg, uint32_t source, uint32_t sub_id, uint32_t dest, uint8_t *data, uint16_t l_data, uint8_t status, int16_t *nerr)
{
   uint8_t packet[64];
   uint16_t l_packet = sizeof(packet);
   uint8_t response[8];
   uint16_t l_response=sizeof(response);
   int16_t return_val=-1;
  
   *nerr=0;
   
   memset(response, 0, 8);
   if(_enocean_build_radio_erp1_packet(rorg, source + sub_id, dest, data, l_data, status, packet, &l_packet)<0)
      return -1;

//   VERBOSE(9)
//   {
//      for(int i=0;i<l_packet;i++)
//         fprintf(stderr,"%02d %02x %03d\n", i, packet[i], packet[i]); 
//   }
   return_val=enocean_send_packet(ed, packet, l_packet, response, &l_response, nerr);
   if(return_val==0) {
      return_val = response[6];
      // voir s'il y a d'autre données à lire pour ce type de packet
   }
   else {
      VERBOSE(1) {
         mea_log_printf("%s (%s) : request error = %d\n", ERROR_STR, __func__,  *nerr);
      }
   }
   
   return return_val;
}


int _enocean_reopen(enocean_ed_t *ed)
{
   int fd; /* File descriptor for the port */
   uint8_t flag=0;
   char dev[255];
   
   if(!ed)
      return -1;

   strncpy(dev, ed->serial_dev_name, sizeof(dev));
   
   VERBOSE(9) mea_log_printf("%s (%s) : try to reset communication (%s).\n",INFO_STR,__func__,dev);
   
   close(ed->fd);
   
   for(int i=0;i<ENOCEAN_NB_RETRY;i++) { // 5 tentatives pour rétablir les communications
      fd = _enocean_open(ed, dev);
      if (fd == -1) {
         VERBOSE(1) {
            mea_log_printf("%s (%s) : try #%d/%d, unable to open serial port (%s) - ",ERROR_STR,__func__, i+1, ENOCEAN_NB_RETRY, dev);
            perror("");
         }
      }
      else {
         flag=1;
         break;
      }
      sleep(5);
   }
   
   if(!flag) {
      VERBOSE(1) mea_log_printf("%s (%s) : can't recover communication now\n", ERROR_STR,__func__);
      return -1;
   }
   
   VERBOSE(5) mea_log_printf("%s (%s) : communication reset successful.\n",INFO_STR,__func__);

   return 0;
}


uint32_t enocean_calc_addr(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{
   uint32_t addr = 0;
   
   addr = a;
   addr = addr << 8;
   addr = addr + b;
   addr = addr << 8;
   addr = addr + c;
   addr = addr << 8;
   addr = addr + d;
   
   return addr;
}


uint32_t _enocean_get_addr_from_packet(uint8_t *data, int16_t l_data)
{
   uint32_t addr=0;
   
   if(data[4]==ENOCEAN_RADIO_ERP1) {
//      int16_t addr_index = 5 + (data[1] << 8) + data[2] - 4;
      int16_t addr_index = (data[1] << 8) + data[2] + 1;
      addr = enocean_calc_addr(data[addr_index],data[addr_index+1],data[addr_index+2],data[addr_index+3]);
   }
   
   return addr;
}


void *_enocean_thread(void *args)
{
   unsigned char packet[ENOCEAN_MAX_FRAME_SIZE];
   uint16_t l_packet;
   int16_t nerr;
   int16_t ret;
   
   enocean_ed_t *ed=(enocean_ed_t *)args;
   
   VERBOSE(5) mea_log_printf("%s (%s) : starting enocean read thread %s\n", INFO_STR, __func__, ed->serial_dev_name);
   while(1) {
      l_packet = sizeof(packet);
      ret=_enocean_read_packet(ed->fd, (uint8_t *)packet, &l_packet, &nerr);
      if(ret==0) {
         switch(packet[4]) {  // type de trame
            case ENOCEAN_RADIO_ERP1:
               if(ed->enocean_callback) {
                  uint32_t addr = 0;
                  addr = _enocean_get_addr_from_packet(packet,l_packet);
                  ed->enocean_callback(packet,l_packet, addr, ed->enocean_callback_data);
               }
               break;
            case ENOCEAN_EVENT:
               if(ed->enocean_callback) {
                  ed->enocean_callback(packet,l_packet, 0, ed->enocean_callback_data);
               }
               break;
            case ENOCEAN_RESPONSE:
                pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)&(ed->sync_lock) );
                pthread_mutex_lock(&ed->sync_lock);

                unsigned long now = _enocean_millis();
                if( _enocean_diffMillis(ed->in_buffer.timestamp, now) < 500) {
                   ed->in_buffer.packet_l = l_packet;
                   for(int i=0;i<sizeof(ed->in_buffer.packet) && i<l_packet;i++) {
                      ed->in_buffer.packet[i]=packet[i];
                   }
                   ed->in_buffer.err = 0;
                }
                else {
                   ed->in_buffer.packet_l = 0;
                   ed->in_buffer.err=ENOCEAN_ERR_TIMEOUT;
                }
                pthread_cond_broadcast(&ed->sync_cond);
                pthread_mutex_unlock(&ed->sync_lock);
                pthread_cleanup_pop(0);
                break;
            default:
               VERBOSE(1) {
                  mea_log_printf("%s (%s) : unsupported packet recepted\n", ERROR_STR, __func__);
                  for(int i=0; i<l_packet; i++) {
                     if(i && (i % 10) == 0)
                        fprintf(stderr, "\n");
                     fprintf(stderr, "0x%02x ", packet[i]);
                  }
               }
         }
      }
      
      if(ret<0) {
         switch(nerr)
         {
            case ENOCEAN_ERR_TIMEOUT:
               break;
            case ENOCEAN_ERR_SELECT:
            case ENOCEAN_ERR_READ:
            case ENOCEAN_ERR_SYS:
               VERBOSE(1) {
                  mea_log_printf("%s (%s) : communication error (nerr=%d) - ", ERROR_STR, __func__, nerr);
                  perror("");
               }
               if(_enocean_reopen(ed)<0) {
                  ed->signal_flag=1;
                  VERBOSE(1) {
                     mea_log_printf("%s (%s) : enocean thread goes down\n", ERROR_STR,__func__);
                  }
                  pthread_exit(NULL);
               }
               ed->signal_flag=0;
               break;

            default:
               VERBOSE(1) {
                  mea_log_printf("%s (%s) : error=%d.\n", ERROR_STR,__func__,nerr);
                  break;
               }
         }
      }
      pthread_testcancel();
   }
}
