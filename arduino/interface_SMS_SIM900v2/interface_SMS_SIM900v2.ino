#include <Arduino.h>
#include <AltSoftSerial.h>
#include <EEPROM.h>
#include <inttypes.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>

#define __DEBUG__ 2

// utilisation des ports (ARDUINO UNO) Dans ce sketch
/* PORTD
 * PIN 0 : PC(TX) [RX]
 * PIN 1 : PC(RX) [TX]
 * PIN 2 : - [INT] : <non utilisée>
 * PIN 3 : X [INT / PWM] : SORTIE : RESET du sim900
 * PIN 4 : X P0 - ENTREE : détection POWER UP/DOWN (LOW = DOWN, HIGH = UP), dispo en lecture seule
 * PIN 5 : X P1 [PWM] : ENTREE/SORTIE - dispo pour I/O SMS
 * PIN 6 : X P2 [PWM] : ENTREE/SORTIE - dispo pour I/O SMS
 * PIN 7 : X P3 - ENTREE : détection ALARME, dispo en lecture seule
 *
 * PORDB
 * PIN 8 : SIM900_RX
 * PIN 9 : SIM900_TX
 * PIN 10 : X [PWM / SS]   - ENTREE : LOW = SIM900 desactivé
 * PIN 11 : X [PWM / MOSI] - ENTREE : LOW = command vers MCU uniquement (mode configuration/commande)
 * PIN 12 : X [MISO]       - ENTREE : LOW = interprétation par le MCU des messages non solicités desactivée (mode transparent)
 * PIN 13 : LED1 [LED / SCK] - SORTIE : led d'état - clignotement toutes les 1s = fonctionnement nominal / toutes les 125ms = erreur, pas d'opération SIM900 possible
 * PIN NA : x
 * PIN NA : x
 *
 * PORTC
 * PIN A0 : X P4 : ENTREE/SORTIE - dispo pour I/O SMS
 * PIN A1 : X P5 : ENTREE/SORTIE - dispo pour I/O SMS
 * PIN A2 : X P6 : ENTREE/SORTIE - dispo pour I/O SMS
 * PIN A3 : X P7 : ENTREE/SORTIE - dispo pour I/O SMS
 * PIN A4 : X P8 : ENTREE/SORTIE - dispo pour I/O SMS
 * PIN A5 : X P9 : ENTREE/SORTIE - dispo pour I/O SMS
 * PIN A6*: x PAS DISPO SUR UNO
 * PIN A7*: x PAS DISPO SUR UNO
 */

/*
 Interface de commandes SMS via SIM900
 
 Ce programme permet de "transformer" un Arduino en un module "autonome et/ou connecté" 
 capable de  contrôler 10 entrées/sorties. Relié à une interface SIM900, le module permet
 de piloter à distance des E/S en fonction d'ordres reçus par SMS. Il émet également des
 alerte SMS (POWER UP/DOWN et  ALARM ON/OFF) lorsque l'une des deux entrées réservées à
 cet effet change d'état.
 
 Ce module dispose de :
 - 8 E/S libres, dont 6 peuvent être utilisées en entrées Analogiques et 2 en sorties PWM
 (analogiques). 
 - 2 Entrées logiques spécifiques qui permettent :
  * pour l'une de connaitre l'état d'un alimentation électrique (via un interface à 
 réaliser)
  * pour l'autre de connaitre l'état d'une alarme (interface à réaliser)
 Un traitement spécifique est associé à ces deux entrées en plus de pouvoir être
 interrogées comme les 8 autres entrés/sorties. Ces entrées réagissent sur
 front pour le déclenchement d'alarmes. Les alarmes sont envoyées sous forme de SMS
 (POWER DOWN, POWER UP, ALARM ON, ALARM OFF).
 
 Les entrées/sorties sont numérotées de la façon suivante :
 
 MODULE /  ARDUINO  / Fonction
 
 0      /  PIN 4    / détection POWER UP/DOWN (LOW = DOWN, HIGH = UP), dispo en lecture
 1      /  PIN 5    / E/S dont PWN
 2      /  PIN 6    / E/S dont PWN
 3      /  PIN 7    / détection ALARME, dispo en lecture seule
 4      /  PIN A0   / E/S dont entrée analogique
 5      /  PIN A1   / E/S dont entrée analogique
 6      /  PIN A2   / E/S dont entrée analogique
 7      /  PIN A3   / E/S dont entrée analogique
 8      /  PIN A4   / E/S dont entrée analogique
 9      /  PIN A5   / E/S dont entrée analogique
 
 Les communication/réaction au SMS sont contrôlés (ACL). Le module dispose d'un carnet
 de  numéro de téléphone (10 numéros disponibles) stockés en EEPROM (commandes MCU pour
 gérer la liste). Seul les numéros contenus dans cette liste peuvent agir sur le module
 par SMS. Le carnet fait également fonction de "liste de diffusion", les alarmes sont
 transmises à tous les numéro enregistrés.
 
 Le module s'interface avec un ordinateur via :
 
 - la liaison serie (USB sur UNO ou plus généralement pin 0 et 1 d'un ATMEGA328)
 - 3 lignes de contrôle (P10, 11 et 12)
 
 Il se connecte au SIM900 via :
 - une liaison serie "logiciel" (AltSoftSerial) sur PIN 8 et 9
 - une ligne "reset" (P3) à connecter à l'entrée Reset d'un module SIM900.
 
 La LED (pin13) indique l'état de fonctionnement (clignotement lent = OK ou 
 rapide = pas de sim900 ou mode commande)
 
 Le module peut être contrôle depuis le SIM900 (via SMS) ou depuis la liaison Serie. De
 plus il fait fonction de passerelle (transparente) entre les deux équipements, permettant
 ainsi à l'ordinateur de recevoir et d'émettre des données de/vers le module GSM.  En
 fonctionnement normal (lignes P10, P11 et P12 à HIGH ou non connectées), tous les
 octets reçus depuis le SIM900 sont retransmis sur la ligne serie et inversement.
 Neanmoins, les données en provenance du SIM900 sont interprétés par le MCU après après
 avoir été retransmission. Les données en provenance de l'ordinateur sont retransmises
 sans interprétation par le MCU.
 
 Ce comportement peut être modifié de la façon suivante :
 - en envoyant la séquence "~~~~" depuis le PC on passe le MCU en mode commande.
   La sortie du mode command est réalisée en envoyant la commande "END".
 - en mettant P10 à LOW : la retransmission des données du SIM900 vers le PC est bloqué.
 - en mettant P11 à LOW : le MCU doit interpréter les commandes en provenance du PC (les
 données ne sont alors pas transmises au SIM900). Lors de l'initialisation du module
 si la communication ne peut pas être établie dans les 10s avec un SIM900, le module
 passe automatiquement dans ce mode (les états de P10, 11 et P12 n'ont aucun effet).
 Lorsque le MCU est en mode commande la LED (13) clignote rapidement.
 - en mettant P12 à LOW : le MCU n'interprète pas les données en provenance du SI900
 (mode transparent).
 
 - en mode commande, les ordres suivants peuvent être
 transmises au MCU :
 
 <LF/CR>       - affiche le prompt (>)
 PIN:<xxxx>    - stockage du code pin xxxx de la sim en ROM
 PIN:C         - efface le code pin
 NUM:<x>,<nnnnnnnnnnnnnnnnnnn>
 - ajoute un numero de telephone dans l'annuaire du MCU (10 positions 
 disponibles, de 0 à 9 et 20 caractères maximum par numéro)
 NUM:<x>,C     - efface un numéro
 CMD:##<...>## - voir format des commandes SMS
 LST - affiche le contenu de la ROM
 RST - redémarrage (reset) du MCU
 END - fin du mode commandes "soft"
 
 Attention : saisissez les commandes sans espace (y compris au début et à la fin). Si le
 prompt n'est pas affiché, appuyer "entrée".
 
 Exemples :
 
 Stocker le conde PIN 1234
 > PIN:1234
 DONE
 >
 
 Effacer le code PIN
 > PIN:C
 DONE
 >
 
 Ajouter un numéro de téléphone à la position 1
 > NUM:1,+33066166XXXX
 DONE
 >
 
 Supprimer un numéro
 > NUM:1,C
 DONE
 >
 
 Liste le carnet de numéro de téléphone
 > LST
 <à faire>
 
 Changer l'état des lignes :
 PIN 1 positionnée à HIGH
 PIN 2 PWM à 100
 lecture PIN 4
 
 > CMD:##1,L,H;1,A,100;4,A,G##
 <à faire>
 
 Les commandes interprétés par le modules doivent avoir le format suivant :
 
 ##{PIN1},{COMMANDE1},{PARAM1};{PIN2},{COMMANDE2},{PARAM2};...##
 
 avec :
 PIN       numéro de la sortie/sortie Interface_type_003 correspondance avec
 I/O Arduino (Pin Arduino/Interface_type_003) : 0:4, 1:5, 2:6, 3:7,
 4:14(A0), 5:15(A1), 6:16(A2), 7:17(A3), 8:18(A4), 9:19(A5)
 COMMANDE  action à réaliser :
 - 'L' : nouvelle valeur logique
 - 'A' : nouvelle valeur analogique
 - 'P' : impulsion sur la ligne (passage 1 puis retour 0 après x ms)
 PARAM     parametre de la commande :
 - pour 'L' => H (set high), L (set low)
 - pour 'A' => entier positif
 - pour 'P' => entier positif (durée de l'impulsion en ms)
 - pour 'L' et 'A' => 'G' = Get value : lecture d'une valeur

 
 Les réponses sont de la forme :
 <à faire><mettre les codes erreurs disponible>
 */

#ifndef prog_char
typedef const char prog_char;
#endif

prog_char starting_str[]      PROGMEM  = { "$$STARTING$$\n" };
prog_char syncok_str[]        PROGMEM  = { "$$SYNCOK$$\n" };
prog_char syncko_str[]        PROGMEM  = { "$$SYNCKO$$\n" };
prog_char powerup_str[]       PROGMEM  = { "$$POWERUP$$\n" };
prog_char powerdown_str[]     PROGMEM  = { "$$POWERDOWN$$\n" };
prog_char alarmon_str[]       PROGMEM  = { "$$ALARMON$$\n" };
prog_char alarmoff_str[]      PROGMEM  = { "$$ALARMOFF$$\n" };
prog_char cmndon_str[]        PROGMEM  = { "$$CMNDON$$\n" };
prog_char cmndoff_str[]       PROGMEM  = { "$$CMNDOFF$$\n" };
prog_char sig_str[]           PROGMEM  = { "$$SIG="};
prog_char dollar_dollar_str[] PROGMEM  = { "$$" };
prog_char nosignal_str[]      PROGMEM  = { "$$NOSIGNAL$$\n" };

#if __DEBUG__ > 0
prog_char lastSMS[]           PROGMEM  = { "LAST SMS: " };
prog_char unsolicitedMsg[]    PROGMEM  = { "CALL UNSOLICITED_MSG CALLBACK\n" };
#endif

#if __DEBUG__ > 1
int freeRam () {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}
#define FREERAM Serial.print("FR="); Serial.println(freeRam())
#else
#define FREERAM (void)0
#endif

void printStringFromProgmem(prog_char *s)
{
  int i=0;
  while(1)
  {
    char c =  pgm_read_byte_near(s + i);
    if(c)
    {
      Serial.write(c);
    }
    else
      break;
    i++;
  }
}


