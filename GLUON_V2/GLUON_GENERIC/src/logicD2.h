
/*

Logic function for the module D2 :

- SERVO motor
- no sensor

Operation:

- WHEN: when receiving data on any inlet
- ACTUATOR: set depending on the inlet-bang. If first, then copy numericData. If second, go to min, if third, to max.
- OUT: the output is the data on the inlet that generated the bang

*/
#include "BaseLogic.h"

class logicD2 : public BaseLogic {

public:
	logicD2() : BaseLogic("SERVO") {}

	void init(bool firstTimeBuild) {
		if (firstTimeBuild)
		setUpdateMode(ANY_INLET_BANG);
		else
		loadUpdateMode();
	}

	virtual void compute() override;

private:

};

void logicD2::compute() {
	Data outputData; // this sets new data as true

	// 1) Process INPUT: get the data from the inlet that generated the event:
	for (uint8_t i = 0; i < inletArrayPtr->size(); i++) {
		if (inletArrayPtr->isBang(i)) outputData=inletArrayPtr->getData(i);
	}

	// 2) set OUTPUT to the value of the inlet data that generated the event:
	outletArrayPtr->setNewData(outputData);

	// 3) set ACTUATOR:
 	if (inletArrayPtr->isBang(1)) outputData.numericData=0;
	// Third inlet: move servo to max position:
	else if (inletArrayPtr->isBang(2)) outputData.numericData=180;
	// otherwise, it is the first inlet data, and we don't change the numeric value:
	actuatorArrayPtr->setNewData(outputData);

	// 4) Reset the new data flag:
	inletArrayPtr->setAllInletsDataUsed(); // equivalent to a for loop (*inletArrayPtr)[i].setNewDataFlag(false);
}
