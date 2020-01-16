/**
 * \file      xbee.c
 * \author    patdie421
 * \version   0.1
 * \date      23 mai 2013
 * \brief     moteur de gestion de communication avec XBEE et fonctions "clientes"
 *
 * \details   bla
 *            bla
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <errno.h>
#include <sys/select.h>
#include <signal.h>
#include "mea_verbose.h"
#include <inttypes.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>

#include "serial.h"
#include "xbee.h"
#include "mea_queue.h"

#define XBEE_TIMEOUT_DELAY 1
#define XBEE_NB_RETRY 5

char *xbee_at_io_id_list[XBEE_NB_ID_PIN]={"D0","D1","D2","D3","D4","D5","D6","D7","D8","", "P0", "P1", "P2", "P3"};

typedef struct xbee_queue_elem_s
{
   unsigned char cmd[255];
   int l_cmd;
   uint32_t tsp;
   int xbee_err;
   
} xbee_queue_elem_t;


struct xbee_map_nd_resp_data
{
   unsigned char MY[2];
   unsigned char SH[4];
   unsigned char SL[4];
   unsigned char DB[1];
   char NI[];
} __attribute__((packed));


// prototype des fonctions internes
void *_xbee_thread(void *args);
int   _xbee_build_frame(unsigned char *frame, unsigned char *cmd, uint16_t l_cmd);
int   _xbee_read_cmd(int fd, unsigned char *frame, uint16_t *l_frame, int16_t *nerr);
int   _xbee_write_cmd(int fd, unsigned char *cmd, uint16_t l_cmd, int16_t *nerr);
int   _xbee_network_discovery_resp(xbee_xd_t *xd, char *data, uint16_t l_data);
int16_t
      _xbee_add_response_to_queue(xbee_xd_t *xd, unsigned char *cmd, uint16_t l_cmd);

void  _xbee_display_frame(int ret, unsigned char *resp, uint16_t l_resp);

void  _xbee_host_set_addr_16(xbee_host_t *xbee_host, char *addr_16);
void  _xbee_host_set_name(xbee_host_t *xbee_host, unsigned char *name, uint16_t l);
void  _xbee_host_init_addr_64(xbee_host_t *xbee_host, uint32_t int_addr_64_h, uint32_t int_addr_64_l);
void  _xbee_host_set_addr(xbee_host_t *xbee_host, uint32_t int_addr_64_h, uint32_t int_addr_64_l);

int   _xbee_update_hosts_tables(xbee_hosts_table_t *table, char *addr_64_h, char *addr_64_l,
                        char *addr_16, char *name);
xbee_hosts_table_t 
     *_hosts_table_create(int max_nb_hosts, int16_t *nerr);
xbee_host_t
     *_xbee_add_new_to_hosts_table_with_addr64(xbee_hosts_table_t *table, uint32_t addr_64_h, uint32_t addr_64_l, int16_t *nerr);
xbee_host_t
     *_xbee_host_find_host_by_name(xbee_hosts_table_t *table, char *name);
xbee_host_t
     *_xbee_host_find_host_by_addr64(xbee_hosts_table_t *table, uint32_t addr64_h, uint32_t addr64_l);

void  _xbee_free_queue_elem(void *d);
uint32_t
_xbee_get_timestamp(void);
void  _xbee_flush_old_responses_queue(xbee_xd_t *xd);

int   _xbee_build_at_cmd(unsigned char *frame, uint8_t id, unsigned char *at_cmd, uint16_t l_at_cmd);
int   _xbee_build_at_remote_cmd(unsigned char *cmd, uint8_t id, xbee_host_t *addr, unsigned char *at_cmd, uint16_t l_at_cmd);
uint16_t
      _xbee_get_frame_data_id(xbee_xd_t *xd);


int _xbee_open(xbee_xd_t *xd, char *dev, int speed)
{
   int fd=serial_open(dev, speed);
   if(fd != -1) {
      strncpy(xd->serial_dev_name, dev,sizeof(xd->serial_dev_name)-1);
      xd->serial_dev_name[sizeof(xd->serial_dev_name)-1]=0;
      xd->speed=speed;
      xd->fd=fd;
   }

   return fd;
}


int   xbee_init(xbee_xd_t *xd, char *dev, int speed)
/**
 * \brief     Initialise les mécanismes de communication avec un periphérique serie XBEE
 * \details   Cette fonction assure :
 *            - l'initialisation du "descripteur" (ensemble des éléments nécessaire à la gestion des échanges avec un XBEE). Le descripteur sera utilisé par toutes les fonctions liées à la gestion des XBEE (toujours premier paramètre des fonctions xbee_*).
 *            - le parametrage et l'ouverture du port serie (/dev/ttyxxx)
 * \param     xd     descripteur à initialiser. Il doit etre alloue par l'appelant.
 * \param     dev    le chemin "unix" (/dev/ttyxxx) de l'interface série (ou USB)
 * \param     speed  le debit de la liaison serie (constante de termios)
 * \return    -1 en cas d'erreur, 0 sinon
 */
{
   int16_t nerr;

   memset (xd,0,sizeof(xbee_xd_t));

   if(_xbee_open(xd, dev, speed)<0)
     return -1;
     
   // préparation synchro consommateur / producteur
   pthread_cond_init(&xd->sync_cond, NULL);
   pthread_mutex_init(&xd->sync_lock, NULL);

   // verrou de section critique interne
   pthread_mutex_init(&xd->xd_lock, NULL);
                      
   xd->queue=(mea_queue_t *)malloc(sizeof(mea_queue_t));
   if(!xd->queue) {
      close(xd->fd);
      return -1;
   }
   
   mea_queue_init(xd->queue); // initialisation de la file
   
   xd->hosts=_hosts_table_create(XBEE_MAX_HOSTS, &nerr);
   if(!xd->hosts) {
      if(xd->queue) {
         free(xd->queue);
         xd->queue=NULL;
      }
      close(xd->fd);
      xd->fd=-1;
      return -1;
   }
   
   xd->io_callback=NULL;
   xd->commissionning_callback=NULL;
   
   xd->frame_id=1;
   xd->signal_flag=0;

   if(pthread_create (&(xd->read_thread), NULL, _xbee_thread, (void *)xd)) {
      if(xd->queue) {
         free(xd->queue);
         xd->queue=NULL;
      }

      close(xd->fd);
      xd->fd=-1;
      return -1;
   }
   
   return xd->fd;
}