char *trim(char *buffer)
{
  // deplacement du point jusqu'au premier caractère non blanc
  char *p1=buffer;     
  while(*p1 && isspace(*p1))
    p1++;
  
  // déplacement jusqu'à la fin de la ligne
  char *p2=p1;
  while(*p2)
    p2++;

  // retour en arrière tant qu'on a des caractères blancs on remplace par des 0
  while(p2>p1)
  {
     p2--;
     if(isspace(*p2))
        *p2=0;
     else
        break;
  }
  
  return p1;
}


/*
DDRx avec x :
-B (digital pin 8 to 13) 
-C (analog input pins) 
-D (digital pins 0 to 7)
*/
int getPinDirection(int pinNum)
{
   int num=pinNum;
   unsigned char _ddrx=0;

   if(pinNum>=0 && pinNum <=7)
      _ddrx=DDRD;

   else if(pinNum>=8 && pinNum<=13)
   {
      _ddrx=DDRB;
      num=pinNum-8;
   }
   else if(pinNum>=14 && pinNum<=21)
   {
      _ddrx=DDRC;
      num=pinNum-14;
   }

   _ddrx=_ddrx ^ 0xFF;

   int state= _ddrx & (1 << num);

   if(state>0)
      return 1;
   else
      return 0;
}


int interfacePinNum(int arduinoPin)
{
   if(arduinoPin >=4 && arduinoPin <=7)
      return arduinoPin - 4;
   else if(arduinoPin>=14 && arduinoPin <=21)
      return arduinoPin - 14;
   else
      return -1;
}


/******************************************************************************/
//
// Pulse
//
/******************************************************************************/
#define MAX_PULSES 5

struct pulse_context_s
{
   unsigned long prev_millis;
   unsigned int  duration;
   unsigned char pin;
   unsigned char front;
};

struct pulse_context_s pulses_context_list[MAX_PULSES];
unsigned char nb_active_pulses;

unsigned char pin_direction(unsigned char pin)
{
   unsigned char port;
   unsigned char output;
   unsigned char state;
   unsigned char mask;
   
   port=pin / 8;
   output=pin % 8;
   mask=1 << output;
   
   switch(port)
   {
      case 0:
         state = DDRD & mask;
         break;
      case 1:
         state = DDRB & mask;
         break;
      case 2:
         state = DDRC & mask;
         break;
   }

   if(state>0)
      return HIGH;
   else
      return LOW;
}


void init_pulses()
{
   for(int i=0;i<MAX_PULSES;i++)
      pulses_context_list[i].pin=0;
   nb_active_pulses=0;
}


int pulseUp(unsigned char pin, unsigned int duration, unsigned char reactivable)
{
   // réactivable or not
   for(int i=0;i<MAX_PULSES;i++)
   {
      if(pulses_context_list[i].pin==pin)
      {
         if(reactivable==1)
         {
            pulses_context_list[i].prev_millis=millis();
            return 0;
         }
         else
            return -1;
      }
   }
   
   if(nb_active_pulses==MAX_PULSES)
      return -1;
   
   for(int i=0;i<MAX_PULSES;i++)
   {
      if(pulses_context_list[i].pin==0)
      {
         digitalWrite(pin,HIGH);
         // toggle_output_pin(pulses_context_list[i].pin);
         nb_active_pulses++;
         pulses_context_list[i].pin=pin;
         pulses_context_list[i].duration=duration;
         pulses_context_list[i].prev_millis=millis();
         pulses_context_list[i].front=1; // pulsation positive
         break;
      }
   }
}


int pulse(unsigned char pin, unsigned int duration, unsigned char reactivable, unsigned char front)
{
   // réactivable or not
   for(int i=0;i<MAX_PULSES;i++)
   {
      if(pulses_context_list[i].pin==pin)
      {
         if(reactivable==1)
         {
            pulses_context_list[i].prev_millis=millis();
            return 0;
         }
         else
            return -1;
      }
   }
   
   if(nb_active_pulses==MAX_PULSES)
      return -1;
   
   for(int i=0;i<MAX_PULSES;i++)
   {
      if(pulses_context_list[i].pin==0)
      {
         if(front)
            digitalWrite(pin,HIGH);
         else
            digitalWrite(pin,LOW);

         nb_active_pulses++;
         pulses_context_list[i].pin=pin;
         pulses_context_list[i].duration=duration;
         pulses_context_list[i].prev_millis=millis();
         pulses_context_list[i].front=front;
         break;
      }
   }
}


void pulses()
{
   if(!nb_active_pulses)
      return;
   
   unsigned long now = millis();
   
   for(int i=0;i<MAX_PULSES;i++)
   {
      if(pulses_context_list[i].pin!=0)
      {
         if( (now - pulses_context_list[i].prev_millis) > pulses_context_list[i].duration)
         {
            if(pulses_context_list[i].front==1)
               digitalWrite(pulses_context_list[i].pin, LOW);
            else
               digitalWrite(pulses_context_list[i].pin, HIGH);
               
            nb_active_pulses--;
            pulses_context_list[i].pin=0;
         }
      }
   }
}


/******************************************************************************/
//
// Class BlinkLeds - HEADER
//
/******************************************************************************/
class BlinkLeds
{
public:
  BlinkLeds(unsigned int i);
  void run();
  inline int getLedState() {
    return ledState;
  }
  void setInterval(unsigned int i);
private:
  int interval;
  int ledState;
  unsigned long previousMillis;
  inline unsigned long diffMillis(unsigned long chrono, unsigned long now)
  {
    return now >= chrono ? now - chrono : 0xFFFFFFFF - chrono + now;
  }
};


/******************************************************************************/
//
// Class BlinkLeds - BODY
//
/******************************************************************************/
BlinkLeds::BlinkLeds(unsigned int i)
{
  interval = i;
  ledState = LOW;
  previousMillis = 0;
}


void BlinkLeds::run()
{
  unsigned long currentMillis = millis();

  if(diffMillis(previousMillis, currentMillis) > interval) {
    previousMillis = currentMillis;
    ledState = !ledState;
  }
}


void BlinkLeds::setInterval(unsigned int i)
{
  interval = i;
}


/******************************************************************************/
//
// Class Sim900 - HEADER
//
/******************************************************************************/
#define SIM900_BUFFER_SIZE 200
extern const char *SIM900_ok_str;
extern const char *SIM900_error_str;
extern const char *SIM900_standard_returns[];

class Sim900
{
public:
  Sim900();
  int sync(long timeout);
  int init();

  int getSMSInFlag() { return SMSInFlag; };
  int echoOff();
  int echoOn();
  inline int getEchoOnOff() {
    return echoOnOff; 
  };

  int setAtTimeout(int t) {
    atTimeout = t; 
  };
  int setEchoTimeout(int t) {
    echoTimeout = t; 
  };
  int setSmsTimeout(int t) {
    smsTimeout = t; 
  };

  int waitLines(char *vals[], long timeout);
  unsigned char *readLine(char *str, int l_str, long timeout);
  int waitString(char *val, long timeout);
  int readStringTo(char *str, int l_str, char *val, long timeout);

  int getChar();
  int sendChar(char c, int isEchoOn);
  inline int sendChar(char c) {
    return sendChar(c, echoOnOff); 
  };

  int sendCr(int isEchoOn) {
    return sendChar('\r',isEchoOn) | sendChar('\n', isEchoOn);
//    return sendChar('\r',isEchoOn); 
  };
  inline int sendCr() {
    return sendCr(echoOnOff); 
  };

  int sendString(char *str, int isEchoOn);
  inline int sendString(char *str) {
    return sendString(str, echoOnOff); 
  };

  int sendStringFromProgmem(prog_char *str, int isEchoOn);
  inline int sendStringFromProgmem(prog_char *str) {
    return sendStringFromProgmem(str, echoOnOff); 
  };

  int sendATCmnd(char *cmnd, int isEchoOn);
  inline int sendATCmnd(char *cmnd) {
    return sendATCmnd(cmnd, echoOnOff); 
  };

  int sendATCmndFromProgmem(prog_char *str, int isEchoOn);
  inline int sendATCmndFromProgmem(prog_char *str) {
    return sendATCmndFromProgmem(str, echoOnOff); 
  };

  int sendSMS(char *num, char *text, char isEchoOn);
  inline int sendSMS(char *num, char *text) {
    return sendSMS(num, text, echoOnOff); 
  };

  int sendSMSFromProgmem(char *num, prog_char *text, char isEchoOn);
  inline int sendSMSFromProgmem(char *num, prog_char *text) {
    return sendSMSFromProgmem(num, text, echoOnOff); 
  };

  inline void MCUAnalyseOn() { MCUAnalyseFlag=1; };
  inline void MCUAnalyseOff() { MCUAnalyseFlag=0; };
  
  inline void resetCheckTimer() { check_timer = millis(); };
  int connectionCheck();
  float getSignalQuality();
  int connectionStatus();

  void setPinCode(char *code);
  void run();
  int (* read)(void);
  int (* write)(char c);
  int (* available)(void);
  int (* flush)(void);

  void setRead(int (*f)(void)) {
    read=f; 
  };
  void setWrite(int (*f)(char)) {
    write=f; 
  };
  void setAvailable(int (*f)(void)) {
    available=f; 
  };
  void setFlush(int (*f)(void)) {
    flush=f;
  };

  void *userData;
  void setUserData(void *data) {
    userData = data; 
  };
  unsigned char *getLastSMSPhoneNumber() {
    return lastSMSPhoneNumber; 
  };

  int (* SMSCallBack)(char *, void *); // callback appeler lorsqu'un SMS est recu (première ligne)

  void setSMSCallBack(int (*f)(char *, void *)) {
    SMSCallBack = f; 
  };

private:
  // les différents timeout
  int echoTimeout; // echo du caractère du SIM900
  int atTimeout; // timeout d'une commande AT
  int smsTimeout; // timeout de la commande d'envoie d'un SMS
  // timer
  unsigned long check_timer;
  
  // buffers d'entrée de données
  unsigned char buffer[SIM900_BUFFER_SIZE];
  int bufferPtr;

  // numéro de téléphone de l'expéditeur du dernier SMS recu
  unsigned char lastSMSPhoneNumber[20];

  // code pin de la carte SIM
  unsigned char pinCode[9];

  // indicateur d'écho ON/OFF
  char echoOnOff;

  // drapeaux
  int MCUAnalyseFlag;
  int SMSInFlag;
  
  // methodes internes
  void analyse(int c);
  
  inline unsigned long diffMillis(unsigned long chrono, unsigned long now) {
    return now >= chrono ? now - chrono : 0xFFFFFFFF - chrono + now;
  };
};


/******************************************************************************/
//
// Class Sim900 - BODY
//
/******************************************************************************/
const char *SIM900_ok_str="OK";
const char *SIM900_error_str="ERROR";
const char *SIM900_standard_returns[]={
  SIM900_ok_str, SIM900_error_str,NULL};
const char *SIM900_smsPrompt="> ";

inline int _sim900_read()
/**
 * \brief     fonction de lecture par défaut d'un caractère.
 * \details   Point de lecture pour la réception des commandes/données à traiter par sim900 si aucune autre fonction n'a été positionnée par la methode setReadFunction. Lit le premier caractère du buffer d'entrée
 * \return    < 0 (-1) si pas de données disponible, le caractère lu sinon.
 */
{
  int c=Serial.read();
  return c;
}


