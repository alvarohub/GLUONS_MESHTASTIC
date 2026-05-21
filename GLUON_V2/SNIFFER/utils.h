
#ifndef utils_h
#define utils_h

#include "Arduino.h"

#define AUTO_SET_RF_POWER  // define it to use automatic gain for the RF transmitter

// ========================================================
// #define DEBUG_MODE 

#ifdef DEBUG_MODE
#define LOG_PRINT(...)    Serial.print(__VA_ARGS__)
#define LOG_PRINTLN(...)  Serial.println(__VA_ARGS__)
#else
# define LOG_PRINT(...)
# define LOG_PRINTLN(...)
#endif


/*
* ********************************************************************************************************************
* 	            				     GLOBAL UTILIY FUNCTIONS
* ********************************************************************************************************************
*/
unsigned long toUnsignedLong(const String& _stringtoconvert);

/*
* ********************************************************************************************************************
* 	            				            NodeID
*
* The identifier for sender/receiver nodes.
* ********************************************************************************************************************
*/
typedef uint8_t NodeID;

/*
* ********************************************************************************************************************
* 	            				            Data
*
* This is a container for the data (data can come from inlets, logic function or sensor, and go to actuator or outlet)
* ********************************************************************************************************************
*/
struct Data {

  String stringData; // this field can contain information about the origin of the data, or other command-like action modifiers
  int16_t numericData;   // I will allow for negative values too... so when parsing I need to use String::toInt()
  bool event;		   // event is just a boolean "output" of either a sensor or some logic operation (NOTE: maybe not used in the future... - in particular when we will define a type for the data)
  unsigned long timeStamp;  // data time stamp is set at creation/updating time for the DATA (not message)

  Data(): stringData(""), numericData(0), event(false),  timeStamp(millis()) {}
  Data(bool _event): stringData("event"), numericData(0), event(_event),  timeStamp(millis()) {}
  Data(String _str): stringData(_str), numericData(0), event(true),  timeStamp(millis()) {}
  Data(int16_t _numericData, bool _event = false): stringData(F("numeric")), numericData(numericData), event(_event),  timeStamp(millis()) {}
  Data(String _stringData, int16_t _numericData, bool _event, unsigned long _timeStamp): stringData(_stringData), numericData(_numericData), event(_event), timeStamp(_timeStamp) {}

  // Setters: (note: we also have the default copy constructor (direct T a(b), and copy initialization T a=b;)
  void set(String _stringData, int16_t _numericData, bool _event, unsigned long _timeStamp);
  bool setDataFromString(const String& ); // used to set the fields from a string message (usually from the RF module)

  String getStringToSend() const {
    return (stringData + F("/") + String(numericData) + F("/") + (event ? F("1") : F("0"))  + "/" + String(timeStamp));
  }

  void dataDump();
};


/*
* ********************************************************************************************************************
* 					Message
*
* Structure and methods for exchanging messages between the Nodes.
* For the time being, i will use my own simple protocol (not OSC, not JSON, YAML or XML)
* Note that the MAXIMUM length of a single RFM96 packet is 61 bytes. I will try to send all the data in ONE packet.
* The ASCII string sent/received by the RF module is:

* ********************************************************************************************************************
*/

// Types of messages:
//const char UPDATE[] PROGMEM = "UPDATE";   // "String 0" etc are strings to store - change to suit.
//const char ACK_LINK[] PROGMEM = "ACK_LINK";
//const char SNIFFER[] PROGMEM = "SNIFFER";
//const char* const messageTypes[] PROGMEM = {type1, type2, type3};
#define UPDATE "UPDATE"
#define ACK_LINK "ACK_LNK"
#define ACK_LINK_DELETED "ACK_LNK_DEL"

#define SNIFFER_MODE_OFF "SNIFF_OFF"
#define SNIFFER_MODE_ON "SNIFF_ON"
#define SNIFFER_MODE_ON_VERBOSE "SNIFF_ON_VBS"

#define SYNCH_CLK "SYNCH_CLK"
#define SCAN_NET  "SCAN_NET"
#define RESET_CON  "RESET_CON"

class Message {

  private:

    // Header:
    String myType;// type of the message: UPDATE, ACK_LINK, ACK_LINK_DELETED, TOGGLE_SNIFFER...
    NodeID senderID, receiverID;
    unsigned long timeStamp;

    // Payload:
    String myPayloadString;
    // the payload can be: "" for ACK_LINK and ACK_LINK_DELETED, etc
    // "NAME/VAL/EVENT/TIME(data)",  in case of UPDATE packet.
    // "UPDATE/ID1/ID2/TIME(packet)+NAME/VAL/EVENT/TIME(data)" in case of SNIFFER packet.

  public:

    Message() : timeStamp(millis()), myPayloadString("") {
      //  NOTE: since we will concatenate the String object, this will generate many allocations/dealocations and may fragment memory: use reserve(), and do it only
      // once for the permanent object:
      // myPayloadString.reserve(40);
    }

    // Header getter and setters:
    void setType(const String & _type) {
      myType = _type;
    }
    const String& getType() {
      return (myType);
    }

    void setTimeStamp(unsigned long _time) {
      timeStamp = _time;
    }
    unsigned long getTimeStamp() const {
      return (timeStamp);
    }

    void setSenderId(uint8_t _node) {
      senderID = _node;
    }
    uint8_t getSenderId() const {
      return (senderID);
    }

    void setReceiverId(uint8_t _node) {
      receiverID = _node;
    }
    uint8_t getReceiverId() const {
      return (receiverID);
    }

    String getHeaderToSend() const {
      return ( String(myType) );
    }
    String getExtendedHeaderToSend() const { // used to generate the SNIFFER packet
      return (String(myType)  +  F("/")  + String(senderID) + F("/")  +  String(receiverID) );// + F("/") + String(timeStamp));
    }

    bool setMessageFromString(const String& stringToParse); // this sets myType first, and then payloadString depending on the type

    void createDataPayload(const Data& _data) {// this sets the PAYLOAD STRING from a data object
      myPayloadString = _data.getStringToSend();
      //"			     NAME/VAL/EVENT/TIME(data)"
    }

    bool getData(Data& _data) { // parse the data string and build a Data object
      // Note: for the time being, we assume that when we call this, the payload does indeed correspond to a data object
      return (_data.setDataFromString(myPayloadString));
    }

    const Data getData() const {
      Data newData;
      newData.setDataFromString(myPayloadString);
      return (newData);
    }


    String getPayload() const {
      return (myPayloadString);
    }

    // The following will dump different things depending on the type of the message:
    String getStringToSend() const {

      if ( myType == UPDATE ) { // packets that contains payload
        return (getHeaderToSend() + F("+") +  getPayload());
        // Ex: UPDATE+NAME/VAL/EVENT/TIME(data)
      } else
        return (getHeaderToSend());
        // EX: ACK_LINK (no payload)
    }

    void messageDump() {
      LOG_PRINTLN(F("MSG:"));
      LOG_PRINT(F("-type: ")); LOG_PRINTLN(myType);
      LOG_PRINT(F("-SID: ")); LOG_PRINT(senderID); LOG_PRINT(F(", RID: ")); LOG_PRINTLN(receiverID);
      LOG_PRINT(F("-time: ")); LOG_PRINTLN(timeStamp);
      LOG_PRINT(F("-payload: ")); LOG_PRINTLN(myPayloadString);
    }

};





#endif
