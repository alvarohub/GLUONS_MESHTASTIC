/*
Class for the analog SHARP RANGEFINDER
*/

#include "BaseSensor.h"

class SensorSharp : public BaseSensor {
public:

	// Constructor: basic definition (name, type output)
	//SensorSharp() : BaseSensor ("IRANGE", ANALOG_OUTPUT), analogInputPin(A6) {}
	SensorSharp(const char* _name, uint8_t _analogInputPin) : BaseSensor (_name, ANALOG_OUTPUT), analogInputPin(_analogInputPin) {}
	virtual ~SensorSharp() override {}

	// Initializer: other things, like seting of conditions, detection modes and learning
	void init(bool firstTimeBuild=false) {
		BaseSensor::init(firstTimeBuild);
		if (firstTimeBuild) {
			// Save on memory the initial sensor(s) conditions (if needed!!):
			setConditions(15, 2);
			saveConditions();
		} else 
			loadConditions();

		// TODO: detection mode and learning also on EEPROM?
		setDetectionMode(ON_ENTERING_COND); // meaning that we go from a value not in range, to a value in range.
		enableLearning(); // we set default condition range, but enable learning (using IR or button)
	}

	// Overriding implementation of the base class virtual function:
	//virtual void updateData() override ;
	virtual uint16_t readAnalogOutput() override {
		return (distanceCm());
	}

private:
	uint8_t analogInputPin;
	uint16_t distanceCm();
};

// conversion function for the short range sharp sensor:
//uint16_t SensorSharp::distanceCm() {
//  // This is a formula that works sensibly well - but for more precision, it would be good to fit the curve:
//  //distance = (0.30*distance+(1-0.30)*(6000.0/analogRead(pinAnalog))); // filter (but we would need to pull all the time)
//  float distance = 6000.0 / analogRead(analogInputPin);
//  if (distance > 30) distance = 30;
//  return ((uint16_t)distance);
//}

// conversion function for the LONG range sharp sensor (2YOAO2), assuming 10bit conversion, from 0 to 3.3V
uint16_t SensorSharp::distanceCm() {
	float distance = 16096.0 / analogRead(analogInputPin) + 4.4;
	if (distance > 200) distance = 200;
	return ((uint16_t)distance);
}