inline int _sim900_write(char car)
/**
 * \brief     fonction d'écriture par défaut d'un caractère.
 * \param     car   caractère a insérer dans le buffer de sortie
 * \details   fonction d'écriture des résultats renvoyés par sim900 si aucune autre fonction n'a été positionnée par la methode setWriteFunction. Ajoute un caractere dans le buffer de sortie
 * \return    toujours 0
 */
{
  Serial.write(car);
  return 0;
}


inline int _sim900_available()
/**
 * \brief     fonction permettant de connaitre le nombre de caractères présent dans le buffer d'entrée.
 * \details   fonction par defaut si aucune autre fonction n'a été positionnée par la methode setAvailableFunction.
 * \return    nombre de caractères disponibles
 */
{
  return Serial.available();
}


inline int _sim900_flush()
/**
 * \brief     fonction permettant d'attendre le "vidage" du buffer de sortie.
 * \details   fonction par defaut si aucune autre fonction n'a été positionnée par la methode setFlushFunction.
 * \return    toujours 0
 */
{
  Serial.flush();
  return 0;
}


Sim900::Sim900()
{
  bufferPtr = 0;
  buffer[0]=0;
  lastSMSPhoneNumber[0]=0;

  pinCode[0]=0;
  
  echoOnOff=1; // echo activé par defaut

  write=_sim900_write;
  read=_sim900_read;
  available=_sim900_available;
  flush=_sim900_flush;

  echoTimeout=150; // 50 ms
  atTimeout=200;
  smsTimeout=1000;

  userData = NULL;
  SMSCallBack = NULL;
  
  check_timer=0;
  SMSInFlag=0;
}


int Sim900::echoOff()
/**
 * \brief     desactive l'écho des caractères du SIM900.
 * \details   Transmet la commande ATE0 et met à jour l'indicateur d'état (echoOnOff) en concéquence.
 * \return    résultat de l'emission de la commande AT (-1 : erreur de communication (echoTimeout), 0 = reponse "OK", 1 = reponse "ERROR").
 */
{
  if(sendATCmnd((char *)"E0",0)<0) // suppression de l'echo
    return -1;
  echoOnOff=0;
  return waitLines((char **)SIM900_standard_returns, atTimeout);
}


int Sim900::echoOn()
/**
 * \brief     active l'écho des caractères du SIM900.
 * \details   Transmet la commande ATE1 et met à jour l'indicateur d'état (echoOnOff) en concéquence.
 * \return    résultat de l'emission de la commande AT (-1 : erreur de communication (echoTimeout), 0 = reponse "OK", 1 = reponse "ERROR").
 */
{
  if(sendATCmnd((char *)"E1",0)<0)  // suppression de l'echo
    return -1;
  echoOnOff=1;
  return waitLines((char **)SIM900_standard_returns, atTimeout);
}


void Sim900::setPinCode(char *code)
/**
 * \brief     stock le pin code qui pourra être utilisé pour l'initialisation de SIM900.
 * \return    aucun retour.
 */
{
  strcpy((char *)pinCode, code);
}


int Sim900::sendSMS(char *tel, char *text, char isEchoOn)
/**
 * \brief     Emission d'un SMS "texte" var le numero de telephone indiqué.
 * \details   
 * \param     tel : chaine de caractère contenant le numéro de telephone
 * \param     text : message à emettre (140 caractères maximum)
 * \param     isEchoOn : 1 si des caractères d'echo sont attendus (et à ne pas traiter) 0 sinon
 * \return    -1 en cas de timeout "echoTimeout", 0 sinon. /!\ la commande peut ne pas être allée jusqu'au bout.
 *            Utiliser sim900.sync si nécessaire.
 */
{
//  if(sendATCmnd((char *)"", isEchoOn)<0)
//    return -1;
//  if(waitString((char *)SIM900_standard_returns, smsTimeout)<0)
//    return -1;
  if(sendString((char *)"AT+CMGS=\"", isEchoOn)<0)
    return -1;
  if(sendString(tel, isEchoOn)<0)
    return -1;
  if(sendChar('"',isEchoOn)<0)
    return -1;
  if(sendCr(isEchoOn)<0)
    return -1;
  if(waitString((char *)SIM900_smsPrompt, smsTimeout)<0)
    return -1;
  if(sendString(text, isEchoOn)<0)
    return -1;
  if(sendCr(isEchoOn)<0)
    return -1;
  if(waitString((char *)SIM900_smsPrompt, smsTimeout)<0)
    return -1;
  if(sendChar(26,isEchoOn)<0) // CTRL-Z
    return -1; 

  return 0;
}


int Sim900::sendSMSFromProgmem(char *tel, prog_char *text, char isEchoOn)
/**
 * \brief     Emission d'un SMS "texte" vers le numero de telephone indiqué. Le texte du message est stocké en memoire programme (flash)
 * \param     tel   chaine de caractères contenant le numéro de telephone
 * \param     text  message à émettre (140 caractères maximum)
 * \param     isEchoOn : 1 si des caractères d'echo sont attendus (et à ne pas traiter) 0 sinon
 * \return    -1 en cas de timeout "echoTimeout" ou "smsTimeout", 0 sinon. /!\ la commande peut ne pas être allée jusqu'au bout.
 *            Utiliser sim900.sync si nécessaire.
 */
{
  if(sendATCmnd((char *)"", isEchoOn)<0)
    return -1;
  if(waitString((char *)SIM900_standard_returns, smsTimeout)<0)
    return -1;
  if(sendString((char *)"AT+CMGS=\"",isEchoOn)<0)
    return -1;
  if(sendString(tel,isEchoOn)<0)
    return -1;
  if(sendChar('"',isEchoOn)<0)
    return -1;
  if(sendCr(isEchoOn)<0)
    return -1;
  if(waitString((char *)SIM900_smsPrompt, smsTimeout)<0)
    return -1;
  if(sendStringFromProgmem(text,isEchoOn)<0)
    return -1;
  if(sendCr(isEchoOn)<0)
    return -1;
  if(waitString((char *)SIM900_smsPrompt, smsTimeout)<0)
    return -1;
  if(sendChar(26,isEchoOn)<0) // CTRL-Z
    return -1;
}


void Sim900::analyse(int c)
{
  // +CMT: "+33661665082","","15/11/19,22:57:29+04",145,32,0,0,"+33660003151",145,140
  static char state = 0;
  static char numTelPtr = 0;
  static char cntr=0;
  static int msgLen=0;
  static int nbCar=0;
  
  nbCar++;
  switch(state)
  {
     case 0:
       SMSInFlag=1;
       lastSMSPhoneNumber[0]=0;
       if(c == '+')
         state++;
       break;
     case 1:
       if(c == 'C')
         state++;
       else
         state = -1;
        break;
     case 2:
       if(c == 'M')
         state++;
       else
         state = -1;
        break;
     case 3:
       if(c == 'T')
         state++;
       else
         state = -1;
        break;
     case 4:
       if(c == ':')
         state++;
       else
         state = -1;
        break;
     case 5:
       if(c == ' ')
         state++;
       else
         state = -1;
     break;

     // lecture numtel
     case 6:
       if(c == '"')
       {
         state++;
         numTelPtr=0;
       }
       else
         state = -1;
       break;
     case 7:
       if(c=='"')
       {
         lastSMSPhoneNumber[numTelPtr]=0;
         cntr=0;
         state++;
       }
       else
       {
         if(numTelPtr<(sizeof(lastSMSPhoneNumber)-1))
            lastSMSPhoneNumber[numTelPtr++]=(char)c;
         else
            state = -1;
       }
       break;

     // lecture taille message
     case 8:
       if(c==',')
       {
         cntr++;
         if(cntr==10)
         {
           state++;
           msgLen=0;
         }
       }
       else
       {
          if(nbCar>128) // Arbitrairement si on a pas trouvé les 10 "," en 128 caractères, on s'arrête
             state=-1;
       }
       break;
     case 9:
       if(isdigit(c))
         msgLen=msgLen*10+(c-'0');
       else
       {
         state++;
       }
       break;
     
     case 10:
       bufferPtr=0;
       state++;
       break;

     case 11:
     // lecture message
       if(bufferPtr<(sizeof(buffer)-1))
       {
          buffer[bufferPtr++]=toupper(c);
          msgLen--;
       }
       if(!msgLen)
       {
          buffer[bufferPtr]=0;
/*
          Serial.println("");
          Serial.print("MESSAGE:");
          Serial.println((char *)buffer);
          Serial.print("FROM:");
          Serial.println((char *)lastSMSPhoneNumber);
*/          
          state=-1;
          if(SMSCallBack)
          {
             char *_buffer=trim((char *)buffer);
             SMSCallBack((char *)_buffer, userData);
             bufferPtr=0;
          }
       }
       break;
       
     default:
       state=-1;
       break;
   }
   
   if(state==-1)
   {
      SMSInFlag=0;
      state=0;
      nbCar=0;
   }
}


/**
 * \brief     récupération d'un caractère du SIM900
 * \details   
 * \return    -1 si pas de caractère dispo, le caractère sinon
 */
int Sim900::getChar()
{
  int c = this->read();
  
  if(c >= 0)
    resetCheckTimer();
  
  if(MCUAnalyseFlag)
     analyse(c);
  return c;
}


/**
 * \brief     Emission d'un caractère vers le SIM900
 * \details   
 * \param     c : caractère à transmettre
 * \param     isEchoOn : 1 si un echo du caractère est attendu (et à ne pas traiter) 0 sinon
 * \return    -1 timeout de l'écho atteint (voir setEchoTimeout), 0 OK.
 */
int Sim900::sendChar(char c, int isEchoOn)
{
  resetCheckTimer();
  this->write(c);
  // lecture de l'echo si nécessaire
  if(isEchoOn)
  {
    unsigned long start = millis(); // démarrage du chrono
    while(!this->available())
    {
      unsigned long now = millis();      
      if(diffMillis(start,  now) > echoTimeout)
      {
        return -1;
      }
    };
    getChar();
  }
  return 0;
}


int Sim900::sendATCmnd(char *atCmnd, int isEchoOn)
/**
 * \brief     Emission d'une commande AT vers le SIM900
 * \details   "atCmnd" doit contenir la commande sans le préfixe "AT" et sans le CR ou CR/LF. Ces éléments seront
 *            ajoutés par la commande avant l'émission.
 *            Ex : sim900.sendATCmnd("E0",0) ~ sim900.sendString("ATE0\r",0)
 *            Pour récupérer le resultat de la commande utiliser waitString ou équivalant.
 * \param     atCmnd    commande à transmettre
 * \param     isEchoOn  1 si un echo du caractère est attendu (et à ne pas traiter) 0 sinon
 * \return    -1 en cas de timeout "echoTimeout", 0 sinon. /!\ la commande peut ne pas être allée jusqu'au bout.
 *            Utiliser sim900.sync si nécessaire.
 */
{
  if(sendChar('A', isEchoOn)<0)
    return -1;
  if(sendChar('T', isEchoOn)<0)
    return -1;
  if(sendString(atCmnd, isEchoOn)<0)
    return -1;
  if(sendCr(isEchoOn)<0)
    return -1;

  return 0;
}


