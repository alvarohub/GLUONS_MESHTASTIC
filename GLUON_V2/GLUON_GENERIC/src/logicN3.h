// LOGIC FOR THE DIAL MODULE

// Operation: check sensor analog rotary encoder
// Output: the sensor value
// Action: display sensor value (if there is an actuator)
// Update mode: SENSOR_EVENT_BANG and ANY_INLET_BANG

#include "BaseLogic.h"

class logicN3 : public BaseLogic {

public:
	logicN3() : BaseLogic("DIAL") {}

	void init(bool firstTimeBuild) {
		if (firstTimeBuild)
			setUpdateMode(SENSOR_EVENT_BANG | ANY_INLET_BANG);
		else
			loadUpdateMode();
	}

	virtual void compute() override;

private:

};

void logicN3::compute() {
	//Data outputData;

	// In case of rotary encoder sensor ("dial"), we want to send the value to the outlet WHENEVER THERE IS A BANG or a CHANGE ON THE ROTARY ENCODER VALUE. In the first case, the "event" may be false, unless the inlet is carring a true event, hence the logic OR:

	// For anaheim video shooting scenarios:
	Data outputData((*sensorArrayPtr)[0].getData()); // from FIRST sensor (even if there are others...)
	//outputData.numericData *=15; //multiplier (used for frequency setting for instance) could be set by the learning "dial". But better, just use a real rotary encoder or a potentiometer.

	// SOMETHING IS WRONG HERE!!
	//if (inletArrayPtr->isSomeBang()) outputData.event |= inletArrayPtr->checkEvent();

	// set output:
	outletArrayPtr->setNewData(outputData); // this saves the data into all the outlets, and tells them that the data is "new"

 	// reset the bang:
	inletArrayPtr->setAllInletsDataUsed();
}
