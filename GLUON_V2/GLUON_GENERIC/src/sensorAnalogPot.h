
#include "BaseSensor.h"

/*
Class for the analog Potentiometer. I will normalize the output to 0-1023
*/


class SensorAnalogPot : public BaseSensor {
public:

	//SensorAnalogPot() : BaseSensor("POT", ANALOG_OUTPUT), analogInputPin(A6) {}
	SensorAnalogPot(const char* _name, uint8_t _analogInputPin) : BaseSensor (_name, ANALOG_OUTPUT), analogInputPin(_analogInputPin) {}
	virtual ~SensorAnalogPot() override {}

	void init(bool firstTimeBuild=false) {
		if (firstTimeBuild) {
			// Save on memory the initial sensor(s) conditions (if needed!!):
			setToleranceValue(16); // only tolerance is needed here to detect a CHANGE. Note: 16 means 1024/16= 64 LEVELS detected...
			saveConditions();
		} else {
			loadConditions();
		}
		setDetectionMode(ON_SIMPLE_CHANGE); // meaning that we changed more than the tolerance value
		disableLearning();
	}

	// Overriding implementation of the base class virtual function:
	//virtual void updateData() override ;
	virtual uint16_t readAnalogOutput() override;

private:

	// pins:
	uint8_t analogInputPin;

};

uint16_t SensorAnalogPot::readAnalogOutput() {
	return (analogRead(analogInputPin));
}
