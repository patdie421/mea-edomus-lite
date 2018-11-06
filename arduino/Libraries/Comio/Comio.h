#ifndef __COMIO_H__
#define __COMIO_H__

#include <Arduino.h>

// Taille des mémoires  en fonction du type d'arduino (type de microcontroleur)
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1284P__)
// un MEGA
  #define COMIO_MAX_N         54 // nombre max d' E/S logiques
  #define COMIO_MAX_A         16 // nombre max d' Entrées analogiques
  #define COMIO_MAX_M         64 // nombre d'octet réservé pour le partage avec le PC
#elif defined(__AVR_ATmega32u4__)
// un Leonardo ? trouver la bonne constante ...
  #define COMIO_MAX_N         20
  #define COMIO_MAX_A          6
  #define COMIO_MAX_M         32
#else
// un UNO ou équivalant (tous les autres en fait ...)
  #define COMIO_MAX_N         14
  #define COMIO_MAX_A          6
  #define COMIO_MAX_M         32 
#endif

#define COMIO_MAX_FX           8
// code opération
#define OP_ECRITURE            0
#define OP_LECTURE             1
#define OP_FONCTION            2

// type de variable accessible
#define TYPE_NUMERIQUE         0
#define TYPE_ANALOGIQUE        1
#define TYPE_NUMERIQUE_PWM     2
#define TYPE_MEMOIRE           3
#define TYPE_FONCTION          4

// codes erreurs
#define COMIO_ERR_NOERR        0
#define COMIO_ERR_ENDTRAME     1
#define COMIO_ERR_STARTTRAME   2
#define COMIO_ERR_TIMEOUT      3
#define COMIO_ERR_SELECT       4
#define COMIO_ERR_READ         5
#define COMIO_ERR_DISCORDANCE  6
#define COMIO_ERR_SYS          7
#define COMIO_ERR_UNKNOWN     99

typedef int (*callback_f)(int);

//extern unsigned char comio_index_N[COMIO_MAX_N];
//extern unsigned char comio_index_A[COMIO_MAX_A];
extern unsigned char comio_mem[COMIO_MAX_M];

void comio_setup(unsigned char io_defs[][3]);
int comio();
void comio_set_function(unsigned char num_function, callback_f function);
void comio_envoyer_trap(unsigned char num_trap);
void comio_send_long_trap(unsigned char num_trap, char *value, char l_value);

#endif