int Sim900::sendString(char *str, int isEchoOn)
/**
 * \brief     Transmission d'une chaine de caractères vers le SIM900
 * \details   /!\ n'envoie pas le '\r', il doit être contenu dans la chaine.
 *            nota : pour le SIM900 '\n' est suffisant pour le changement de ligne
 * \param     str       chaine de caractères à transmettre
 * \param     isEchoOn  1 si un echo du caractère est attendu (et à ne pas traiter) 0 sinon
 * \return    -1 en cas de timeout "echoTimeout", 0 sinon. /!\ la commande peut ne pas être allée jusqu'au bout.
 *            Utiliser sim900.sync si nécessaire.
 */
{
  char c;
  
  for(int i=0;str[i];i++)
  {
    if(sendChar(str[i],isEchoOn)<0)
      return -1;
  }
  return 0;
}


int Sim900::sendStringFromProgmem(prog_char *str, int isEchoOn)
/**
 * \brief     Transmission d'une chaine de caractères contenue en mémoire Flash(programme) vers le SIM900
 * \details   /!\ n'envoie pas automatiquement le CR/LF ('\r\n'), il doit être contenu dans la chaine.
 *            nota : pour le SIM900 '\n' est suffisant pour le changement de ligne
 * \param     str       chaine de caractères à transmettre
 * \param     isEchoOn  1 si un echo du caractère est attendu (et à ne pas traiter) 0 sinon
 * \return    -1 en cas de timeout "echoTimeout", 0 sinon. /!\ la commande peut ne pas être allée jusqu'au bout.
 *            Utiliser sim900.sync si nécessaire.
 */
{
  int i=0;

  while(1)
  {
    char c =  pgm_read_byte_near(str + i);
    if(c)
      if(sendChar(c,isEchoOn)<0)
        return -1;
      else
        break;
  }
  return 0;
}


unsigned char *Sim900::readLine(char *str, int l_str, long timeout)
/**
 * \brief     lit une ligne en provenance du SIM900.
 * \details   Tous les caractères en provenance du SIM900 sont récupérés jusqu'à trouver un CR/LF.
 *            La fonction retourne alors un pointeur sur la chaine de caractères contenant cette ligne sans le CR/LF.
 *            Si "timeout" ms se sont écoulés avant d'obtenir un CR/LF la lecture s'arrête (la fonction retour NULL dans ce cas).
 *            Si le nombre de caractères obtenues atteint l_str, la lecture s'arrête et la fonction retourne NULL.
 * \param     str      contient la chaine lue
 * \param     str_l    longueur max de la chaine (str)
 * \param     timeout  durée maximum en ms pour obtenir une chaine de caractères terminée par CR/LF
 * \return    pointeur sur le buffer contenant la chaine ou NULL. /!\ le buffer sera réutilisé lors du prochain
 *            appel à readLine.
 *            NULL est retourné en cas de timeout ou si la taille max (SIM900_BUFFER_SIZE) du buffer est atteinte
 *            avant l'arrivée d'un CR/LF.
 */
{
  int c;
  unsigned long start = millis(); // démarrage du chrono
  int strPtr = 0;

  while(1)
  {
    unsigned long now = millis();
    if(this->available())
    {
//      c = (unsigned char)this->read();
      c = (unsigned char)getChar();
      if(c == 10) // fin de ligne trouvée
      {
        str[strPtr]=0;
        return (unsigned char *)str;
      }
      else if(c == 13)
      {
      }
      else
      {
        str[strPtr]=c;
        strPtr++;
        if(strPtr > (l_str-1))
        {
          return NULL; // erreur : overflow
        }
      }
    }

    if(diffMillis(start,  now) > timeout)
    {
      return NULL;
    }
  }
}


int Sim900::waitLines(char *vals[], long timeout)
/**
 * \brief     Attent qu'une ligne de la liste des lignes (tableau de chaines "vals") ait été lue sur le SIM900 et retourne le numéro de ligne trouvée.
 * \details   Cette fonction prend en paramètre un tableau de chaines dont le dernier élément doit être NULL. Les chaines seront
 *            comparées aux lignes lues sur le SIM900. La longueur maximum de la chaine à attendre est limité à 80 octets
 *            Dès qu'une ligne (sans le CR/LF) est exactement identique à l'une des chaines de la liste, le numéro de la chaine
 *            trouvée est retourné. La numérotation des lignes commence à 0.
 *            Si "timeout" ms se sont écoulées avant d'obtenir l'une des lignes attendues, la lecture s'arrête (la fonction retour -1 dans ce cas).
 * \param     vals    : tableau de chaine contenant les chaines à trouver. Le dernier élément du tableau doit être NULL.
 * \param     timeout : durée maximum en ms pour obtenir l'une chaine de caractères recherchée
 * \return    numéro de la ligne (premier élément == 0).
 *            -1 timeout avant de trouver la ligne
 */
{
  unsigned long start = millis(); // démarrage du chrono
  char str[80];
  while(1)
  {
    unsigned long now = millis();
    long rl_timeout = timeout - diffMillis(start,  now);
    if(rl_timeout < 0)
    {
      return -1;
    }

    char *l=(char *)readLine(str, 80, rl_timeout);
    if(l==NULL) // overflow ou timeout
    {
      return -1;
    }
    for(int i=0;vals[i];i++)
    {
      if(strcmp(l,vals[i])==0)
      {
        return (i);
      }
    }
  }
}


int Sim900::waitString(char *val, long timeout)
/**
 * \brief     Attent que la chaine "val" ait été lue sur le SIM900.
 * \details   lit tous les caractères arrivant du SIM900 et dès que la séquence de caractères est strictement identique à "val"
 *            la fonction retourne 0.
 *            Si "timeout" ms se sont écoulées avant la séquence de caractères attendus, la lecture s'arrête (la fonction retour -1 dans ce cas).
 *            Cette fonction n'interprête pas CR/LF. Il est donc possible de faire sim900.waitString("\r\n",100); pour attendre une fin de ligne.
 * \param     timeout  durée maximum en ms pour obtenir la séquence de caractères recherchée
 * \return    0 lorsque la chaine est trouvée,
 *            -1 si "timeout" est atteint avant de trouver la chaine.
 */
{
  unsigned long start = millis(); // démarrage du chrono
  int i=0;

  if(!val[0])
    return 0;
  while(1)
  {
    unsigned long now = millis();

    if(diffMillis(start,  now) > timeout)
    {
      return -1;
    }

    if(this->available())
    {
//      int c = (unsigned char)this->read();
      int c = (unsigned char)getChar();
      if(c == val[i])
        i++;
      else
      {
        i=0;
        if(c == val[i])
          i++;
      }

      if(!val[i])
      {
        return 0;
      }
    }
  }
}


int Sim900::readStringTo(char *str, int l_str, char *val, long timeout)
/**
 * \brief     récupère tous les caractères en provenance du SIM900 dans "str" jusqu'à ce que la séquence de caractères "val" soit trouvée.
 * \details   "l_str" précise la taille maximum possible pour "str". Si la taille max ('\0' de fin de chaine compris) est atteinte avant 
 *            de trouver la séquence, la fonction retourne -1. str contient cependant tous les caractères déjà lus.
 *            Si "timeout" ms se sont écoulées avant la lecture de la séquence de caractères attendu, la lecture s'arrête,
 *            la fonction retour -1 et str[0] contient '\0';
 *            tous les caractères attendus de "val" sont présents en fin de chaine.
 *            Cette fonction n'interprête pas CR/LF. Il est donc possible de faire sim900.readStringTo("\r\n",100); pour lire une ligne complète.
 * \param     str      contient la chaine trouvée
 * \param     l_str    taille maximum de la chaine (0 de fin de chaine compris)
 * \param     val      séquence de caractères attendue
 * \param     timeout  durée maximum en ms pour obtenir la séquence de caractères recherchée
 * \return    0 lorsque la chaine est trouvée, -1 si "timeout" atteint avant de trouver la chaine (dans ce cas str[0]==0) 
 *            ou si taille max atteinte (avec donc str[l_str-1]==0);
 */
{
  unsigned long start = millis(); // démarrage du chrono
  int i=0;
  int strptr=0;
  while(1)
  {
    unsigned long now = millis();

    if(diffMillis(start,  now) > timeout)
    {
      str[0]=0;
      return -1;
    }

    if(this->available())
    {
      if(str)
      {
         if(strptr >= l_str-1)
         {
           str[l_str-1]=0;
           return -1;
         }
      }

//      int c = (unsigned char)this->read();
      int c = (unsigned char)getChar();
      if(str)
      {
         str[strptr]=c;
         strptr++;
      }

      if(c == val[i])
        i++;
      else
      {
        i=0; // réinitialisation de la séquence
        if(c == val[0]) // mais ça peut être le début d'une nouvelle séquence
          i++;
      }

      if(!val[i])
      {
        return 0;
      }
    }
  }
}


/**
 * \brief     récupère et affiche la qualité de reception du module sim900
 * \return    -1.0 en cas d'erreur de lecture, la qualité estimée en %
 */
float Sim900::getSignalQuality()
{
  int csq=-1;
  char *resp=NULL;
  int ret=-1;
  char str[20];
  
  sendATCmnd((char *)"+CSQ");
  if(readStringTo(NULL, 0, (char *)"+CSQ: ", atTimeout)==0)
  {
     resp=(char *)readLine(str, 80, atTimeout);
     if(resp!=NULL)
     {
       unsigned char i=0;
       while(resp[i] && isdigit(resp[i]))
         i++;
       resp[i]=0;
       csq=atoi(resp);
    }
    ret=waitLines((char **)SIM900_standard_returns, atTimeout);
    
    if(csq<0 || csq >31)
       return -1.0;
    else
       return (float)csq * 100.0 / 31.0;
  }
  else
    return -1.0;
}


/**
 * \brief     récupère l'état de la connexion au réseau GSM
 * \return    -1 en cas d'erreur de lecture, l'état de la connexion voir "AT+CREG?"
 */
int Sim900::connectionStatus()
{
  int creg=-1;
  char *resp=NULL;
  int ret=-1;
  char str[20];
  
  sendATCmnd((char *)"+CREG?");
  if(readStringTo(NULL, 0, (char *)"+CREG: ", atTimeout)==0)
  {
    resp=(char *)readLine(str, 80, atTimeout);
    if(resp!=NULL)
    {
      unsigned char i=0;
      while(resp[i] && resp[i]!=',')
        i++;
      resp[i]=0;
       /* int v=atoi(resp); */
      i++;
      resp=resp+i;
      while(resp[i] && isdigit(resp[i]))
        i++;
      resp[i]=0;
      creg=atoi(resp);
    }
  } 
  ret=waitLines((char **)SIM900_standard_returns, atTimeout);

  if(ret==0)
    return creg;
  else
    return -1;
}


/**
 * \brief     test la connexion au réseau GSM et déclenche la reconnexion si nécessaire
 * \return    -1 une tentative de reconnexion à eu lieu, 0 sinon 
 */
int Sim900::connectionCheck()
{
  if(bufferPtr)
      return 0;
 
  int ret=0;
  
  if(diffMillis(check_timer, millis())>60000)
  {
    float signal=getSignalQuality();
    if(signal>-1.0)
    {
       printStringFromProgmem(sig_str);
       Serial.print(signal); 
       printStringFromProgmem(dollar_dollar_str);
       Serial.println("");
    }
    else
    {
       printStringFromProgmem(nosignal_str);
    }
     
    if(connectionStatus() != 5)
    {
      init();
      ret=-1;
    }

    check_timer=millis();
  }
  
  return ret;
}


