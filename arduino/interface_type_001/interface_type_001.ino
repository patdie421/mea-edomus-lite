#include <Arduino.h>
#include <AltSoftSerial.h>

#include <InterfaceCompteurERDF.h>
#include <Comio2.h>
#include <Pulses.h>

// #define COUNTERS_SIMULATOR 1

// utilisation des ports (ARDUINO UNO) Dans ce sketch
/* PORTD
 * PIN  0 : PC(TX)      [RX]
 * PIN  1 : PC(RX)      [TX]
 * PIN  2 : CNTR0       [INT]
 * PIN  3 : CNTR1       [INT / PWM]
 * PIN  4 : E1
 * PIN  5 : S1          [PWM]
 * PIN  6 : CNTR_SEL    [PWM]
 * PIN  7 : CNTR_ACT
 *
 * PORDB
 * PIN  8 : CNTR_RX
 * PIN  9 : CNTR_TX     [PWM]
 * PIN 10 : E2          [PWM / SS] - pas utilisable en PWM Ã  cause de ALTSOFTSERIAL
 * PIN 11 : S2          [PWM / MOSI] 
 * PIN 12 : E3          [MISO]
 * PIN 13 : LED1        [LED / SCK]
 * PIN NA : -
 * PIN NA : -
 *
 * PORTC
 * PIN A0 : (RELAI1/S)
 * PIN A1 : (RELAI2/S)
 * PIN A2 : (RELAI3/S)
 * PIN A3 : (TEMPERATURE/E)
 * PIN A4 : (I2C)
 * PIN A5 : (I2C)
 * PIN A6*: - PAS DISPO SUR UNO
 * PIN A7*: - PAS DISPO SUR UNO
 */

#define ENABLE_INT0_D2  EIMSK |=  (1 << INT0)  // enable interruption INT0 (PIN2)
#define DISABLE_INT0_D2 EIMSK &= ~(1 << INT0)  // disable interruption INT0 (PIN2)
#define ENABLE_INT1_D3  EIMSK |=  (1 << INT1)  // enable interruption INT1 (PIN3)
#define DISABLE_INT1_D3 EIMSK &= ~(1 << INT1)  // disable interruption INT1 (PIN3)

#define INTERVAL 50 // pulse must be longer than 50 milliseconds

// adresse COMIO
#define CNTR0_TRAP    1
#define CNTR1_TRAP    2

#define CNTR0_DATA0   0
#define CNTR0_DATA1   1
#define CNTR0_DATA2   2
#define CNTR0_DATA3   3

#define CNTR1_DATA0  10
#define CNTR1_DATA1  11
#define CNTR1_DATA2  12
#define CNTR1_DATA3  13

#define TMP36_DATA0  6
#define TMP36_DATA1  7

#define SELECTION_PIN  7
#define ACTIVATION_PIN 6

#define LED_DELAY    200


Comio2 comio2;
unsigned char *comio_mem=NULL; // pour accès direct à la mémoire comio2

// spécifique pour TMP36
#define TMP36_NB_READ 10000
int _tmp36=-1;

// Objet d'accès au compteur
InterfaceCompteurERDF compteurs(ACTIVATION_PIN, SELECTION_PIN, myReadFn, myAvailableFn);

#define COUNTERS_READ_INTERVAL 1000*20 // 1 lecture toutes les 20 secondes
typedef struct var_s
{
  char *label;
  unsigned char numCntr;
  unsigned char addr;
} 
var_t;


#define NB_VARS 2
char *base="BASE";
var_t vars[]={
  { base, 0, CNTR0_DATA0 },
  { base, 1, CNTR1_DATA0 }
};


/* pour un clignotement de led synchronisé */
class BlinkLeds
{  
public:
  BlinkLeds(unsigned int i);
  void run();
  int getLedState() {
    return ledState;
  }
private:
  int interval;
  int ledState;
  unsigned long previousMillis;
};


/*
 * Création d'un clignotement de led
 */
BlinkLeds::BlinkLeds(unsigned int i)
{
  interval = i;
  ledState = LOW;
  previousMillis = 0;
}
 
 
/*
 * mis à jour de l'état d'un clignotement en fonction du temps passé
 */
