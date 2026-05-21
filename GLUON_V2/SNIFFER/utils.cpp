#include "utils.h"

/*
* ********************************************************************************************************************
* 	            				     GLOBAL UTILIY FUNCTIONS
* ********************************************************************************************************************
*/
template <class T>
T toNumber(const String& _stringtoconvert) {
	// An unsigned long is an uint32_t, meaning an ascii representation of at most 10 characters (4,294,967,295),
	// so I cannot use String::toInt() method
	T sum = 0;
	uint8_t len = _stringtoconvert.length();
	for (int i = 0; i < len; i++) {
		sum += (1 << i) * _stringtoconvert[len - 1 - i] - 48; // 48 is ASCII code for 0
	}
	return sum;
}

#define toUint32 toNumber<uint32_t>
#define toUint16 toNumber<uint16_t>
#define toUint8 toNumber<uint8_t>

/*
* ********************************************************************************************************************
* 	            				            Data
* ********************************************************************************************************************
*/

void Data::set(String _stringData, int16_t _numericData, bool _event, unsigned long _timeStamp) {
	stringData = _stringData;
	numericData = _numericData;
	event = _event;
	timeStamp = _timeStamp;
}

bool Data::setDataFromString(const String& stringToParse) {
	/*
	The data string is pure ASCII and human readable and looks like:
	stringData/numericData/event/timeStamp.

	For instance:
	"hello/431/0/3245"
	NOTE: it can also be: "/431/0/3245" in case the string is null ("")
	*/

	LOG_PRINT(F("Data string: ")); LOG_PRINTLN(stringToParse);

	uint8_t completedfields = 0;

	// 1) stringData:
	int8_t indexFirst = 0, indexLast; // the indexes of the first and last character of the relevant field
	if (stringToParse[0]=='/') { // particular case when the stringData is null
		stringData="";
		completedfields++;
		indexLast=indexFirst;
	}
	else {
		indexLast = stringToParse.indexOf('/', indexFirst); if (indexLast > indexFirst) completedfields++;
		stringData = stringToParse.substring(indexFirst, indexLast); // note: ending index is exclusive
	}

	// 2) numericData:
	indexFirst = indexLast + 1;
	indexLast = stringToParse.indexOf('/', indexFirst); if (indexLast > indexFirst) completedfields++;
	numericData = stringToParse.substring(indexFirst, indexLast).toInt(); // numericData is an int

	// 3) event:
	indexFirst = indexLast + 1;
	indexLast = stringToParse.indexOf('/', indexFirst); if (indexLast > indexFirst) completedfields++;
	event = (stringToParse.substring(indexFirst, indexLast) == F("1") ? 1 : 0);

	// 4) timeStamp:
	indexFirst = indexLast + 1;

	String aux = stringToParse.substring(indexFirst); // the rest of the string
	if (aux != "")  {
		completedfields++;
		timeStamp = toUint32(stringToParse.substring(indexFirst)); // the rest of the string
	}

	LOG_PRINT(F("Data parse "));
	if (completedfields == 4) {
		LOG_PRINTLN(F("OK"));
		dataDump();
		return (true);
	} else {
		LOG_PRINTLN(F("FAIL"));
		return (false);
	}
}

void Data::dataDump() {
	LOG_PRINTLN(F("DATA"));
	LOG_PRINT(F("-str ")); LOG_PRINTLN(stringData);
	LOG_PRINT(F("-num ")); LOG_PRINTLN(numericData);
	LOG_PRINT(F("-event ")); if (event) LOG_PRINTLN(F("1")); else LOG_PRINTLN(F("0"));
	LOG_PRINT(F("-time ")); LOG_PRINTLN(timeStamp);
}




/*
* ********************************************************************************************************************
* 					Message class
* ********************************************************************************************************************
*/

// The input parameter string is the raw string received by the RFM96 module:
bool Message::setMessageFromString(const String& stringToParse)  {

	bool parsedOk = false;

	uint8_t completedfields = 0;
	int8_t indexFirst = 0, indexLast; // the indexes of the first and last character of the relevant field

	// First, get the HEADER:
	// 1) MessageType (UPDATE, ACK_LINK, ACK_LINK_DELETED...)
	indexLast = stringToParse.indexOf('+', indexFirst); if (indexLast > indexFirst) completedfields++;
	// NOTE: if there is no '+' (as in the case of ACK_LINK or ACK_LINK_DELETED), stringToParse will give the whole string.
	myType = stringToParse.substring(indexFirst, indexLast); // note: ending index is exclusive
	// 2) The rest of the header (sender ID, receiver ID and timeStamp are set by the RF class)

	// Second, get the PAYLOAD (if any)
	indexFirst = indexLast + 1;
	myPayloadString = stringToParse.substring(indexFirst); // the rest of the string is the payload...
	if (myPayloadString != "")  completedfields++;

	// Then, depending on the message type, parse differently the rest of the string: 
	if ( ( myType == UPDATE ) && completedfields == 2) parsedOk=true; // packets with payload (for the time being, only UPDATE)
	else if ( completedfields == 1 ) parsedOk = true;// all the rest (packets without payload)

	LOG_PRINT(F("Parsed "));
	if (parsedOk) {
		LOG_PRINTLN(F("OK"));
		messageDump();
	}
	else LOG_PRINTLN(F("FAIL"));

	return (parsedOk);
}


