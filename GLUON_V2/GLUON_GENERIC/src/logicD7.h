
/*

Logic function for the module D7
- Button Switch
- 3 LEDS

Operation:

- WHEN: when receiving something on any inlet, or when pressing the button
- ACTUATOR: LEDs blink when there is a TRUE event data on corresponding inlet
- OUT: only when pressing the button

*/

#include "BaseLogic.h"

class logicD7 : public BaseLogic {

public:
	logicD7() : BaseLogic("D7") {}

	void init(bool firstTimeBuild) {
		if (firstTimeBuild)
		setUpdateMode(ANY_INLET_BANG | SENSOR_EVENT_BANG);
		else
		loadUpdateMode();
	}

	virtual void compute() override;

private:

};

void logicD7::compute() { // this will be executed when there is a bang on first inlet, so I don't need to re-check that.
Data outputData;

// 1) Copy EVENT values for each INLET (with NEW data) on the array of ACTUATORS (RGB leds):
for (uint8_t i = 0; i < actuatorArrayPtr->size(); i++) {
	if ((*inletArrayPtr)[i].isNewData()) {
	outputData=inletArrayPtr->getData(i); // equal to: (*inletArrayPtr)[i].getData();
	(*actuatorArrayPtr)[i].setNewData(outputData);
	}
}

// 2) OUTPUT "event" from switch (numeric Data is arbitrary)
if (sensorArrayPtr->checkEvent()) { // this is TRUE if we entered here because of a sensor event bang (on ANY sensor...)
	outputData.set("button", 255, true, millis()); // numeic data is arbitrary, here 255...
	// Data(String _stringData, uint16_t _numericData, bool _event, unsigned long _timeStamp)
	outletArrayPtr->setNewData(outputData); // this saves the data into all the outlets, and tells them that the data is "new"
}

// 3) Finally, it is important to reset the "new" data flag (the bang), at least for the first inlet, but we do all:
inletArrayPtr->setAllInletsDataUsed();

}