void BlinkLeds::run()
{
  unsigned long currentMillis = millis();
  if(my_diff_millis(previousMillis, currentMillis) > interval) {
    previousMillis = currentMillis;
    ledState = !ledState;
  }
}

int comio2_serial_read()
{
   return Serial.read();
}

int comio2_serial_write(char car)
{
   Serial.write(car);
   return 0;
}

int comio2_serial_available()
{
   return Serial.available();
}

int comio2_serial_flush()
{
   Serial.flush();
   return 0;
}

/******************************************************************************************/
/* Fonctions "utilitaires"                                                                */
/******************************************************************************************/
void long_to_array(unsigned long val, unsigned char data[])
{
  for(int i=0;i<4;i++)
  {
    data[i]= val & 0x000000FF;
    val = val >> 8;
  }
}

inline unsigned long my_diff_millis(unsigned long chrono, unsigned long now)
{
  return now >= chrono ? now - chrono : 0xFFFFFFFF - chrono + now;
}


/******************************************************************************************/
/* Préparation liaison série avec le compteur                                             */
/******************************************************************************************/
AltSoftSerial counterSerial;

// callback de test caractère disponible pour InterfaceCompteurERDF
int myAvailableFn()
{
  return counterSerial.available();
}

// callback de lecture pour InterfaceCompteurERDF
int myReadFn()
{
  return counterSerial.read();
}


/******************************************************************************************/
/* Gestion des compteurs d'impulsions                                                     */
/******************************************************************************************/

/*
 * Les gestionnaires d'interruptions :
 * Pour le traitement des interruptions, la ressource à économiser est le "temps". On fait donc en sorte de prendre le moins possible
 * de cycle CPU. Pour cela, on limite le nombre d'instructions et surtout les appels de fonctions. Il y a donc 2 gestionnaires d'
 * interruptions presque identiques (seul les variables changes) et volontairement nous ne cherchons pas Ã  factoriser pour Ã©viter de
 * rajouter des cycles (sauvegarde contexte, mise en pile des variables, ... typique des appels de fonctions nÃ©cessaire Ã  cette factorisation).
 */

//
// LES COMPTEURS D'IMPULSION
//

// COMPTEUR0
#define COUNTER_PIN0 2 // le compteur 0 est sur l'entrée 2
volatile unsigned long counter0; // compteur du nombre d'impulsion
volatile unsigned long prev_chrono0; // dernière prise de chrono
volatile unsigned char prev_state_pin0; // état précédent de l'entrée 2
volatile unsigned char counter_flag0;

void counter0_inter()
{
  char pin_state = PIND & (1 << COUNTER_PIN0); // avant tout, lecture de l'état de l'entrée sans digitalRead qui prend trop de temps
  unsigned long chrono=millis(); // maintenant qu'on connait l'état de l'entrée on peut prendre le "temps" de lire le temps actuel en millisecondes

  if(pin_state > prev_state_pin0) // detection d'un front montant
  {
    prev_chrono0=chrono; // on prend le chrono
  }
  else if(pin_state < prev_state_pin0) // detection d'un front descendant
  {
    if((chrono - prev_chrono0) > INTERVAL) // impulsion suffisament long ?
    {
      counter0++;
      counter_flag0=1;
    }
    // si le temps entre les deux fronts n'est pas suffisant, on considÃ¨re que c'est du bruit.
  }
  // si pin_state == prev_state_pin0 on ne fait rien

  prev_state_pin0=pin_state;
}

// COMPTEUR1
#define COUNTER_PIN1 3 // le compteur 1 est sur l'entrée 3
volatile unsigned long counter1; // compteur du nombre d'impulsion
volatile unsigned long prev_chrono1; // dernière prise de chrono
volatile unsigned char prev_state_pin1; // etat précédente de l'entrée 3
volatile unsigned char counter_flag1;

