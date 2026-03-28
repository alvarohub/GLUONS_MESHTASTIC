#ifndef utils_h
#define utils_h

#include "Arduino.h"

// ========================================================
//#define DEBUG_MODE           // for minimal debugging and control using serial port
//#define DEBUG_MODE_FULL    // For high-level debugging using the Serial port [note: takes lots of memory and my produce undefined behaviour... need more memory!]

#ifdef DEBUG_MODE
#define DELAY(x) delay(x)
#define PRINT(...)      Serial.print(__VA_ARGS__)
#define PRINTLN(...)    Serial.println(__VA_ARGS__)
#define PRINT_CSTRING(...) for (uint8_t i=0; i<strlen(__VA_ARGS__); i++) Serial.print(__VA_ARGS__[i]); Serial.println()
#else
#define DELAY(x)
#define PRINT(...)
#define PRINTLN(...)
#define PRINT_CSTRING(...)
#endif

#ifdef DEBUG_MODE_FULL
#define LOG_DELAY(x) delay(x)
#define LOG_PRINT(...)      Serial.print(__VA_ARGS__)
#define LOG_PRINTLN(...)    Serial.println(__VA_ARGS__)
#define LOG_PRINT_CSTRING(...) for (uint8_t i=0; i<strlen(__VA_ARGS__); i++) Serial.print(__VA_ARGS__[i]); Serial.println()
#else
#define LOG_DELAY(x)
#define LOG_PRINT(...)
#define LOG_PRINTLN(...)
#define LOG_PRINT_CSTRING(...)
#endif


// #define toUint32(...)      (uint32_t)atoi(__VA_ARGS__)
// #define toUint16(...)      (uint16_t)atoi(__VA_ARGS__)
// #define toUint8(...)      (uint8_t)atoi(__VA_ARGS__)

// ========================================================

#define MAX_FAN_IN      3       //this is the number of input ports (nodeId) that can be accepted by one INLET
#define MAX_FAN_OUT     4       //this is the number of output ports (nodeId) that can be handled by one OUTLET

// An inlet link does not become "fixed" until this time has passed
#define LOOSE_PERIOD    2000 // in ms.

// Port mode:
#define ACTIVE_PORT     true
#define IDLE_PORT       false

/*
 * ********************************************************************************************************************
 *                                 GLOBAL UTILIY FUNCTIONS
 * ********************************************************************************************************************
 */
uint8_t indexOf(const char* str, char myChar, uint8_t startingFromIndex);

// Test available SRAM:
int freeRam();

#define MAX_LENGTH_STRING_DESCRIPTOR 8 // this is the length of the string describing the gluon, sensors, data, etc (+1 because of the '\0')
#define MAX_LENGTH_PAYLOAD_RF 40        // MAX: 61 - max length of the payload that can be send at once by the RFM module (+1 because we want strings)
#define MAX_LENGTH_TYPE_STRING 6       // max length of the string defining the message (+1)

/*
 * ********************************************************************************************************************
 *                                        NodeID
 *
 * The identifier for sender/receiver nodes.
 * ********************************************************************************************************************
 */
typedef uint8_t NodeID;

/*
 * ********************************************************************************************************************
 *                                        Data
 *
 * This is a container for the data (data can come from inlets, logic function or sensor, and go to actuator or outlet)

        NOTE: THIS HAS TO BE THROUGHLY REVISITED!!
                        - In particular, the "event" field is just a binary field, not an "event" in the sense
                  that it automatically generates a message (a BANG in MAX/MSP terminology?). It would be perhaps interesting to
                  copy the MAX/MSP message structure more closely.

                  - Another important consideration: why not using OSC like STRINGS for messages, and store then as string (and having
                  methods to convert to numerical values when needed) instead of creating all those Data structs with lots of methods?

                  - Finally, WHY passing always a long message containing all those fields? (string data, numeric, event, timestamp...)?
                  In fact, each object may just generate one or more of these fields. If the receiver needs something that is not sent, well, nothing happens!!

 * ********************************************************************************************************************
 */

struct Data {
	char stringData[MAX_LENGTH_STRING_DESCRIPTOR];
	//String stringData;   // this field can contain information about the origin of the data, or other command-like action modifiers
	int16_t numericData;    // I will allow for negative values too... so when parsing I need to use String::toInt()
	bool event;             // event is just a boolean "output" of either a sensor or some logic operation (NOTE: maybe not used in the future... - in particular when we will define a type for the data)
	uint32_t timeStamp;     // data time stamp is set at creation/updating time for the DATA (not message)

