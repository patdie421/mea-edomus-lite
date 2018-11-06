#include "Arduino.h"

#include "InterfaceCompteurERDF.h"


int _ICE_readSerial()
{
   return Serial.read();
}


int _ICE_availableSerial()
{
   return Serial.available();
}


unsigned long _ICE_diff_millis(unsigned long chrono, unsigned long now)
{
   return now >= chrono ? now - chrono : 0xFFFFFFFF - chrono + now;
}


char checksum(char *var, char *val)
{
   int i;
   int cs=0;
   
   for(i=0;var[i];i++)
      cs=cs+(int)(var[i]);
   cs=cs+0x20;
   for(i=0;val[i];i++)
      cs=cs+(int)(val[i]);
   
   cs=(cs & 0x3F) + 0x20;
   
   return (char)cs;
}


InterfaceCompteurERDF::InterfaceCompteurERDF(unsigned char selectionPin, unsigned char activationPin, int (* readFn)(void), int (*availableFn)(void))
{
   selection=selectionPin;
   activation=activationPin;
   readF=readFn;
   availableF=availableFn;

   status=ICE_IDLE;
   stage=0;

   pinMode(activation,OUTPUT);
   digitalWrite(activation, LOW);
   pinMode(selection,OUTPUT);
   digitalWrite(selection, LOW);
}


InterfaceCompteurERDF::InterfaceCompteurERDF(unsigned char selectionPin, unsigned char activationPin)
{
   InterfaceCompteurERDF(selectionPin, activationPin, &_ICE_readSerial, &_ICE_availableSerial);
}


void InterfaceCompteurERDF::reset()
{
   digitalWrite(activation, LOW);
   digitalWrite(selection, LOW);

   status=ICE_IDLE;
}


unsigned char InterfaceCompteurERDF::isIdle()
{
   return status == ICE_IDLE ? 1 : 0;
}


unsigned char InterfaceCompteurERDF::isDataAvailable()
{
   return status == ICE_DATAAVAILABLE ? 1 : 0;
}


unsigned char InterfaceCompteurERDF::isError()
{
   return status == ICE_ERROR ? 1 : 0;
}


unsigned char InterfaceCompteurERDF::isRunning()
{
   return status & ICE_RUNNING_MASK ? 1 : 0;
}


int InterfaceCompteurERDF::startRead(char *aLabel, unsigned char aCntr)
{
   if(aCntr!=0 && aCntr!=1)
   {
      _ICE_errno = ICE_NUM_COUNTER_ERR;

      return -1;
   }

   memset(resultStr,0,ICE_MAXSTR); // initialisation des variables variable a 0

   strcpy(searchStr, aLabel); // memorisation de la chaine a rechercher
   cntr=aCntr;

   status=ICE_STARTREAD;

   _ICE_errno=ICE_NO_ERROR;

   return 0;
}


unsigned char InterfaceCompteurERDF::getStrVal(char *val, unsigned char lenval)
{
   strncpy(val, resultStr, lenval);
   
   return ICE_NO_ERROR;
}


int InterfaceCompteurERDF::run()
{
 static long start_read_millis;
 static char label[ICE_MAXSTR];
 static char value[ICE_MAXSTR];
 static char err=0;
 static unsigned char i;

   _ICE_errno=ICE_NO_ERROR;

   if(status & ICE_RUNNING_MASK)
   {
      if(status==ICE_STARTREAD) // lancement d'une lecture demande
      {
         // on initialise tout ici
         status = ICE_COMPUTING;
         start_read_millis = millis();
         
         digitalWrite(activation, HIGH); // activation de l'interface physique
         digitalWrite(selection, cntr);  // selection du compteur

         stage=0; // première etape de l'automate
      }

      if(_ICE_diff_millis(start_read_millis, millis()) > ICE_TIMEOUT)
      {
         status = ICE_ERROR;
         _ICE_errno = ICE_TIMEOUT_ERR;
         digitalWrite(activation, LOW);
         return ICE_ERROR;
      }

      if(availableF()) // un caractere a traiter ?
      {
         char car=readF() & 0x7F; // lecture d'1 caractere (On lit 7E1 avec un parametrage 8N1 => on oubli le bit de point fort)
         
         if(car==0x03 || car==0x04) // fin de trame => reset
         {
            stage=0;
            err=0;
            return ICE_END_FRAME_INF;
         }

         switch(stage)
         {
            case 0: // en attente debut de trame
               if(car == 0x02)
               {
                  stage=1;

                  return ICE_FRAME_START_INF;
               }
               return ICE_WAIT_FRAME_START_INF;

            case 1: // en attente debut de champ
               if(car == 0x0A)
               {
                  memset(label, 0, sizeof(label));
                  i=0;

                  stage=10; // étape suivante

                  return ICE_LINE_START_INF;
               }
               else
               {
                  stage=0; // pas vue de debut de trame, retour en etape 0 (attente debut de trame)

                  return ICE_LINE_START_ERR;
               }

            case 10: // lecture etiquette
               if(i>=sizeof(label)) // overflow => reinitialisation
               {
                  stage=0;

                  return ICE_OVERFLOW_ERR;
               }
               else if(car == 0x20)
               {
                  // label[i]=0; // pas nŽcessaire car 
                  memset(value,0,sizeof(value));
                  i=0;

                  stage=11;

                  return ICE_LABEL_FOUND_INF;
               }
               else
               {
                  label[i++]=car;

                  return ICE_READING_LABEL_INF;
               }

            case 11: // lecture valeur
               if(i>=sizeof(value))
               { // overflow => reinitialisation
                  stage=0;

                  return ICE_OVERFLOW_ERR;
               }
               else if(car == 0x20)
               {
                  value[i]=0;

                  stage=12;

                  return ICE_VALUE_FOUND_INF;
               }
               else
               {
                  value[i++]=car;

                  return ICE_READING_VALUE_INF;
               }

            case 12:
               stage=13;

               if(car == checksum(label,value))
               {
                  err=0;
                  return ICE_CHECKSUM_OK_INF;
               }
               else
               {
                  err=1;
                  return ICE_CHECKSUM_KO_ERR;
               }

            case 13:
               if(car == 0x0D) // fin de champ
               {
                  // validation information
                  if(!err)
                  {
                     if(strcmp(searchStr, label) == 0)
                     {
                        strcpy(resultStr,value);
                        status=ICE_DATAAVAILABLE;
                        digitalWrite(activation, LOW);

                        return ICE_LABEL_INF; // donnee dispo
                     }   
                  }
                  stage=1; // ligne suivante

                  return ICE_LABEL_NOT_FOUND_INF;
               }
               else
               {
                  stage=0; // reinitialisation lecture depuis le debut

                  return ICE_LINE_ERROR_ERR;
               }
         }
      }
   }
   return ICE_NOP_INF;
}
