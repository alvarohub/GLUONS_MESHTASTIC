#include "utils.h"

/*
* ********************************************************************************************************************
*                                 GLOBAL UTILIY FUNCTIONS
* ********************************************************************************************************************
*/

uint8_t indexOf(const char* str, char myChar, uint8_t startingFromIndex)
{
	uint8_t index = strlen(str); // note: if the starting index is beyond the string, we return the string length (security)
	if (startingFromIndex<index) {
		// Note: strstr(s1, s2) returns a pointer to the first occurrence of string s2 in string s1.
		//       If the character is not found, the function returns a null pointer!
		const char* aux = &(str[startingFromIndex]);
		char* ptr = strchr(aux, myChar); // Returns a pointer to the first occurrence of character myChar in the C string aux.

		if (ptr) index =  (int8_t)(ptr - aux) + startingFromIndex;
		else  index = strlen(aux); // in case we don't find the character, we return the length of the string (last index + 1)
	}
	return(index);
}

/*
* ********************************************************************************************************************
*                                        Data
* ********************************************************************************************************************
*/

uint32_t Data::offsetTime = 0; // initialization of static variable for the clock synchronization

void Data::set(const char* _stringData, int16_t _numericData, bool _event, unsigned long _timeStamp)
{
	strcpy(stringData, _stringData);
	numericData = _numericData;
	event       = _event;
	timeStamp   = _timeStamp - offsetTime;
}

void Data::getStringToSend(char* buffer) const    // be sure to allocate enough space on the buffer
{   //return(stringData + "/" + String(numericData) + "/" + (event ? "1" : "0" + "/" + String(timeStamp));

	sprintf(buffer, "%s/%hi/%hhu/%lu", stringData, numericData, (event ? 1 : 0), timeStamp);

	// NOTE: PSTR does really work here?
	//sprintf_P(buffer, PSTR("%s/%hi/%hhu/%lu"), stringData, numericData, (event ? 1 : 0), timeStamp);

	// %hi: signed short int = int16_t (numericData)
	// %hu : unsigned short int = uint16_t
	// %lu : unsigned long int = uint32_t
	//for (int j=0; j< strlen(buffer); j++) PRINT(buffer[j]); PRINTLN(); PRINTLN(strlen(buffer));
}

bool Data::setDataFromString(const char* stringToParse) // note: a C-string ends by a null character '\0'
{
	/*
	* The data string is pure ASCII and human readable and looks like:
	* stringData/numericData/event/timeStamp.
	*
	* For instance:
	* "button/128/1/89247"
	* NOTE: it can also be: "/431/0/3245" in case the stringData is empty ("")
	*/

	LOG_PRINT("Data string: "); LOG_PRINTLN(stringToParse);
	//for (int j=0; j< strlen(stringToParse); j++) PRINT(stringToParse[j]); PRINTLN(); PRINTLN(strlen(stringToParse));

	uint8_t completedfields = 0;

	// 1) stringData:
	uint8_t indexFirst = 0, indexLast;   // the indexes of the first and last character of the relevant field
	indexLast = indexOf(stringToParse, '/', indexFirst); //PRINT(indexLast); PRINT(" - > "); PRINTLN(stringToParse[indexLast]);
	if ( indexLast > indexFirst ) {
		//stringData = stringToParse.substring(indexFirst, indexLast);           // note: ending index is exclusive
		strncpy(stringData, stringToParse, indexLast); // not indexLast-1 because the indexes start from 0
		//char * strncpy ( char * destination, const char * source, size_t num );
		// NOTE: NO null-character is implicitly appended at the end of destination if source is longer than num. Thus, in this case, destination shall not be considered a null terminated C string (reading it as such would overflow).
	} // else { }  // particular case when the stringData is empty (no name for the data) (nothing to do)
	stringData[indexLast] = '\0'; // necessary to form a proper C string!
	completedfields++;

	// 2) numericData:
	indexFirst = indexLast + 1;
	//indexLast  = stringToParse.indexOf('/', indexFirst);
	indexLast  = indexOf(stringToParse, '/', indexFirst);
//    PRINT(indexFirst); PRINT(" - > "); PRINTLN(stringToParse[indexFirst]);
//    PRINT(indexLast); PRINT(" - > "); PRINTLN(stringToParse[indexLast]);
	if (indexLast > indexFirst) {
		completedfields++;
		//numericData = stringToParse.substring(indexFirst, indexLast).toInt();      // numericData is an int
		char auxString[7]; // numericData is int16_t ([−32767, +32767], and we need to add to terminator '\0' so we can use atoi
		strncpy(auxString, &(stringToParse[indexFirst]), indexLast-indexFirst);
        auxString[indexLast-indexFirst] = '\0';
        numericData = atoi(auxString);
	}

	// 3) event:
	indexFirst = indexLast + 1;
	//indexLast  = stringToParse.indexOf('/', indexFirst);
	indexLast  = indexOf(stringToParse, '/', indexFirst); //PRINTLN(stringToParse[indexLast]);
	if (indexLast > indexFirst) {
		completedfields++;
		//event = (stringToParse.substring(indexFirst, indexLast) == "1" ? 1 : 0);
		event = (stringToParse[indexLast - 1] == '1');
		//if (event) PRINTLN("1"); else PRINTLN("0");
	}

	// 4) timeStamp:
	indexFirst = indexLast + 1;
	//String aux = stringToParse.substring(indexFirst);      // the rest of the string
	if (indexFirst < strlen(stringToParse)) {
		completedfields++;
		//timeStamp = toUint32(stringToParse.substring(indexFirst)); // Note that here we don't set the time using the offset...
		timeStamp = atol(&(stringToParse[indexFirst]));
	}

	LOG_PRINT("Data parse ");
	if (completedfields == 4) {
		LOG_PRINTLN("OK");
		dataDump();
		return(true);
	}else  {
		LOG_PRINTLN("FAIL");
		return(false);
	}
}


