
#include "Arduino.h"
#include "actuatorBaseAnalog.h"
#include "LedControl.h"

// Seven Segment Display (up to 8 digits) using MAX7221 (SPI).
// "LedControl.h" uses SOFTWARE SPI (shifOut, shiftIn), so the pins are arbitrary.
// I will use by default:
//              A2 as MOSI (toDIN)
//              A3 as clock (toCLK)
//              A4 as CS (toCS)

class DisplaySevenSegment_MAX7221: public BaseActuatorAnalog {
public:
	// Parameter-less constructor (assumes max 8 digits)
	// DisplaySevenSegment_MAX7221() : BaseActuatorAnalog("7Seg", UPDATE_ALWAYS), numDigits(8) {
	//      myDisplay = new LedControl(A2, A3, A4, 1); // 1 means only one MAX7221
	// }
	DisplaySevenSegment_MAX7221(uint8_t _numDigits, uint8_t _toDIN, uint8_t _toCLK, uint8_t _toCS) : BaseActuatorAnalog("7Seg", UPDATE_ALWAYS), toDIN(_toDIN), toCLK(_toCLK), toCS(_toCS){
		myDisplay = new LedControl(toDIN, toCLK, toCS, 1); // last 1 means only one MAX7221
		numDigits = _numDigits;
	}
	virtual ~DisplaySevenSegment_MAX7221() override { delete myDisplay; }

	void init(bool firstTimeBuild)
	{
		// Set things for an "analog" actuator
		BaseActuatorAnalog::init(firstTimeBuild);
		int16_t halfRange = pow(10, numDigits) - 1;

		setMinMax(-halfRange, halfRange); // ex: for a 3 digit display, this goes from -999 to 999

		// Set things particular to this device:
		// The MAX72XX is in power-saving mode on startup, we have to do a wakeup call:
		myDisplay->shutdown(0, false);
		// Set the brightness to a medium value (min is 0, max is 15):
		myDisplay->setIntensity(0, 15);
		// clear the display:
		myDisplay->clearDisplay(0);

		// Set initial value:
		setActuator(0);
	}

	virtual void setActuator(int16_t value) override;

protected:

	//pins (software SPI):
	/*
	   Arduino				MAX7221
	   MOSI		-->     DIN (pin 1)
	      SS			-->		LOAD (~CS)
	      SCK			-->     CLK

	      Note: is using software SPI, MOSI (toDin), SS (toCS) and SCK (toCKK) can be anything.
	                Otherwise, in the case of the UNO, these pins are respectively:
	                MOSI = pin 11, SCK = pin 13, and SS = pin 10 (slave).
	                These pins are being used by the RFM69 module (uses hardware SPI). Of course
	                we could disable/enable the chip select (CS) pins alternatively, but it complicates things...
	 */
	uint8_t toDIN, toCLK, toCS;
	uint8_t numDigits;

	LedControl* myDisplay;
};


void DisplaySevenSegment_MAX7221::setActuator(int16_t val)   // can be positive or negative
{       /*
	 * Display a hexadecimal digit on a 7-Segment Display
	 * Params:
	 * addr	address of the display (matrix/display number - here only one, hence 0)
	 * digit	the position of the digit on the display (0..7)
	 * value	the value to be displayed. (0x00..0x0F)
	 * dp	sets the decimal point:

	                        setDigit(int addr, int digit, byte value, boolean dp);
	 */

	boolean sign = false;

	if (val < 0) {
		sign = true;
		val = -val; //make _val positive (note: for metronome, this will be always positive anyway)
	}
	myDisplay->clearDisplay(0);
	uint8_t digit = 0;
	while ((val > 0) && (digit < numDigits - 1 )) {
		myDisplay->setDigit(0, digit, val % 10, false);
		val /= 10;
		digit++;
	}
	// The final digit has the sign (the dot in the last digit will indicate negative values)
	if (val>0) {
		myDisplay->setDigit(0, digit, val % 10, sign);
	} else {
		if (sign) myDisplay->setDigit(0, digit, 11, false); // this means we outed because val is 0 (11 means '-')
	}

}