	// CONSTRUCTORS:
	Data() : numericData(0), event(false) {
		setTime(millis());
		stringData[0] = '\0';
	}
	// Data(bool _event) : stringData("event"), numericData(0), event(_event) { setTime(millis()); }
	// Data(String _str) : stringData(_str), numericData(0), event(true) { setTime(millis()); }
	// Data(int16_t _numericData, bool _event = false) : stringData(F("numeric")), numericData(_numericData), event(_event) {
	//      setTime(millis());
	// }
	Data(const char* _stringData, int16_t _numericData, bool _event, uint32_t _timeStamp) : numericData(_numericData), event(_event) {
		setTime(_timeStamp); strcpy(stringData, _stringData);
	}

	// Copy constructor (I need to do it here, because otherwise the stringData won't be fully copied - it was ok when using the String objects, since it would call it's own copy constructor, but here we are using a simple char buffer)
	Data(const Data &source) {clone(source);}

	// Assignement operator (again, I need to do deep copy)
	//void operator=( const Data& source ) {clone(source);}
	Data& operator=( const Data& source ) {clone(source); return *this;}

	// Setters: (note: we also have the default copy constructor (direct T a(b), and copy initialization T a=b;)
	void set(const Data& source) {clone(source);}
	void set(const char* _stringData, int16_t _numericData, bool _event, unsigned long _timeStamp);
	bool setDataFromString(const char*);    // used to set the fields from a string message (usually from the RF module)

	static uint32_t offsetTime;        // to synchronize the DATA time-stamp for all the gluons in the network
	void setTime(uint32_t _time) { timeStamp = _time - offsetTime;}

	void getStringToSend(char* buffer) const; // I cannot return a const char*, since this string is created on the fly by the method

	void  dataDump();

private:
	void clone(const Data &source) {
		strcpy(stringData, source.stringData);
		numericData = source.numericData;
		event = source.event;
		timeStamp = source.timeStamp;
	}
};


/*
 * ********************************************************************************************************************
 *                                                 Link
 *
 * A link does NOT contains "data", but the information about the one-to-one communication channel. This is:
 * - Address of sender or receiver (and better both, for future network maping - but this is left to LinkPair object),
 * - Time of creation of the link.
 * Address is the nodeId of the SENDER (in case of INLETS) or the RECEIVER (in case of OUTLETS).
 * Methods are useful to check for "loose" links (links not yet FIXED and that can move from inlet to inlet) and other
 * things like creating and setting links.
 * ********************************************************************************************************************
 */

struct Link {
	enum LinkState { FIXED, LOOSE };

	NodeID nodeId; // Here, nodeId is either "from" or "to", and is the ID of the node to which THIS gluon is connected
	unsigned long timeCreation;
	// ... and other Data if we would like to.

	// I need a parameterless constructor to declare the Link array... why I didn't need when using list<Link> listLinks???
	Link() {
	}
	Link(NodeID _nodeId, unsigned long _timeCreation = 0) : nodeId(_nodeId), timeCreation(_timeCreation) {
	}

	void setLink(NodeID _nodeId, unsigned long _timeCreation)
	{
		nodeId       = _nodeId;
		timeCreation = _timeCreation;
	}

	LinkState stateLink()
	{
		if (millis() - timeCreation < LOOSE_PERIOD) return(LOOSE);
		else return(FIXED);
	}

	// Equality operator match (used by the "disconnect" method): just equality of node ID:
	bool operator == (const Link &other) const {
		return(this->nodeId == other.nodeId);   // && ( abs(this->timeCreation - other.timeCreation) < LOOSE_PERIOD )  );
	}
};

/*
 * ********************************************************************************************************************
 *                Port is a container for active or idle Links. There is a fixed number of Ports per inlet/outlet
 *
 * NOTE: I don't like the structure of this, this needs refactoring.
 * ********************************************************************************************************************
 */
struct Port {

	Link myLink;

	Port() : state(IDLE_PORT) {
	}

	bool isIdle() const
	{
		return(state == IDLE_PORT);
	}
	bool isActive() const
	{
		return(state == ACTIVE_PORT);
	}

	void setState(bool _use)
	{
		state = _use;
	}
	void setIdle()
	{
		state = IDLE_PORT;
	}
	void setActive()
	{
		state = ACTIVE_PORT;
	}

	bool matchNode(const NodeID& otherId)
	{
		return (this->myLink.nodeId == otherId);
	}

