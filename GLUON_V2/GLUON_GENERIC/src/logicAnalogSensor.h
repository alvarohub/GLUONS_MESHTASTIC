
/*

Logic function for a simple analog sensor (Analog Rangefinder, Light Dependent Resistor sensor, etc)

*/

#include "BaseLogic.h"

class logicAnalogSensor : public BaseLogic {

public:
  logicAnalogSensor() : BaseLogic("AS") {}

  void init(bool firstTimeBuild) {
    if (firstTimeBuild)
      setUpdateMode(SENSOR_EVENT_BANG | ANY_INLET_BANG);
    else
      loadUpdateMode();
  }

  virtual void compute() override;

private:

};

void logicAnalogSensor::compute() { // this will be executed if there is a switch push, or if there is data on ANY inlet
	Data outputData;

  //1) ANALOG SENSOR DATA. Note: a "bang" with event=true is generated only when we are in the range condition;
  // otherwise, if we receive a bang (normally in the first inlet, but here on any inlet is ok) we read the value, but
  // event will be false)
  outputData=(*sensorArrayPtr)[0].getData(); // if in the sensor condition range, EVENT is true.

  //2) from INLETS:
  inletArrayPtr->setAllInletsDataUsed();  // reset the bang

  // 3) ACTUATOR (blue led indicating if the sensor is in the condition range)
  //outputData.numericData=map(outputData.numericData, 0, 60, 0, 10);
  //outputData.event = inletArrayPtr->isSomeBang(); //indicate ANY new data on inlet
  //outputData.event = true; // test...
  actuatorArrayPtr->setNewData(outputData); // indicate if there is a SENSOR EVENT

  //4) OUTLET: the sensor data (if an event is generated, event is equal to true; otherwise we read the data because of an inlet bang, but event is false)
  outletArrayPtr->setNewData(outputData);

}
