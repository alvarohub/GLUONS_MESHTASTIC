#include "HardDiskEncoder.h"
#include "PinChangeInt.h"  

HDEncoder::HDEncoder (uint8_t _pinA, uint8_t _pinB, uint8_t _pinC, uint8_t _modePullup = INPUT, uint8_t _modeInterrupt = CHANGE) : pinA(_pinA), pinB(_pinB), pinC(_pinC), absoluteAngle(0), oldAngle(0), validChange(false), degreesPerStep(15) {
  pinMode(pinA, _modePullup); 
  pinMode(pinB, _modePullup); 
  pinMode(pinC, _modePullup);
   PCintPort::attachInterrupt (_pinA, &HDEncoder::pinA_ISR, (void*)this, _modeInterrupt);
   PCintPort::attachInterrupt (_pinB, &HDEncoder::pinB_ISR, (void*)this, _modeInterrupt);
   PCintPort::attachInterrupt (_pinC, &HDEncoder::pinC_ISR, (void*)this, _modeInterrupt);
  firstTime = true;
    
  //ATTN! we need to start in a VALID state! otherwise nothing will ever happen:
    stateA=true; stateB=false; stateC=false;
}



void HDEncoder::pinA_ISR (void* arg) {
  HDEncoder* this_ = (HDEncoder*) arg;
  bool auxStateA = digitalRead (this_->pinA);
  if (this_->stateB != this_->stateC) { // otherwise is an invalid state: don't change states. ATTN! we need to start in a VALID state! otherwise nothing will ever happen.
    this_->validChange = true;
    this_->stateA = auxStateA;
    this_->oldAngle = this_->absoluteAngle;
    this_->absoluteAngle += this_->degreesPerStep * (bin2sign (auxStateA == this_->stateC));
    this_->computeSpeed();
  }
}


void HDEncoder::pinB_ISR (void* arg) {
  HDEncoder* this_ = (HDEncoder*) arg;
  bool auxStateB = digitalRead (this_->pinB);
  if (this_->stateA != this_->stateC) {
    this_->validChange = true;
    this_->stateB = auxStateB;
    this_->oldAngle = this_->absoluteAngle;
    this_->absoluteAngle += this_->degreesPerStep * (bin2sign (auxStateB == this_->stateA));
    this_->computeSpeed();
  }
}


void HDEncoder::pinC_ISR (void* arg) {
  HDEncoder* this_ = (HDEncoder*) arg;
  bool auxStateC = digitalRead (this_->pinC);
  if (this_->stateA != this_->stateB) {
    this_->validChange = true;
    this_->stateC=auxStateC;
    this_->oldAngle = this_->absoluteAngle;
    this_->absoluteAngle += this_->degreesPerStep * (bin2sign (auxStateC == this_->stateB));
    this_->computeSpeed();
  }
}

