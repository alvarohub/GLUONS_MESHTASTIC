// LOGIC FOR THE SWITCH + RGB LED MODULE


#include "BaseLogic.h"

class logicN5 : public BaseLogic {

public:
  logicN5() : BaseLogic("N5") {}

  void init(bool firstTimeBuild) {
    if (firstTimeBuild)
      setUpdateMode(SENSOR_EVENT_BANG | ANY_INLET_BANG);// because we want to compute things to send from the switch only, and also modify the actuator from the inputs only
    else
      loadUpdateMode();
  }

  virtual void compute() override;

private:

};

void logicN5::compute() { // this will be executed if there is a switch push, or if there is data on ANY inlet
  Data outputData;

  // 1) ACTUATORS (here three digital PULSE LEDs activated by event=true on each inlet).
  // Set each actuator (LED) using the events on each inlet with new data (NOTE: there is a redundancy between new data and event - WE NEED TO CHANGE THIS):
  for (uint8_t i = 0; i < actuatorArrayPtr->size(); i++) {
    if (inletArrayPtr->isBang(i) ) { //if ( inletArrayPtr->isBang(i) && inletArrayPtr->checkEvent(i) ) {
      // Set actuator number i:
	  outputData.set("in", 128, true, millis());
      (*actuatorArrayPtr)[i].setNewData(outputData);
      // (*inletArrayPtr)[0].setNewDataFlag(false);// reset the new data flag (this is to reset the BANG)

	  // Save the data from the inlet (to pass to output in case we didn't also press the switch)
	  outputData.set(inletArrayPtr->getData(i)); // equal to outputData = inletArrayPtr->getData(i);
    }
  }
  inletArrayPtr->setAllInletsDataUsed();// reset the new data flag (this is to reset the BANG)

  // 2) OUTPUT "event" from switch (numeric Data is arbitrary) or inlet number in case of new data on inlet.
  if (sensorArrayPtr->checkEvent()) { // this is TRUE if we entered here because of a sensor event bang (on ANY sensor...)
	outputData.set("sw", 128, true, millis()); // numeic data is arbitrary here
    // Data(String _stringData, uint16_t _numericData, bool _event, unsigned long _timeStamp)
    outletArrayPtr->setNewData(outputData); // this saves the data into all the outlets, and tells them that the data is "new"
  } else {
    outletArrayPtr->setNewData(outputData); // pass the data set on previous loop
  }

}
