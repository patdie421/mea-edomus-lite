#include <LowPower.h>

#include <DHT22.h> // attention : version modifiée pour supprimer controle interval entre deux lectures (incompatibilité avec
                   // la mise en sommeil

#define DHT22_PIN         4
#define ACTIVITY_LED_PIN 13
#define ERROR_LED_PIN     7
#define XBEE_SLEEP_PIN    8
#define XBEE_CTS_PIN      9
#define LOCK_AWAKE_PIN    2

#define SLEEP_DURATION   60
// #define DEBUG             0

DHT22 dht22(DHT22_PIN);
unsigned long sn=00000001;

volatile char interrupt_flag;

// pour le reveil
void inter() // interruption pin 2
{
  detachInterrupt(0);
  interrupt_flag=1;
}


/*
 * Mesure la référence interne à 1v1 de l'ATmega
 */
unsigned int get_1v1_vs_vcc()
{
  ADMUX = B00001100; // Vcc comme référence de tension et 1v1 (Vbg) comme entrée de l'ADC
  ADCSRA |= (1 << ADEN); // Active le convertisseur analogique -> numérique
  delay(2); // attendre le bon démarrage 
  ADCSRA |= (1 << ADSC); // Lance une conversion analogique -> numérique
  while(ADCSRA & (1 << ADSC)); // Attend la fin de la conversion
  return ADCL | (ADCH << 8); // Récupère le résultat de la conversion
}  



void setup()
{
  ACSR |= _BV(ACD);     // disable the analog comparator => pour économiser un peu

  Serial.begin(57600);

  pinMode(DHT22_PIN,INPUT);
  digitalWrite(DHT22_PIN,HIGH); // activation résistance de pullup
   
  pinMode(LOCK_AWAKE_PIN,INPUT);
  digitalWrite(LOCK_AWAKE_PIN,HIGH); // pullup ON
   
  pinMode(XBEE_SLEEP_PIN,OUTPUT);
  pinMode(XBEE_CTS_PIN,INPUT);
   
  pinMode(ACTIVITY_LED_PIN,OUTPUT);
  pinMode(ERROR_LED_PIN,OUTPUT); // erreur
   
  pinMode(A0,INPUT);
}


void blink_led(char pin, int nb_blink, int duration)
{
   digitalWrite(pin, HIGH);
   for(int i=0;i<nb_blink;i++)
   {
      digitalWrite(pin, !digitalRead(pin));
      delay(duration);
   }
   digitalWrite(pin, LOW);
}


void loop()
{
   digitalWrite(XBEE_SLEEP_PIN,LOW); // activation du XBEE
   digitalWrite(ACTIVITY_LED_PIN,HIGH); // activation LED "en fonction"
   
   int chk = dht22.readData();
   switch(chk)
   {
      case DHT_ERROR_NONE:
      {
         int i=0;
         char err=0;
         float pile_v=0; // tension des piles

         sbi(PRR, PRADC); // horloge ADC réactivée
         int vp = get_1v1_vs_vcc(); // pour la référence de mesure de la tension des piles
         // real_vcc = (1023 * 1.1) / vp;

         while(digitalRead(XBEE_CTS_PIN)) // on attend que le XBEE soit dispo
         {
            if(i>1000) // environ une seconde max pour que l'XBEE soit dispo
            {
               err=1;
               break;
            }
            delay(1);
            i++;
         } // attendre CTS

         if(!err) // CTS OK dans les temps ?
         {
            // moyen de 10 lectures pour avoir une valeur "stable" de la tension des piles
            for(int i=0;i<10;i++)
            {
               // pile_v=pile_v+analogRead(A0)*real_vcc/1023;
               pile_v=pile_v+analogRead(A0)*1.1/vp;
            }
            pile_v=pile_v/10;

            Serial.print("<SN=");
            Serial.print(sn);
            Serial.print(";H=");
            Serial.print(dht22.getHumidity());
            Serial.print(";");
            Serial.print("T=");
            Serial.print(dht22.getTemperatureC());
            Serial.print(";");
            Serial.print("P=");
            Serial.print((float)pile_v);
            Serial.println(">");
            Serial.flush();
            delay(2);
         }
         break;
      }
      default:
         blink_led(ERROR_LED_PIN, 8, 125); // erreur de lecture avec DHT22
         break;
   }
   
   // On va faire dodo ?
   if(digitalRead(LOCK_AWAKE_PIN)==HIGH)
   {
      digitalWrite(XBEE_SLEEP_PIN, HIGH); // dodo xbee
      digitalWrite(ACTIVITY_LED_PIN, LOW); // desactivation LED "en fonction"

      interrupt_flag=0;
      attachInterrupt(0, inter, LOW);
      for(int i=0; i<SLEEP_DURATION; i++)
      {
         LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);
         if(interrupt_flag)
            break;
      }
   }
   else
   {
      digitalWrite(XBEE_SLEEP_PIN, HIGH);
      delay(5000);
   }
}

