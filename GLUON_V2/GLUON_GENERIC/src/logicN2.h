// Logic for the BUZZER/LED MODULE

// Operation: OR (modified)
// Update mode: ANY_INLET_BANG

#include "BaseLogic.h"

class logicN2 : public BaseLogic {

public:
	logicN2() : BaseLogic("N2") {}

	void init(bool firstTimeBuild) {

		if (firstTimeBuild)
			setUpdateMode(ANY_INLET_BANG);
		else
			loadUpdateMode();
	}

	virtual void evolve() override;
	virtual void compute() override;

private:

};


void logicN2::evolve()
{

	// for tests:
	// arg, this is true: error: 'class BaseActuator' has no member named 'setActuator'
	//(*actuatorArrayPtr)[0].setActuator( millis()%3000 == 0);
	// (*actuatorArrayPtr)[1].setActuator( millis()%2000 == 0);

}

void logicN2::compute() {

// OUTLET:
// I will select the data from the first inlet that has new data
// ATTENTION: new data does not mean that EVENT is true...
	Data outputData;
	for (uint8_t i = 0; i < inletArrayPtr->size(); i++) {
		//  select the first inlet that has new data to pass to the output:
		if ((*inletArrayPtr)[i].isNewData()) {
			outputData.numericData = (*inletArrayPtr)[i].getData().numericData;
			outputData.event = (*inletArrayPtr)[i].checkEvent();

			outletArrayPtr->setNewData(outputData); // this saves the data into all the outlets, and tells them that the data is "new"
			break;
		}
	}

// ACTUATORS:
	// a) WHITE LED (as PULSE digital): it will indicate new, true event on inlet 0:
	if ((*inletArrayPtr)[0].isNewData()) {
		outputData.event = (*inletArrayPtr)[0].checkEvent();
		(*actuatorArrayPtr)[0].setNewData(outputData); // LED as actuator 0
	}

	// b) BUZZER (as PULSE digital): it will indicate new, true event on inlet 1:
	if ((*inletArrayPtr)[1].isNewData()) {
		outputData.event = (*inletArrayPtr)[1].checkEvent();
		(*actuatorArrayPtr)[1].setNewData(outputData);

	}

    // c) SMALL MOTOR (as PULSE analog): it will indicate new, true event on inlet 2, with strength given in the data.numericData field
    if ((*inletArrayPtr)[2].isNewData()) {
        outputData = (*inletArrayPtr)[2].getData();
        (*actuatorArrayPtr)[2].setNewData(outputData);
    }
    //.. or, if using a digital PULSE:
//  if ((*inletArrayPtr)[2].isNewData()) {
//        outputData.event = (*inletArrayPtr)[2].checkEvent();
//        (*actuatorArrayPtr)[2].setNewData(outputData);
//   }

	inletArrayPtr->setAllInletsDataUsed();  // set inlet data as used (this is only to control the compute "BANG"):
}
