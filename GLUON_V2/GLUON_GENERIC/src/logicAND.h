
// Very simple logic thing for the time being:
// The computation does jus this:  perform a logic AND  between inlets and sensor that are in SYNCH,
// and set the result to all outputs and actuators
#include "BaseLogic.h"

class SimpleAND : public BaseLogic {

public:
	SimpleAND() : BaseLogic("&") {}

	void init(bool firstTimeBuild) {
	   if (firstTimeBuild)
		setUpdateMode(SENSOR_EVENT_BANG | ANY_INLET_BANG);
	   else
		loadUpdateMode();
	}

	virtual void compute() override;

private:

};

void SimpleAND::compute() {

	// In this simple test, we will send the AND of all the inlets EVENTS to all the outlets:
	// Note that the execution of this computation is done depending on the synchMode (synchrounous, bang, manual or any)
	bool result = false;

	for (uint8_t i = 0; i < inletArrayPtr->size(); i++) {
		result &= inletArrayPtr->checkEvent(i); // same as (*inletArrayPtr)[i].checkEvent() or (*inletArrayPtr)[i].getData().event
	}
	inletArrayPtr->setAllInletsDataUsed();

	// Also AND this with ANY sensor event? no
	//result &= sensorArrayPtr->checkEvent();

	// Create the data to be stored on the output - for the time being we don't care about all the parameters,
	// only the event (we can specify this dynamically using the string, for example putting "event" on it)
	Data outputData(result);
	//Data(String _stringData, uint16_t _numericData, bool _event, unsigned long _timeStamp): stringData(_stringData), numericData(_numericData),
	outletArrayPtr->setNewData(outputData); // this saves the data into all the outlets, and tells them that the data is "new"

	// .. and also set the actuator:
	outputData.numericData=255;
	actuatorArrayPtr->setNewData(outputData);

}
