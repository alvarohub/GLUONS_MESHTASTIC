#include "BaseSensor.h"

#define DEFAULT_DEBOUNCE_TIME 100 // ms

BaseSensor::BaseSensor() : firstTime(true), lastTimeChange(millis()) {

	myPersonalString[0]='\0';
	// Initialize the current and old state: this cannot be done here but on the  children
	// constructor (because readAnalogOutput or readDigitalOutput are not yet properly overriden).
	// bool currentState, oldState;
	// Another method is to use a boolean to signal when it is the first time we enter the update() method

	// DEFAULT values (possible overriden by the init method of children)
	// loadConditions(); // better not done at all, but in the init(), which will be dependent on the sensor definition
	learningEnable = false;
	sensorActive = true;
	setDebounceTime(DEFAULT_DEBOUNCE_TIME); // in ms
	setDetectionMode(ON_TRUE_COND);
}

BaseSensor::BaseSensor(const char* _text, const TypeSensor& _type) :  lastTimeChange(millis()) { // mySensorType(_type), firstTime(true),
	strcpy(myPersonalString,_text);

	mySensorType = _type;
	firstTime = true;

	// PRINT_CSTRING(myPersonalString);
	// if (mySensorType == BOOLEAN_OUTPUT) PRINTLN("BOOLEAN");
	// else if (mySensorType == ANALOG_OUTPUT) PRINTLN("ANALOG");
	// else PRINTLN("UNKNOWN TYPE");

	// DEFAULT values (possible overriden by the init method of children)
	// loadConditions(); // better not done at all, but in the init(), which will be dependent on the sensor definition
	learningEnable = false;
	sensorActive = true;
	setDebounceTime(DEFAULT_DEBOUNCE_TIME); // in ms
	setDetectionMode(ON_TRUE_COND);
}


void BaseSensor::update() {
	if (!sensorActive) return;

	// NOTE: the sensor update needs to update the sensor Data fields. Now, some sensors (for instance those with a digital boolean output) cannot
	// generate a meanigfull analog output. This will be arbitrary then...
	// Reminder: the data fields are:
	// String stringData;  --> wil be set with the name of the sensor in the constructor of the derived object.
	// int numericData;    --> only meaninful for analog sensor
	// bool event;         --> either generated directly by the sensor or computed form the numericData using the learned conditions.
	// uint32_t timeStamp;  --> data time stamp is set at creation/updating time for the DATA (not message)


	// a) First, READ the RAW STATE (this is NOT equal to the final value of EVENT)

	switch (this->getSensorType()) {
		case ANALOG_OUTPUT:

		myData.numericData = readAnalogOutput();

		if (firstTime)  pastValue =  myData.numericData ; // this needs to be here... ON_SIMPLE_CHANGE is special.
		if (myEventDetectionMode == ON_SIMPLE_CHANGE) {
			rawState = fabs(myData.numericData - pastValue) >= 0.5 * toleranceValue; // ie. data changed too much

			//			 Serial.print("toleranceValue: ");  Serial.println(toleranceValue);
			// if (rawState) {        Serial.print("past: ");Serial.print(pastValue);  Serial.print(" new:"); Serial.println(myData.numericData);  }
			//			 if (rawState) Serial.println("rawState true"); else Serial.println("rawState false");
			//		   if (oldRawState) Serial.println("oldRawState true"); else Serial.println("oldRawState false");

		}
		else
		rawState = fabs(myData.numericData - conditionValue) <= toleranceValue; // ie. data on range?
		break;

		case BOOLEAN_OUTPUT:
		rawState = readBooleanOutput();
		break;

		default:
		PRINTLN(F("sensor type?"));
		break;
	}

	if (firstTime) {
		firstTime = false;
		oldRawState = rawState;
		currentState = rawState;
		oldState = currentState;
	}

	// b) Perform debouncing:
	if (myEventDetectionMode != ON_SIMPLE_CHANGE) {
		// (1) http://www.eng.utah.edu/~cs5780/debouncing.pdf)
		// static uint16_t State = 0; // Current debounce status
		// State = (State << 1) | !newState | 0b11100000; // five consecutive similar values required
		// if (State == 0xf000) return TRUE;
		// return FALSE;
		// (2) Using a timer: we want to read the same value during at least debounceTime ms:
		if (oldRawState != rawState) lastTimeChange = millis();
		else if (millis() - lastTimeChange > debounceTime) {
			currentState = rawState;
		}
		oldRawState = rawState;
	} else { // this is in case of simple change:
		currentState = rawState;
	}

	// if (oldState!=currentState) {
	// Serial.print("old state: ");	Serial.println(oldState);
	// Serial.print("current state: ");	Serial.println(currentState);
	// }

	//b) Then, compute the value of the event, depending on the  myEventDetectionMode
	switch (myEventDetectionMode) {

		case  ON_SIMPLE_CHANGE: // this is a special case... I need to update the pastValue
		myData.event = currentState; //(!oldState && currentState); //currentState != oldState);
		if ( currentState ) {
			PRINT(pastValue); PRINT(F(" -> ")); PRINTLN(myData.numericData);
			pastValue = myData.numericData;
		}
		break;

		case ON_TRUE_COND:
		myData.event = currentState;
		break;

		case ON_FALSE_COND:
		myData.event = !currentState;
		break;

		case ON_CHANGE_COND:
		myData.event = (currentState != oldState);
		break;
		case ON_LEAVING_COND:
		myData.event = (oldState && !currentState);
		break;

		case ON_ENTERING_COND:
		myData.event = (!oldState && currentState);
		break;
	}

	// Other data fields:
	if (this-> getSensorType() == BOOLEAN_OUTPUT) myData.numericData = (rawState ? 255 : 0); //set arbitrarily. Note: the choice of 255 and 0 is because we may want to use the value as PWM
	// in case of analog, myData.numericData was already set

	strcpy(myData.stringData,myPersonalString); // the string identifying the sensor (NOTE: we could also add the type of event condition)

	myData.timeStamp = millis();

	// Finally, update old state:
	oldState = currentState;

}

