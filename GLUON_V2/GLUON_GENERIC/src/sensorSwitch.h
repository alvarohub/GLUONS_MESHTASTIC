#include "BaseSensor.h"

/*
  Class for a simple mechanical switch.
  NOTES:
  	- I cannot use normal interruptions (both INT0 and INT1 are already used by the RFM69 module, and by the IR modulation routine), but in the future I can use pin change interrupts.
  	- a "CONDITION" means a press. Hence, ON_ENTERING_COND means the switch has been pressed (pin from HIGH to LOW)
*/

// Analog pins from A0 to A5 can be used as digital I/O, but A4 and A5 are used for I2C,.
//  In general, I will try to use A0 and A1 for analog input, A2 for digital input, and A3 for digital output (analog output - PWM - is D9)

//#define SWITCH_DEBOUNCE_TIME  120 // in ms
//#define digitalInputPin_switch A2

// namespace switch {
// static const uint8_t SWITCH_DEBOUNCE_TIME = 120; // in ms
//static const uint8_t digitalInputPin = A2;
// }

class SensorSwitch : public BaseSensor {
  public:

    // SensorSwitch() : BaseSensor("SWITCH", BOOLEAN_OUTPUT), digitalInputPin(A0) {
    //   pinMode(digitalInputPin, INPUT_PULLUP);
    //   setDebounceTime(SWITCH_DEBOUNCE_TIME);
    // }

    SensorSwitch(const char* _name, uint8_t _digitalInputPin) : BaseSensor(_name, BOOLEAN_OUTPUT) {
      digitalInputPin=_digitalInputPin;
      pinMode(_digitalInputPin, INPUT_PULLUP);
      setDebounceTime(SWITCH_DEBOUNCE_TIME);
    }

	virtual ~SensorSwitch() override {}

    // Initializer: other things, like seting of conditions, detection modes and learning
    void init(bool firstTimeBuild=false) {
      // no need to set, load or save conditions (this is a boolean sensor)
      setDetectionMode(ON_ENTERING_COND); // meaning that we pressed the switch / event set at pressed
      //disableLearning(); // disabled by default in the base class
    }

    virtual bool readBooleanOutput() override {
      return (!digitalRead(digitalInputPin)); // attention: I want a TRUE for pressed.
	  										  // ON_ENTERING_COND means a PRESS.
    }

  private:
    uint8_t digitalInputPin;
    static const uint8_t SWITCH_DEBOUNCE_TIME = 35; // in ms
};