	// ... better to have a function called "isLoose", "isFixed"?
	Link::LinkState stateLink()
	{
		return(myLink.stateLink());
	}

private:
	bool state;

};

/*
 * ********************************************************************************************************************
 *                                ArrayPort class
 *
 *  A very simple "list-like" class for the ports (without possiblity of changing its size after initialization)
 * By using the second "size" parameter, we avoid the dynamic allocation in the constructor (so, memory is
 * allocated in the stack and not in the heap)
 * ********************************************************************************************************************
 */
template < uint8_t max_size >
class ArrayPort {
public:
	ArrayPort();
	//~ArrayPort(); // not needed for stack allocated memory ("local data")

	Port const& operator[] (const uint8_t _index)const;     // this enables operations such as: Port p=listPorts[i]
	Port&       operator[] (const uint8_t _index);          // this enables operations such as:  listPorts[i]=p

	uint8_t maxSize()
	{
		return(max_size);
	}

	uint8_t getNumActivePorts()
	{
		uint8_t numActivePorts = 0;

		for (int i = 0; i < max_size; i++) if (myarray[i].isActive()) numActivePorts++;
		return(numActivePorts);
	}

	bool isFull()
	{
		return (getNumActivePorts() == max_size);
	}

	bool connect(const Link& _link);        // false if we could not add. This will set the port as active.
	bool disconnect(const Link& _link);     // will find the port with that link, and set it to IDLE_PORT
	void disconnectAll();

	// How to go through the list? one simple way is to check the occupancy, read, and go on until we have read exactly "size()" elements...
	// I could will simplify this with a simplified iterator.. but I won't do that yet.
	// Link& iteratorBegin ();
	bool findFirstIdleIndex(uint8_t& _indexEmpty);

private:
	Port myarray[max_size];  // array of ports - allocation at *compile* time
};

/*
 * ********************************************************************************************************************
 *  ArrayPort class implementation - for a template  we need to put the "implementation" also in the header file!
 *
 * ********************************************************************************************************************
 */


template < uint8_t max_size >
void ArrayPort < max_size > ::disconnectAll() {
	for (uint8_t i = 0; i < max_size; i++) myarray[i].setIdle();
}

template < uint8_t max_size >
ArrayPort < max_size > ::ArrayPort() {
	disconnectAll();
}

template < uint8_t max_size >
Port const & ArrayPort < max_size > ::operator[]  (const uint8_t _index)const { // this enables operations such as: Port p=listPorts[i]
	// if (_index < max_size)
	return myarray[_index];                                                 // attention! no safety check for now...
	// else return  dummy; // this is kind of dangerous: this content is meaningless... (perhaps I could define a dummy Link with special values.. )
}

template < uint8_t max_size >
Port & ArrayPort < max_size > ::operator[] (const uint8_t _index) {     // this enables operations such as:  listPorts[i]=p
	// if (_index < max_size)
	return myarray[_index];                                         // attention! no safety check for now...
	// else return  dummy; // this is kind of dangerous: this content is meaningless... (perhaps I could define a dummy Link with special values.. )
}

template < uint8_t max_size >
bool ArrayPort < max_size > ::findFirstIdleIndex(uint8_t & _indexIdle) {
	for (uint8_t i = 0; i < max_size; i++) {
		if (myarray[i].isIdle()) {      // let's find the first inactive link:
			_indexIdle = i;
			return(true);           // break the for: we only check the first available space in the array
		}
	}
	return(false); // no place for a new port
}

template < uint8_t max_size >
bool ArrayPort < max_size > ::connect(const Link &_link) {
	uint8_t indexFree;

	if (findFirstIdleIndex(indexFree)) {            // findFirstIdleIndex modifies the input parameter (passed by ref)
		myarray[indexFree].myLink = _link;      // copy assignement used
		myarray[indexFree].setActive();
		//numActivePorts++; // this should be ok... but better ALWAYS to recalculate it when needed
		return(true);
	}else  { // no place for new connection...
		return(false);
	}
}

template < uint8_t max_size >
bool ArrayPort < max_size > ::disconnect(const Link &_link) {
	// Note: this will disconnect (set to idle) at ONCE all the ports that have the link in question.
	bool couldDisconnect = false;

	for (uint8_t i = 0; i < max_size; i++) {
		//if (myarray[i].myLink.nodeId==matchNode(_link.nodeId)) ...
		if (myarray[i].isActive() && (myarray[i].myLink == _link)) { // attention: important to define the "equality" of Links
			myarray[i].setIdle();
			//	numActivePorts--; // this should be ok.. but perhaps better to avoid this variable and recalculate it always.
			couldDisconnect = true;
		}
	}
	return(couldDisconnect); // true if we could disconnect *something*
}


