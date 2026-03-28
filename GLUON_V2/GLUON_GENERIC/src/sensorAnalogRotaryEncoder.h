
#include "BaseSensor.h"

/*
   Class for the analog "Rotary Encoder" from DF ROBOT (www.DFRobot.com), which is just a potentiometer able to detect 12 values, but it is really not linear. Ideally, I should use the sensorDiscreteValues class and a better potenriometer.
   SensorAnalogRotaryEncoder code MODIFIED for N3, because this rotary encoder is a SHIT.
 */

const float potValues[] = { 2.51, 2.64, 2.73, 2.85, 2.93, 2.99, 3.035, 3.07, 3.095, 3.13, 3.15, 3.185};

class SensorAnalogRotaryEncoder : public BaseSensor {
public:

	//SensorAnalogRotaryEncoder() : BaseSensor("12_DIAL", ANALOG_OUTPUT), analogInputPin(A0) {}
	SensorAnalogRotaryEncoder(const char* _name, uint8_t _analogInputPin) : BaseSensor(_name, ANALOG_OUTPUT), analogInputPin(_analogInputPin){}
	virtual ~SensorAnalogRotaryEncoder() override {}

	void init(bool firstTimeBuild = false) {
		setToleranceValue(1);   // Condition value not needed, only tolerance (we will read from 0 to 11, and we want to detect a change every step - tolerance value is inclusive). Also, no need to save on EEPROM (learning is disabled)
		setDetectionMode(ON_SIMPLE_CHANGE);
		disableLearning();      // this way, the tolerance is fixed.
	}

	virtual uint16_t readAnalogOutput() override {
		return(discretizedOutput(NUM_VALUES));
	}

private:

	uint16_t discretizedOutput(uint16_t _numValues);

	//pins:
	uint8_t analogInputPin;
	static const uint8_t NUM_VALUES = 12;
};


uint16_t SensorAnalogRotaryEncoder::discretizedOutput(uint16_t _numValues) {
	static uint16_t value = 0;
	// Assuming the potentiometer is linear, then the discrete value k is:
	// k = NUM_VALUES * analogValue/1023
	// return ((uint16_t)( 1.0*_numValues*analogRead(analogInputPin)/1023 ));

	//... AFTER CHECKING WITH A POTENTIOMETER, I can see that this is not the case. At "0", the voltage is 2.5V...
	// The sequence of voltages is like this: 2.51, 2.65, 2.73, 2.86, 2.94, 2.99, 3.04, 3.07, 3.10, 3.14, 3.16, 3.19
	// This potentiometer is a SHIT. It is supposed to work from 3.3 to 12V... Even the leds get dimmer as we go up.

	float readvalueVolts = 3.3 * analogRead(analogInputPin) / 1023;

	for (uint8_t i = 0; i < 12; i++) {
		if ( (readvalueVolts > (potValues[i] - 0.017) ) && (readvalueVolts < (potValues[i] + 0.017) )) {
			value = i;
			break;
		}
	}

	// PRINT("ADC: ");PRINTLN(readvalueVolts);
	// PRINT("VALUE: ");PRINTLN(value);
	// delay(500);

	return(value);
}
