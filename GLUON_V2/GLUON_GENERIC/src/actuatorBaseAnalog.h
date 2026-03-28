#ifndef _BaseActuatorAnalog_h
#define _BaseActuatorAnalog_h

#include "Arduino.h"
#include "BaseActuator.h"

// note1: this is a GENERIC ANALOG output actuator, from which we can construct other things (motors, etc) directly or by deriving children
// note2: in the Moteino R4, there is a LED connected to pin 9 (which is PWM capable by the way, so I will reserve it for analog output)
// note3: the only other available (i.e., not used by RFM69 or the FLASH memory) for PWM are 5 and 6, which I am using for the ShiftOut control of
// the ledIndicators (but if necessary, we can use them - but then we NEED to disable the Led indicators!! otherwise there will be interferences)

#define ANALOG_PULSE_DURATION_DEFAULT 1000 // in ms

//how to update the analog output:
enum ModeAnalogOut {UPDATE_ALWAYS = 0, UPDATE_ON_TRUE_EVENT, PULSE_ON_TRUE_EVENT, UPDATE_ON_TRUE_EVENT_ZERO_ON_FALSE_EVENT};

class BaseActuatorAnalog : public BaseActuator {

public:
	// Parameter-less constructor with default values (name and mode)
	//BaseActuatorAnalog() :  BaseActuator("A-OUT"), myMode(UPDATE_ALWAYS) {}
	// Constructor with parameters to either create the object in the main file, or derive a class and use this in its constructor initialization list:
	BaseActuatorAnalog(const char* _name, ModeAnalogOut _mode) :  BaseActuator(_name), myMode(_mode) {}

	void init(bool firstTimeBuild) {
		BaseActuator::init(firstTimeBuild);
		//statePulse=false; // NOTE: not used (no blink method for analog actuators for the time being, not "getStatus" method)
		startOn = millis();
		pulseDuration=ANALOG_PULSE_DURATION_DEFAULT;
		// Initial value of the analog actuator and range? no, this is done by each children (we don't know what setActuator may do...)
		//setMinMax(0,10);
		//setActuator(0);
	}

	// Implementation of the virtual methods (override make it clear - and compile safe (it is C++11)
	void update() override;

	// Methods that ONLY belong to this child:
	// virtual method for analog actuators:
	virtual void setActuator(int16_t value) {} //{analogWrite(pinAnalogOut, value);}
	void setMode(ModeAnalogOut _mode) {myMode = _mode;}
	void setMinMax(int16_t _min, int16_t _max) {min = _min; max = _max;}

protected:
	ModeAnalogOut myMode;
	//bool statePulse;
	uint32_t pulseDuration;
	uint32_t startOn;
	int16_t min, max;
};

void BaseActuatorAnalog::update() {
	if ( isNewData() ) {

		setDataAsUsed(); // In any case, signal that we "used" the data

		int16_t value = myData.numericData;
		value=constrain(value, min, max);

		switch (myMode) {
			case UPDATE_ON_TRUE_EVENT :
			if (myData.event) setActuator(value); // update analog output only when receiving an "positive" event
			break;
			case UPDATE_ALWAYS:
			setActuator(value); // update the analog output regardeless of the value of EVENT (this can be used for logging everything)
			break;
			case UPDATE_ON_TRUE_EVENT_ZERO_ON_FALSE_EVENT:  // update analog output only when receiving an "positive" event, and ZERO (or min) otherwise
			if (myData.event) setActuator(value); else setActuator(min);
			break;
			case PULSE_ON_TRUE_EVENT: // NOTE: the strength of the pulse is given by the numeric data field
			//statePulse = myData.event;
			if (myData.event) {
				startOn = millis();
				setActuator(value);
			}
			break;
			// ... other modes?
		}
	}
	else if (myMode == PULSE_ON_TRUE_EVENT) { // in case of no new data, just evolve in case of pulsed mode:
		if (millis() - startOn > pulseDuration) {
			//statePulse = false;
			setActuator(min);
		}
	}
}


#endif
