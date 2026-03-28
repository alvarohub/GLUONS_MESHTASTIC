#ifndef _BaseActuator_h
#define _BaseActuator_h

#include "Arduino.h"
#include "utils.h"

#define MAX_NUM_ACTUATORS 3

class BaseActuator { // QUESTION: better to have a BaseConnection??? and Output or Input are subclasses

public:
	BaseActuator() {myPersonalString[0]='\0';}
	BaseActuator(const char* _text) {strcpy(myPersonalString,_text);}
	virtual ~BaseActuator() {};

	virtual void init(bool firstTimeBuild=false) {}; // Must be overrided by the children methods (note: not pure virtual, because some children don't need init, AND I need to create an array - not of pointers)

	void setIndex(uint8_t _index) {indexActuator=_index;} // this may or may not be used (it will perhaps if the actuators have EEPROM data)
	uint8_t getIndex() {return (indexActuator);}

	//void updateData(const Data& _data) {myData = _data;}
	void setNewData(const Data& _data) {// called by the logic unit
		myData = _data;
		newDataFlag = true;
	}
	void setDataFlag(bool flg) {
		newDataFlag = flg;
	}
	void setDataAsUsed() {
		newDataFlag = false;
	}

	void setString(const char* _text) {
		strcpy(myPersonalString , _text);
	}
	const char* getString() {
		return (myPersonalString); // maybe protected
	}

	Data checkOutData() {
		newDataFlag = false;
		return (myData);  // maybe protected...
	}

	bool isNewData() {
		return (newDataFlag);
	}

	virtual void setDuration(float){};

	virtual void update() {}; // This must be overriden (I cannot make it pure virtual because I need to instantiate an array - and not an array of pointers for reasons of heap allocation on embedded code). HOW this is done (using myData) will depend on the type of actuator.

	void dump(); // data dump

protected:
	// Note: the concept of "used" data makes sense for the actuator - we may want to do something once only, or many times.
	bool newDataFlag;

	// Data structure: { String stringData; uint16_t numericData; bool event; unsigned long timeStamp;  bool newData; };
	Data myData;

	char myPersonalString[MAX_LENGTH_STRING_DESCRIPTOR]; // characterises the actuator
	uint8_t indexActuator;
};


class ActuatorArray {
public:
	ActuatorArray(): numActuatorsUsed(0) {};

	// will be called by the NodeClass init method:
	void init(bool firstTimeBuild=false) {
		for (uint8_t i = 0; i < numActuatorsUsed; i++) actuatorPtrArray[i]->init(firstTimeBuild);
	}

	uint8_t size() {return (numActuatorsUsed);}

	void clear() {
		for (uint8_t i = 0; i < numActuatorsUsed; i++) delete actuatorPtrArray[i];
		numActuatorsUsed = 0;
	}

	bool addNewActuator(BaseActuator* _newActuatorPtr) {
		if (numActuatorsUsed < MAX_NUM_ACTUATORS) {
			_newActuatorPtr->setIndex(numActuatorsUsed);
			actuatorPtrArray[numActuatorsUsed++] = _newActuatorPtr; // save the pointer
			return (true);
		} else
		return (false);
	}

	// overloaded operators for getter and setter using simple [] notation:
	const BaseActuator& operator[] (const uint8_t  _index) const {
		if (_index < numActuatorsUsed)
		return (*actuatorPtrArray[_index]); // dereference...
		else
		return (*actuatorPtrArray[numActuatorsUsed - 1]); // let's return the last one
	}

	BaseActuator& operator[] (const uint8_t  _index) {
		if (_index < numActuatorsUsed)
		return (*actuatorPtrArray[_index]);
		else
		return (*actuatorPtrArray[numActuatorsUsed - 1]); // let's return the last one
	}

	void update() {
		for (uint8_t i = 0; i < numActuatorsUsed; i++) actuatorPtrArray[i]->update();
	}

	void dump() {
		for (uint8_t i = 0; i < numActuatorsUsed; i++) {
			LOG_PRINT(F("Actuator ")); LOG_PRINTLN(i);
			actuatorPtrArray[i]->dump();
		}
	}

	// global functions:
	bool isNewData() { // OR result...
		bool result = false;
		for (uint8_t i = 0; i < numActuatorsUsed; i++) result |= actuatorPtrArray[i]->isNewData();
		return (result);
	}

	void setNewData(const Data& _data) {
		for (uint8_t i = 0; i < numActuatorsUsed; i++) actuatorPtrArray[i]->setNewData(_data);
	}

	// NOTE: all other functions will be called using the child ( ex: actuatorPtrArray[j].setNewData(...) )

private:
	BaseActuator* actuatorPtrArray[MAX_NUM_ACTUATORS];
	uint8_t numActuatorsUsed;
};

#endif