uint16_t _xbee_get_frame_data_id(xbee_xd_t *xd)
/**
 * \brief     retour un identifiant "unique" à utiliser pour construire une de trame API AT.
 * \details   Les identifiants sont séquentiels et se trouvent dans la plage 1 - XBEE_MAX_USER_FRAME_ID. Les id > XBEE_MAX_USER_FRAME_ID sont resevés pour d'autres utilisations.
 * \param     xd     descripteur xbee.
 * \return    id     entre 1 et XBEE_MAX_USER_FRAME_ID
 */
{
   unsigned char ret;
   
   pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)&(xd->xd_lock) );
   pthread_mutex_lock(&(xd->xd_lock));

   ret=xd->frame_id;
   
   xd->frame_id++;
   if (xd->frame_id>XBEE_MAX_USER_FRAME_ID)
      xd->frame_id=1;
   
   pthread_mutex_unlock(&(xd->xd_lock));
   pthread_cleanup_pop(0);

   return ret;
}


int _xbee_build_at_cmd(unsigned char *frame, uint8_t id, unsigned char *at_cmd, uint16_t l_at_cmd)
/**
 * \brief     construit un "frame data" pour une commande AT local.
 * \details   Il s'agit de concaténer le frame_type (0x08), l'id de trame (frame_id) et la commande AT accompagné son paramètre (si nécessaire) dans un tableau de char.
 * \param     frame     la trame data complete (allouee par l'appelant)
 * \param     id        l'identifiant de la trame
 * \param     at_cmd    la commande AT et son parametre
 * \param     l_at_cmd  la longueur de la commande AT
 * \return    longueur de la trame
 */
{
   uint16_t i=0;
   
   frame[i++]=0x08;
   frame[i++]=id;
   for(uint16_t j=0;j<l_at_cmd;j++)
      frame[i++]=at_cmd[j];
   return i;
}


int _xbee_build_at_remote_cmd(unsigned char *frame, uint8_t id, xbee_host_t *addr, unsigned char *at_cmd, uint16_t l_at_cmd)
/**
 * \brief     construit un "frame data" pour une commande AT remote.
 * \details   Il s'agit de concaténer le frame_type (0x17), l'adresse de destination, l'id de trame (frame_id) et la commande AT et son parametre dans un tableau de char (uint8). 
 * \param     frame     la trame data complete (allouee par l'appelant)
 * \param     id        l'identifiant de la trame
 * \param     destination    les adresses (courte et longue) du destinataire
 * \param     at_cmd    la commande AT et son paramètre
 * \param     l_at_cmd  la longueur de la commande AT
 * \return    longueur de la trame
 */
{
   uint16_t i=0;
   
   frame[i++]=0x17;
   frame[i++]=id;
   
   for(uint16_t j=0;j<4;j++)
      frame[i++]=addr->addr_64_h[j];
   
   for(uint16_t j=0;j<4;j++)
      frame[i++]=addr->addr_64_l[j];
   
   frame[i++]=addr->addr_16[0];
   frame[i++]=addr->addr_16[1];
   
   frame[i++]=0x02; // apply change
   
   for(uint16_t j=0;j<l_at_cmd;j++)
      frame[i++]=at_cmd[j];
   
   return i;
}


mea_error_t xbee_set_iodata_callback(xbee_xd_t *xd, callback_f f)
/**
 * \brief     met en place le callback qui sera déclenché à la réception d'une trame iodata.
 * \details   reférence le pointeur de la fonction à appeler dans le descripteur.
 * \param     xd     descripteur xbee.
 * \param     f      pointeur sur la fonction à appeler
 * \return    -1 si xd n'est pas initialisé (ie=NULL) 0 sinon
 */
{
   if(!xd)
      return ERROR;
   
   xd->io_callback=f;
   xd->io_callback_data=NULL;
   
   return NOERROR;
}


mea_error_t xbee_set_iodata_callback2(xbee_xd_t *xd, callback_f f, void *data)
/**
 * \brief     met en place le callback qui sera declenché à la réception d'une trame iodata et stock un pointeur sur des données qui seront toujours tramisses au callback.
 * \details   référence le pointeur de la fonction à appeler et un pointeur sur des données (libre, a "caster" void *) dans le descripteur.
 * \param     xd     descripteur xbee.
 * \param     f      pointeur sur la fonction à appeler
 * \param     data   pointeur sur la zone de données
 * \return    toujours 0
 */
{
   if(!xd)
      return ERROR;

   xd->io_callback=f;
   xd->io_callback_data=data;
   
   return NOERROR;
}


mea_error_t xbee_remove_iodata_callback(xbee_xd_t *xd)
/**
 * \brief     retiré le callback sur réception de trame iodata
 * \details   déréference la fonction et les donnees éventuellement associées.
 * \param     xd     descripteur xbee.
 * \return    toujours 0
 */
{
   if(!xd)
      return ERROR;

   xd->io_callback=NULL;
   xd->io_callback_data=NULL;
   
   return NOERROR;
}


mea_error_t xbee_set_dataflow_callback(xbee_xd_t *xd, callback_f f)
/**
 * \brief     met en place le callback qui sera déclenché à la réception d'un flot de données xbee
 * \details   référence le pointeur de la fonction à appeler dans le descripteur.
 * \param     xd     descripteur xbee.
 * \param     f      pointeur sur la fonction a appeler
 * \return    toujours 0
 */
{
   if(!xd)
      return ERROR;

   xd->dataflow_callback=f;
   xd->dataflow_callback_data=NULL;
   
   return NOERROR;
}


mea_error_t xbee_set_dataflow_callback2(xbee_xd_t *xd, callback_f f, void *data)
/**
 * \brief     met en place le callback qui sera declenche a la reception d'un flot de donnees xbee et stock un pointeur sur des donnees qui seront toujours tramisses au callback.
 * \details   reference le pointeur de la fonction a appeler et un pointeur sur des donnees (libre, a caster void *) dans le descripteur. 
 * \param     xd     descripteur xbee.
 * \param     f      pointeur sur la fonction a appeler
 * \param     data   pointeur sur la zone de donnees
 * \return    toujours 0
 */
{
   if(!xd)
      return ERROR;

   xd->dataflow_callback=f;
   xd->dataflow_callback_data=data;
   
   return NOERROR;
}


mea_error_t xbee_remove_dataflow_callback(xbee_xd_t *xd)
/**
 * \brief     retire le callback sur reception d'un flot de donnees
 * \details   dereference la fonction et les donnees eventuellement associees. 
 * \param     xd     descripteur xbee.
 * \return    toujours 0
 */
{
   if(!xd)
      return ERROR;

   xd->dataflow_callback=NULL;
   xd->dataflow_callback_data=NULL;
   
   return NOERROR;
}


mea_error_t xbee_set_commissionning_callback(xbee_xd_t *xd, callback_f f)
/**
 * \brief     met en place le callback qui sera declenche a la reception d'une demande de commissionnement
 * \details   reference le pointeur de la fonction a appeler dans le descripteur. 
 * \param     xd     descripteur xbee.
 * \param     f      pointeur sur la fonction a appeler
 * \return    toujours 0
 */
{
   if(!xd)
      return ERROR;

   xd->commissionning_callback=f;
   xd->commissionning_callback_data=NULL;
   
   return NOERROR;
}


