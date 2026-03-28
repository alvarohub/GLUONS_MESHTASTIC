
#include "Arduino.h"
#include "actuatorBaseAnalog.h"

// I2C Display Seven Segment (4 digits)
// Note: in the future, we could have a generic "display class"?

// It is an I2C device. If we use the harware I2C, the pins are not negociable:
// On the Arduino (or Moteino), these are:
// 		A5 (analog 5) is the clock (SCL)
// 		A4 (analog 4) is the data (SDA)

#include <Wire.h>
#include "Adafruit_LEDBackpack.h"
//#include "Adafruit_GFX.h"

class DisplaySevenSegment_I2C : public BaseActuatorAnalog {
public:
	// Parameter-less constructor:
	// DisplaySevenSegment_I2C() : BaseActuatorAnalog("7Seg_4Dig", UPDATE_ALWAYS) {
	// 	myDisplay = new Adafruit_7segment();
	// }
	DisplaySevenSegment_I2C(const char* _name) : BaseActuatorAnalog(_name, UPDATE_ALWAYS) {
		myDisplay = new Adafruit_7segment();
	}
	virtual ~DisplaySevenSegment_I2C() override { delete myDisplay;}

	void init(bool firstTimeBuild) {
		BaseActuatorAnalog::init(firstTimeBuild);

		myDisplay->begin(0x70);  // (default I2C address without soldering the pads)
		myDisplay->setBrightness(15);

		setMinMax(-999, 999);

        // TEST actuator:
        //for (int16_t i = min; i <max ; i++) {setActuator(i); delay(10);}
        setActuator(0);
	}

	virtual void setActuator(int16_t value) override;

protected:

  //pins (I2C pins)
  //static const uint8_t clockPin=A5;
  //static const uint8_t dataPin=A4;

	Adafruit_7segment* myDisplay;
};


void DisplaySevenSegment_I2C::setActuator(int16_t _val) { // can be positive or negative
    //PRINTLN(_val);
	myDisplay->print(_val, DEC);
	myDisplay->writeDisplay(); 	// latch (refresh)
	// no scroll for the time being...
}