void Data::dataDump()
{
	LOG_PRINTLN("DATA");
	LOG_PRINT("-str ");
	LOG_PRINTLN(stringData);
	LOG_PRINT("-num ");
	LOG_PRINTLN(numericData);
	LOG_PRINT("-event ");
	if (event) {
		LOG_PRINTLN("1");
	}else  {
		LOG_PRINTLN("0");
	}
	LOG_PRINT("-time ");
	LOG_PRINTLN(timeStamp);
}

/*
* ********************************************************************************************************************
*                Message class
* ********************************************************************************************************************
*/

// The input parameter string is the raw string received by the RFM96 module:
bool Message::setMessageFromString(const char* stringToParse)
{
	//for (int j=0; j< strlen(stringToParse); j++) PRINT(stringToParse[j]); PRINTLN(); PRINTLN(strlen(stringToParse));

	bool parsedOk = false;

	uint8_t completedfields = 0;
	uint8_t indexFirst      = 0, indexLast;// the indexes of the first and last character of the relevant field

	// First, get the HEADER:
	// 1) MessageType (UPDATE, ACK_LINK, ACK_LINK_DELETED, SNIFFED, TOGGLE_SNIFFER)
	//indexLast = stringToParse.indexOf('+', indexFirst);
	indexLast = indexOf(stringToParse, '+', 0);
	//PRINT(indexLast);
	// NOTE: if there is no '+' (ACK_LINK or ACK_LINK_DELETED), indexOf will give the string length (= last index of the string + 1).
	if (indexLast > 0) {
		completedfields++;
		//myType = stringToParse.substring(indexFirst, indexLast);      // note: ending index is exclusive
		strncpy(myType, stringToParse, indexLast); // note: strncpy will copy indexLast bytes (but note that first index is 0)
		myType[indexLast] = '\0'; // necessary to forma a proper C string!
	}

	// 2) The rest of the header (sender ID, receiver ID and timeStamp are set by the RF class)

	// Second, get the PAYLOAD (if any)
	indexFirst = indexLast + 1;
	if (indexFirst < strlen(stringToParse) ) {
		// myPayloadString = stringToParse.substring(indexFirst);      // the rest of the string is the payload...
		strcpy(myPayloadString, &(stringToParse[indexFirst])); // note: strcpy copies until finding the NULL character '\0'
		completedfields++;
	}

	// Then, depending on the message type, parse differently the rest of the string:
	if (!strcmp(myType, UPDATE) && (completedfields == 2)) parsedOk = true;         // packets with payload (for the time being, only UPDATE)
	else if (completedfields == 1) parsedOk = true;                                 // all the rest (packets without payload)

	//PRINT_CSTRING(myType);
	//PRINTLN(completedfields);

	LOG_PRINT("Parsed ");
	if (parsedOk) {
		LOG_PRINTLN("OK");
		messageDump();
	}else  {
		LOG_PRINTLN("FAIL");
	}

	return(parsedOk);
}

void Message::getStringToSend(char* buff) const
{
	//for (int j=0; j< strlen(myType); j++) PRINT(myType[j]); PRINTLN();
	//for (int j=0; j< strlen(myPayloadString); j++) PRINT(myPayloadString[j]); PRINTLN();

	if (!strcmp(myType, UPDATE)) {   // packets that contains payload
		//sprintf(buff, "%s+%s",getHeaderToSend(), getPayload());

		sprintf(buff, "%s+%s", myType, myPayloadString);
		//sprintf_P(buff, PSTR("%s+%s"), myType, myPayloadString);

		//return(getHeaderToSend() + "+" + getPayload()); // Ex: UPDATE+NAME/VAL/EVENT/TIME(data)
	} else strcpy(buff, myType); //strcpy(buff, getHeaderToSend());
	// EX: ACK_LINK (no payload)

	//for (int j=0; j< strlen(buff); j++) PRINT(buff[j]); PRINTLN(); PRINTLN(strlen(buff));
}

/*
* ********************************************************************************************************************
*                                MessageIR
* ********************************************************************************************************************
*/
void MessageIR::setRawCode(uint32_t _raw)
{
	rawcode = _raw;
	command = (uint16_t)(_raw >> 16);
	value   = (uint16_t)(_raw & 0x0000FFFF);
}


void MessageIR::setCommandValue(uint16_t _com, uint16_t _val)
{
	command = _com;
	value   = _val;
	rawcode = (uint32_t)(((uint32_t)command << 16) + value);
}


int freeRam()
{
	extern int __heap_start, *__brkval;
	int v;

	return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}
