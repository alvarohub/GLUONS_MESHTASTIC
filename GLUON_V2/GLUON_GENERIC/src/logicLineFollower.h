
// Operation: LINE FOLLOWER
// Update mode: ANY_INLET_BANG (there will be two active inlets, one for each line sensor)
#include "BaseLogic.h"

class LineFollower : public BaseLogic {

public:
	LineFollower() : BaseLogic("FOLLOW") {}

	void init(bool firstTimeBuild) {

	    if (firstTimeBuild)
			setUpdateMode(ANY_INLET_BANG);
	   else
		loadUpdateMode();
	}

	virtual void compute() override;

private:

};

void LineFollower::compute() {

	// We will use the data on the first two inlets:
	bool lineOnRight, lineOnLeft;
	int valueMotorDC; // can be positive or negative (direction)

	// There are two options of how this may be done: either the other node sensors ALREADY send data, or they just send DIGITAL events,
	// indicating that the sensor detected the line! I will assume this second option:

	lineOnRight=(*inletArrayPtr)[0].checkOutData().event; // note: checkOutData also set the data flag as used
	lineOnLeft=(*inletArrayPtr)[1].checkOutData().event;

	// THE LOGIC OF THE LINE FOLLOWER:
	if ( lineOnRight && !lineOnLeft ) valuePWM=-255;  // this means that we need to move away, towards the LEFT:
	else if (!lineOnRight && lineOnLeft ) valuePWM=255;
	else if (lineOnRight && lineOnLeft ) value=0; // this means that the line is too big...
	else if (!lineOnRight && !lineOnLeft ) value=0;  // this means that we went outside...

	// Also OR this with the sensor event (if there is a sensor)
	//result |= sensorPtr->checkEvent();  // equivalent to sensorPtr->getData().event;

	// Appropiately create the output data:
	Data outputData(valuePWM);

	// Save this on all the output(s), if any:
	//Data outputData(result);
	//outletPtr->setNewData(outputData); // this saves the data into all the outlets, and tells them that the data is "new"

	// Set the actuator (here it should be a DC motor:)
	actuatorArrayPtr->setNewData(outputData);
}