mea_error_t xbee_set_commissionning_callback2(xbee_xd_t *xd, callback_f f, void *data)
/**
 * \brief     met en place le callback qui sera declenche a la reception d'une demande de commissionnement et stock un pointeur sur des donnees qui seront toujours tramisses au callback.
 * \details   reference le pointeur de la fonction a appeler et un pointeur sur des donnees (libre, a caster void *) dans le descripteur. 
 * \param     xd     descripteur xbee.
 * \param     f      pointeur sur la fonction a appeler
 * \param     data   pointeur sur la zone de donnees
 * \return    toujours 0
 */
{
   if(!xd)
      return ERROR;

   xd->commissionning_callback=f;
   xd->commissionning_callback_data=data;
   
   return NOERROR;
}


mea_error_t xbee_remove_commissionning_callback(xbee_xd_t *xd)
/**
 * \brief     retire le callback sur reception d'un flot de donnees
 * \details   dereference la fonction et les donnees eventuellement associees. 
 * \param     xd    descripteur xbee.
 * \return    toujours 0
 */
{
   if(!xd)
      return ERROR;

   xd->commissionning_callback=NULL;
   xd->commissionning_callback_data=NULL;
   
   return NOERROR;
}


xbee_host_t *_xbee_host_find_host_by_addr64(xbee_hosts_table_t *table, uint32_t addr64_h, uint32_t addr64_l)
/**
 * \brief     recherche dans la table des hosts xbee un xbee avec l'adresse 64 bits precisee et le retourne. 
 * \details   retourne un pointeur sur l'element de la table repondant au critere ou NULL si non trouve. C'est une fonction a usage interne.
 * \param     table  pointeur sur la table host
 * \param     addr_64_h mot haut de l'adresse xbee 64 bits
 * \param     addr_64_l mot bas de l'adresse xbee 64 bits
 * \return    pointeur sur l'entree de la table contenant le host recherche ou NULL
 */
{
   uint16_t nb=0;
   
   if(!table)
      return NULL;
   
   for(uint16_t i=0;i<table->max_hosts;i++) {
      if(nb<table->nb_hosts && table->hosts_table[i]) {
         if(table->hosts_table[i]->l_addr_64_l==addr64_l && table->hosts_table[i]->l_addr_64_h==addr64_h) {
            return table->hosts_table[i];
         }
         nb++;
      }
      else
         break;
   }
   
   return NULL;
}


xbee_host_t *_xbee_host_find_host_by_name(xbee_hosts_table_t *table, char *name)
/**
 * \brief     recherche dans la table des hosts xbee un xbee avec nom precise et le retourne. 
 * \details   retourne un pointeur sur l'element de la table repondant au critere ou NULL si non trouve. C'est une fonction a usage interne.
 * \param     table  pointeur sur la table host
 * \param     name non (NI) de l'xbee recherche
 * \return    pointeur sur l'entree de la table contenant le host recherche ou NULL
 */
{
   uint16_t nb=0;
   
   if(!table)
      return NULL;

   for(uint16_t i=0;i<table->max_hosts;i++) {
      if(nb<table->nb_hosts && table->hosts_table[i]) {
         if(strcmp(table->hosts_table[i]->name,name)==0) {
            return table->hosts_table[i];
         }
         nb++;
      }
      else
         break;
   }
   
   return NULL;
}


void _xbee_host_set_addr_16(xbee_host_t *xbee_host, char *addr_16)
/**
 * \brief     ajoute (ou modifie) l'adresse 16 bits d'un xbee
 * \details   
 * \param     xbee_host  
 * \param     addr_16 adresse 16 bits de l'xbee
 */
{
   xbee_host->addr_16[0]=addr_16[0];
   xbee_host->addr_16[1]=addr_16[1];
   
   xbee_host->l_addr_16=xbee_host->addr_16[0]*256+xbee_host->addr_16[1];
}


void _xbee_host_set_name(xbee_host_t *xbee_host, unsigned char *name, uint16_t l)
/**
 * \brief     ajoute (ou modifie) le nom (NI) d'un xbee
 * \details   
 * \param     xbee_host
 * \param     name nom de l'xbee
 * \param     l longueur de la chaine de caractere name
 */
{
   memcpy(xbee_host->name,name,l);
   xbee_host->name[l]=0;
}


void _xbee_host_init_addr_64(xbee_host_t *xbee_host, uint32_t int_addr_64_h, uint32_t int_addr_64_l)
/**
 * \brief     initialisation d'un host xbee
 * \details   l'adresse 64 bits est positionnée à la valeur demandée, name="" et l'adresse 16 bits est possitionnée à 0xFFFE.
 * \param     xbee_host  doit être alloué par le demandeur
 * \param     addr_64_h  mot haut de l'adresse xbee 64 bits
 * \param     addr_64_l  mot bas de l'adresse xbee 64 bits
 */
{
   xbee_host->name[0]=0;
   
   xbee_host->addr_16[0]=0xFF;
   xbee_host->addr_16[1]=0xFE;
   
   xbee_host->l_addr_64_h=int_addr_64_h;
   xbee_host->l_addr_64_l=int_addr_64_l;
   
   xbee_host->l_addr_16=0xFFFE;
   
   xbee_host->addr_64_h[3]=int_addr_64_h & 0xFF;
   int_addr_64_h=int_addr_64_h>>8;
   xbee_host->addr_64_h[2]=int_addr_64_h & 0xFF;
   int_addr_64_h=int_addr_64_h>>8;
   xbee_host->addr_64_h[1]=int_addr_64_h & 0xFF;
   int_addr_64_h=int_addr_64_h>>8;
   xbee_host->addr_64_h[0]=int_addr_64_h & 0xFF;
   
   xbee_host->addr_64_l[3]=int_addr_64_l & 0xFF;
   int_addr_64_l=int_addr_64_l>>8;
   xbee_host->addr_64_l[2]=int_addr_64_l & 0xFF;
   int_addr_64_l=int_addr_64_l>>8;
   xbee_host->addr_64_l[1]=int_addr_64_l & 0xFF;
   int_addr_64_l=int_addr_64_l>>8;
   xbee_host->addr_64_l[0]=int_addr_64_l & 0xFF;
}


