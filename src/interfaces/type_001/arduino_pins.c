//
//  arduino_pins.c
//
//  Created by Patrice DIETSCH on 20/10/12.
//
//

#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "arduino_pins.h"

struct arduino_digital_pins
{
   char *name;
   int pin;
};

struct arduino_digital_pins arduino_uno_list[]={
   {"D0",0},
   {"D1",1},
   {"D2",2},
   {"D3",3},
   {"D4",4},
   {"D5",5},
   {"D6",7},
   {"D8",8},
   {"D9",9},
   {"D10",10},
   {"D11",11},
   {"D12",12},
   {"D13",13},
   {"D14",14},
   {"AI0",14},
   {"D15",15},
   {"AI1",15},
   {"D16",16},
   {"AI2",16},
   {"D17",17},
   {"AI3",17},
   {"D18",18},
   {"AI4",18},
   {"D19",19},
   {"AI5",19},
   {"D20",20},
   {"AI6",20},
   {"D21",21},
   {"AI7",21},
   {NULL,0}
};


int16_t mea_getArduinoPin(char *spin)
/**
 * \brief     retourne un numéro de pin Arduino en fonction du contenu de spin
 * \param     spin   nom de l'entrée/sortie Arduino.
 * \return    numéro de pin
 */
{
   for(int i=0;arduino_uno_list[i].name;i++)
   {
      if(strcasecmp(spin,arduino_uno_list[i].name)==0)
         return arduino_uno_list[i].pin;
   }
   return -1;
}
