
/*

Logic function for the module D1
- RGB leds
- no sensor

Operation:

- WHEN: when receiving something on first inlet, then compute (BANG)
- ACTUATOR: the even value (boolean) of the data on the three inlets, are sent to the RGB leds (they are digital)
- OUT: the output is the data on first inlet.

*/

#include "BaseLogic.h"

class logicD1 : public BaseLogic {

public:
  logicD1() : BaseLogic("RGB") {}

  void init(bool firstTimeBuild) {
    if (firstTimeBuild)
      setUpdateMode(FIRST_INLET_BANG); // processing when receiveing data on first inlet
    else
      loadUpdateMode();
  }

  virtual void compute() override;

private:

};

void logicD1::compute() { // this will be executed when there is a bang on first inlet, so I don't need to re-check that.
  Data outputData;

	// 1) Get the data on first inlet and put it into the OUTLET:
	outputData = inletArrayPtr->getData(0);// equal to: (*inletArrayPtr)[0].getData();
	outletArrayPtr->setNewData(outputData); // set to "all" the outlets

  // 2) Copy event values for each INLET on the array of ACTUATORS (RGB leds):
  for (uint8_t i = 0; i < actuatorArrayPtr->size(); i++) {
      outputData=inletArrayPtr->getData(i); // equal to: (*inletArrayPtr)[i].getData();
      (*actuatorArrayPtr)[i].setNewData(outputData);
    }

	// 3) Finally, it is important to reset the "new" data flag (the bang), at least for the first inlet, but we do all:
  inletArrayPtr->setAllInletsDataUsed();

}