mea_error_t xbee_get_host_by_name(xbee_xd_t *xd, xbee_host_t *host, char *name, int16_t *nerr)
/**
 * \brief     Recherche dans la table des xbee connus l'xbee qui porte le nom demandé. 
 * \details   retourne une structure contenant la rescription d'un "host" xbee (les adresses, le nom, ...). Le nom n'est recherche que dans la table (pas d'interrogation du réseau si non trouvé)
 * \param     xd[in]     descripteur xbee.
 * \param     host  la description du host. Doit être alloué par le demandeur.
 * \param     name  le nom recherche.
 * \param     nerr  XBEE_NOERR (pas d'erreur) ou XBEE_HOSTNOTFOUND (pas dans la table)
 * \return    -1 si non trouvé dans la table host ou 0 sinon
 */
{
   xbee_host_t *h;
   
   h=_xbee_host_find_host_by_name(xd->hosts, name);
   if(h)
      memcpy(host,h,sizeof(xbee_host_t));
   else {
      DEBUG_SECTION mea_log_printf("%s (%s) : %s not found in xbee hosts table.\n",DEBUG_STR,__func__,name);
      
      *nerr=XBEE_ERR_HOSTNOTFUND;
      return ERROR;
   }
   *nerr=XBEE_ERR_NOERR;
   return NOERROR;
}


mea_error_t xbee_get_host_by_addr_64(xbee_xd_t *xd, xbee_host_t *host, uint32_t addr_64_h, uint32_t addr_64_l, int16_t *nerr)
/**
 * \brief     Recherche un xbee dans la table des xbee connus par son adresse 64 bits. 
 * \details   L'adresse 64 bits est passées à l'aide de de mot de 32 bits (h et l). Si l'adresse n'éxite pas le hosts est construit à partir de l'adresse fournie, les autres élements prennent une valeur par défaut.
 * \param     xd    descripteur xbee.
 * \param     host  la description du host trouvé ou construit. Doit être alloué par le demandeur.
 * \param     addr_64_h  mot haut de l'adresse xbee 64 bits
 * \param     addr_64_l  mot bas de l'adresse xbee 64 bits
 * \param     nerr    XBEE_NOERR (pas d'erreur) ou XBEE_HOSTNOTFOUND (pas dans la table)
 * \return    -1 si non trouvé dans la table host ou 0 sinon
 */
{
   xbee_host_t *h;
   
   h=_xbee_host_find_host_by_addr64(xd->hosts, addr_64_h, addr_64_l);
   if(h)
      memcpy(host,h,sizeof(xbee_host_t));
   else {
      _xbee_host_init_addr_64(host, addr_64_h, addr_64_l);
      
      DEBUG_SECTION mea_log_printf("%s (%s) : %08lx-%08lx not found in xbee hosts table.\n", DEBUG_STR,__func__,(unsigned long int)addr_64_h,(unsigned long int)addr_64_l);
      *nerr=XBEE_ERR_HOSTNOTFUND;
      return ERROR;
   }
   *nerr=XBEE_ERR_NOERR;
   return NOERROR;
}


xbee_hosts_table_t *_hosts_table_create(int max_nb_hosts, int16_t *nerr)
/**
 * \brief     Création d'une nouvelle table "hosts" xbee.
 * \details   Allocation de la table et initialisation des données de la table à vide.
 * \param     max_nb_hosts   taille (en nombre d'entrées) de la table
 * \param     nerr           erreur en cas d'échec de création
 * \return    pointeur sur une nouvelle table ou NULL en cas d'erreur
 */
{
   xbee_hosts_table_t *t;
   
   t=malloc(sizeof(xbee_hosts_table_t));
   if(!t) {
      *nerr=XBEE_ERR_SYS;
      return 0;
   }
   
   t->hosts_table=(xbee_host_t **)malloc(sizeof(xbee_host_t *)*max_nb_hosts);
   if(!t->hosts_table) {
      free(t);
      t=NULL;
      *nerr=XBEE_ERR_SYS;
      return 0;
   }
   
   memset(t->hosts_table,0,sizeof(xbee_host_t *)*max_nb_hosts);
   
   t->nb_hosts=0;
   t->max_hosts=max_nb_hosts;
   
   *nerr=XBEE_ERR_NOERR;
   
   return t;
}


void _hosts_table_delete(xbee_hosts_table_t *table)
/**
 * \brief     detruit une table "hosts" xbee.
 * \details   desalocation complete d'une table host.
 * \param     table table à détruire
 */
{
   uint16_t nb=0;

   for(uint16_t i=0;i<table->max_hosts;i++) {
      if(nb<table->nb_hosts && table->hosts_table[i]) {
         free(table->hosts_table[i]);
         table->hosts_table[i]=NULL;
      }
      else
         break;
      nb++;
   }
   
   free(table->hosts_table);
   table->hosts_table=NULL;
   
   free(table);
   table=NULL;
}


mea_error_t hosts_table_display(xbee_hosts_table_t *table)
/**
 * \brief     affiche le contenu d'une table hosts xbee.
 * \param     table la table à afficher
 * \return    toujours 0
 */
{
   DEBUG_SECTION {
      uint16_t nb=0;
      
      if(!table)
         return ERROR;

      for(uint16_t i=0;i<table->max_hosts;i++) {
         if(nb<table->nb_hosts && table->hosts_table[i]) {
            printf("%s (%s) : [%02d] %s %lx-%lx %lx\n",DEBUG_STR,__func__,i,
                   table->hosts_table[i]->name,
                   (long unsigned int)table->hosts_table[i]->l_addr_64_h,
                   (long unsigned int)table->hosts_table[i]->l_addr_64_l,
                   (long unsigned int)table->hosts_table[i]->l_addr_16);
            nb++;
         }
         else
            break;
      }
   }
   return NOERROR;
}


xbee_host_t *_xbee_add_new_to_hosts_table_with_addr64(xbee_hosts_table_t *table, uint32_t addr_64_h, uint32_t addr_64_l, int16_t *nerr)
/**
 * \brief     ajoute un nouvel xbee dans une table host.
 * \details   Le nouveau host est créé par la fonction à partir uniquement de son adresse 64bits. Les autres éléments prennent des valeurs par defaut, l'adresse 16 bits est notamment positionnée à 0XFFFE (voir _xbee_host_init_addr_64).
 * \param     addr_64_h  mot haut de l'adresse xbee 64 bits
 * \param     addr_64_l  mot bas de l'adresse xbee 64 bits
 * \param     nerr    XBEE_NOERR (pas d'erreur) ou XBEE_ERR_HOSTTABLEFULL (plus de place dans la table)
 * \return    pointeur sur une nouvelle table ou NULL en cas d'erreur
 */
{
   xbee_host_t *host;
   
   if(!table) {
      host=NULL;
      *nerr=XBEE_ERR_SYS; // à remplacer par une autre erreur
      return 0;
   }
   
   host=malloc(sizeof(xbee_host_t));
   if(!host) {
      *nerr=XBEE_ERR_SYS;
      return 0;
   }
   memset(host,0,sizeof(xbee_host_t));
   
   _xbee_host_init_addr_64(host, addr_64_h, addr_64_l);
   
   for(uint16_t i=0;i<table->max_hosts;i++) {
      if(table->hosts_table[i]==0) {
         table->hosts_table[i]=host;
         table->nb_hosts++;
         return host;
      }
   }

_xbee_add_new_to_hosts_table_with_addr64_error:
   free(host);
   host=NULL;
   *nerr=XBEE_ERR_HOSTTABLEFULL;
   return 0; // plus de place dans la table
}


