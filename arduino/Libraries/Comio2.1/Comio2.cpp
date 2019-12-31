#include "comio2.h"


//
//
// Nouvelles méthodes pour le protocole V2
//
//

void Comio2::_comio_send_errorV2(unsigned char id, unsigned char error)
/**
 * \brief     envoie une réponse signalant une erreur.
 * \details
 * \param     id    identifiant de la demande qui a provoqué une erreur
 * \param     error numéro de l'erreur
 */
{
   int cchecksum=0;
   
   _comio_debut_trameV2(id, COMIO2_CMD_ERROR, 1, &cchecksum);
   cchecksum+=error;
   writeF(error);
   _comio_fin_trameV2(&cchecksum);
}


int Comio2::_comio_read_frameV2(unsigned char *id, unsigned char *cmd, char *data, int *l_data)
/**
 * \brief     lecture d'une trame "commande" de type "Comio2" dans le buffer d'entrée.
 * \details   Lit un flot d'entrée, l'analyse et le découpe si la trame est correctement formatée
 * \param     cmd    code commande
 * \param     data   données de la commande
 * \param     l_data nombre de caractères de la zone data (max 80 octets)
 * \return    0 => trame correcte,  < 0 => erreur
 */
{
   /*
    Structure d'une trame "question" de type "Comio2"
    
    Start(1)[[] : octet de début de trame. Permet d'identifier le type de trame
    Longueur(1) : nombre d'octet de la trame (inclus start et fin de trame)
    Id_trame(1) : numéro de trame question (0 = pas de réponse attendu, sauf erreur)
    Commande(1) : opération à réaliser
    Data(x)     : données de l'opération (0<=x<80)
    CheckSum(1) : checksum calculer sur la trame hors start et end (0xFF - (somme_de_tous_les_octets & 0xFF)).
    End(1)[]]   : octet de fin de trame.
    */
   int c=0;
   int16_t j=0;
   uint8_t stat=0;
   int cchecksum=0;
   
   while(stat<6)
   {
      // 5 ms max pour obtenir une données
      for(int i=0;i<5;i++)
      {
         c=readF();
         if(c>=0)
            break;
         delay(1); // 1 millisecondes max entre 2 lectures
      }
      
      if(c<0)
         return 0; // pas de données => sortie direct "sans rien dire"
      
      // automate à états pour traiter les différents champs de la trame
      switch(stat)
      {
         case 0: // lecture de la longueur de trame
            *l_data=c-6;
            cchecksum+=c;
            stat++; // étape suivante à la prochaine itération
            break;
            
         case 1: // lecturde de l'id de trame
            *id=c;
            cchecksum+=c;
            stat++;
            break;
            
         case 2: // lecture de la commande
            *cmd=c;
            cchecksum+=c;
            j=0;
            stat++;
            break;
            
         case 3: // lecture des données
            if(j<COMIO2_MAX_DATA)
            {
               data[j]=c;
               cchecksum+=c;
               j++;
               if(j>=*l_data)
                  stat++;
            }
            else
               return 0; // overflow ...
            break;
            
         case 4: // lecture checksum
            cchecksum+=(unsigned char)c;
            stat++;
            break;
            
         case 5:
            if(c!=']')
               return 0; // pas de fin de trame correct ... on oubli tout et on sort
            if((cchecksum & 0xFF) != 0xFF)
               return 0; // checksum incorrect ... on oubli tout et on sort
            stat++;
            break;
      }
   }
   return -1;
}


void Comio2::_comio_debut_trameV2(unsigned char id, unsigned char cmd, int l_data, int *cchecksum)
/**
 * \brief     Construit et envoie les octets de début d'une trame (checksum + '}') et met à jour le cchecksum à partir des éléments en paramètre.
 * \param  id      id de la trame
 * \param  cmd     commande (type de trame)
 * \param  l_data  nombre d'octet de la partie données de la trame
 */
{
   writeF('{');
   
   *cchecksum=6+l_data;
   writeF(6+l_data);
   
   *cchecksum+=id;
   writeF(id);
   
   *cchecksum+=cmd;
   writeF(cmd);
}


void Comio2::_comio_fin_trameV2(int *cchecksum)
/**
 * \brief     Construit et envoie les octets de cloture d'une trame (checksum + '}').
 * \param     cchecksum  état du checksum jusqu'à présent
 */
{
   *cchecksum=0xFF - (*cchecksum & 0xFF);
   writeF(*cchecksum);
   
   writeF('}');
   flushF();
}


