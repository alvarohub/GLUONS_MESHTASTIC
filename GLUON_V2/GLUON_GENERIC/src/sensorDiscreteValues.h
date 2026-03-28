#ifndef _SensorDiscreteValues_h_
#define _SensorDiscreteValues_h_

#include "BaseSensor.h"

/*
   Class for an analog sensor on an ANALOG PIN, with DISCRETIZED values (from 0 to numLevels-1)
	NOTE1: this could be an abstract class for children on analog pins or another thing (I2C, etc), but here we will limit to just analog input pins.
	NOTE2: an analog pin that is also a GPIO also have internal pullup set (this is useful for instance to detect level on sensors that work by changing the resistance only...). Unfortunately, A6 and A7 are pure analog pins, hence NO internal pullup cannot be set.
    NOTE3: Levels are calculated with thresholds either:
                - LINEARLY spaced in the interval between maxAnalogValue and minAnalogValue
                - arbitrary (need to be loaded on the vector of voltLevelThresholds): NOT DONE YET (memory problems)
 */

class SensorDiscreteValues: public BaseSensor {
public:

	SensorDiscreteValues(const char* _name, uint16_t _numLevels, float _maxVolts, float _minVolts, uint8_t _analogInputPin, uint8_t _inputMode = INPUT) : BaseSensor(_name, ANALOG_OUTPUT), analogInputPin(_analogInputPin), numLevels(_numLevels), maxAnalogValue(_maxVolts), minAnalogValue(_minVolts) {
		// Setting pullups for the analog pins (this works for analog pins too!)
		// In case of a simple variable resistor (or a switch) it may come handy.
		//pinMode(analogInputPin, _inputMode); // INPUT, INPUT_PULLUP (note: A6 and A7 cannot be set with pullup)
	}
	virtual ~SensorDiscreteValues() override {}

	// void setLevels(uint8_t _numLevels) {numLevels=_numLevels;}
	// void setMaxValue(float _max) {maxAnalogValue=_max;}
	// void setMinValue(float _min) {minAnalogValue=_min;}

	void init(bool firstTimeBuild = false)
	{
		// We will read from 0 to numValues, and we want to detect a change every step - tolerance value is inclusive).
		setToleranceValue(1);                   // Condition value not needed, only tolerance.
		// Also, no need to save on EEPROM (learning is disabled)
		setDetectionMode(ON_SIMPLE_CHANGE);     // REM: on simple change with respect to the DISCRETIZED output of course.
		//disableLearning(); // disabled by default in the base class (the tolerance is fixed here)
	}

	virtual uint16_t readAnalogOutput() override
	{
		uint16_t level = 0;
		float readVolts= 3.3/1023*analogRead(analogInputPin); // 10 bit precision (from 0 to 1023). This gives a value between 0 and 1.
		float intervalVolts = 1.0 * (maxAnalogValue - minAnalogValue) / numLevels;
		// numLevels should be > 0. So numLevels - 1 is positive
		while ( (level < uint16_t(numLevels - 1)) && ( readVolts > 1.0 * (level + 1) * intervalVolts + minAnalogValue ) ) level++;
		return (level);
	}

private:
	uint8_t analogInputPin;
	//vector <float> voltLevelThresholds;
	uint8_t numLevels;
	float maxAnalogValue, minAnalogValue;
};

#endif