mea_error_t _xbee_update_hosts_tables(xbee_hosts_table_t *table, char *addr_64_h, char *addr_64_l,  char *addr_16, char *name)
/**
 * \brief     ajoute ou met à jour la table host avec les éléments transmis en paramètre.
 * \details   recherche dans la table une entrée avec l'adresse 64bits spécifiées et modifie les éléments en concéquence si l'entrée existe. Si l'entrée n'existe pas elle est créée.
 * \param     addr_64_h  mot haut de l'adresse xbee 64 bits
 * \param     addr_64_l  mot bas de l'adresse xbee 64 bits
 * \param     addr_16  adresse 16 bits de l'xbee
 * \param     name  nom (NI) de l'xbee
 * \return    0 si la mise à jour a été effectuée, -1 si un nouvel xbee n'a pas pu être créée.
 */
{
   uint16_t l_addr_16;
   uint32_t l_addr_64_h,l_addr_64_l;
   int16_t nerr;
   xbee_host_t *host;
   
   if(!table)
      return ERROR;
   
   l_addr_16=addr_16[0]*256+addr_16[1];
   
   l_addr_64_l=0;
   l_addr_64_h=0;
   for(int i=0;i<4;i++)
      l_addr_64_h=l_addr_64_h+((unsigned char)addr_64_h[i] << (3-i)*8);
   
   for(int i=0;i<4;i++)
      l_addr_64_l=l_addr_64_l+((unsigned char)addr_64_l[i] << (3-i)*8);
   
   host=_xbee_host_find_host_by_addr64(table, l_addr_64_h, l_addr_64_l);
   if(!host) {
      host=_xbee_add_new_to_hosts_table_with_addr64(table, l_addr_64_h, l_addr_64_l, &nerr);
      if(!host)
         return ERROR;
   }
   
   host->l_addr_16=l_addr_16;
   
   if(name) {
      strncpy(host->name,name,sizeof(host->name)-1);
      host->name[sizeof(host->name)-1]=0;
   }
   
   return NOERROR;
}


int16_t xbee_start_network_discovery(xbee_xd_t *xd, int16_t *nerr)
/**
 * \brief     Lance une découverte du réseau xbee.
 * \details   Envoie la commande ATND en broadcast. Les résultats seront trappés par le thread xbee et la table hosts sera construite ou mise à jour.
 * \param     xd         descripteur de communication xbee
 * \param     nerr       numero d'erreur
 * \return    0 = OK, -1 = KO, voir nerr pour le type d'erreur.
 */
{
   uint16_t  l_cmd;
   unsigned char cmd[16];
   
   l_cmd=_xbee_build_at_cmd(cmd, XBEE_ND_FRAME_ID, (unsigned char *)"ND", 2);
   return _xbee_write_cmd(xd->fd, cmd, l_cmd, nerr);
}


int16_t xbee_atCmdSend(xbee_xd_t *xd,
                       xbee_host_t *destination,
                       unsigned char *frame_data, // zone donnee d'une trame
                       uint16_t l_frame_data, // longueur zone donnee
                       int16_t *xbee_err)
/**
 * \brief     Transmet une commande (AT) à un xbee sans attendre de réponse.
 * \details   Si destination == NULL, la commande est transmise à l'xbee local, sinon elle est transmise à un xbee distant
 * \param     xd           descripteur de communication xbee
 * \param     destination  description de l'xbee destination
 * \param     frame_data   commande AT et le paramettre associé en transmettre à l'xbee
 * \param     l_frame_data nombre d'octet à transmettre à l'xbee
 * \param     xbee_err     numero d'erreur
 * \return    -1 en cas d'erreur, 0 ..., 1 ...
 */
{
   unsigned char xbee_frame[XBEE_MAX_FRAME_SIZE];
   uint16_t l_xbee_frame;
   int16_t nerr;
   
   if(xd->signal_flag<0) {
      if(xbee_err) {
         *xbee_err=XBEE_ERR_DOWN;
         return -1;
      }
   }
   
   if(pthread_self()==xd->read_thread) { // risque de dead lock si appeler par un call back => on interdit
      if(xbee_err)
      {
         *xbee_err=XBEE_ERR_IN_CALLBACK;
         return -1;
      }
   }
   
   // construction de la trame xbee a partir de la la destination, la zone data transmise et d'un identifiant de trame "unique"
   unsigned int frame_data_id=0; // 0 = pas de réponse attendues
   if(destination)
      l_xbee_frame=_xbee_build_at_remote_cmd(xbee_frame, frame_data_id, destination, frame_data, l_frame_data);
   else
      l_xbee_frame=_xbee_build_at_cmd(xbee_frame, frame_data_id, frame_data, l_frame_data);
   
   if(_xbee_write_cmd(xd->fd, xbee_frame, l_xbee_frame, &nerr)==0) { // envoie de l'ordre
      return 0;
   }

   if(xbee_err)
      *xbee_err=nerr;
   return 1;
}

int16_t xbee_atCmdSendAndWaitResp(xbee_xd_t *xd,
                                  xbee_host_t *destination,
                                  unsigned char *frame_data, // zone donnee d'une trame
                                  uint16_t l_frame_data, // longueur zone donnee
                                  unsigned char *resp,
                                  uint16_t *l_resp,
                                  int16_t *xbee_err)