int Comio2::_comio_do_operationV2(unsigned char id, unsigned char cmd, char *data, int l_data)
/**
 * \brief     traite une demande de type "Comio2".
 * \details   Traite une commande (cmd)
 * \param     cmd    code commande
 * \param     data   donnÃ©es de la commande
 * \param     l_data nombre de caractères de la zone data (max 80 octets)
 * \return    0 => trame correcte,  < 0 => erreur
 */
{
   /*
    Start(1)[{] : octet de début de trame. Permet d'identifier le type de trame
    Longueur(1) : longueur de la réponse (start et end compris)
    Id_trame(1) : réponse à la question id_trame
    Response(1) : type de la réponse
    Data(x)     : données de la réponse
    CheckSum(1) : checksum calculé sur la trame hors start et end.
    End(1)[}]   : octet de fin de trame.
    */
   int cchecksum = 0;
   int j=0; // compteur du nombre d'octet écrit
   int m=0;
   
   switch(cmd)
   {
      case COMIO2_CMD_READMEMORY: // memory read - data contient la liste des cases mémoires à lire
         if(!id)
            return 0;
         
         if(l_data < 1)
         {
            if(id)
               _comio_send_errorV2(id, COMIO2_ERR_PARAMS);
            return 0;
         }
         
         _comio_debut_trameV2(id, COMIO2_CMD_READMEMORY, l_data, &cchecksum);
         for(int i=0;i<l_data;i++)
         {
            if((int)data[i]>=COMIO2_MAX_MEMORY)
               m=255;
            else
               m=comio_memory[(int)data[i]];
            
            cchecksum+=m;
            writeF(m);
         }
         _comio_fin_trameV2(&cchecksum);
         return -1;
         break;

      case COMIO2_CMD_WRITEMEMORY: // memory write - data contient la liste des couples Adresse/valeur
         if(l_data < 1 || l_data % 2 != 0)
         {
            if(id)
               _comio_send_errorV2(id, COMIO2_ERR_PARAMS);
            return 0;
         }
         
         for(int i=0; i<l_data; i+=2)
         {
            if((int)data[i]<COMIO2_MAX_MEMORY)
            {
               comio_memory[(int)data[i]]=data[i+1];
               j++;
            }
         }
         
         if(id)
         {
            _comio_debut_trameV2(id, COMIO2_CMD_WRITEMEMORY, 1, &cchecksum);
            cchecksum+=j;
            writeF(j); // nombre d'octets écrit
            _comio_fin_trameV2(&cchecksum);
         }
         return -1;
         break;
         
      case COMIO2_CMD_CALLFUNCTION:
         if(l_data < 1 || data[0] >= COMIO2_MAX_FX) // il faut un code fonction valide
         {
            if(id)
               _comio_send_errorV2(id, COMIO2_ERR_PARAMS);
            return 0;
         }
         if(comio_functionsV2[(int)data[0]])
         {
            int l_buffer = 0;
            int retval=comio_functionsV2[(int)data[0]](id, &(data[1]), l_data-1, buffer, &l_buffer, userdata);
            if(id)
            {
               uint8_t r=0;
               
               _comio_debut_trameV2(id, COMIO2_CMD_CALLFUNCTION, 2+l_buffer, &cchecksum);
               
               // retour fonction sur les deux premiers caractères de la réponse
               r=retval & 0xFF;
               cchecksum+=r;
               writeF(r);
               r=(retval >> 8) & 0xFF;
               cchecksum+=r;
               writeF(r);
               
               // données retournées par le callback
               for(int i=0;i<l_buffer;i++)
               {
                  cchecksum+=buffer[i];
                  writeF(buffer[i]);
               }
               
               _comio_fin_trameV2(&cchecksum);
            }
         }
         else
         {
            if(id)
               _comio_send_errorV2(id, COMIO2_ERR_UNKNOWN_FUNC);
            return 0;
         }
         return -1;
         break;
         
      default:
         return 0;
   }
}


void Comio2::sendTrap(unsigned char num_trap, char *data, char l_data)
/**
 * \brief     émission d'une trame TRAP de type "Comio2" dans le buffer de sortie.
 * \details   Construit une trame de TRAP et l'envoie dans le buffer de sortie
 * \param     num_trap  numéro du trap
 * \param     value     données d'accompagnement du trap
 * \param     l_value   nombre de données dans le champ valeur
 */
{
   int cchecksum=0;
   
   _comio_debut_trameV2(COMIO2_TRAP_ID, num_trap, l_data, &cchecksum);
   for(int i=0;i<l_data;i++)
   {
      cchecksum+=data[i];
      writeF(data[i]);
   }
   _comio_fin_trameV2(&cchecksum);
}


