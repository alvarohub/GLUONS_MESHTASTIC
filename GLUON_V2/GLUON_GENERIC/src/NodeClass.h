#ifndef _NodeClass_h_
#define _NodeClass_h_

#include "Arduino.h"

#include "Inlet.h"
#include "Outlet.h"

#include "BaseLogic.h"
#include "BaseActuator.h"
#include "BaseSensor.h"

// Sensors & actuators for display and control of common properties of gluons (cross-global pre-instantiated objects)
//#include "sensorDiscreteValues.h"
#include "learningButton.h"  // --> myLearningButton object
#include "tiltSensor.h"      // --> myTiltSensor
#include "ledIndicators.h"   //...
#include "chirpSpeaker.h"
#include "vibrator.h"

// Module RF and memory hardware:
#include "moduleRF.h"
#include "moduleIR.h"
#include "nonVolatileData.h"

// cross-global utility objects:
#include "utils.h"

// default minimum period to "latch" the computed output:
#define DEFAULT_MIN_PERIOD_LATCH 300 // in ms (200 is 5 per second, nice to see the data goint through the network..)

class Node {
  public:
    Node() : myLogicPtr(NULL) {}
    //~Node;

    void init(bool firstTimeBuild = false); // << --- PUT HERE ALL initializations that need to be done OUTSIDE the CONSTRUCTOR (we cannot be sure there in which ORDER the objects are instantiated... )

    void setNodeId(NodeID _nodeid) {
      myNodeId = _nodeid; // must be from 0 to 255
    }
    NodeID getNodeId() {
      return (myNodeId);
    }

    void setString(const char* _text) {
      strcpy(myPersonalDataString , _text); // this will be used in the future to pass complex messages (type of gluon, etc)
    }
    const char* getString() {
      return (myPersonalDataString);
    }

    void createInlets(uint8_t _numInlets);
    void createOutlets(uint8_t _numOutlets);

    void addInlet(const Inlet& _inlet) {
      myInletArray.addNewInlet(_inlet);
    }
    void addOutlet(const Outlet& _outlet) {
      myOutletArray.addNewOutlet(_outlet);
    }

    void disconnectEverything() {
      myInletArray.disconnectEverything();
      myOutletArray.disconnectEverything();
    }

    void loadConnexions(); // this will load all the links (IN/OUT) from EEPROM
    void saveConnexions();// this will save all the links (IN/OUT) from EEPROM. It may be abusive to call it each time there is a small
    // change, but it is much safer to avoid bugs.

    uint8_t getNumInlets() {
      return (myInletArray.size());
    }
    uint8_t getNumOutlets() {
      return (myOutletArray.size());
    }

    uint8_t getNumActiveInletPorts() {
      uint8_t tot = 0;
      for (uint8_t i = 0; i < myInletArray.size(); i++) tot += myInletArray[i].getNumActivePorts();
      return (tot);
    }

    uint8_t getNumActiveOutletPorts() {
      uint8_t tot = 0;
      for (uint8_t i = 0; i < myOutletArray.size(); i++) tot += myOutletArray[i].getNumActivePorts();
      return (tot);
    }

    uint8_t getNumActiveInletPorts(uint8_t index) {
      return (myInletArray[index].getNumActivePorts()); // beware of index overflow
    }
    uint8_t getNumActiveOutletPorts(uint8_t index) {
      return (myOutletArray[index].getNumActivePorts());
    }

    // a) Sensor:
    void addSensor(BaseSensor* _sensor) {
      mySensorArray.addNewSensor(_sensor);
    }

    // b) Actuator:
    //void setActuator(BaseActuator* _actuator)  {myActuatorPtr = _actuator;}
    void addActuator(BaseActuator* _actuator) {
      myActuatorArray.addNewActuator(_actuator);
    }

    // c) Logic (custom logic AND memory - "custom logic block" in FPGA terminology...).
    // NOTE: this should be called AFTER setting inlets, outlets, sensors and actuators!!!
    void setLogic(BaseLogic* _logicPtr) {
      myLogicPtr = _logicPtr;
      myLogicPtr->attachTo(&myInletArray, &myOutletArray, &mySensorArray, &myActuatorArray); // how this is done depends on the instantiation of logic unit
    }

    void setUpdateMode(uint8_t _updateMode) {
      myLogicPtr->setUpdateMode(_updateMode);
    }
    void loadUpdateMode() {
      myLogicPtr->loadUpdateMode(); // load from EEPROM
    }

	void setMinPeriodLatch(uint32_t _minPeriodLatch) {minPeriodLatch = _minPeriodLatch;}

    void update(); // compute logic and update the outputs using the (*myLogicPtr) obnject

	void learnAnalogConditions(); // learn range conditions for ALL analog sensors on this node.

	// Perhaps these two should be merged:
    void dumpNodeData(); // this is for debugging on serial port

  private:

    NodeID myNodeId;
    char myPersonalDataString[MAX_LENGTH_STRING_DESCRIPTOR]; // this can be used to identify the gluon

    // Inlets/Outlets:
    InletArray myInletArray;
    OutletArray myOutletArray;

	// Control of sending speed (to avoid blocking loops - see comments in the BaseLogic class about queueing computation
	//results in the Outlet class:
	uint32_t minPeriodLatch, lastTimeLatch;

    // Sensors and actuators:
    SensorArray mySensorArray;
    ActuatorArray myActuatorArray;

    // Only ONE logic block per gluon:
    BaseLogic* myLogicPtr;

    // RF message processing:
    void processRFMessage();

    void processRequestCreationOutletLink();
    void processRequest(const NodeID & _toNode);

	// Send message in senderMessage (=RF.getSenderMessage()):
	bool sendMessageToChildren(Outlet& fromOutlet);
    bool sendNewDataMessageRF();
    bool sendSameMessageRF();

	// Sending sniffed data (no ack)
	void sendSnifferNO(Message& msgToSniff, uint8_t outletIndex);
	void sendSnifferDO(Message& msgToSniff, uint8_t outletIndex);
	void sendSnifferNI(Message& msgToSniff, uint8_t inletIndex);
	void sendSnifferDI(Message& msgToSniff, uint8_t inletIndex);
	void sendSnifferMOV(uint8_t _myNodeId, uint8_t _toNode, uint8_t fromInlet, uint8_t toInlet);
	void sendSniffedDataPacket(Message& msgToSniff, int outletIndex, int inletIndex);
	void sendSniffedConnections();
	void sendSnifferClearAllInConnections();
	void sendSnifferClearAllOutConnections();
	void sendSnifferClearAllConnections();

	void showPulledPatchChord(NodeID _toThisID);

    // IR message processing:
    void processIRMessage();

    // Serial processing (optional, may be used for debugging)
    void processSerialMessage();

};

#endif