/**
 * \brief     Transmet une commande AT vers un xbee et lit la réponse
 * \details   Transmet une question de type AT à l'xbee dont l'adresse est precisée par "destination". Cette fonction est compatible multi-tread grâce à l'utilisation d'une file, d'un identifiant de trame et d'un timestamp. La demande est immediatement appliquée. On attends toujours une réponse. Si la réponse n'arrive pas dans les temps, la fonction retourne -1. Elle retourne 0 en cas de succès et met a jour resp, l_resp et xbee_err avec les données de la réponse.
 * \param     xd           descripteur de communication xbee
 * \param     destination  adresse de l'xbee à commander. Si destination == NULL la commande est destinée à l'xbee local. destination est de type xbee_host_t et doit au moins disposer de l'adresse 64bits correctement positionnée.
 * \param     frame_data   ensemble de la commande AT à transmettre.
 * \param     l_frame_data longueur de la commande.
 * \param     resp         ensemble de la réponse de l'xbee.
 * \param     l_resp       longueur de la réponse.
 * \param     xbee_err     numero d'erreur
 * \return    0 = OK, -1 = KO, voir nerr pour le type d'erreur.
 */
{
   unsigned char xbee_frame[XBEE_MAX_FRAME_SIZE];
   uint16_t l_xbee_frame;
   xbee_queue_elem_t *e;
   int16_t nerr;
   int16_t return_val=1;

   if(xd->signal_flag<0) {
      if(xbee_err) {
         *xbee_err=XBEE_ERR_DOWN;
      }
      return -1;
   }

   if(pthread_self()==xd->read_thread) { // risque de dead lock si appeler par un call back => on interdit
      if(xbee_err) {
         *xbee_err=XBEE_ERR_IN_CALLBACK;
      }
      return -1;
   }
   
   // construction de la trame xbee a partir de la la destination, la zone data transmise et d'un identifiant de trame "unique"
   unsigned int frame_data_id=_xbee_get_frame_data_id(xd);
   if(destination)
      l_xbee_frame=_xbee_build_at_remote_cmd(xbee_frame, frame_data_id, destination, frame_data, l_frame_data);
   else
      l_xbee_frame=_xbee_build_at_cmd(xbee_frame, frame_data_id, frame_data, l_frame_data);

   if(_xbee_write_cmd(xd->fd, xbee_frame, l_xbee_frame, &nerr)==0) {  // envoie de l'ordre
      int16_t ret;
      int16_t boucle=XBEE_NB_RETRY; // 5 tentatives de 1 secondes
      uint16_t notfound=0;
      do {
         // on va attendre le retour dans la file des reponses
         pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)&(xd->sync_lock) );
         pthread_mutex_lock(&(xd->sync_lock));
         if(xd->queue->nb_elem==0 || notfound==1) {
            // rien a lire => on va attendre que quelque chose soit mis dans la file
            struct timeval tv;
            struct timespec ts;
            gettimeofday(&tv, NULL);
            ts.tv_sec = tv.tv_sec + XBEE_TIMEOUT_DELAY;
            ts.tv_nsec = 0;
            
            ret=pthread_cond_timedwait(&xd->sync_cond, &xd->sync_lock, &ts);
            if(ret) {
               if(ret!=ETIMEDOUT) {
                  if(xbee_err) {
                     *xbee_err=XBEE_ERR_SYS;
                  }
                  return_val=-1;
                  goto next_or_return;
               }
            }
         }
         
         // a ce point il devrait y avoir quelque chose dans la file ou TIMEOUT.
         uint32_t tsp=_xbee_get_timestamp();
         
         if(mea_queue_first(xd->queue)==0) { // parcours de la liste jusqu'a trouver une reponse pour nous
            do {
               if(mea_queue_current(xd->queue, (void **)&e)==0) {
                  if((uint16_t)(e->cmd[1] & 0xFF)==frame_data_id) {  // le deuxieme octet d'une reponse doit contenir le meme id que celui de la question
                     if(e->xbee_err!=XBEE_ERR_NOERR) { // la reponse est une erreur
                        if(xbee_err) {
                           *xbee_err=e->xbee_err; // on la retourne directement
                        }  
                        // et on fait le menage avant de sortir
                        mea_queue_remove_current(xd->queue);
                        _xbee_free_queue_elem(e);
                        e=NULL;

                        return_val=-1;
                        goto next_or_return;
                     }
                     
                     if((tsp - e->tsp)<=10) { // la reponse est pour nous et dans les temps
                        // recuperation des donnees
                        memcpy(resp,e->cmd,e->l_cmd);
                        *l_resp=e->l_cmd;
                        if(xbee_err) {
                           *xbee_err=e->xbee_err;
                        } 
                        // et on fait le menage avant de sortir
                        _xbee_free_queue_elem(e);
                        xd->queue->current->d=NULL; // pour evite le bug
                        mea_queue_remove_current(xd->queue);
                        e=NULL;
                        
                        return_val=0;
                        goto next_or_return;
                     }
                     
                     // theoriquement pour nous mais donnees trop vieilles, on supprime
                     DEBUG_SECTION mea_log_printf("%s (%s) : data have been found but are too old\n", DEBUG_STR,__func__);
//                     mea_queue_remove_current(xd->queue);
//                     _xbee_free_queue_elem(e);
//                     e=NULL;
                  }
                  else {
                     DEBUG_SECTION mea_log_printf("%s (%s) : data aren't for me (%d != %d)\n", DEBUG_STR,__func__, e->cmd[1], frame_data_id);
                     e=NULL;
                  }
               }
            }
            while(mea_queue_next(xd->queue)==0);
            notfound=1;
         }
next_or_return:
         pthread_mutex_unlock(&(xd->sync_lock));
         pthread_cleanup_pop(0);
         
         if(return_val ==0 || return_val==-1)
            return return_val;
      }
      while (--boucle);
   }
   else {
      if(xbee_err) {
         *xbee_err=nerr;
      }
   }
error_exit:
   return -1;
}


void xbee_clean_xd(xbee_xd_t *xd)
/**
 * \brief     Vide les "conteneurs" (queue et hosts) d'un descipteur xd
 * \details   Appeler cette fonction avant de libérer la mémoire allouée pour un descipteur de communication xbee afin de vider proprement (pas de memory leak).
 * \param     xd           descripteur de communication xbeeio
 */
{
   if(xd) {
      if(xd->queue) {
         mea_queue_cleanup(xd->queue,_xbee_free_queue_elem);
         free(xd->queue);
         xd->queue=NULL;
      }
      if(xd->hosts) {
         _hosts_table_delete(xd->hosts);
         xd->hosts=NULL;
      }
   }
}


void xbee_close(xbee_xd_t *xd)
{
/**
* \brief     fermeture d'une communication avec un xbee
* \details   arrête le thead de gestion de la communication, ménage dans xd et fermeture du "fichier".
* \param     xd           descripteur de communication xbeeio
*/
   pthread_cancel(xd->read_thread);
   pthread_join(xd->read_thread, NULL);
   
   pthread_cond_destroy(&xd->sync_cond);
   pthread_mutex_destroy(&xd->sync_lock);
   pthread_mutex_destroy(&xd->xd_lock);

   xbee_clean_xd(xd);
   close(xd->fd);
}


void _xbee_display_frame(int ret, unsigned char *resp, uint16_t l_resp)
{
   DEBUG_SECTION {
      if(!ret) {
         for(int i=0;i<l_resp;i++)
            fprintf(stderr, "%02x-[%c] ", resp[i], resp[i]);
         printf("\n");
      }
   }
}


void _xbee_free_queue_elem(void *d) // pour vider_file2
{
   xbee_queue_elem_t *e=(xbee_queue_elem_t *)d;
   
   free(e);
   e=NULL;
}


int _xbee_build_frame(unsigned char *frame, unsigned char *cmd, uint16_t l_cmd)
{
   uint16_t i=0;
   uint16_t checksum=0;
   
   frame[i++]=0x7E;
   frame[i++]=l_cmd/256;
   frame[i++]=l_cmd%256;
   for(uint16_t j=0;j<l_cmd;j++)
      frame[i++]=cmd[j];
   
   for(uint16_t j=3;j<(3+l_cmd);j++)
      checksum=checksum+frame[j];
   
   frame[i++]=0xFF - (checksum & 0xFF);
   
   return i;
}