void counter1_inter()
{
  char pin_state = PIND & (1 << COUNTER_PIN1); // avant tout, lecture de l'état de l'entrée sans digitalRead qui prend trop de temps
  unsigned long chrono=millis(); // maintenant qu'on connait l'état de l'entrée on peut prendre le "temps" de lire le temps actuel en millisecondes

  if(pin_state > prev_state_pin1) // detection d'un front montant
  {
    prev_chrono1=chrono; // on prend le chrono
  }
  else if(pin_state < prev_state_pin1) // detection d'un front descendant
  {
    if((chrono - prev_chrono1) > INTERVAL) // impulsion suffisament long ?
    {
      counter1++;
      counter_flag1=1;
    }
    // si le temps entre les deux fronts n'est pas suffisant, on considère que c'est du bruit.
    // et on ne fait rien
  }
  // si pin_state == prev_state_pin0 on ne fait rien, mais cela ne devrait jamais arriver ...

  prev_state_pin1=pin_state;
}


/******************************************************************************************/
/* Gestion des compteurs E(R)DF                                                           */
/******************************************************************************************/
#ifdef COUNTERS_SIMULATOR

unsigned long simulation_counter0=1000
unsigned long simulation_counter1=100

void process_simulator()
{
  long c=random(100)
  if(c<10 && !counter_flag0) {
     counter_flag0=1;
     simulation_counter0++;
  }
  long c=random(100)
  if(c<5 && !counter_flag1) {
     counter_flag1=1;
     simulation_counter1++;
  }
}


void read_counters()
{
  long_to_array(simulation_counter0 / 1000, (unsigned char *)&(comio_mem[vars[0].addr]));
  long_to_array(simulation_counter1 / 1000, (unsigned char *)&(comio_mem[vars[1].addr]));
}

void process_counters()
{
  unsigned char flag;

  process_simulator();

  // récupération des valeurs du comptage d'impulsions "LED" des compteurs et émission de trap
  flag=counter_flag0;
  counter_flag0=0;
  if(flag)
     comio2.sendTrap(CNTR0_TRAP, NULL, 0);

  flag=counter_flag1;
  counter_flag1=0;
  if(flag)
     comio2.sendTrap(CNTR1_TRAP, NULL, 0);

  read_counters();
}

#else

void read_counters()
{
  static unsigned long chrono = 0;
  static unsigned char current_counter=0;

  if(my_diff_millis(chrono, millis())>COUNTERS_READ_INTERVAL)
  {
    if(compteurs.isIdle())
      compteurs.startRead(vars[current_counter].label, vars[current_counter].numCntr);

    if(compteurs.isDataAvailable() || compteurs.isError())
    {
      if(compteurs.isDataAvailable())
      {
        char buff[12];

        memset(buff,0,sizeof(buff));
        compteurs.getStrVal(buff,sizeof(buff));
        long_to_array(atol(buff), (unsigned char *)&(comio_mem[vars[current_counter].addr]));
        // Serial.println(buff);
      }
      else
      {
        // error : on signal l'erreur de lecture (clignotement des LED ?)
      }
      compteurs.reset(); // préparation pour une nouvelle lecture

      if(++current_counter == NB_VARS) // passage au compteur suivant si nécessaire
      { // on à traiter les deux compteurs
        current_counter=0; // au recommencera au début
        chrono=millis(); // on réinitialise le chrono, prochaine lecture dans COUNTERS_READ_INTERVAL milli-secondes
      }
    }
  }
}


void process_counters()
{
  unsigned char flag;

  // récupération des valeurs du comptage d'impulsions "LED" des compteurs et emission de trap
  DISABLE_INT0_D2;
  flag=counter_flag0; // on récupère le flag
  counter_flag0=0; // quelque soit la valeur on remet à 0. Ca coute moins cher en CPU que de faire un test et en fonction de mettre à 0.
  ENABLE_INT0_D2;
  if(flag) // une interruption pour le compteur 0
     comio2.sendTrap(CNTR0_TRAP, NULL, 0); // Emission d'un TRAP pour un traitement "temps réel" éventuel par le PC

  DISABLE_INT1_D3;
  flag=counter_flag1; // on récupère le flag
  counter_flag1=0; // quelque soit la valeur on remet à 0. Ca coute moins cher en CPU que de faire un test et en fonction de mettre à 0.
  ENABLE_INT1_D3;
  if(flag) // une interruption pour le compteur 1
     comio2.sendTrap(CNTR1_TRAP, NULL, 0);

  // lecture des info du compteur ERDF
  read_counters();
}