// NOTE: this only has meaning in case of an analog sensor.
void BaseSensor::learnConditions(uint32_t _testDuration) {
	if (!sensorActive) return;
	if (learningEnable && mySensorType == ANALOG_OUTPUT) {
		uint32_t sum = 0;
		uint16_t count = 0;
		// Sample mean:
		uint32_t learnStart = millis();
		while (millis() - learnStart < _testDuration / 2 ) {
			sum += readAnalogOutput();
			count++;
            myLedIndicator.oscillate(white);
		}
		conditionValue = (uint16_t)(1.0 * sum / count);
		// Sample standard deviation (biased estimator of the standard deviation):
		// (note that I resample the sensor because we don't have memory to store data here!)
		sum = 0;  count = 0;
		learnStart = millis();
		while (millis() - learnStart < _testDuration / 2 ) {
			uint16_t readata = readAnalogOutput();
			sum += (readata - conditionValue) * (readata - conditionValue);
			count++;
            myLedIndicator.oscillate(white);
		}
		toleranceValue = (uint16_t) sqrt( 1.0 * sum / (count + 1) );

        myLedIndicator.blinkAll(green);

		PRINT(F("Sensor: ")); PRINT(indexSensor + 1); PRINT(F(") ")); PRINT_CSTRING(myPersonalString);
		PRINT(F("Cond: ")); PRINT(conditionValue); PRINT(F(" +/- ")); PRINTLN(toleranceValue);

		// Also, save conditions to EEPROM? NO. This should be done outside this method only if needed.
	}
}


void BaseSensor::dump() {
	LOG_PRINT(F("Sensor: ")); LOG_PRINT(indexSensor + 1); LOG_PRINT(F(") ")); LOG_PRINT_CSTRING(myPersonalString);
	if (sensorActive) LOG_PRINT(F("ENABLED")); else LOG_PRINT(F("DISABLED"));
	LOG_PRINT(F("Cond: ")); LOG_PRINT(conditionValue); LOG_PRINT(F(" +/- ")); LOG_PRINTLN(toleranceValue);
	myData.dataDump();
	//LOG_PRINTLN(F("---------- End sensor data dump ---------------"));
}
