#ifndef _BaseSensor_h
#define _BaseSensor_h

#include "Arduino.h"
#include "utils.h"
#include "nonVolatileData.h"
#include "ledIndicators.h" // <-- in particular to show something during learning


// TODO: add shmitt trigger and circular buffer as well as period sampling value!!

/*NOTE: I cannot use external interrupts on the Arduino Uno (or Moteino)!!! (INT0 is used by the RFM96, and INT1 cannot be used because D3 is used by TIMER_2 used to generate the PWM for the IR LED signal - it controls D3 and D11).*/

// Max number of sensors in sensor array (NOTE: must reserve space for saving these things on EEPROM, for now only for two)
#define MAX_NUM_SENSORS 2 // note: this do NOT include the "common" sensors such as the tilt sensor and learning button.

#define LEARN_PERIOD 3000 // TODO: during learning, stop everything and indicate the mode (led indicators rotating?)

/* NOTE: When to set Data.event to true from the sensor value?
enum EventDetectionMode {ON_SIMPLE_CHANGE, ON_TRUE_COND, ON_FALSE_COND, ON_CHANGE_COND, ON_LEAVING_COND, ON_ENTERING_COND};
  	- ON_SIMPLE_CHANGE: analog value changed more than toleranceValue (the conditionValue is not used, only the past value)
  	- ON_TRUE_COND means on "true condition"
	- ON_CHANGE_COND means when it changed from FALSE to TRUE or visceversa.
*/

class BaseSensor {

public:
	enum TypeSensor {BOOLEAN_OUTPUT = 0, ANALOG_OUTPUT};
	enum EventDetectionMode {ON_SIMPLE_CHANGE, ON_TRUE_COND, ON_FALSE_COND, ON_CHANGE_COND, ON_LEAVING_COND, ON_ENTERING_COND}; // NOTE: of course, the event detection mode determines when the "event" boolean becomes true

	BaseSensor();
	BaseSensor(const char* _text, const TypeSensor& _type);
	virtual ~BaseSensor() {};

	virtual void init(bool firstTimeBuild=false) {}; // important: will be called by the NodeClass init method. Must be overrided by the children methods (note: not pure virtual, because some children don't need init)

	// enable/disable may be necessary ("common sensors" or during tests and development).
	// TODO: save on EEPROM too?
	void disable() {sensorActive = false;}
	void enable() {sensorActive = true;}

	void setIndex(uint8_t _index=255) {indexSensor=_index;} // I need an index to compute the EEPROM memory address for each sensor, but this is only relevant for sensors that are ADDED to the node.
	uint8_t getIndex() {return (indexSensor);}

	void setString(const char* _text) {strcpy(myPersonalString, _text);}
	const char* getString() {return (myPersonalString);}

	void setSensorType(const TypeSensor& _type) {mySensorType = _type;}
	TypeSensor getSensorType() const {return (mySensorType);}

	void setDetectionMode(const EventDetectionMode& _type) {myEventDetectionMode = _type;}
	EventDetectionMode getDetectionMode() const {return (myEventDetectionMode);}

	void setDebounceTime(unsigned long _delay) {debounceTime = _delay;}

	void learnConditions(unsigned long testDuration = LEARN_PERIOD); // this will NOT save on EEPROM
	void enableLearning() {learningEnable = true;}
	void disableLearning() {learningEnable = false;}

	void setConditionValue(uint16_t _newConditionValue) { // this will NOT save on EEPROM
		conditionValue = _newConditionValue;
		//myMemory.saveConditionValueEEPROM(indexSensor, conditionValue);
	}
	void setToleranceValue(uint16_t _newToleranceValue) { // this will NOT save on EEPROM
		toleranceValue = _newToleranceValue;
		//myMemory.saveToleranceValueEEPROM(indexSensor, toleranceValue);
	}
	void setConditions(uint16_t _newConditionValue, uint16_t _newToleranceValue) { // this won't save on EEPROM
		conditionValue = _newConditionValue;
		toleranceValue = _newToleranceValue;
		//myMemory.saveConditionValueEEPROM(indexSensor, conditionValue);
		//myMemory.saveToleranceValueEEPROM(indexSensor, toleranceValue);
	}
	uint16_t getConditionValue() {return (conditionValue);}
	uint16_t getToleranceValue() {return (toleranceValue);}

	bool checkEvent() { return (sensorActive && myData.event); }  // a shortcut for myData.event (but it is private)
	Data getData() { return (myData); } // do not forget to set data as "used" (this will depend on how it is used... )

	// Memory save/load:
	void loadConditionValue() {conditionValue = myMemory.loadConditionValue(indexSensor);}
	void loadToleranceValue() {toleranceValue = myMemory.loadToleranceValue(indexSensor);}
	void loadConditions() {loadConditionValue();loadToleranceValue();}

	void saveConditionValue() {myMemory.saveConditionValue(indexSensor, conditionValue);}
	void saveToleranceValue() {myMemory.saveToleranceValue(indexSensor, toleranceValue);}
	void saveConditions() { if (sensorActive) {saveConditionValue();saveToleranceValue();}}

