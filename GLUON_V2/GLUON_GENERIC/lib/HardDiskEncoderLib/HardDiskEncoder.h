/* Name: hardDriveEncoder.h
   Declares an class to handle a hard disk motor as an encoder
   Author: Alvaro Cassinelli & Pablo Gindel, 5.2015
   Dependencies: Use contributed PinChangeInt.h library by Chris J. Kiick, Lex Talionis & Michael Schwager
   (Modified by Pablo Gindel?)
*/


#ifndef HDENCODER_INT_H
#define HDENCODER_INT_H

#include <Arduino.h>
#include <util/atomic.h>

class HDEncoder {

  public:
    HDEncoder(uint8_t pinA, uint8_t pinB, uint8_t pinC, uint8_t modePullup, uint8_t modeInterrup);

    //Note: It is better to use accessor classes, or functions to get/set volatile variables:
    void resetAngle() {
      ATOMIC_BLOCK (ATOMIC_RESTORESTATE) {
          validChange=false;
          firstTime=true;
        absoluteAngle = 0;
          oldAngle=0;
      }
    }
    void setDegresPerStep(int degStep) {
      ATOMIC_BLOCK (ATOMIC_RESTORESTATE) {
        degreesPerStep = degStep;
      }
    }

    boolean isChange() {
      if (validChange) {
        validChange = false;
        return (true);
      } else return (false);
    }
    
    boolean isChange(long& newAngle) {
      if (validChange) {
        newAngle = getAngle();
        validChange = false;
        return (true);
      } else return (false);
    }
    
    String getStateString() const { // get the state of all pins as a string (ex: "001")
        String stateString="";
        ATOMIC_BLOCK (ATOMIC_RESTORESTATE) {
            stateString+=(stateA? '1' : '0');
            stateString+=(stateB? '1' : '0');
            stateString+=(stateC? '1' : '0');
        }
        return (stateString);
    }
    
    long getAngle() const {
      long auxAbsoluteAngle;
      ATOMIC_BLOCK (ATOMIC_RESTORESTATE) {
        auxAbsoluteAngle = absoluteAngle;
      } return (auxAbsoluteAngle);
    }
    
    float getSpeed() const {
      float auxSpeed;
      ATOMIC_BLOCK (ATOMIC_RESTORESTATE) {
        auxSpeed = angularSpeed;
      } return (1.0*auxSpeed/360*1000000); // in turns/sec
    }

  private:
    uint8_t pinA, pinB, pinC;
    boolean stateA, stateB, stateC;  // do we really need them to be volatile? not sure! the ISRs don't interrupt each other.
    boolean firstTime;
    
    unsigned long lastTimeChanged; // note: we will measure speed by computing the time between two changes. Arduino 16MHz micros() function has a resolution
    // of 4 us. Assuming a step is 10degrees, this means the max speed we can compute is about 6944 rotations per SECOND, and at that speed we may have an
    // impresision of 10 degrees/s.. but at real (very low) speeds, the speed will be measured with great precision (ex: at 10 rotations/s, which is already quite large,
    // we have 10*360/30=120 ticks/sec, or 8333us/tick. A difference of 4us is negligible.

    int degreesPerStep; // shared between the "main" and accessed by the ISRs but not modified by any of them (no need to calify the variable as volatile)
    volatile float angularSpeed;
    volatile long absoluteAngle, oldAngle;
    volatile boolean validChange;

    static void pinA_ISR (void* arg);
    static void pinB_ISR (void* arg);
    static void pinC_ISR (void* arg);

    static inline int bin2sign (const boolean var) {
      return 2 * var - 1;
    }

    inline void computeSpeed() {
      if (!firstTime) {
        unsigned long timeNow = micros();
        angularSpeed = 1.0 * (absoluteAngle - oldAngle) / (timeNow - lastTimeChanged);
        lastTimeChanged = timeNow;
      } else {
        firstTime = false;
        angularSpeed = 0;
        oldAngle=absoluteAngle;
        lastTimeChanged=micros();
      }
    }

};


#endif
