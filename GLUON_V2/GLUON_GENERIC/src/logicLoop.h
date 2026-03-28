
// Operation: LOOP a certain number of times (then it reset to the original value)
// INPUTS: 2 (one for decreasing the counter, another to set the value of the counter)
// OUTPUTS: a BANG with the sensor data (if any) each time the counter is decreased
// Update mode: ANY_INLET_BANG (but we will do different things)
#include "BaseLogic.h"

class Loop : public BaseLogic {

public:
	Loop() : BaseLogic("LOOP") {}

	void init(bool firstTimeBuild) {

	   if (firstTimeBuild)
			setUpdateMode(ANY_INLET_BANG);
	   else
		loadUpdateMode();

		counterTop = 10;
		counter = 10;
	}

	virtual void compute() override;

private:

	int counter, counterTop;

};

void Loop::compute() {

	// a) IF THERE IS NEW DATA on the SECOND inlet, use the numeric data to set the counter:
	if ((*inletArrayPtr)[1].checkOutData().event) { // checkOutData set the data as used, so we won't be resetting the counter top all the time
		counterTop = (*inletArrayPtr)[0].getData().numericData;
	}

	// b) if there is an EVENT on first inlet, decrease the counter and send a bang when the counter reaches ZERO:
	if ((*inletArrayPtr)[0].checkOutData().event) {
		counter--;
		if (counter == 0) {
			counter = counterTop;

			// Appropiately create the output data:
			// Check the sensor event (if there is a sensor) to compose the analog part of the data.
			// Note: we can even use the string to specify WHICH module zeroed, but this is up to the logic part of the nodes connected to this node.
			Data outputData(F("counter_zero"), sensorPtr->getData().numericData, true, millis());

			// Save this on all the output(s), if any:
			outletArrayPtr->setNewData(outputData); // this saves the data into all the outlets, and tells them that the data is "new"

			// Set the actuator (for instance an LED), with a specific value:
			Data ledData(F("PULSE_LONG")); // use the string to set the mode of the led:
			actuatorArrayPtr->setNewData(ledData);
		}

		// We can also indicate on this actuator that there was a simple decrease:
		Data ledData(F("PULSE_SHORT")); // use the string to set the mode of the led:
		actuatorArrayPtr->setNewData(ledData);
	}


}