/*
 * ********************************************************************************************************************
 *                Message
 *
 * Structure and methods for exchanging messages between the Nodes.
 * For the time being, i will use my own simple protocol (not OSC, not JSON, YAML or XML)
 * Note that the MAXIMUM length of a single RFM96 packet is 61 bytes. I will try to send all the data in ONE packet.
 * The ASCII string sent/received by the RF module is:
 *
 ************************************************************* // this is the length of 61 bytes...
 *
 * HEADER:{TYPE:UPDATE, SENDID:ID1, RECID:ID2, TIME:235}, DATA:{NAME:sensor, VAL:153, EVENT:1, TIME:12304}
 * HEADER:ACK_LINK
 * HEADER:ACK_LINK_DELETED
 * HEADER:SNIFFER, PACKET:{HEADER:{TYPE:UPDATE, SENDID:ID1, RECID:ID2, TIME:235}, DATA:{NAME:sensor, VAL:153, EVENT:1, TIME:12304}}
 * HEADER:TOGGLE_SNIFFER
 *
 * ... FOR THE TIME BEING:
 * ACK_LINK PACKET:			"ACK_LINK"
 * ACK_LINK_DELETED PACKET:	"ACK_LINK_DELETED"
 * TOGGLE_SNIFFER PACKET:	"TOGGLE_SNIFFER"
 * UPDATE PACKET:            "UPDATE/NAME/VAL/EVENT/TIME(data)" (note there is no need to send ID1, ID2 nor TIME(packet))
 * SNIFFER PACKET:			 "SNIFFER/UPDATE/ID1/ID2/TIME(packet)/NAME/VAL/EVENT/TIME(data)" (note: all other types are not sniffed)
 *
 *
 * NOTE that the senderID, receiverID and msg time stamp may not be in the UPDATE packet payload
 * NOTE that in the future, this ASCII string may be binary serialized for optimal speed / size.
 * ********************************************************************************************************************
 */

// Types of messages:
//const char UPDATE[] PROGMEM = "UPDATE";   // "String 0" etc are strings to store - change to suit.
//const char ACK_LINK[] PROGMEM = "ACK_LINK";
//const char SNIFFER[] PROGMEM = "SNIFFER";
//const char* const messageTypes[] PROGMEM = {type1, type2, type3};

// NOTE: the types strings lengths should be < MAX_LENGTH_TYPE_STRING
#define UPDATE                     		"UP" // NOTE: this is a C-string, so it HAS an implicit '\0' at the end.

// Connection transactions:
#define ACK_LINK                        "NOUT"       // ack creation of an inlet, so we need to create the respective link on the outlet on the other gluon
#define ACK_LINK_DELETED                "DOUT"       // ack deletion of an inlet, so we need to delete the respective link on the outlet on the other gluon

// Sniffer related:
#define NEW_IN                          "NIN"        //not used for gluon-to-gluon communication, but for sending to sniffer (when in verbose mode)
#define DEL_IN                          "DIN"        //not used for gluon-to-gluon communication, but for sending to sniffer (when in verbose mode)
#define SNIFFER_MODE_OFF                "SN0"
#define SNIFFER_MODE_ON                 "SN1"
#define SNIFFER_MODE_ON_VERBOSE         "SN2"

// To synchronize the clocks of all the gluons:
#define SYNCH_CLK                       "SCLK"

// SCANNING the network of active gluons:
#define SCAN_NET                        "NET"

// RESET all connections in the network (delete links):
#define RESET_CON                       "RCON"

// CHECK connection "pulled":
#define PULL_INLET_PATCH_CHORD			"PULL"

class Message {
	//enum TypeMessage {UPDATE, ACK_LINK, ACK_LINK_DELETED, NEW_IN, DEL_IN, SNIFFER_MODE_OFF, SNIFFER_MODE_ON, SNIFFER_MODE_ON_VERBOSE, SYNCH_CLK};

private:

	// Header:
	char myType[MAX_LENGTH_TYPE_STRING];       // type of the message: UPDATE, ACK_LINK, ACK_LINK_DELETED, TOGGLE_SNIFFER...
	NodeID senderID, receiverID;
	unsigned long timeStamp;

	// Payload c-string:
	char myPayloadString[MAX_LENGTH_PAYLOAD_RF];
	// the payload can be: "" for ACK_LINK and ACK_LINK_DELETED, etc
	// "NAME/VAL/EVENT/TIME(data)",  in case of UPDATE packet.
	// "UPDATE/ID1/ID2/TIME(packet)+NAME/VAL/EVENT/TIME(data)" in case of SNIFFER packet.

public:

