#ifndef _baselogic_h_
#define _baselogic_h_

#include "Inlet.h"
#include "Outlet.h"
#include "BaseSensor.h"
#include "BaseActuator.h"

#include "utils.h"


// Update modes are NOT exclusive.
// ATTN: the way the packing is done is compiler dependent (little/big endianness...!), so just doing an assignement, like:
//         updateModeMask = SENSOR_EVENT_BANG |  SYNCH_BANG, may not work properly !!!
#define ONLY_MANUAL_BANG	B00000000  // this is just to do manual update
#define SENSOR_EVENT_BANG 	B00000001
#define FIRST_INLET_BANG    B00000010
#define ANY_INLET_BANG      B00000100
#define SYNCH_BANG          B00001000
#define PERIODIC_BANG       B00010000
// Note that an IR BANG code can be sent at any time and it will force a logical update regardless of the current logicUpdateMode.

class BaseLogic {

public:

	// Attention! doing an union directly on a bitfield is absurd: all the bits will be mapped to the same memory!
	struct Bits {
		uint8_t sensorEventBang: 1, firstInletBang: 1, anyInletBang: 1, synchBang: 1, periodicBang: 1, : 3; // unused 3 bits for the time being...
	};
	union LogicUpdateMode
	{
		uint8_t byteValue;
		Bits bitValue;
	};

	BaseLogic(): firstTime(true) {
		myPersonalString[0]='\0';
	}

	BaseLogic(const char* _name): firstTime(true) {
		strcpy(myPersonalString,_name);
	} // note: no update mode parameter (this can be changed and saved by command)

	//INIT METHOD? (loading from memory certain states, etc)
	virtual void init(bool firstTimeBuild=false) {}; // overloaded as needed, not pure virtual since many logic blocks won't use this.

	void attachTo(InletArray* inPtr, OutletArray* outPtr, SensorArray* senPtr, ActuatorArray* actPtr) {
		inletArrayPtr = inPtr; // the number of inlets is given by inletArrayPtr->size()
		outletArrayPtr = outPtr; // the number of outlets is given by outletArrayPtr->size()
		sensorArrayPtr = senPtr; // the number of sensors is given by sensorArrayPtr->size()
		actuatorArrayPtr = actPtr;// the number of actuators is given by actuatorArrayPtr->size()
	}

	void loadUpdateMode() {setUpdateMode(myMemory.loadUpdateMode());}
	void setUpdateMode(uint8_t _updateModeMask); // will also save in the EEPROM
	void toggleUpdateMode(uint8_t _mode); // will also save in the EEPROM

	void update(); // this is the function called by the NodeClass (note: it sets the data as used)
	void forceUpdate(); // this can  be called by the NodeClass, in case of an IR command that forces a bang.

	const char* getString() {return (myPersonalString);}
	void setString(const char* _text) {strcpy(myPersonalString, _text);}

	void dump();

protected:

	// This is the method the user needs to override when defining a new logic module, so it is pure virtual here.
	virtual void compute() = 0; // perform the logic operation between inlets and outlets, including memory and sensors.

	// This perform evolution that does not depend on inlets and oulets (for example, metro update).
	// Many modules don't need it, so it does not need to be defined as pure virtual.
	virtual void evolve() {};
	// ******************************************************************************************************

	// We will use POINTERS to point to the relevant arrays (of inlets, outlets, sensors and actuators)
	InletArray* inletArrayPtr;
	OutletArray* outletArrayPtr;
	SensorArray* sensorArrayPtr;
	ActuatorArray* actuatorArrayPtr;

	LogicUpdateMode updateModeMask; // This will determine WHEN the logic will perfom a calculation on the inputs (modes are not exclusive)

	bool firstTime; // NOTE: what is this for? I forgot... for the evolve method?

	char myPersonalString[MAX_LENGTH_STRING_DESCRIPTOR];

};

#endif
