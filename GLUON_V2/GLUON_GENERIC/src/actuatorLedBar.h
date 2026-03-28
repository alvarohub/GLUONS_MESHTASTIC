
#include "Arduino.h"
#include "actuatorBaseAnalog.h"

// LED BAR (10 leds) controlled by serial interface
// Note: in the future, we could have a generic "display class"?

// NOTE: this led bar from GROVE does not uses I2C or ISP,
// but it own serial protocol on any digital pin.
// We will use by default:
//              A2 is the clock
//              A3  is the data

#include <Grove_LED_Bar.h>

class ledBar: public BaseActuatorAnalog {
public:
	// Parameter-less constructor:
	ledBar() : BaseActuatorAnalog("LedBar", UPDATE_ALWAYS) {
		pinMode(A2, OUTPUT);                            //pinMode(clockPin, OUTPUT);
		pinMode(A3, OUTPUT);                            //pinMode(dataPin, OUTPUT);
		myLedBar = new Grove_LED_Bar(A2, A3, 1);        // (clockPin, dataPin, 1); // last value is the orientation
	}
	virtual ~ledBar() override { delete myLedBar; }

	void init(bool firstTimeBuild)
	{
		BaseActuatorAnalog::init(firstTimeBuild);

		myLedBar->begin();
		setMinMax(0, 10);
		setActuator(5);
	}

	virtual void setActuator(int16_t value) override
	{
		myLedBar->setLevel(value);
	}

protected:

	//      static const uint8_t clockPin=A2;
	//      static const uint8_t dataPin=A3;

	Grove_LED_Bar* myLedBar;// bar(9, 8, 0);  // Clock pin, Data pin, Orientation
};