	Message() {
		//  NOTE: since we will concatenate the String object, this will generate many allocations/dealocations and may fragment
		// memory: in case of problems, use reserve(), and do it only once for the permanent object: myPayloadString.reserve(40);
		myPayloadString[0] = '\0';
		setTimeStamp(millis());
	}

	// Header getter and setters:
	void setType(const char* _type)
	{
		strcpy(myType, _type);
	}
	const char* getType()
	{
		return(myType);
	}
	const char* getHeaderToSend() const
	{
		return(myType);
	}

	void setTimeStamp(uint32_t _time)
	{
		timeStamp = _time - Data::offsetTime; // this may not be required... the timer for the packets is irrelevant, only for the data is important.
	}
	unsigned long getTimeStamp() const
	{
		return(timeStamp);
	}

	void setSenderId(uint8_t _node)
	{
		senderID = _node;
	}
	uint8_t getSenderId() const
	{
		return(senderID);
	}

	void setReceiverId(uint8_t _node)
	{
		receiverID = _node;
	}
	uint8_t getReceiverId() const
	{
		return(receiverID);
	}

	bool setMessageFromString(const char* stringToParse);   // this sets myType first, and then payloadString depending on the type

	void createDataPayload(const Data& _data)               // this sets the PAYLOAD STRING from a data object
	{
		_data.getStringToSend(myPayloadString);
		//"NAME/VAL/EVENT/TIME(data)"
	}

	bool getData(Data& _data)   // parse the data string and re-build the referenced Data object
	{   // Note: for the time being, we assume that when we call this, the payload does indeed correspond to a data object
		return(_data.setDataFromString(myPayloadString));
	}
	const Data getData() const
	{
		Data newData;

		newData.setDataFromString(myPayloadString);
		return(newData);
	}

	const char* getPayload() const
	{
		return(myPayloadString);
	}                                         // since myPayloadString is a class variable, I can have a const return

	// The following will dump different things depending on the type of the message:
	void getStringToSend(char* buff) const; // I cannot have a const return: the string to send is created on the fly by this method

	void messageDump()
	{
		LOG_PRINTLN("MSG:");
		LOG_PRINT("-type: ");
		LOG_PRINTLN(myType);
		LOG_PRINT("-SID: ");
		LOG_PRINT(senderID);
		LOG_PRINT(", RID: ");
		LOG_PRINTLN(receiverID);
		LOG_PRINT("-time: ");
		LOG_PRINTLN(timeStamp);
		LOG_PRINT("-payload: ");
		LOG_PRINTLN(myPayloadString);
	}
};


/*
 * ********************************************************************************************************************
 *                                MessageIR
 *
 *  Very simple struct for the IR messages. An IR "rawcode" is a 32 bit number (NEC).
 *  The two MSBytes code the COMMAND, and the 2 LSBytes code the VALUE.
 *
 * ********************************************************************************************************************
 */
struct MessageIR {

	MessageIR(){
	}
	MessageIR(uint32_t _raw) {
		setRawCode(_raw);
	}
	MessageIR(uint16_t _com, uint16_t _val) {
		setCommandValue(_com, _val);
	}

	uint32_t getRawCode()
	{
		return(rawcode);
	}
	uint16_t getValue()
	{
		return(value);
	}
	uint16_t getCommand()
	{
		return(command);
	}

	// Setting methods take care of the coherence between the rawcode and the (command, value) pair:
	void     setRawCode(uint32_t _raw);
	void     setCommandValue(uint16_t _com, uint16_t _val);

	uint32_t rawcode;
	uint16_t command;
	uint16_t value;
};




/* ********************************************************************************************************************
 *                                OTHER USEFUL structures and FUNCTIONS
 *
 *
 * ********************************************************************************************************************
 */

// struct Bits
// {
//     unsigned b0:1, b1:1, b2:1, b3:1, b4:1, b5:1, b6:1, b7:1; // (consecutive one bit bitfields) -  likely to be packed into a byte by the compilerString
// };
// union CBits
// {
//     Bits bits;
//     unsigned char byte;
// };


// ====================  SIGNAL PROCESSING =========================
//  (for events, or even for data on the inlet/outlets like in the MSP part of MAX/MSP...
// Averaging, Filters, Pattern matching
// Debouncing, Schmitt triggers, etc
// ... to do

#endif
