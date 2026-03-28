#ifndef _PCInt_Simple_H
#define _PCInt_Simple_H

// Pin change interrupt library that works for Moteino.
// https://lowpowerlab.com/forum/moteino/pinchange-interrupt/

#include "Arduino.h"
#include "pins_arduino.h"
/* Pin to interrupt map:
 * D0-D7 = PCINT 16-23 = PCIR2 = PD = PCIE2 = pcmsk2
 * D8-D13 = PCINT 0-5 = PCIR0 = PB = PCIE0 = pcmsk0
 * A0-A5 (D14-D19) = PCINT 8-13 = PCIR1 = PC = PCIE1 = pcmsk1
 */
//
// class PCIntSimple {
// public:
//
// private:
// }

typedef void (*voidFuncPtr)(void);

 extern volatile uint8_t *port_to_pcmask[];

 volatile static uint8_t PCintMode[24]; // note: modes (RISING, FALLING and CHANGE are #defined in Arduino.h and are 1, 2 ans 3)
 volatile static voidFuncPtr PCintFunc[24] = { NULL };

 volatile static uint8_t PCintLast[3];

void PCattachInterrupt(uint8_t pin, void (*userFunc)(void), uint8_t mode);
// NOTE: the values RISING, FALLING, CHANGE are #defined in Arduino.h, and are 1, 2 and 3

void PCdetachInterrupt(uint8_t pin);

static void PCint(uint8_t port);


/* NOTE: The SIGNAL macro is defined in hardware/tools/avr/avr/include/avr/interrupt.h as below:
#define __INTR_ATTRS used, externally_visible
#define SIGNAL(vector)                                        \
    extern "C" void vector(void) __attribute__ ((signal, __INTR_ATTRS));        \
    void vector (void)
*/

SIGNAL(PCINT0_vect);
SIGNAL(PCINT1_vect);
SIGNAL(PCINT2_vect);

#endif
