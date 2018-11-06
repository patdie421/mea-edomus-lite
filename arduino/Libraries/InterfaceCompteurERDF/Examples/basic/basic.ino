#include <Arduino.h>
#include <SoftwareSerial.h>

#include <InterfaceCompteurERDF.h>

#define ACTIVATION_PIN 6
#define SELECTION_PIN  7

// Objet d'accès au compteur
InterfaceCompteurERDF compteurs(ACTIVATION_PIN, SELECTION_PIN, myReadFn, myAvailableFn);


/******************************************************************************************/
/* Préparation liaison série avec le compteur                                             */
/******************************************************************************************/
SoftwareSerial counterSerial(8, 9); // RX, TX

// callback de test "caractère disponible" sur la ligne pour InterfaceCompteurERDF
int myAvailableFn()
{
  return counterSerial.available();
}

// callback de lecture pour InterfaceCompteurERDF
int myReadFn()
{
  return counterSerial.read();
}


void setup()
{
  counterSerial.begin(1200);
  Serial.begin(9600);
}

void loop()
{
  // demarrage de la lecture
  compteurs.startRead("BASE", 0);
  
  // attendre le résultat
  do
  {
    compteurs.run();
  }
  while(!compteurs.isDataAvailable() && !compteurs.isError());
  
  // traitement du résultat
  if(compteurs.isDataAvailable())
  {
    char resultat[12];
         
    compteurs.getStrVal(resultat,sizeof(resultat));
    Serial.println(resultat);
  }
  else
  {
    Serial.println("Erreur de lecture");
  }
  
  // réinitialisation du moteur (indispensable avant de relancer une lecture)
  compteurs.reset();
  
  // attendre avant prochaine lecture
  delay(5000);
}