#endif


/******************************************************************************************/
/* Gestion des sorties                                                                    */
/******************************************************************************************/

//
// fonction COMIO : positionne la sortie logique spécifiée à la valeur spécifiée.
// cette fonction test la pertinance de la demande.
// Sortie disponible : 5, 11, 13, A0, A1, A2
//
int comio2_fn_output_onoff(int id, char *data, int l_data, char *res, int *l_res, void *userdata)
{
  unsigned char pin=(unsigned char)data[0];
  unsigned char state=(unsigned char)data[1];

  // controler ici si pin OK en fonction de l'arduino courant
  switch(pin)
  {
  case A0:
  case A1:
  case A2:
  case  5:
  case 11:
  case 13:
    if(state)
      digitalWrite(pin,HIGH);
    else
      digitalWrite(pin,LOW);
    return 0;
  default:
    return -1;
  }
}

//
// fonction COMIO : déclanche une impulsion de durée x en centieme de seconde (entre 1 et 255 centieme de seconde)
// cette fonction test la pertinance de la demande.
// Sortie disponible : 5, 11, 13, A0, A1, A2
//
int comio2_fn_output_pulse_csec(int id, char *data, int l_data, char *res, int *l_res, void *userdata)
{
  unsigned char pin=(unsigned char)data[0];
  unsigned char nb_msec=(unsigned char)data[1];

  // controler ici si pin OK en fonction de l'arduino courant
  switch(pin)
  {
  case A0:
  case A1:
  case A2:
  case  5:
  case 11:
  case 13:
    pulseUp(pin, nb_msec * 100, 0);
    return 0;
  default:
    return -1;
  }
}


//
// fonction COMIO : sortie "analogique" spécifiée positionnée à la valeur spécifiée
// cette fonction test la pertinance de la demande.
// Sortie disponible : 5, 11
//
int comio2_fn_output_pwm(int id, char *data, int l_data, char *res, int *l_res, void *userdata)
{
  unsigned char pin=(unsigned char)data[0];
  unsigned char pwm_val=(unsigned char)data[1];

  // controler ici si pin OK en fonction de l'arduino courant
  switch(pin)
  {
  case  5:
  case 11:
    analogWrite(pin,pwm_val);
    return 0;
  default:
    return -1;
  }
}


/******************************************************************************************/
/* Gestion des entrees                                                                    */
/******************************************************************************************/

//
// fonction COMIO : retourne la valeur de l'entrée spécifiée
// cette fonction test la pertinance de la demande.
// Sortie disponible : 4, 10, 12
//
int comio2_fn_return_digital_in(int id, char *data, int l_data, char *res, int *l_res, void *userdata)
{
  unsigned char pin=(unsigned char)data[0];

  switch(pin)
  {
  case 4:
  case 10:
  case 12:
    return digitalRead(pin);
  default:
    return -1;
  }
}


//
// fonction COMIO : retourne la valeur analogique de l'entrée spécifiée
// cette fonction test la pertinance de la demande.
// Sortie disponible : A3
//
int comio2_fn_return_analog_in(int id, char *data, int l_data, char *res, int *l_res, void *userdata)
{
  unsigned char pin=(unsigned char)data[0];

  switch(pin)
  {
  case A3:
    return analogRead(pin);
  default:
    return -1;
  }
}


//
// fonction COMIO : traitement spécifique d'un capteur TMP36 connecté sur A3
//
int comio2_fn_return_tmp36(int id, char *data, int l_data, char *res, int *l_res, void *userdata)
{
  unsigned char pin=(unsigned char)data[0];

  switch(pin)
  {
  case A3:
    return _tmp36;
  default:
    return -1;
  }
}


/******************************************************************************************/
/* Gestion spécifiques                                                                    */
/******************************************************************************************/

//
// traitement spécifique pour un capteur de type TMP36
// lecture de 10000 valeurs successives et calcul de la moyenne
//
void process_tmp36()
{
  static int i=0;
  static float average=0;

  average = average + analogRead(A3);
  i++;
  if(i==TMP36_NB_READ)
  {
    _tmp36=average/TMP36_NB_READ;
    average=0;
    i=0;
  }
}