int _xbee_read_cmd(int fd, char unsigned *frame, uint16_t *l_frame, int16_t *nerr)
{
   unsigned char c;
   fd_set input_set;
   struct timeval timeout;
   
   int ret=0;
   uint16_t step=0;
   int ntry=0;
   
   uint16_t i=0;
   int checksum=0;
   
   FD_ZERO(&input_set);
   FD_SET(fd, &input_set);
   
   
   *nerr=0;
   
   while(1) {
      timeout.tv_sec  = 1; // timeout après 1 secondes
      timeout.tv_usec = 0;

      ret = select(fd+1, &input_set, NULL, NULL, &timeout);
      if (ret <= 0) {
         if(ret == 0)
            *nerr=XBEE_ERR_TIMEOUT;
         else
            *nerr=XBEE_ERR_SELECT;
         goto on_error_exit_xbee_read;
      }
      
      ret=(int)read(fd,&c,1);
      if(ret!=1) {
         if(ntry>(XBEE_NB_RETRY-1)) { // 5 essais si pas de caratères lus
            *nerr=XBEE_ERR_READ;
            goto on_error_exit_xbee_read;
         }
         ntry++;
         continue; // attention, si aucun caractère lu on boucle
      }
      ntry=0;
      
      switch(step)
      {
         case 0:
            if(c==0x7E) {
               step++;
               break;
            }
            break;
            *nerr=XBEE_ERR_STARTTRAME;
            goto on_error_exit_xbee_read;
         case 1:
            *l_frame=c;
            step++;
            break;
         case 2:
            *l_frame=*l_frame*256+c;
            i=0;
            step++;
            break;
         case 3:
            frame[i]=c;
            i++; // maj du reste à lire
            if(i>=*l_frame)
               step=10; // read checksum
            break;
         case 10:
            for(i=0;i<(*l_frame);i++) {
               checksum+=frame[i];
            }
            if(((checksum+c) & 0xFF) == 0xFF) {
                /* DEBUG_SECTION {
                mea_log_printf("%s (%s) : new frame recept\n", DEBUG_STR,__func__);
                 _xbee_display_frame(0, frame, *l_frame);
               } */
               return 0;
            }
            else {
               VERBOSE(9) mea_log_printf("%s  (%s) : xbee return an error - checksum error.\n",INFO_STR,__func__);
               *nerr=XBEE_ERR_CHECKSUM;
               return -1;
            }
      }
   }
   *nerr=XBEE_ERR_UNKNOWN; // ne devrait jamais se produire ...
   
on_error_exit_xbee_read:
   return -1;
}


int _xbee_write_cmd(int fd, unsigned char *cmd, uint16_t l_cmd, int16_t *nerr)
{
   uint16_t l_frame=0;
   unsigned char *frame=NULL;
   int ret;
   
   *nerr=0;
   
   frame=malloc(l_cmd+4);
   if(!frame)
   {
      *nerr=XBEE_ERR_SYS;
      return -1;
   }
   
   l_frame=_xbee_build_frame(frame,cmd,l_cmd);
   
   ret=(int)write(fd,frame,l_frame);

   free(frame);
   frame=NULL;
   
   if(ret<0)
   {
      *nerr=XBEE_ERR_SYS;
      return -1;
   }
   return 0;
}


int _xbee_network_discovery_resp(xbee_xd_t *xd, char *data, uint16_t l_data)
{
   
   struct xbee_map_nd_resp_data *map;
   
   if(l_data==0)
      return 0; // fin de transmission OK, on traite pas
   
   map=(struct xbee_map_nd_resp_data *)data; // -1 pour corriger l'alignement			
   
   DEBUG_SECTION {
      mea_log_printf("%s (%s) : MY = %x\n",DEBUG_STR,__func__,map->MY[0]*256+map->MY[1]);
      mea_log_printf("%s (%s) : NI = %s\n",DEBUG_STR,__func__,(map->NI)-1); // décallage de 1 a cause de l'alignement
   }
   
   _xbee_update_hosts_tables(xd->hosts, (char *)map->SH, (char *)map->SL, (char *)map->MY, (map->NI)-1);
      
   return 0;
}


void _xbee_flush_old_responses_queue(xbee_xd_t *xd)
{
   xbee_queue_elem_t *e;
   uint32_t tsp=_xbee_get_timestamp();
   
   pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)&(xd->sync_lock) );
   pthread_mutex_lock(&xd->sync_lock);
   
   if(mea_queue_first(xd->queue)==0) {
      while(1) {
         if(mea_queue_current(xd->queue, (void **)&e)==0) {
            if((tsp - e->tsp) > 10) {
               _xbee_free_queue_elem(e);
               mea_queue_remove_current(xd->queue); // remove current passe sur le suivant
            }
            else
               mea_queue_next(xd->queue);
         }
         else
            break;
      }
   }
   
   pthread_mutex_unlock(&xd->sync_lock);
   pthread_cleanup_pop(0);
}


int16_t _xbee_add_response_to_queue(xbee_xd_t *xd, unsigned char *cmd, uint16_t l_cmd)
{
   xbee_queue_elem_t *e;
   
   if(!xd)
      return -1;

   e=malloc(sizeof(xbee_queue_elem_t));
   if(e) {
      memcpy(e->cmd,cmd,l_cmd);
      e->l_cmd=l_cmd;
      e->xbee_err=XBEE_ERR_NOERR;
      e->tsp=_xbee_get_timestamp();
      
      pthread_cleanup_push( (void *)pthread_mutex_unlock, (void *)&(xd->sync_lock) );
      pthread_mutex_lock(&xd->sync_lock);

      mea_queue_in_elem(xd->queue, e);
      
      if(xd->queue->nb_elem>=1)
         pthread_cond_broadcast(&xd->sync_cond);
      
      pthread_mutex_unlock(&xd->sync_lock);
      pthread_cleanup_pop(0);
   }
   
   return 0;
}


