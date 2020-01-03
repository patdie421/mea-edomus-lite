#ifndef Comio2_h
#define Comio2_h

#define __DEBUG

#include <Arduino.h>


// COMIO2 : version 2 du protocole
#define COMIO2_MAX_FX          16 // nombre maximum de fonctions déclarables
#define COMIO2_MAX_MEMORY      32 // en octet
#define COMIO2_MAX_DATA        80 // taille maximum d'une trame de données

#define COMIO2_CMD_ERROR        0
#define COMIO2_CMD_READMEMORY   1
#define COMIO2_CMD_WRITEMEMORY  2
#define COMIO2_CMD_CALLFUNCTION 3

#define COMIO2_TRAP_ID        254

#define COMIO2_ERR_NOERR        0
#define COMIO2_ERR_PARAMS       1
#define COMIO2_ERR_UNKNOWN_FUNC 2
#define COMIO2_ERR_UNKNOWN     99

#ifndef COMIO_MAX_M
#define COMIO_MAX_M             COMIO2_MAX_MEMORY
#endif

//
// Définition des objets de type comio2
//
typedef int (*callback2_f)(int, char *, int, char *, int *, void *);

class Comio2
{
public:
  Comio2();

  void init();
  int run();

  void setFunction(unsigned char num_function, callback2_f function);
  int  setMemory(unsigned int addr, unsigned char value);
  int  getMemory(unsigned int addr);
  unsigned char *getMemoryPtr() { return comio_memory; };
  void setReadF(int (*f)(void)) { 
    readF=f; 
  };
  void setWriteF(int (*f)(char)) { 
    writeF=f; 
  };
  void setAvailableF(int (*f)(void)) { 
    availableF=f; 
  };
  void setFlushF(int (*f)(void)) { 
    flushF=f; 
  };
  void setUserdata(void *ud) { 
    userdata = ud; 
  };
  void sendTrap(unsigned char num_trap, char *data, char l_data);

private:
  // memoire "partagée"
  unsigned char comio_memory[COMIO_MAX_M];

  callback2_f comio_functionsV2[COMIO2_MAX_FX];

  int (* readF)(void);
  int (* writeF)(char car);
  int (* availableF)(void);
  int (* flushF)(void);
  void *userdata;

  char buffer[COMIO2_MAX_DATA];

  void _comio_debut_trameV2(unsigned char id, unsigned char cmd, int l_data, int *ccheksum);
  void _comio_fin_trameV2(int *cchecksum);
  void _comio_send_errorV2(unsigned char id, unsigned char error);
  int _comio_read_frameV2(unsigned char *id, unsigned char *cmd, char *data, int *l_data);
  int _comio_do_operationV2(unsigned char id,unsigned char cmd, char *data, int l_data);
};

#endif