//
// Surveillance des entrées logiques
// emission d'un trap comio si la valeur d'une entrée change
//
void process_digital_in_change_dectection()
{
  static char pin_num[3]={
    4,10,12  };
  static char pin_prev_state[3]={
    0,0,0  };

  for(int i=0;i<sizeof(pin_num);i++)
  {
    char pin_state=digitalRead(pin_num[i]);

    if(pin_prev_state[i]!=pin_state)
       comio2.sendTrap(pin_num[i]+10, &pin_state, 1);

    pin_prev_state[i]=pin_state;
  }
}


/******************************************************************************************/
/* initialisation                                                                         */
/******************************************************************************************/

void setup()
{
  counterSerial.begin(1200);

  // initialisation des I/O
  pinMode(A0,OUTPUT);
  pinMode(A1,OUTPUT);
  pinMode(A2,OUTPUT);
  pinMode( 5,OUTPUT);
  pinMode(11,OUTPUT);
  pinMode(13,OUTPUT);

  pinMode(A3, INPUT);

  pinMode( 4, INPUT);
  digitalWrite(4, HIGH);
  pinMode(10, INPUT);
  digitalWrite(10, HIGH);
  pinMode(12, INPUT);
  digitalWrite(12, HIGH);

  analogReference(INTERNAL); // référence 1,1 V interne pour plus de précision sur TMP36
  analogRead(A3); // activation du convertisseur analogique / numérique

  // signalisation
  pinMode(13, OUTPUT);

  // initialisation du gestionnaire d'impulsions
  init_pulses();

  // initialisation COMIO
  Serial.begin(57600);
//  Serial.begin(9600);

  comio2.setReadF(comio2_serial_read);
  comio2.setWriteF(comio2_serial_write);
  comio2.setAvailableF(comio2_serial_available);
  comio2.setFlushF(comio2_serial_flush);

  comio2.setFunction(0,comio2_fn_output_pulse_csec); // impulsion 'ON' de x centiemes de seconde
  comio2.setFunction(1,comio2_fn_output_onoff); // sorties logiques
  comio2.setFunction(2,comio2_fn_output_pwm); // sorties analogiques
  comio2.setFunction(6,comio2_fn_return_digital_in); // lecture entrées logiques
  comio2.setFunction(7,comio2_fn_return_analog_in); // lecture entrées analogiques
  comio2.setFunction(5,comio2_fn_return_tmp36); // lecture TMP36  

  comio_mem=comio2.getMemoryPtr();

  // initialisation du gestionnaire d'interruption du compteur 0
  long_to_array(0, &(comio_mem[CNTR0_DATA0])); // ou memset(&(comio_mem[CNTR0_DATA0]),0,4);
  prev_chrono0=0;
  pinMode(COUNTER_PIN0, INPUT);
  digitalWrite(COUNTER_PIN0, HIGH); // activation rÃ©sistance de pull-up
  prev_state_pin0=PIND & (1 << COUNTER_PIN0);
  if(prev_state_pin0>0)
    prev_chrono0=millis();
  counter_flag0=0;
  attachInterrupt(INT0,counter0_inter, CHANGE);

  // initialisation du gestionnaire d'interruption du compteur 1
  long_to_array(0, &(comio_mem[CNTR1_DATA0])); // ou memset(&(comio_mem[CNTR1_DATA0]),0,4);
  prev_chrono1=0;
  pinMode(COUNTER_PIN1, INPUT);
  digitalWrite(COUNTER_PIN1, HIGH); // activation rÃ©sistance de pull-up
  prev_state_pin1=PIND & (1 << COUNTER_PIN1);
  if(prev_state_pin1>0)
    prev_chrono0=millis();
  counter_flag1=0;
  attachInterrupt(INT1,counter1_inter, CHANGE);
}

BlinkLeds myBlinkLeds_500ms(500);

void loop()
{
  myBlinkLeds_500ms.run();
  digitalWrite(13, myBlinkLeds_500ms.getLedState()); // clignotement de la led "activité" (D13) de l'ATmega

  comio2.run();
  pulses();
  compteurs.run();

  process_counters();
  process_tmp36();
  process_digital_in_change_dectection();
}