	// This will update myData (including analog/digital data AND boolean conditions - the event).
	// We can also have DIFFERENT detection methods (CHANGE, RISING, etc... ). For the time being, only "MATCH_CONDITION_TOLERANCE"
	void update();
	void dump(); // this is for checking the structure of the sensor as well as present values of data

protected:

// The following are the method that the user has to implement:
	virtual uint16_t readAnalogOutput() {return (pastValue);} // note: I don't make it pure virtual, because some children WON'T instantiate them (for instance, boolean sensors). But if they do, they will override it.
	virtual bool readBooleanOutput() {return (false);}

	// bool newData; // it does not makes much sense to have this field in the embedded sensor, since when checking its value, we just read anew...
	Data myData;// this is struct Data { String stringData; uint16_t numericData; bool event; unsigned long timeStamp; };

	bool firstTime;
	bool rawState, oldRawState; // this is for debouncing
	bool oldState, currentState; // this is used to compute the final myData.event value depending on the event trigger mode and debounce time

	char myPersonalString[MAX_LENGTH_STRING_DESCRIPTOR]; // description of the sensor
	uint8_t indexSensor;

	TypeSensor mySensorType; // boolean, analog...
	EventDetectionMode myEventDetectionMode;
	unsigned long lastTimeChange, debounceTime;

	uint16_t pastValue; // useful for ON_CHANGE mode

	bool learningEnable; // we may want to have sensor for which the learning mode is disabled, and set once and for all.
	uint16_t conditionValue, toleranceValue; // "range" of conditions to generate a boolean event in case of an analog sensor

	bool sensorActive;// enable/disable are needed in case there is no hardware for the sensor (in particular, in the case of the learning button, but this can also happen during development)
};


class SensorArray {
public:
	SensorArray(): numSensorsUsed(0) {};

	void init(bool firstTimeBuild=false) { for (uint8_t i = 0; i < numSensorsUsed; i++) sensorPtrArray[i]->init(firstTimeBuild); }  // will be called by the NodeClass init method

	uint8_t size() {return (numSensorsUsed);}

	void clear() {
		for (uint8_t i = 0; i < numSensorsUsed; i++) delete sensorPtrArray[i]; // use delete because the array store pointers to the sensor objects
		numSensorsUsed = 0;
	}

	bool addNewSensor(BaseSensor* _newSensorPtr) {
		if (numSensorsUsed < MAX_NUM_SENSORS) {
			_newSensorPtr->setIndex(numSensorsUsed);
			sensorPtrArray[numSensorsUsed++] = _newSensorPtr; // store the pointer
			return (true);
		} else
			return (false);
	}

	// Overloaded operators for getter and setter using simple [] notation (note that the index of the sensor should always correspond to the index in the array)
	const BaseSensor& operator[] (const uint8_t  _index) const {
		if (_index < numSensorsUsed)
			return (*sensorPtrArray[_index]); // dereference...
		else
			return (*sensorPtrArray[numSensorsUsed - 1]); // let's return the last one
	}
	BaseSensor& operator[] (const uint8_t  _index) {
		if (_index < numSensorsUsed)
			return (*sensorPtrArray[_index]);
		else
			return (*sensorPtrArray[numSensorsUsed - 1]); // let's return the last one
	}

	// void loadConditionValue() {
	// 	for (uint8_t i = 0; i < numSensorsUsed; i++) sensorPtrArray[i]->loadConditionValue();
	// }
	//
	// void loadToleranceValue() {
	// 	for (uint8_t i = 0; i < numSensorsUsed; i++) sensorPtrArray[i]->loadToleranceValue();
	// }

	void loadConditions() { for (uint8_t i = 0; i < numSensorsUsed; i++) sensorPtrArray[i]->loadConditions(); }

	// void saveConditionValue() {
	// 	for (uint8_t i = 0; i < numSensorsUsed; i++) sensorPtrArray[i]->saveConditionValue();
	// }
	//
	// void saveToleranceValue() {
	// 	for (uint8_t i = 0; i < numSensorsUsed; i++) sensorPtrArray[i]->saveToleranceValue();
	// }

	void saveConditions() { for (uint8_t i = 0; i < numSensorsUsed; i++) sensorPtrArray[i]->saveConditions(); } // will be called when doing learning

	void learnConditions() { // will be called by the NodeClass when receiving proper IR command
		for (uint8_t i = 0; i < numSensorsUsed; i++) {
			sensorPtrArray[i]->learnConditions(LEARN_PERIOD);
//			sensorPtrArray[i]->saveConditions(); // save data on EEPROM
//			sensorPtrArray[i]->update();
//			sensorPtrArray[i]->dump();
		}
	}

	void update() {for (uint8_t i = 0; i < numSensorsUsed; i++) sensorPtrArray[i]->update();}

	void dump() {
    LOG_PRINT(F("Num sensors: "));LOG_PRINTLN(numSensorsUsed);
		for (uint8_t i = 0; i < numSensorsUsed; i++) sensorPtrArray[i]->dump();
	}

	bool checkEvent() { // check for ANY event on the array of sensors (used or not by the logic unit)
		bool result = false;
		for (uint8_t i = 0; i < numSensorsUsed; i++) {
			result |= sensorPtrArray[i]->checkEvent();
		}
		return (result);
	}

private:
	BaseSensor* sensorPtrArray[MAX_NUM_SENSORS]; // array of POINTERS to base class
	uint8_t numSensorsUsed;

};

#endif