int Sim900::init()
/**
 * \brief     initialise le SIM900 pour recevoir des SMS texte et supprime l'echo
 * \return    toujours 0.
 */
{
  // configuration du sim900 pour recevoir des SMS
  echoOff();

  if(pinCode[0])
  {
    sendString((char *)"AT+CPIN=");
    sendString((char *)pinCode);
    sendCr();
    delay(100);
    waitLines((char **)SIM900_standard_returns, atTimeout);
  }

  sendATCmnd((char *)"+CMGF=1"); // mode "text" pour les sms
  waitLines((char **)SIM900_standard_returns, atTimeout);

  // format UCS2 pour les données reçu
  // sendATCmnd((char *)"+CSCS=\"UCS2\"");
  // waitLines((char **)SIM900_standard_returns, atTimeout);

  // pas de stockage des SMS, transmission direct sur la ligne serie
  sendATCmnd((char *)"+CNMI=2,2,0,0,0");
  waitLines((char **)SIM900_standard_returns, atTimeout);

  sendATCmnd((char *)"+CSDH=1"); // pour avoir la longueur dans un retour CMT:
  // sendATCmnd((char *)"+CSDH=0"); // pour avoir à la reception d'un : +CMT: "+12223334444","","14/05/30,00:13:34-32"
  waitLines((char **)SIM900_standard_returns, atTimeout);
  
  return 0;
}


int Sim900::sync(long timeout)
/**
 * \brief     synchronise la communication avec un SIM900
 * \details   vide le buffer d'entrée (lecture et oubli) jusqu'à plus rien à lire.
 *            envoie une commande AT et attend en retour un OK. Cela permet à l' "autobaud" de choisir la bonne vitesse de communication.
 *            si le retour n'est pas "OK", un nouveau cycle est réalisé.
 *            la tentative de synchro s'arrête après "timeout" ms.
 * \param     timeout  durée maximum accordée à la synchronisation
 * \return    0 si la synchronisation est réussie.
 *            -1 si timeout atteint.
 */
{
  unsigned long start = millis(); // démarrage du chrono

  // attente disponibilité du sim900
  while(1)
  {
    unsigned long now = millis();

    // on vide le buffer d'entrée
    for(;this->available();)
    {
      char c = this->read();
    }

    // Commande AT de synchro
    // on force l'attente d'un echo car on ne sait pas dans quel état est le sim900. Donc s'il y a un écho on le lit
    // sinon le timeout nous rendra la main.
    sendChar('A', 1);
    sendChar('T', 1);
    sendCr(1);
    
    // on attend un retour OK
    int ret = waitLines((char **)SIM900_standard_returns, 500);
    if(ret==0)
    {
      return 0;
    }
    // on a pas eu de retour OK
    
    // fin du temps imparti ?
    if(diffMillis(start,  now) > timeout)
    {
      return -1;
    }

    delay(500);
  }
}


void Sim900::run()
/**
 * \brief     gere la communication avec le SIM900.
 * \details   cette fonction permet un traitement asynchrone des données reçues sur la ligne serie du SIM900.
 *            exemple d'utilisation dans la boucle loop :
 *
 *            Sim900 sim900;
 *            void setup()
 *            {
 *               // ne pas oublier de déclarer les fonctions d'entrée/sortie à SIM900 voir setAvailable, setRead, setWrite et setFlush
 *            }
 *
 *            void loop()
 *            {
 *               sim900.run();
 *            }
 */
{
  if(this->available())
  {
     char c=(char)getChar();
  }
  
  return;
}


/******************************************************************************/
/******************************************************************************/

// Entrées/Sorties Réservées
#define PIN_SIM900_RESET 3
#define PIN_SIM900_ENABLE 10
#define PIN_MCU_CMD_ONLY 11
#define PIN_MCU_BYPASS_PROCESSING 12


// adresse de base des données en EEPROM
#define EEPROM_INIT_SGN  0
#define EEPROM_ADDR_PIN 10
#define EEPROM_ADDR_NUM 20


// taille max des données en EEPROM
#define NUMSIZE 21 // taille d'un numero de telephone (20 caractères maximum)
#define PINCODESIZE 9 // taille d'un code pin (8 caractères maximum)

#define BCAST_CREDIT 10 // crédit de broadcast
int nb_broadcast_credit=BCAST_CREDIT;

// Variables globales :
// Buffer des résultats
#define CMNDRESULTSBUFFERSIZE 141
unsigned char cmndResultsBuffer[CMNDRESULTSBUFFERSIZE]; // zone de donnée du buffer
int cmndResultsBufferPtr=0; // pointeur arrivée de caractères dans le buffer
// Buffer d'entrées pour les traitement MCU
#define MCUSERIALBUFFERSIZE 200
int mcuSerialBufferPtr=0; // pointeur arrivée de caractères dans le buffer
unsigned char mcuSerialBuffer[MCUSERIALBUFFERSIZE];

enum dest_e { TO_MCU=0, TO_SIM900=1 };

// structure "userData" (pour les callback et les fonctions de traitement)
struct data_s
{
  enum dest_e dest;
  unsigned char *cmndResultsBuffer;
  int *cmndResultsBufferPtr;
  unsigned char *lastSMSPhoneNumber;
};
struct data_s sim900UserData, mcuUserData; // deux zones de données utilisateurs.

// paramétrage du pin watcher
//#define  PINSWATCHER_NBPINS 2
#define  PINSWATCHER_NBPINS 10
struct pinsWatcherData_s
{
  unsigned char pin;
  unsigned long lastChrono;
  char lastState;
};
/*
struct pinsWatcherData_s pinsWatcherData[PINSWATCHER_NBPINS] = {
  { 4,0L, -1 },
  { 7,0L, -1 },
};
*/
struct pinsWatcherData_s pinsWatcherData[PINSWATCHER_NBPINS] = {
  { 4,  0L, -1 }, 
  { 5,  0L, -1 },
  { 6,  0L, -1 },
  { 7,  0L, -1 },
  { 14, 0L, -1 },
  { 15, 0L, -1 },
  { 16, 0L, -1 },
  { 17, 0L, -1 },
  { 18, 0L, -1 },
  { 19, 0L, -1 },
};

// objets globaux
AltSoftSerial sim900Serial;
Sim900 sim900;
BlinkLeds myBlinkLeds(125);


// variables globales
char soft_cmd_mode_flag = 0;
char sim900_connected_flag = 0; // passé à 1 si un sim900 est détecté lors de l'initialisation
char char_from_pc_flag=0; // pour éviter les échos des emissions du côté PC


int checkSgn()
/**
 * \brief   verifie qu'une signature d'initialisation est présente
 *
 */
{
   if(EEPROM.read(EEPROM_INIT_SGN)  !=0xFD)
      return -1;
   if(EEPROM.read(EEPROM_INIT_SGN+1)!=0xFC)
      return -1;
   if(EEPROM.read(EEPROM_INIT_SGN+2)!=0xFB)
      return -1;
   if(EEPROM.read(EEPROM_INIT_SGN+3)!=0xFA)
      return -1;
   return 0;
}


int setSgn()
/**
 * \brief   met en place du signature d'initialisation dans la ROM 
 *
 */
{
   EEPROM.write(EEPROM_INIT_SGN,   0xFD);
   EEPROM.write(EEPROM_INIT_SGN+1, 0xFC);
   EEPROM.write(EEPROM_INIT_SGN+2, 0xFB);
   EEPROM.write(EEPROM_INIT_SGN+3, 0xFA);
   
   return 0;
}


int setPin(char *num)
/**
 * \brief     copie le "code pin" d'une carte SIM en EEPROM.
 * \param
 * \return    -1 si taille de "num" >= PINCODESIZE, 0 sinon;
 */
{
  int i;
  if(strlen(num)>=PINCODESIZE)
    return -1;
  for(i=0;num[i] && i<(PINCODESIZE-1);i++)
    EEPROM.write(i+EEPROM_ADDR_PIN,num[i]);
  EEPROM.write(i+EEPROM_ADDR_PIN,0);
  return 0;
}


int getPin(char *pin, int l_pin)
/**
 * \brief     récupère dans une chaine le code pin stocké en EEPROM
 * \details   récupère au moins les "l_pin" premier caractères. Prévoir "l_pin" >= à PINCODESIZE
 *            pour récupérer le code en entier.
 * \return    -1 si aucun code n'est positionné, 0 sinon;
 */
{
  int i;
  char c=EEPROM.read(EEPROM_ADDR_PIN);

  if(c==0)
    return -1; // pas de code PIN positionné
  else
    pin[0]=c;

  for(i=1;i<(PINCODESIZE-1) && i<(l_pin-1);i++)
    pin[i]=EEPROM.read(i+EEPROM_ADDR_PIN);
  pin[i]=0;
  return 0;
}


int clearPin()
/**
 * \brief     efface le code pin de l'EEPROM
 * \return    toujours 0;
 */
{
  for(int i=0;i<PINCODESIZE;i++)
    EEPROM.write(i+EEPROM_ADDR_PIN,0);
  return 0;
}


int setNum(int pos, char *num)
/**
 * \brief     ajoute un numéro de téléphone à la position "pos" du carnet
 * de numéro de téléphone stocké en EEPROM.
 * \param     pos   position dans le carnet du numéro de tel (0 à 9)
 * \param     num   chaine de caractères contenant le numéro à écrire
 * \return    -1 si taille de "num" >= NUMSIZE ou "pos" incorrect, 0 sinon;
 */
{
  int i;
  if(pos<0 && pos>9)
    return -1;
  if(strlen(num)>=NUMSIZE)
    return -1;
  int base=EEPROM_ADDR_NUM+pos*NUMSIZE;
  for(i=0;num[i] && i<(NUMSIZE-1);i++)
    EEPROM.write(base++,num[i]);
  EEPROM.write(base, 0);
  return 0;
}


int getNum(int pos, char *num, int l_num)
/**
 * \brief     récupère dans une chaine le numéro de telephone de la position "pos"
 *            stocké en EEPROM
 * \details   récupère au moins les "l_num" premiers caractères. Prévoir "l_num" >= à NUMSIZE
 *            pour récupérer le numéro en entier.
 * \param     pos   position dans le carnet du numéro de tel (0 à 9)
 * \param     num   chaine de caractères contenant le numéro lu
 * \param     l_num nombre maximum de caractères de "num" (0 de fin compris)
 * \return    -1 erreur de position, 0 sinon;
 */
{
  int i;
  if(pos<0 && pos > 9)
    return -1;
  int base=EEPROM_ADDR_NUM+pos*NUMSIZE;
  for(i=0;i<(NUMSIZE-1) && i<(l_num-1);i++)
    num[i]=EEPROM.read(base++);
  num[i]=0;
  return 0;
}


int numExist(char *num)
/**
 * \brief     controle l'existance d'un numéro dans le carnet de numéro de téléphone
 *            stocké en EEPROM
 * \param     num   chaine de caractères contenant le numéro à controler
 * \param     l_num nombre maximum de caractères de "num" (0 de fin compris)
 * \return    0 si le numéro est présent, -1 sinon;
 */
{
  if(num[0]==0) // num ne peut pas être "rien"
    return -1;

  for(int i=0;i<10;i++)
  {
    int flag=0;
    for(int j=0;j<NUMSIZE;j++)
    {
      char c=EEPROM.read(j+EEPROM_ADDR_NUM);
      if(c != num[j])
        break;
      else if(c==0 && num[j]==0)
      {
        flag=1;
        break;
      }
      else if(c==0 || num[j]==0)
        break;
    }
    if(flag==1)
      return 0;
  }
  return -1;
}


