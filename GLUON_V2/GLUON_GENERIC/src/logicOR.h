
// Operation: OR
// Update mode: SENSOR_EVENT_BANG and ANY_INLET_BANG

#include "BaseLogic.h"

class SimpleOR : public BaseLogic {

public:
	SimpleOR() : BaseLogic("OR") {}

	void init(bool firstTimeBuild) {
		if (firstTimeBuild)
			setUpdateMode(SENSOR_EVENT_BANG);
		else
			loadUpdateMode();
	}

	virtual void compute() override;

private:

};

void SimpleOR::compute() {

	// Note: all data in the inlets is used, EVEN if it is not NEW...
	bool result = inletArrayPtr->checkEvent();  // this checks ANY event (equivalent to a for loop on result |= inletArrayPtr)[i]->checkEvent(); or result |= (*inletArrayPtr)[i].getData().event; )
	inletArrayPtr->setAllInletsDataUsed(); // equivalent to a for loop (*inletArrayPtr)[i].setNewDataFlag(false);

	// Also OR this with the sensor event:
	result |= sensorArrayPtr->checkEvent();  // this checks for an event on ANY sensor on the array.
	// If I want to check an event on a PARTICULAR sensor, I can do: (*sensorArrayPtr)[i]->checkEvent();
	// note: there is no meaning to set the sensor data as "used" since we can always afresh.

	// Save this on all the output(s). Note that the NEW DATA flag on the outlet is NEW each time a computation if performed, regardless of its value (boolean in this case).
	// And as a reminder, computation is triggered either by the synchronization of timestamps, manually, or by new data on a special "bang" inlet.

	// Create the data to be stored - for the time being we don't care about all the parameters, only the event (we can specify this dynamically using
	// the string, for example putting "event" on it)
	Data outputData(result);
	outletArrayPtr->setNewData(outputData); // this saves the data into all the outlets, and tells them that the data is "new"

	// .. and also set the actuator(s) (if we want)
	outputData.numericData = 128;
	actuatorArrayPtr->setNewData(outputData);
}
