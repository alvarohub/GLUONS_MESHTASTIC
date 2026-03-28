
#include "Arduino.h"
#include "actuatorBaseAnalog.h"

// TODO: the PWM pin should be 5 (but now it's being used by the vibrator...)

// The simplest: a simple analog pwm pin (ALWAYS on pin 9 for the time being, which is the only available PWM pin)
class BaseActuatorAnalogPin: public BaseActuatorAnalog {
public:
//	BaseActuatorAnalogPin() : BaseActuatorAnalog("A-PIN", UPDATE_ALWAYS), pinAnalogOut(9) {}
	BaseActuatorAnalogPin(const char* _name, ModeAnalogOut _mode) : BaseActuatorAnalog(_name, _mode), pinAnalogOut(9) {}
	virtual ~BaseActuatorAnalogPin() override {}; // nothing needs to be done...

	virtual void setActuator(int16_t value) override {analogWrite(pinAnalogOut, value);} // note that this can be overrided too!

	void init(bool firstTime) {
		BaseActuatorAnalog::init(firstTime);
        setMinMax(0,255);
  	    setActuator(0);
	}

protected:
	uint8_t pinAnalogOut;
};

// OTHER specially tailored actuators - for reasons of memory I may use instead the base class with the proper parameters

// Normal beeper (i.e. piezo): it needs a PWM signal (on pin 9).
// Note that we cannot control the freq, but only the duty cycle (given by the numeric data field)
class AnalogBeeper : public BaseActuatorAnalogPin {
public:
  AnalogBeeper() : BaseActuatorAnalogPin("PIEZO", PULSE_ON_TRUE_EVENT) {}
  void init(bool firstTime) {
	  BaseActuatorAnalogPin::init(firstTime);
	  setMinMax(0,255);
	  setActuator(0);
  }
};

// This is a DC motor drove by an H-bridge. The value is therefore interpreted as LEFT for negative values, and RIGHT for positive,
// so we need to override the setActuator() method of the base class...
// Note that UPDATE_ON_TRUE_EVENT means here that the logic unit (for, say, a line follower) must generate an event only when it deems necessary to update the motors.
// Also, the exact PWM values are set on that logic unit (see for instance logicLineFollower.h)
class ActuatorMotorDC : public BaseActuatorAnalogPin {
public:
	ActuatorMotorDC() : BaseActuatorAnalogPin("DC_MOTOR", UPDATE_ALWAYS), dirPin(A3){}
	ActuatorMotorDC(const char* _name, uint8_t _dirPin=A3) : BaseActuatorAnalogPin(_name, UPDATE_ON_TRUE_EVENT), dirPin(_dirPin) {}

	void init(bool firstTime) {
		BaseActuatorAnalogPin::init(firstTime);
		setMinMax(0,255);

		// We also need to set the direction pin (digital output)
		pinMode(dirPin, OUTPUT);
		digitalWrite(dirPin, LOW); // direction

		setActuator(0); // initial speed
	}

	// This is specifically tailored for a two directions DC motor and overrides the base method:
	virtual void setActuator(int16_t value) override {
		if (value<=0) {
			digitalWrite(dirPin, LOW);
			analogWrite(pinAnalogOut, value);
		} else {
			digitalWrite(dirPin, HIGH);
			analogWrite(pinAnalogOut, -value);
		}
	}

private:
	uint8_t dirPin;
};
