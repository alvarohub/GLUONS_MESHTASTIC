
// This is a list of STANDARD/COMMON basic digital actuators derived from BaseActuatorDigitalPin with some fixed properties.
// NOTE  The init() method of the BaseActuatorDigitalPin will be called by the NodeClass, and it should NOT be overriden here.
// NOTE: In the Moteino R4, there is a LED connected to pin 9 (which is PWM capable by the way, so I will reserve it for analog output)
// NOTE: Pins A0 to A7 are analog, but A0 to A5 can be used as digital IO.

#include "Arduino.h"
#include "actuatorBaseDigital.h"

#define defaultPinDigitalOut A3

// Simple digital pin:
class BaseActuatorDigitalPin : public BaseActuatorDigital {
public:
	// Reminder: Base class constructors are automatically called for you if they have no argument; If I want to call a superclass
	// constructor with an argument, I MUST use the subclass's constructor initialization list.
	//BaseActuatorDigitalPin() : BaseActuatorDigital("D-OUT", PULSE), pinDigitalOut(defaultPinDigitalOut) {}
	BaseActuatorDigitalPin(const char* _name, uint8_t _pin, ModeDigitalOut _mode) : BaseActuatorDigital(_name, _mode), pinDigitalOut(_pin) {}
	virtual ~BaseActuatorDigitalPin() override {}; // nothing needs to be done

	void init(bool firstTimeBuild) {
		BaseActuatorDigital::init(firstTimeBuild); // call the parent initializer...
		// then set things as needed for this digital PIN actuator:
		pinMode(pinDigitalOut, OUTPUT);
         // set the initial state:
        setActuator(false);
    }

	// The overriden setActuator method for a simple digital pin is just "digitalWrite"
	void setActuator(bool _state) override  {digitalWrite(pinDigitalOut, _state);}

protected:
	uint8_t pinDigitalOut;
};

// NOTE: the following classes can be extended to have more specific functions.
// For memory reasons, I will avoid them and just use BaseActuatorDigitalPin with the proper mode.

class ActuatorLedNormal : public BaseActuatorDigitalPin {
public:
  ActuatorLedNormal() : BaseActuatorDigitalPin("LED-N", A2, NORMAL) {}
  ActuatorLedNormal(const char* _name, uint8_t _pin) : BaseActuatorDigitalPin(_name, _pin, NORMAL) {}
  virtual ~ActuatorLedNormal() override {}; // nothing needs to be done

  void init(bool firstTimeBuild) {
	  BaseActuatorDigitalPin::init(firstTimeBuild);
	  setActuator(false); // set the initial state
  }
};


// Buzzer (not just a piezo: when its digital input pin is HIGH, it BEEPS). It will use PULSE mode:
class ActuatorBuzzer : public BaseActuatorDigitalPin {
public:
	//ActuatorBuzzer() : BaseActuatorDigitalPin("BUZZ", A1, PULSE) {}
	ActuatorBuzzer(const char* _name, uint8_t _pin) : BaseActuatorDigitalPin(_name, _pin, PULSE) {}
	virtual ~ActuatorBuzzer() override {}; // nothing needs to be done

	void init(bool firstTimeBuild) {
		BaseActuatorDigitalPin::init(firstTimeBuild);
		setActuator(false); // set the initial state
	}
};

class ActuatorLedPulse : public BaseActuatorDigitalPin {
public:
	ActuatorLedPulse() : BaseActuatorDigitalPin("LED-P", A2, PULSE) {}
  	ActuatorLedPulse(const char* _name, uint8_t _pin) : BaseActuatorDigitalPin(_name, _pin, PULSE) {}
	virtual ~ActuatorLedPulse() override {}; // nothing needs to be done

	void init(bool firstTimeBuild) {
		BaseActuatorDigitalPin::init(firstTimeBuild);
		setActuator(false); // set the initial state
	}
};

class ActuatorLedToggle : public BaseActuatorDigitalPin {
public:
  ActuatorLedToggle() : BaseActuatorDigitalPin("LED-T", A2, TOGGLE) {}
  ActuatorLedToggle(const char* _name, uint8_t _pin) : BaseActuatorDigitalPin(_name, _pin, TOGGLE) {}
  virtual ~ActuatorLedToggle() override {}; // nothing needs to be done

  void init(bool firstTimeBuild) {
	  BaseActuatorDigitalPin::init(firstTimeBuild);
	  setActuator(false); // set the initial state
  }
};


// This can be used to switch on/off equipment using a power transistor:
class ActuatorPowerMosfet : public BaseActuatorDigitalPin {
public:
	ActuatorPowerMosfet() : BaseActuatorDigitalPin("SWITCH", A2, NORMAL) {} //note the use of NORMAL mode (ON/OFF follows the value of event)
	virtual ~ActuatorPowerMosfet() override {}; // nothing needs to be done

	void init(bool firstTimeBuild) {
		BaseActuatorDigitalPin::init(firstTimeBuild);
		setActuator(false); // set the initial state
	}
};