int _xbee_reopen(xbee_xd_t *xd)
{
   int fd; /* File descriptor for the port */
   uint8_t flag=0;
   char dev[255];
   int speed=0;
   
   if(!xd)
      return -1;

   strncpy(dev, xd->serial_dev_name, sizeof(dev));
   speed=xd->speed;
   
   VERBOSE(2) mea_log_printf("%s  (%s) : try to reset communication (%s).\n",INFO_STR,__func__,dev);
   
   close(xd->fd);
   
   for(int i=0;i<XBEE_NB_RETRY;i++) {  // 5 tentatives pour rétablir les communications
      fd = _xbee_open(xd, dev, speed);
      if (fd == -1) {
         VERBOSE(2) {
            char err_str[256];
            strerror_r(errno, err_str, sizeof(err_str)-1);
            mea_log_printf("%s (%s) : try #%d/%d, unable to open serial port (%s) - %s\n",ERROR_STR,__func__, i+1, XBEE_NB_RETRY, dev,err_str);
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
   
   VERBOSE(5) mea_log_printf("%s  (%s) : communication reset successful.\n",INFO_STR,__func__);

   return 0;
}	


uint32_t _xbee_get_timestamp()
/**
 * \brief     retourne un "timestamp" qui pourra être utilisé pour horodaté les trames reçues
 * \details   c'est actuellement juste un wrapping de la fonction time.
 */
{
    return (uint32_t)time(NULL); 
}


void *_xbee_thread(void *args)
/**
 * \brief     thread assurant la lecture des données en provenance d'un xbee
 * \details   Le thread attends l'arrivée de données sur le port serie/usb, décode les informations et dispatch les données en fonction de la nature des trames reçues.
 * \param     args   pointeur sur la structure contenant les informations nécessaires au fonctionnement du thread. Args est ici simplement un pointeur sur une structure du type xbee_xd_t
 */
{
   unsigned char cmd[255];
   uint16_t l_cmd;
   
   uint8_t status=0;
   int16_t nerr;
   int16_t ret;
   
   xbee_xd_t *xd=(xbee_xd_t *)args;
   
   VERBOSE(5) mea_log_printf("%s  (%s) : starting \"xbee reading thread\" on %s\n",INFO_STR,__func__,xd->serial_dev_name);
   while(1) {
      _xbee_flush_old_responses_queue(xd);
      
      ret=_xbee_read_cmd(xd->fd, cmd, &l_cmd, &nerr);
      if(ret==0) {
         switch((uint8_t)cmd[0]) {
            case 0x88: // response for local AT (0x08)
            {
               struct xbee_cmd_response_s *mcmd=(struct xbee_cmd_response_s *)cmd;
               
               if(mcmd->frame_id>XBEE_MAX_USER_FRAME_ID) { // la requete est interne elle ne sera pas retransmise{ // la requete est interne elle ne sera pas retransmise
                  switch (mcmd->frame_id) {
                     case XBEE_ND_FRAME_ID:
                        if(status==0)
                           _xbee_network_discovery_resp(xd, (char *)mcmd->at_cmd_data,l_cmd-5);
                        break;
                     default:
                        break;
                  }
               }
               else {
                  _xbee_add_response_to_queue(xd, cmd, l_cmd);
               }
               break;
            }
               
            case 0x97:
            { // response for remote AT (0x97)
               struct xbee_remote_cmd_response_s *mcmd=(struct xbee_remote_cmd_response_s *)cmd;
               
               if(mcmd->frame_id>XBEE_MAX_USER_FRAME_ID) { // la requete est interne elle ne sera pas retransmise{ // la requete est interne elle ne sera pas retransmise
                  switch (mcmd->frame_id) {  // pas encore de commande interne
                     default:
                        break;
                  }
               }
               else {
                  _xbee_add_response_to_queue(xd, cmd, l_cmd);
               }
               // on va quand meme récupérer les info addr64 et addr16 pour mettre a jour
               // la table des équipements connus
               ret=_xbee_update_hosts_tables(xd->hosts,(char *)mcmd->addr_64_h,(char *)mcmd->addr_64_l, (char *)mcmd->addr_16,NULL);
               break;
            }
               
            case 0x92: // Trame IO
            {
               struct xbee_map_io_data_s *mcmd=(struct xbee_map_io_data_s *)cmd;
               
               if(xd->io_callback)
                  xd->io_callback(0,cmd,l_cmd, xd->io_callback_data, (char *)mcmd->addr_64_h, (char *)mcmd->addr_64_l);
               
               ret=_xbee_update_hosts_tables(xd->hosts,(char *)mcmd->addr_64_h,(char *)mcmd->addr_64_l,(char *)mcmd->addr_16,NULL);
               break;
            }
               
            case 0x95:
            {
               struct xbee_node_identification_response_s *mcmd=(struct xbee_node_identification_response_s *)cmd;
               
               if(xd->commissionning_callback)
                  xd->commissionning_callback(0,cmd,l_cmd, xd->commissionning_callback_data, (char *)mcmd->remote_addr_64_h, (char *)mcmd->remote_addr_64_l);
               
               ret=_xbee_update_hosts_tables(xd->hosts,(char *)mcmd->remote_addr_64_h, (char *)mcmd->remote_addr_64_l, (char *)mcmd->remote_addr_16,(char *)(mcmd->nd_data));
               break;
            }
               
            case 0x90: // données "séries" en provenance d'un xbee
            {
               struct xbee_receive_packet_s *mcmd=(struct xbee_receive_packet_s *)cmd;
               
//               DEBUG_SECTION mea_log_printf("_xbee_thread : frame recept : cmd=%s, size=%d\n",cmd,l_cmd);

               if(xd->dataflow_callback)
                     xd->dataflow_callback(0,cmd,l_cmd, xd->dataflow_callback_data, (char *)mcmd->addr_64_h, (char *)mcmd->addr_64_l);
                  
               ret=_xbee_update_hosts_tables(xd->hosts,(char *)mcmd->addr_64_h,(char *)mcmd->addr_64_l,(char *)mcmd->addr_16,NULL);
               break;
            }
               
            default:
               VERBOSE(9) mea_log_printf("%s  (%s): Recept unknown frame (%d), skipped\n",INFO_STR,__func__,cmd[0]);
               break;
         }
      }
      if(ret<0) {
         switch(nerr)
         {
            case XBEE_ERR_TIMEOUT:
               break;
            case XBEE_ERR_SELECT:
            case XBEE_ERR_READ:
            case XBEE_ERR_SYS:
               VERBOSE(1) {
                  char err_str[256];
                  strerror_r(errno, err_str, sizeof(err_str)-1);
                  mea_log_printf("%s (%s) : communication error (nerr=%d) - %s\n", ERROR_STR,__func__,nerr,err_str);
               }
               if(_xbee_reopen(xd)<0) {
                  VERBOSE(1) {
                     mea_log_printf("%s (%s) : xbee thread is down\n", ERROR_STR,__func__);
                  }
                  xd->signal_flag=1;
                  pthread_exit(NULL);
               }
               xd->signal_flag=0;
               break;
            case XBEE_ERR_HOSTTABLEFULL:
               VERBOSE(1) {
                  mea_log_printf("%s (%s) : hosts table is full (nerr=%d).\n", ERROR_STR,__func__,nerr);
               }
               break;
            default:
               VERBOSE(1) {
                  mea_log_printf("%s (%s) : uncategorized error (%d).\n", ERROR_STR,__func__,nerr);
               }
               break;
         }
      }
      pthread_testcancel();
   }
   
//   pthread_exit(NULL);
}