int listRom()
/**
 * \brief     affiche le contenu de l'EEPROM
 * \return    toujours 0;
 */
{
  Serial.print("PIN=");
  for(int i=0;i<PINCODESIZE;i++)
  {
    char c=EEPROM.read(i+EEPROM_ADDR_PIN);
    if(c==0)
    {
      Serial.println("");
      break;
    }
    else
    {
      Serial.write(c);
    }
  }

  for(int i=0;i<10;i++)
  {
    Serial.print("NUM");
    Serial.print(i);
    Serial.write('=');
    for(int j=0;j<NUMSIZE;j++)
    {
      char c=EEPROM.read(EEPROM_ADDR_NUM+i*NUMSIZE+j);
      if(c==0)
        break;
      else
        Serial.write(c);
    }
    Serial.println("");
  }

  return 0;
}


int addToCmndResults(struct data_s *data, int pin, int cmnd, long value)
/**
 * \brief     ajoute le résultat d'une commande dans le buffer des résultats d'execution
 * \param     data  données spécifiques au traitement (user data de callback)
 * \param     pin   numéro de la ligne
 * \param     cmnd  la commande
 * \param     value le résultat de la commande
 * \return    -1 le buffer ne peut pas contenir le resultat, 0 sinon
 */
{
  // à mettre dans cmndResults sous la forme : PINx=VALUE1;PINx=VALUE2 ... max 140 caractères
  char tmpstr[10];

  int i=*(data->cmndResultsBufferPtr);

  if(value==-2)
  {
    tmpstr[0]='P';
    tmpstr[1]=0;
  }
  else
    itoa(value, tmpstr, 10);
  if(i < (CMNDRESULTSBUFFERSIZE-strlen(tmpstr)-8) )
  {
    if(i!=0)
      data->cmndResultsBuffer[i++]=';';

    data->cmndResultsBuffer[i++]=cmnd;
    data->cmndResultsBuffer[i++]='/';
    data->cmndResultsBuffer[i++]=(char )(pin+'0');
    data->cmndResultsBuffer[i++]='=';
    for(int j=0;tmpstr[j];j++)
      data->cmndResultsBuffer[i++]=tmpstr[j];
    data->cmndResultsBuffer[i]=0;
    
    (*data->cmndResultsBufferPtr)=i;

    return 0;
  }

  return -1;
}


int doneToCmndResult(struct data_s *data)
/**
 * \brief     ajoute "DONE" dans le buffer des résultats d'execution
 * \param     data  données spécifiques au traitement (user data de callback)
 * \return    -1 le buffer ne peut pas contenir le resultat, 0 sinon
 */
{
  int i=*(data->cmndResultsBufferPtr);

  if(i<CMNDRESULTSBUFFERSIZE-5)
  {
    if(i!=0)
      data->cmndResultsBuffer[i++]=';';

    data->cmndResultsBuffer[i++]='D';
    data->cmndResultsBuffer[i++]='O';
    data->cmndResultsBuffer[i++]='N';
    data->cmndResultsBuffer[i++]='E';
    data->cmndResultsBuffer[i]=0;

    *(data->cmndResultsBufferPtr)=i;
    
    return 0;
  }
  return -1;
}


int errToCmndResult(struct data_s *data, int errno)
/**
 * \brief     ajoute une erreur dans le buffer des résultats d'execution
 * \param     data    données spécifiques au traitement (user data de callback)
 * \param     errno   numéro d'erreur (0 = erreur 'A', 1 = erreur 'B', 26 = erreur 'Z'
 * \return    -1 le buffer ne peut pas contenir le resultat, 0 sinon
 */
{
  int i=*(data->cmndResultsBufferPtr);
  
  if(i<CMNDRESULTSBUFFERSIZE-6)
  {
    if(i!=0)
      data->cmndResultsBuffer[i++]=';';

    data->cmndResultsBuffer[i++]='E';
    data->cmndResultsBuffer[i++]='R';
    data->cmndResultsBuffer[i++]='R';
    data->cmndResultsBuffer[i++]=':';
    data->cmndResultsBuffer[i++]=errno+'A';
    data->cmndResultsBuffer[i]=0;

    *(data->cmndResultsBufferPtr)=i;
  
    return 0;
  }

  return -1;
}


int sendCmndResults(int retour, struct data_s *data)
/**
 * \brief     emet le resultat d'execution vers la ligne Serie ou sous forme d'un SMS.
 * \param     data    données spécifiques au traitement (user data de callback)
 * \return    toujours 0
 */
{
/*
  Serial.println("");
  Serial.print("retour=");
  Serial.println(retour);
  Serial.print("msg=");
  Serial.println((char *)data->cmndResultsBuffer);
*/ 
  if(retour==-1)
     return -1;
 
  if(data->dest==TO_SIM900) // vers sim900
  {
    sim900.sendSMS((char *)data->lastSMSPhoneNumber, (char *)data->cmndResultsBuffer);
  }
  else
  {
    Serial.print("$$");
    Serial.print((char *)data->cmndResultsBuffer);
    Serial.println("$$");
  }
  
  data->cmndResultsBuffer[0]=0;
  *(data->cmndResultsBufferPtr)=0;
}


int doCmnd(struct data_s *data, int pin, int cmnd, long value)
/**
 * \brief     execute la commande spécifié (si conforme)
 * \param     data    données spécifiques au traitement (user data de callback)
 * \param     pin   numéro de la ligne
 * \param     cmnd  code de la commande
 * \param     value parametre de la commande
 * \return    -1 la commande n'a pas été exécuté, 0 sinon
 */
{
  static char pins[] = {
    4,5,6,7,14,15,16,17,18,19 // pin 4 et 5 en "read-only"
  };

  int v;

  if(pin<0 || pin>9)
  {
    errToCmndResult(data, 10); // ERRK (num pin error)
    return -1;
  }

  if(value < -1 || value > 0xFFFF)
  {
    errToCmndResult(data, 11); // ERRL (value error)
    return -1;
  }

  switch(cmnd)
  {
  case 'L':
    if(value==-1)
    {  // lecture état logique
      if(pin!=0 & pin != 3)
      {
        pinMode(pins[pin],INPUT);
        digitalWrite(pins[pin], HIGH);
      }
      v=digitalRead(pins[pin]);
      addToCmndResults(data,pin,cmnd,v);
    }
    else
    {  // positionne état logique sortie
      if(pin==0 || pin == 3) // lecture seule
      {
        errToCmndResult(data, 12); // ERRM (read only pin)
        return -1;
      }
      pinMode(pins[pin],OUTPUT);
      digitalWrite(pins[pin],(int)value);
      addToCmndResults(data,pin,cmnd,(int)value);
    }
    return 0;
  case 'A':
    if(value==-1 && pins[pin] >= 14)
    {
      v=analogRead(pins[pin]-14);
      addToCmndResults(data,pin,cmnd,v);
      return 0;
    }
    else if(value!=-1 && (pin ==1 || pin == 2))
    {
      pinMode(pins[pin],OUTPUT);
      analogWrite(pins[pin],value & 0x3FF);
      addToCmndResults(data,pin,cmnd,value & 0x3FF);
      return 0;
    }
    else
    {
      errToCmndResult(data, 13); // ERRN (not analog pin)
      return -1;
    }
  case 'P':
      if(pin==0 || pin == 3) // lecture seule
      {
        errToCmndResult(data, 12); // ERRM (read only pin)
        return -1;
      }
      
      if(value<10 && value>32000)
      {
        errToCmndResult(data, 15); // ERRM (value error)
        return -1;
      }
      
      pinMode(pins[pin],OUTPUT);
      digitalWrite(pins[pin], 0);
      pulseUp(pins[pin],(int)value, 0);
      addToCmndResults(data,pin,cmnd,(int)value);
      break;
  default:
    errToCmndResult(data, 14); // ERRO (bad command)
    return -1;
  }
}


int evalCmndString(char *buffer, void *dataPtr)
/**
 * \brief     décode une ligne de commandes et déclenche les traitements associés.
 * \details   le format de la ligne de commandes est le suivant :
 * ##{PIN1},{COMMANDE1},{PARAM1};{PIN2},{COMMANDE2},{PARAM2};...## (aucun espace accepté).
 * avec :
 * PIN       numéro de la sortie/sortie Interface_type_003
 * correspondance avec I/O Arduino (Pin Arduino/Interface_type_003) : 0:4, 1:5, 2:6, 3:7, 4:14(A0), 5:15(A1), 6:16(A2), 7:17(A3), 8:18(A4), 9:19(A5)
 * COMMANDE  action à réaliser :
 * - 'L' : nouvelle valeur logique
 * - 'A' : nouvelle valeur analogique
 * - 'P' : pulse
 * PARAM   parametre de la commande :
 * - 'L' => H (set high), L (set low)
 * - 'A' => entier positif
 * - 'P' => entier positif (durée de l'impulsion en ms)
 * - pour 'L' et 'A' => 'G' = Get value : lecture d'une valeur
 * exemple : ##0,L,H;1,A,10;4:A:G##
 * PIN 0 positionnée à HIGH
 * PIN 1 PWM à 10
 * lecture PIN 4
 * Le résultat de la commande sera :
 * <>
 * \param     buffer    pointeur sur la ligne de commande à analyser
 * \param     dataPtr   données spécifiques au traitement (user data de callback)
 * \return    -1 une erreur c'est produite, le traitement n'a pas été jusqu'au bout mais certaine
 *            commande on pu être passé. 0 sinon
 */
{
  int retour=-1;
  int l=strlen((char *)buffer); // nombre de caractères (encore à traiter)
  struct data_s *data = (struct data_s *)dataPtr;

  if(data->dest==TO_SIM900 && data->lastSMSPhoneNumber[0]==0) // pas de numéro de tel pour la réponse, on fait rien
  {
    errToCmndResult(data, 0); // ERRA : no expeditor
    retour = -1;
    goto evalCmndString_clean_exit;
  }

  if(data->dest==TO_SIM900 && numExist((char *)data->lastSMSPhoneNumber)==-1)
  {
    errToCmndResult(data, 1); // ERRB : expeditor unknown
    retour = -2;
    goto evalCmndString_clean_exit;
  }

  *(data->cmndResultsBufferPtr)=0;

  if(buffer[0]  =='#' &&
    buffer[1]   =='#' &&
    buffer[l-2] =='#' &&
    buffer[l-1] =='#') // une commande (elle doit être encadrée par de "##" ?
  {
    int ptr=2;
    l=l-4; // 4 "#" déjà traité

    while(1)
    {
      int pin = 0;
      char cmnd = 0;
      long valeur = 0;
      int err = 0;
      
      if(!l)
      {
        errToCmndResult(data, 2); // ERRC : cmd size error
        goto evalCmndString_clean_exit;
      }

      // lecture d'un numéro de pin : 1 ou 2 caractères numériques attendus
      if(buffer[ptr]>='0' && buffer[ptr]<='9') // premier caractère numérique ?
      {
        pin=buffer[ptr]-'0';
        ptr++; // passage au caractère suivant
        l--;
      }
      else
      {
        errToCmndResult(data, 3); // ERRD : incorrect pin number
        goto evalCmndString_clean_exit;
      }

      if(!l)
      {
        errToCmndResult(data, 4); // ERRE : cmd size error 
        goto evalCmndString_clean_exit;
      }

      if(buffer[ptr]>='0' && buffer[ptr]<='9') // deuxième caractère numérique ?
      {
        pin=pin *10 + buffer[ptr]-'0';
        ptr++;
        l--;
      }
      else // si deuxième caractère non numérique on doit avoir un ';', traité par la lecture du séparateur
      {
      }

      // lecture du séparateur
      if(!l || buffer[ptr]!=',')
      {
        errToCmndResult(data, 5); // ERRF : syntax error (bad separator)
        goto evalCmndString_clean_exit;
      }
      ptr++;
      l--;
      if(!l)
      {
        errToCmndResult(data, 6); // ERRG : cmd size error
        goto evalCmndString_clean_exit;
      }

      // lecture de la commande
      switch(buffer[ptr])
      {
      case 'L': // I/O logique
      case 'A': // I/O analogique
      case 'P':
        cmnd = buffer[ptr];
        break;
      default:
        errToCmndResult(data, 7); // ERRH
        goto evalCmndString_clean_exit;
        break;
      }
      ptr++;
      l--;

      // lecture du séparateur
      if(!l || buffer[ptr]!=',')
        goto evalCmndString_clean_exit;
      ptr++;
      l--;
      if(!l)
      {
        errToCmndResult(data, 8); // ERRI
        goto evalCmndString_clean_exit;
      }

      // lecture parametre
      if(buffer[ptr]=='H' || buffer[ptr]=='L') // High ou Low
      {
        if(buffer[ptr]=='H')
          valeur=1;
        else
          valeur=0;
        ptr++;
        l--;
      }
      else if(buffer[ptr]=='G') // Get
      {
        valeur=-1;
        ptr++;
        l--;
      }
      else // lecture valeur numérique
      {
        while(1)
        {
          if(l && buffer[ptr]>='0' && buffer[ptr]<='9')
          {
            valeur=valeur *10 + buffer[ptr]-'0';
            ptr++;
            l--;
          }
          else if(!l || buffer[ptr]==';')
          {
            break; // point de sortie "normal" de la boucle
          }
          else
          {
            errToCmndResult(data, 9); // ERRJ
            // a tester ...
            for(;;) // on va jusqu'à la fin de la ligne ou au prochain ";"
            {
               ptr++;
               l--;
               if(!l || buffer[ptr]==';')
                  break;
            }
            err=1;
            break;
          }
        }
      }

      // à partir d'ici on traiter la demande
      if(err==0)
         doCmnd(data, pin, cmnd, valeur);
      // fin de traitement de la demande
      if(l)
      {
        ptr++;
        l--;
      }
      else
        break;
    }
    retour=0;
  }

evalCmndString_clean_exit:
  sendCmndResults(retour, data);

  return retour;
}


