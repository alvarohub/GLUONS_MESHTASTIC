#ifndef _BaseActuatorDigital_h
#define _BaseActuatorDigital_h

#include "Arduino.h"
#include "BaseActuator.h"

// note1: this is a GENERIC DIGITAL output actuator, from which we can construct beepers, leds, power switches, etc. directly or by deriving children

#define PULSE_DURATION_DEFAULT 250 // in ms

enum ModeDigitalOut {NORMAL, TOGGLE, PULSE, BLINK};  // how to update the digital output (from the event value)

class BaseActuatorDigital : public BaseActuator {
  public:

    // Parameter-less constructor with default values (name, pinout and mode)
    //BaseActuatorDigital() : BaseActuator("D-OUT"), myMode(NORMAL), pulseDuration(PULSE_DURATION_DEFAULT), numPulses(3) {}
    // Constructor with parameters to either create the object in the main file, or derive a class and use this in its constructor initialization list:
    BaseActuatorDigital(const char* _name, ModeDigitalOut _mode) : BaseActuator(_name), myMode(_mode), pulseDuration(PULSE_DURATION_DEFAULT) {}
	virtual ~BaseActuatorDigital() {};

    void init(bool firstTimeBuild) {
      state = false; // NOTE: only use of this is for the BLINK mode - or in the future, to GET the status of the actuator.
      setActuator(false);
	  //setActuator(true);
      //numPulses = 3; // default num of pulses when in blinking mode
      startOn = millis();
    }

    // Implementation of the virtual methods of the base class (we can add virtual if we want, but override make it clear - and compile safe (it is C++11 though... hope it works here))
    virtual void update() override;

    // Methods that ONLY belong to this child:
    virtual void setActuator(bool _state) {} //{digitalWrite(pinDigitalOut, _state);

	void setMode(ModeDigitalOut _mode) {myMode = _mode;}

    void setDuration(uint32_t _dur) { // in ms
        pulseDuration = _dur;
    }

  protected:
    ModeDigitalOut myMode;
    bool state;
    uint32_t pulseDuration;
    uint8_t numPulses;
    uint32_t startOn;
};

// ATTENTION: By including the method implementation inside the method declaration, it is being implicitally declared as inlined
void BaseActuatorDigital::update() {

  // We take action WHENEVER THERE IS NEW DATA (note that we could not set new data as false...)
  if ( isNewData() ) {

    // Check if we need to change the opeartion mode (by using the string part of the Data)
    // if (myData.stringData=="NORMAL") myMode=NORMAL;
    // else if (myData.stringData=="TOGGLE") myMode=TOGGLE;
    // else if (myData.stringData=="BLINK") myMode=BLINK;
    // else if (myData.stringData=="PULSE_LONG") {myMode=PULSE; pulseDuration=PULSE_DURATION_DEFAULT;}
    // else if (myData.stringData=="PULSE_SHORT") {myMode=PULSE; pulseDuration=PULSE_DURATION_DEFAULT/3;}
    // otherwise do nothing - the string is for something else (NOTE: it would be good to have a more consistant message passing,
    // for instance, specifying on one field for WHICH actuator/inlet, etc is this data intended to be used? maybe not. This breaks the
    // gluon MAX/MSP-like paradigm)

    switch (myMode) {

	case NORMAL:
        state = myData.event;
  		setActuator(state); // change the state to represent the state of the EVENT
        break;

      case TOGGLE:
        if (myData.event) {
          state = !state;  // toggle only when receiving an "positive" event
          setActuator(state);
        }
        break;

      case PULSE:
        state = myData.event;
        if (state) {
          startOn = millis(); // basically, data changes the state (we will use states for the actuators, and event change these)
          setActuator(true);
		  //PRINTLN(F("ON"));
        }
        break;

      case BLINK:
        numPulses = myData.numericData;
        state = true;
        startOn = millis();
        setActuator(state);
        break;

    // ... other modes?

    } // end of switch case

	// IMPORTANT: In any case, signal that we "used" the data:
    setDataAsUsed();

} else { // no new data: evolve the actuator if necessary.
	if (myMode == PULSE) {
	  if (millis() - startOn > pulseDuration) {
		state = false;
		setActuator(false);
	  }
	}
	else if (myMode == BLINK) {
	  if ((numPulses > 0)&&(millis() - startOn > pulseDuration)) {
		  state = !state;
		  setActuator(state);
		  numPulses--;
		  startOn = millis();
		}
	  }
	}
}


#endif