int Comio2::setMemory(unsigned int addr, unsigned char value)
/**
 * \brief     Ecriture d'une case mémoire.
 * \param     addr  adresse de la case à mettre à jour
 * \return    0 si OK ou -1 si addr hors scope
 */
{
   if(addr >= 0 || addr <  COMIO_MAX_M)
   {
      comio_memory[addr]=value;
      return 0;
   }
   return -1;
}


int Comio2::getMemory(unsigned int addr)
/**
 * \brief     Lecture d'une case mÃ©moire.
 * \param     addr  adresse de la case Ã  lire
 * \return    la valeur de la case ou -1 si addr hors scope
 */
{
   if(addr >= 0 || addr <  COMIO_MAX_M)
   {
      return (int)comio_memory[addr];
   }
   return -1;
}


void Comio2::setFunction(unsigned char num_function, callback2_f function)
/**
 * \brief     Association d'une fonction à un "port" (numéro de fonction).
 * \details
 * \param     num_function  numéro de fonction
 * \param     function      fonction associée au numéro / mettre NULL pour desalouer une fonction
 */
{
   if(num_function<COMIO2_MAX_FX)
      comio_functionsV2[num_function]=function;
}


//
//
// Commun V1 et V2
//
//
void Comio2::init(void)
/**
 * \brief     Initialisation.
 */
{
}


inline int _comio2_serial_read()
/**
 * \brief     fonction de lecture d'un caractère utilisée par défaut.
 * \details   Point de lecture pour la réception des commandes/données à traiter par Comio si aucune autre fonction n'a été positionnée par la methode setReadFunction. Lit le premier caractère du buffer d'entrée
 * \return    < 0 (-1) si pas de données disponible, le caractère lu sinon.
 */
{
   return Serial.read();
}


inline int _comio2_serial_write(char car)
/**
 * \brief     fonction d'écriture par défaut.
 * \param     car   caractère a insérer dans le buffer de sorie
 * \details   Point d'écriture des résultats renvoyés par Comio si aucune autre fonction n'a été positionnée par la methode setWriteFunction. Ajoute un caractere dans le buffer de sortie
 * \return    toujours 0
 */
{
   Serial.write(car);
   return 0;
}


inline int _comio2_serial_available()
/**
 * \brief     fonction permettant de connaitre le nombre de caractères présent dans le buffer d'entrée.
 * \details   fonction par defaut si aucune autre fonction n'a été positionnée par la methode setAvailableFunction.
 * \return    nombre de caractères disponibles
 */
{
   return Serial.available();
}


inline int _comio2_serial_flush()
/**
 * \brief     fonction permettant d'attendre le "vidage" du buffer de sortie.
 * \details   fonction par defaut si aucune autre fonction n'a été positionnée par la methode setFlushFunction.
 * \return    toujours 0
 */
{
   Serial.flush();
   return 0;
}


Comio2::Comio2()
/**
 * \brief     constructeur classe Comio2.
 * \details   initialise les données des instances Comio2.
 */
{
   memset(comio_functionsV2,0,sizeof(comio_functionsV2));
   memset(comio_memory,0,sizeof(comio_memory));
   
   writeF=_comio2_serial_write;
   readF=_comio2_serial_read;
   availableF=_comio2_serial_available;
   flushF=_comio2_serial_flush;
   
   userdata = NULL;
}


/**
 */
int Comio2::run()
/**
 * \brief     fait "tourner" COMIO.
 * \details   Fonction à appeler le plus souvant possible pour pouvoir prendre en charge les demandes en provenance d'un client et les traiter.
 * \return    0 = pas de trames traitées ou données incorrectes / <> 0 code de traitement (> 0 Comio1 < 0 Comio2).
 */
{
   if(!availableF())
      return 0;
   
   unsigned char cptr=0;
   boolean flag;

   flag=false;
   while(availableF() && cptr<10) // lecture jusqu'à 10 caractères pour détecter un début de trame
   {
      int c=readF();
      if(c=='[') // nouveau format de trame
      {
         flag=true;
         break;
      }
      cptr++;
   }
   if(!flag)
      return 0; // pas de début de trame, on rend la main pour ne pas trop bloquer la boucle principale
   
   char data[COMIO2_MAX_DATA];
   unsigned char id;
   unsigned char cmd;
   int l_data;

   int ret=_comio_read_frameV2(&id, &cmd, data, &l_data);
   if(ret) {
      ret=_comio_do_operationV2(id, cmd, data, l_data);
   }

   return 0;
}