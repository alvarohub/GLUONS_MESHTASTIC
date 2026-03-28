// Sample code for pin change interrupt on Moteino. Raising the pin defined by PIN1 increments a counter, raising the pin defined by PIN2 decrements a counter.

#include <PCInterrupt.h>

#define PIN1  4 // pin for the first interrupt (can be any pin except 0 - 3)
#define PIN2  5 // pin for the second interrupt (can be any pin except 0 - 3)

volatile int ticktocks = 0;

void setup() {
  Serial.begin(115200);
  pinMode(PIN1, INPUT); // pin mode must be set to INPUT or the interrupt will not read
  pinMode(PIN2, INPUT);
  PCattachInterrupt(PIN1, tick, CHANGE); // attach the interrupt (CHANGE, RISING, FALLING, etc.) There is also a PCDetachInterrupt function: PCDetachInterrupt(PIN)
  PCattachInterrupt(Pin2, tock, CHANGE);
  delay(2000);
  Serial.println("Ready.");
}

int priorTicktocks = 0;

void loop() {
  if (ticktocks != priorTicktocks) {
    Serial.println(ticktocks);
    priorTicktocks = ticktocks;
  }
}

void tick(void) { // interrupt on PIN1
  ticktocks++;
}

void tock(void) { // interrupt on PIN2
  ticktocks--;
}
