/*
Class for the temperature sensor INCLUDED on the RFM96 module
*/

#include "BaseSensor.h"
#include "moduleRF.h" // we are going to access the pre-instantiated myRF object

class SensorTemperature: public BaseSensor {
public:

	SensorTemperature() : BaseSensor("TEMP",ANALOG_OUTPUT) {}
	virtual ~SensorTemperature() override {}

	// Initializer: other things
	void init(bool firstTimeBuild=false) {
		if (firstTimeBuild) {
			// Save on memory the initial sensor(s) conditions (if needed!!):
			setConditions(25, 2);
			saveConditions();
		} else {
			loadConditions();
		}
		setDetectionMode(ON_ENTERING_COND); // meaning that we go from a value not in range, to a value in range.
		enableLearning(); // we set default condition range, but enable learning (using IR or button)
	}

	// Overriding implementation of the base class virtual function:
	virtual uint16_t readAnalogOutput() override {
		return(myRF.getTemperature());
	}

};
