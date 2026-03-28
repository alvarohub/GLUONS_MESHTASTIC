
#include "Arduino.h"
#include "BaseActuator.h"

// I2C LED BAR (10 leds).
// Note: in the future, we could have a generic "display class"?

// NOTE on I2C on the Arduino (or Moteino):
//              A5 (analog 5) is the clock (SCL)
//              A4 (analog 4) is the data (SDA)

#include <Wire.h>
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"

#define DEFAULT_SCROLL_DELAY 500 // in ms
#define MAX_LENGTH_SCROLL 17 // max length of the scrolling text (+1)

// I2C pins:
//#define clockPin A5
//#define dataPin A4

class DisplayAlphaNum4: public BaseActuator {
public:
	// Parameter-less constructor:
	DisplayAlphaNum4() : BaseActuator("AlphaNum4"), scrollDelay(DEFAULT_SCROLL_DELAY) {
		myDisplay = new Adafruit_AlphaNum4();
	}
	virtual ~DisplayAlphaNum4() override { delete myDisplay; }

	void init(bool firstTimeBuild)
	{
		BaseActuator::init(firstTimeBuild);

		myDisplay->begin(0x70); // (default I2C address without soldering the pads)
		myDisplay->setBrightness(8);
		setDisplayString("");   // the actual thing to display
		lastTimeDisplayScroll = millis();
	}

	// Implementation of the virtual methods (we can add virtual if we want, but override make it clear - and compile safe (it is C++11 though... hope it works here))
	virtual void update() override;

	// methods only for this child:
	void setDisplayString(const char* str)
	{
		strcp(scrollingString, str);
		strcat(scrollingString, "   "); // I add three blank spaces for a proper circular scroll
	}
	void setScrollDelay(unsigned long _scrolldelay)
	{
		scrollDelay = _scrolldelay;
	}

protected:

	// static const uint8_t clockPin=A5;
	// static const uint8_t dataPin=A4;

	Adafruit_AlphaNum4* myDisplay;// bar(9, 8, 0);  // Clock pin, Data pin, Orientation

	char scrollingString[MAX_LENGTH_SCROLL];
	unsigned long scrollDelay, lastTimeDisplayScroll;
	uint8_t indexStroll;
};

void DisplayAlphaNum4::update()
{
	if ( isNewData() ) {
		setDataAsUsed();                                // In any case, signal that we "used" the data
		setDisplayString(myData.stringData);
		indexScroll = 0;
		// start displaying:
		displayShifted(0);
	} else {
		// continue scrolling:
		if (millis() - lastTimeDisplayScroll > scrollDelay) {
			displayShifted(indexScroll++);
			lastTimeDisplayScroll = millis();
		}
	}
}

void DisplayAlphaNum4::displayShifted(uint8_t shift)
{
	// The matrix has only four display characters:
	for (uint8_t i = 0; i < 4; i++) {
		uint8_t charAt = (i + shift) % strlen(scrollingString);
		myDisplay->writeDigitAscii( i, scrollingString[charAt]);
	}
	myDisplay->writeDisplay();
}