int sim900_read()
/**
 * \brief     wrapping de read pour la fonction de lecture de Sim900.
 * \param     car    caractère à écrire
 * \return    toujours 0
 */
{
  int c=sim900Serial.read();
  Serial.print((char)c);
  return c;
}


int sim900_write(char car)
/**
 * \brief     wrapping de write pour la fonction d'écriture de Sim900.
 * \param     car    caractère à écrire
 * \return    toujours 0
 */
{
  sim900Serial.write(car);
  if(char_from_pc_flag==0)
     Serial.print(car);
  return 0;
}


int sim900_available()
{
  /**
   * \brief     wrapping "nb caractère disponible" pour Sim900.
   * \return    nombre de caractères disponibles dans le buffer
   */
  return sim900Serial.available();
}


int sim900_flush()
/**
 * \brief     wrapping "flush" pour Sim900.
 * \return    toujours 0
 */
{
  sim900Serial.flush();
  return 0;
}


inline unsigned long diffMillis(unsigned long chrono, unsigned long now)
/**
 * \brief     différence entre chrono et now (now - chrono) en tenant compte
 *            du repassage éventuel par 0.
 * \return    resultat de now - chrono
 */
{
  return now >= chrono ? now - chrono : 0xFFFFFFFF - chrono + now;
}


void sim900_broadcastSMS(char *text)
/**
 * \brief     envoie 1 message à tous les numéros du carnet d'adresse
 * \param     text   message à envoyer.           
 * \return    resultat de now - chrono
 */
{
  static unsigned long last_broadcast=0;

  unsigned long now = millis();
  if(now==0) now=1; // jusque parceque j'ai besoin du 0.

  if(nb_broadcast_credit<=0)
     return;
  
  if(last_broadcast!=0 && diffMillis(last_broadcast, now) < 60000) // pour limiter les broadcasts, max 1 par minute
     return;
  
  last_broadcast=millis();

  char num[NUMSIZE];
  for(int i=0;i<10;i++)
  {
    for(int j=0;j<NUMSIZE;j++)
    {
      char c=EEPROM.read(EEPROM_ADDR_NUM+i*NUMSIZE+j);
      if(c==0)
      {
        num[j]=0;
        break;
      }
      else
        num[j]=c;
    }

    if(num[0])
    {
      Serial.print(num);
      Serial.print(':');
      Serial.println(text);

      sim900.sendSMS(num, text);
      
      nb_broadcast_credit--;
    }
  }
}


void pinsWatcher()
/**
 * \brief     surveille les changements d'état des lignes "POWER" et "ALARM" et réagit en concéquence.
 * \details   la surveillance gère un "anti-rebond" sur les deux lignes.
 *            dès qu'un frond (montant ou décendant) est validés un message SMS est envoyé.
 */
{
  char send_power=0;
  char send_alarm=0;
  
  for(int i=0;i<PINSWATCHER_NBPINS;i++)
  {
    if(getPinDirection(pinsWatcherData[i].pin)==0)
       continue;

    int s=digitalRead(pinsWatcherData[i].pin); // lecture de l'entrée
    if(pinsWatcherData[i].lastState != (char)s)
    {
      if(pinsWatcherData[i].lastState==-1) // premier passage, on initialise
      {
        pinsWatcherData[i].lastState=s;
        break;
      }

      unsigned long now=millis();
      if(now == 0)
        now=1; // ok, on perd 1 ms si on tombe pile poil sur 0 ... j'ai trop besoin du 0 ...

      if(pinsWatcherData[i].lastChrono==0) // front detecté
        pinsWatcherData[i].lastChrono=now; // on démarre le chrono
      else if(diffMillis(pinsWatcherData[i].lastChrono, now) > 20) // plus de 20 ms depuis le front ?
      { //oui : on prend en compte
        pinsWatcherData[i].lastState=s;
        pinsWatcherData[i].lastChrono=0;
        // faire ici ce qu'il y a a faire avec ce nouvel etat
        // à revoir pour rendre plus asynchrone ...
        switch(i)
        {
        case 0:
          if(s==LOW) // front descendant
            send_power=1;
          else // front montant
            send_power=2;
          break;
        case 3:
          if(s==LOW)
            send_alarm=1;
          else
            send_alarm=2;
          break;
        default:
          Serial.print('$');
          Serial.print('$');
          Serial.print(interfacePinNum(pinsWatcherData[i].pin));
          Serial.print(',');
          Serial.print('L');
          Serial.print(',');
          if(s==LOW)
             Serial.print('L');
          else
             Serial.print('H');
          Serial.print('$');
          Serial.print('$');
          Serial.print('\n');
          break;
        };
        // fin partie à revoir
      }
    }
    else
      pinsWatcherData[i].lastChrono=0;
  }
  
  if(send_power==1)
  {
     sim900_broadcastSMS((char *)"POWERDOWN");
     printStringFromProgmem(powerdown_str);
  }
  else if(send_power==2)
  {
     sim900_broadcastSMS((char *)"POWERUP");
     printStringFromProgmem(powerup_str);
  }

  if(send_alarm==1)
  {
     sim900_broadcastSMS((char *)"ALARMEOFF");
     printStringFromProgmem(alarmoff_str);
  }
  else if(send_alarm==2)
  {
     sim900_broadcastSMS((char *)"ALARMEON");
     printStringFromProgmem(alarmon_str);
  }
}


#define MCU_DONE      0
#define MCU_ERROR     1
#define MCU_ROMERROR  2
#define MCU_CMNDERROR 3
#define MCU_PROMPT    4
#define MCU_OVERFLOW  5

prog_char mcu_error[]     PROGMEM  = { 
  "SYNTAX ERROR" };
prog_char mcu_done[]      PROGMEM  = { 
  "DONE" };
prog_char mcu_romerror[]  PROGMEM  = { 
  "ROM ERROR" };
prog_char mcu_cmnderror[] PROGMEM  = { 
  "CMND ERROR" };
prog_char mcu_prompt[]    PROGMEM  = { 
  "> " };
prog_char mcu_overflow[]  PROGMEM  = { 
  "OVERFLOW" };

int mcuError(int errno)
{
  prog_char *str;
  switch(errno)
  {
  case MCU_ERROR:
    str = mcu_error;
    break;
  case MCU_DONE:
    str = mcu_done;
    break;
  case MCU_ROMERROR:
    str = mcu_romerror;
    break;
  case MCU_CMNDERROR:
    str = mcu_cmnderror;
    break;
  case MCU_PROMPT:
    str = mcu_prompt;
    break;
  case MCU_OVERFLOW:
    str = mcu_overflow;
    break;
  default:
    break;
  }

  int i=0;
  while(1)
  {
    char c =  pgm_read_byte_near(str + i);
    if(c)
      Serial.write(c);
    else
    {
      if(errno==MCU_PROMPT)
        break;
      else
      {
        Serial.println("");
        break;
      }
    }
    i++;
  }
  return 0;
}


// attention, ne marche pas avec le bootloader standard de l'arduino
#define soft_reset()        \
do                          \
{                           \
    wdt_enable(WDTO_15MS);  \
    for(;;)                 \
    {                       \
    }                       \
} while(0)


