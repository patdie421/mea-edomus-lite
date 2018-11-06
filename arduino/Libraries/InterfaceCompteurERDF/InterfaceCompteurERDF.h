#ifndef InterfaceCompteurERDF_h
#define InterfaceCompteurERDF_h

//
// Constantes de "compute"
//
#define ICE_ERROR_ERR             -1
#define ICE_LINE_START_ERR        -2
#define ICE_OVERFLOW_ERR          -3
#define ICE_CHECKSUM_KO_ERR       -4
#define ICE_LINE_ERROR_ERR        -5

#define ICE_NOP_INF                0

#define ICE_WAIT_FRAME_START_INF   1
#define ICE_FRAME_START_INF        2
#define ICE_LINE_START_INF         3
#define ICE_READING_LABEL_INF      4
#define ICE_READING_VALUE_INF      5
#define ICE_LABEL_FOUND_INF        6
#define ICE_VALUE_FOUND_INF        7
#define ICE_CHECKSUM_OK_INF        8
#define ICE_LABEL_INF              9
#define ICE_LABEL_NOT_FOUND_INF   10
#define ICE_END_FRAME_INF         11

//
// statut du moteur
//
#define ICE_RUNNING_MASK  0b01000000

#define ICE_IDLE          0b00000001
#define ICE_DATAAVAILABLE 0b00000010
#define ICE_ERROR         0b00000100
#define ICE_COMPUTING     0b01001000
#define ICE_STARTREAD     0b01010000

//
// numéro d'erreur
//
#define ICE_NO_ERROR               0
#define ICE_NUM_COUNTER_ERR        1
#define ICE_TIMEOUT_ERR            2

//
// parametres
//
#define ICE_MAXSTR     20 // taille max des chaines = adresse du teleport
#define ICE_TIMEOUT 10000 // 10 secondes pour trouver une valeur

//
// Définition des objets de type compteur
//
class InterfaceCompteurERDF
{
  public:
    InterfaceCompteurERDF(unsigned char selectionPin, unsigned char activationPin);
    InterfaceCompteurERDF(unsigned char selection, unsigned char activation, int (* readFn)(void), int (*availableFn)(void));
   
    int startRead(char *var, unsigned char cntr);
    void reset();
    unsigned char isIdle();
    unsigned char isRunning();
    unsigned char isDataAvailable();
    unsigned char isError();
    unsigned char getStrVal(char *, unsigned char lenval);
    int run();

  private:
    int (* readF)(void);
    int (* availableF)(void);

    char selection;
    char activation;
   
    unsigned char stage;
    unsigned char status;

    unsigned char _ICE_errno;

    char resultStr[ICE_MAXSTR];
    char searchStr[ICE_MAXSTR];
    char cntr;
};

#endif
