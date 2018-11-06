//
//  arduino_pins.h
//
//  Created by Patrice DIETSCH on 20/10/12.
//
//

#ifndef __arduino_pins_h
#define __arduino_pins_h

#include <inttypes.h>

/* PORTD
 * PIN  0 : [RX]
 * PIN  1 : [TX]
 * PIN  2 : [INT]
 * PIN  3 : [INT / PWM]
 * PIN  4 :
 * PIN  5 : [PWM]
 * PIN  6 : [PWM]
 * PIN  7 :
 *
 * PORDB
 * PIN  8 :
 * PIN  9 : [PWM]
 * PIN 10 : [PWM / SS]
 * PIN 11 : [PWM / MOSI]
 * PIN 12 : [MISO]
 * PIN 13 : [LED / SCK]
 * PIN NA : -
 * PIN NA : -
 *
 * PORTC
 * PIN A0 : (RELAI1)
 * PIN A1 : (RELAI2)
 * PIN A2 : (RELAI3)
 * PIN A3 : (TEMPERATURE)
 * PIN A4 : (I2C)
 * PIN A5 : (I2C)
 * PIN A6*: - PAS DISPO SUR UNO
 * PIN A7*: - PAS DISPO SUR UNO
 */

#define ARDUINO_D0 0
#define ARDUINO_D1 1
#define ARDUINO_D2 2
#define ARDUINO_D3 3
#define ARDUINO_D4 4
#define ARDUINO_D5 5
#define ARDUINO_D6 6
#define ARDUINO_D7 7
#define ARDUINO_D8 8
#define ARDUINO_D9 9
#define ARDUINO_D10 10
#define ARDUINO_D11 11
#define ARDUINO_D12 12
#define ARDUINO_D13 13

#define ARDUINO_D14 14 // = AI0
#define ARDUINO_D15 15 // = AI1
#define ARDUINO_D16 16 // = AI2
#define ARDUINO_D17 17 // = AI3
#define ARDUINO_D18 18 // = AI4
#define ARDUINO_D19 19 // = AI5
#define ARDUINO_D20 20 // = AI6
#define ARDUINO_D21 21 // = AI7

#define ARDUINO_AI0 14
#define ARDUINO_AI1 15
#define ARDUINO_AI2 16
#define ARDUINO_AI3 17
#define ARDUINO_AI4 18
#define ARDUINO_AI5 19
#define ARDUINO_AI6 20
#define ARDUINO_AI7 21

int16_t mea_getArduinoPin(char *spin);

#endif