int analyseMCUCmnd(char *buffer, struct data_s *data)
{
  // les commandes :
  // PIN:NNNNNN - stockage du code pin de la sim en ROM (ex : PIN:1234)
  // PIN:C - efface le code pin
  // NUM:x,nnnnnnnnnnnnnnnnnnn - ajoute un numero de telephone dans l'annuaire du MCU (10 positions disponibles, de 0 à 9), 20 caractères maximum
  // NUM:x,C - efface un numéro
  // CMD:##...## - voir evalCmndString - demande l'execution de commandes
  // LST - list le contenu de la ROM
  // RST - reset
  // BCR - réinitialisation du nombre de credit de broadcast
  // END - sortie du mode commande demandé par le PC
  Serial.println(""); // saut de ligne après réception d'une commande
  
  if(buffer[0]==0) // rien à traiter
    return 0;

  if(strstr((char *)buffer,"PIN:")==(char *)buffer) // la ligne commande par "PIN:"
  {
    // "PIN:" avec quel parametre ?
    if(buffer[4]=='C' && buffer[5]==0) // C : clear
    {
      int ret = clearPin();
      if(ret<0)
        mcuError(MCU_ROMERROR);
      else
        mcuError(MCU_DONE);
      return ret;
    }

    // PIN:NNNNNN ?
    char pinCode[9];
    int i;
    for(i=0;i<8 && buffer[i+4];i++)
    {
      if(buffer[i+4] >= '0' && buffer[i+4] <='9')
        pinCode[i]=buffer[i+4];
      else
      {
        mcuError(MCU_ERROR);
        return -1; // syntax error
      }
    }
    pinCode[i]=0;
    // stocker le code en ROM
    setPin(pinCode);
    mcuError(MCU_DONE);
    return 0;
  }

  if(strstr((char *)buffer,"NUM:")==(char *)buffer)
  {
    char num[21];
    int x=buffer[4]-'0';
    if(x>=0 && x<=9)
    { 
      if(buffer[5]==',')
      {
        if(buffer[6]=='C' && buffer[7]==0)
        {
          int ret;
          ret = setNum(x, (char *)"");
          if(ret<0)
            mcuError(MCU_ROMERROR);
          else
            mcuError(MCU_DONE);
          return ret;
        }

        int i;
        for(i=0;i<(NUMSIZE-1) && buffer[i+6];i++)
          num[i]=buffer[i+6];
        num[i]=0;
        // stocker le numero à la position x en ROM
        int ret=setNum(x, num);
        if(ret<0)
          mcuError(MCU_ROMERROR);
        else
          mcuError(MCU_DONE);
        return ret;
      }
      else
      {
        mcuError(MCU_ERROR);
        return -1;
      }
    }
    else
    {
      mcuError(MCU_ERROR);
      return -1;
    }
  }

  if(strstr((char *)buffer,"CMD:")==(char *)buffer) // c'est une commande I/O
  {
    int ret = evalCmndString((char *)&(buffer[4]), (void *)data);
    return ret;
  }

  if(strstr((char *)buffer,"LST")==(char *)buffer)
  {
    int ret = listRom();
    if(ret<0)
      mcuError(MCU_ROMERROR);
    else
      mcuError(MCU_DONE);
    return ret;
  }

  if(strstr((char *)buffer,"BCR")==(char *)buffer) // reset le nombre de broadcast disponible
  {
     nb_broadcast_credit=BCAST_CREDIT;
     return 0;
  }

  if(strstr((char *)buffer,"RST")==(char *)buffer)
  {
//    soft_reset();
    return 0;
  }

  if(strstr((char *)buffer,"END")==(char *)buffer)
  {
    soft_cmd_mode_flag = 0;
    return 1;
  }

  mcuError(MCU_ERROR);
  return -1;
}


int processCmndFromSerial(unsigned char car, struct data_s *data)
{
  char c=toupper(car);

  if(c == 13) // fin de ligne, on la traite
  {
    mcuSerialBuffer[mcuSerialBufferPtr]=0;
    int ret=analyseMCUCmnd((char *)mcuSerialBuffer, data);
    mcuSerialBufferPtr=0; // RAZ du buffer
    if(ret!=1)
       mcuError(MCU_PROMPT);
  }
  else if(c == 10) // on n'a rien à faire de LF
  {
  }
  else // on range la donnée dans le buffer de ligne
  {
    Serial.write((char)c);
    mcuSerialBuffer[mcuSerialBufferPtr]=(char)c;
    mcuSerialBufferPtr++;
    if(mcuSerialBufferPtr >= MCUSERIALBUFFERSIZE) // arg, plus de place dans le buffer ...
    {
      mcuSerialBufferPtr = 0;
      mcuError(MCU_OVERFLOW);
      return -1;
    }
  }
  return 0;
}


unsigned long signal_timer=0;

void setup()
{
  // initialisation port de communication Serie
  Serial.begin(57600);
  printStringFromProgmem(starting_str);

  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);

  // initialisation EEPROM en cas de besoin  
  if(checkSgn()<0) // premier démarrage ?
  {
    setPin((char *)"");
    for(int i=0;i<10;i++)
       setNum(i,(char *)"");
    setSgn();
  }

  // attente (5s) pour initialisation complete du sim900 après reset
  unsigned long chrono=millis();
  
  // initialisation buffer partagé
  cmndResultsBufferPtr=0;
  cmndResultsBuffer[0]=0;
  
  mcuUserData.dest=TO_MCU;
  mcuUserData.cmndResultsBuffer=cmndResultsBuffer;
  mcuUserData.cmndResultsBufferPtr=&cmndResultsBufferPtr;
  mcuUserData.lastSMSPhoneNumber=NULL;
  
  sim900UserData.dest=TO_SIM900;
  sim900UserData.cmndResultsBuffer=cmndResultsBuffer;
  sim900UserData.cmndResultsBufferPtr=&cmndResultsBufferPtr;
  sim900UserData.lastSMSPhoneNumber=sim900.getLastSMSPhoneNumber();
  
  // association sim900 avec le port de com.
  sim900.setRead(sim900_read);
  sim900.setWrite(sim900_write);
  sim900.setAvailable(sim900_available);
  sim900.setFlush(sim900_flush);

  // déclaration des données utilisateur à passer aux callbacks
  sim900.setUserData((void *)&sim900UserData);

  // déclaration des callbacks
  sim900.setSMSCallBack(evalCmndString);

  char i=0;
  char init_done=-1;

  while(i<3) // trois chance de s'initialiser
  {
  // Reset "hardware" du sim900
     pinMode(PIN_SIM900_RESET, OUTPUT);
     digitalWrite(PIN_SIM900_RESET, HIGH);
     delay(100);
     digitalWrite(PIN_SIM900_RESET, LOW);
     delay(1000);
     digitalWrite(PIN_SIM900_RESET, HIGH);

     // on attend que 5s se soit écoulée depuis la fin de la demande de reset en faisant cligoter rapidement la led
     myBlinkLeds.setInterval(125);
     while( (millis()-chrono) < 5000 )
     {
        myBlinkLeds.run();
        digitalWrite(13, myBlinkLeds.getLedState());
     }
     digitalWrite(13, HIGH); // led allumée en continue pendant l'init de communication avec sim900

     // intialisation port communication avec sim900
     sim900Serial.begin(9600);

     // synchronisation avec le sim900
     if(sim900.sync(5000)!=-1) // 5 secondes pour se synchroniser avec le sim900
     {
        init_done=0;
        break;
     }
     i++;
  }
 
  if(init_done==0)
  { 
    // récupération du code PIN dans l'EEPROM
    char pinCode[PINCODESIZE];
    if(getPin(pinCode, PINCODESIZE)==0)
      sim900.setPinCode(pinCode);

    sim900.init(); // préparation "standard" du sim900

    delay(2500);

//    sim900_broadcastSMS("STARTUP"); // information démarrage envoyé par SMS
    nb_broadcast_credit=BCAST_CREDIT;

    sim900_connected_flag = 1;
    printStringFromProgmem(syncok_str);
    myBlinkLeds.setInterval(1000); // cligontement lent dans la boucle principale => mode normal
  }
  else
  {
    // sim900 pas détecté
    sim900_connected_flag = 0;
    printStringFromProgmem(syncko_str);
    myBlinkLeds.setInterval(125); // clignotement rapide dans la boucle principale => pas de communication
  }
  digitalWrite(13, LOW); // initialisation terminée, led éteinte

  // préparation de I/O
  pinMode(PIN_MCU_CMD_ONLY, INPUT); // selection mode commande
  digitalWrite(PIN_MCU_CMD_ONLY, HIGH); // pullup activé, pas de mode commande
  pinMode(PIN_SIM900_ENABLE, INPUT); // selection activation sim900
  digitalWrite(PIN_SIM900_ENABLE, HIGH); // pullup activé, communication vers sim900 activé
  pinMode(PIN_MCU_BYPASS_PROCESSING, INPUT); // selection traitement des données par MCU
  digitalWrite(PIN_MCU_BYPASS_PROCESSING, HIGH); // pullup activé, le MCU traite les données du sim900

  // pour la gestion des impulsions
  init_pulses();

  // entrée d'alarme
  pinMode(4, INPUT); // detection de tension
  digitalWrite(4, HIGH); // pullup activé
  pinMode(7, INPUT); // déclenchement alarme
  digitalWrite(7, HIGH); // pullup activé

  // autres entrées
  pinMode(5, INPUT);
  digitalWrite(5, HIGH);
  pinMode(6, INPUT);
  digitalWrite(6, HIGH);
  pinMode(14, INPUT);
  digitalWrite(14, HIGH);
  pinMode(15, INPUT);
  digitalWrite(15, HIGH);
  pinMode(16, INPUT);
  digitalWrite(16, HIGH);
  pinMode(17, INPUT);
  digitalWrite(17, HIGH);
  pinMode(18, INPUT);
  digitalWrite(18, HIGH);
  pinMode(19, INPUT);
  digitalWrite(19, HIGH);

  FREERAM;
}


void loop()
{
  myBlinkLeds.run();
  pulses();
  
  if(sim900_connected_flag == 0 || sim900.getSMSInFlag()==0)
    pinsWatcher();

  if(digitalRead(PIN_MCU_BYPASS_PROCESSING)==HIGH)
     sim900.MCUAnalyseOn();
  else
     sim900.MCUAnalyseOff();

  if(sim900_connected_flag)
    sim900.connectionCheck();
  
  if(sim900_connected_flag)
    sim900.run();

  digitalWrite(13, myBlinkLeds.getLedState()); // clignotement de la led "activité" (D13) de l'ATmega

  if(Serial.available())
  {
    unsigned char serialInByte = (unsigned char)Serial.read();

    // On est en mode commande si :
    if(   !sim900_connected_flag                   // pas de sim900 détecté
       || digitalRead(PIN_MCU_CMD_ONLY)==LOW  // PIN MCU_CMD_ONLY est à bas
       || soft_cmd_mode_flag == 1)                 // on est passé en mode commande avec la séquence "~~~~" précédemment
    {
      myBlinkLeds.setInterval(125); // en mode commande on clignote plus vite
      processCmndFromSerial(serialInByte, &mcuUserData);
    }
    else // Sinon on transmet au sim900 mais on vérifie qu'on à pas un séquence "~~~~"
    {
      unsigned char buff[4]; // mini buffer
      char buff_ptr=1;
      buff[0]=serialInByte;
      
      myBlinkLeds.setInterval(1000); // hors mode commande on clignote moins vite
      if(serialInByte=='~') // premier '~' d'une serie de 4 ?
      {
         soft_cmd_mode_flag=1; // on est potentiellement en situation de passer en mode commande
         for(char i=0;i<3;i++) // on attend la suite de la serie (encore 3)
         {
            // attente du caractère suivant avec timeout
            char timeout_flag=0;
            unsigned long chrono = millis();
            for(;;)
            {
               if(Serial.available())
                  break;
               if(diffMillis(chrono, millis())>125)
               {
                  timeout_flag=1;
                  break;
               }
            }

            if(timeout_flag==0) // caractère reçu et pas de timeout
            {
               buff[buff_ptr]=(unsigned char)Serial.read();
               if(buff[buff_ptr++]!='~')
               {
                  soft_cmd_mode_flag=0;
                  break;
               }
            }
            else
              break; // timeout => on sort de la boucle d'attente des caractères de mode commande
         }
      }

      if(soft_cmd_mode_flag==0) // on n'est pas passer en mode commande
      {
        // on vide le mini buffer vers le sim900.
        char_from_pc_flag=1; // les données viennent du PC => pas d'echo
        for(char i=0;i<buff_ptr;i++)
           sim900.write(buff[i]); // toutes les données de la ligne serie sont envoyées vers le sim900
        char_from_pc_flag=0; // fin des données du PC
      }
    }
  }
}

