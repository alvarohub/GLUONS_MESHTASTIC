
#include "BaseSensor.h"

/*
	Class for the analog LDR (light sensitive resistor).
	NOTE: I could have used internal pullups for the analog pin, but we may need to tune the resistor in series depending on the LDR characteristics. In this case, it is 20KOhms (so I could have used internal pullups...)
*/


//#define analogInputPin A0

class SensorLDR : public BaseSensor {
public:

	//SensorLDR() : BaseSensor("LDR", ANALOG_OUTPUT), analogInputPin(A6) {}
	SensorLDR(const char* _name, uint8_t _analogInputPin) : BaseSensor (_name, ANALOG_OUTPUT), analogInputPin(_analogInputPin) {} // no internal pull-up resistors set.
	virtual ~SensorLDR() override {}

	void init(bool firstTimeBuild=false) {
		if (firstTimeBuild) {
			// Save on memory the initial sensor(s) conditions (if needed!!):
			setConditions(7500, 600);
			saveConditions();
		} else {
			loadConditions();
		}

		setDetectionMode(ON_ENTERING_COND); // meaning that we go from a value not in range, to a value in range.
		enableLearning(); // we set default condition range, but enable learning (using IR or button)
	}

	// Overriding implementation of the base class virtual function:
	//virtual void updateData() override ;
	virtual uint16_t readAnalogOutput() override;

private:

	// pins:
	uint8_t analogInputPin;

};

uint16_t SensorLDR::readAnalogOutput() {
	return (analogRead(analogInputPin));
}
