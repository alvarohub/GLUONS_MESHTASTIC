#ifndef _moduleRF_h_
#define _moduleRF_h_

#include "Arduino.h"
#include <SPI.h>
#include <RFM69.h> // TODO: use automatic power library RFM69_ATC
#include "utils.h"

// This will use the SPI interface (check the library RFM12B.h to see what pins to connect depending on the microcontroller):
// Example: for the ArduinoPro mini 328 (Atmega328):
// connect nSEL to 10 (SPI_SS)
// connect SCK to 13  (SPI_SCK)
// connect SDI to 11  (SPI_MOSI)
// connect nRQ ro 2   (SS_BIT)
// connect SDC to 12  (SPI_MISO)
// By default the SPI-SS line used is D10 on Atmega328. You can change it by calling .SetCS(pin) where pin can be {8,9,10}

// We need to initialize the radio by telling it what ID it has and what network it's on
// The NodeID takes values from 1-127, 0 is reserved for sending broadcast messages (send to all nodes)
// The Network ID takes values from 0-255
#define  NETWORKID         99 //the network ID we are on - the same on all nodes that talk to each other (will be fixed for the time being)
#define FREQUENCY          RF69_433MHZ
//#define FREQUENCY    RF69_868MHZ
//#define FREQUENCY    RF69_915MHZ
#define ENCRYPTKEY         "sampleEncryptKey" //exactly the same 16 characters/bytes on all nodes!

#define DEFAULT_ACK_MODE true

// when using sendWithRetry:
#define ACK_WAIT_TIME    20 // in ms (ATTENTION: this is the time to wait for ACKNOWLEDGMENT. It is program BLOCKING)
#define NUM_RETRIES      1

// The address of the sniffer for the whole system:
#define SNIFFER_ID    254 // ATTENTION!!! address 255 is reserved as the BROADCAST address!!!

class ModuleRF {
public:

	enum SnifferMode {SNIFFER_INACTIVE = 0, SNIFFER_ACTIVE, SNIFFER_ACTIVE_VERBOSE }; // make this belong to the class or use a namespace, to avoid polluting the global namespace

	ModuleRF();

	void init(uint8_t _mynodeid);

	bool updateReceive();

	Message& getReceivedMessage() { return(receivedMessage); }   // not a const& because we may want to modify the message
	Message& getSenderMessage() { return(senderMessage); }

	//void setMessage(const Message& _msg) {receivedMessage=_msg;}

	// Send generic message:
	bool sendTo(const Message& msgToSend, const NodeID& toNode, bool _ackMode);
	bool sendTo(const Message& msgToSend, const NodeID& toNode) { return(sendTo(msgToSend, toNode, requestACK)); }

	// Send the current "senderMessage":
	bool send(bool _ackMode) { return(sendTo(senderMessage, senderMessage.getReceiverId(), _ackMode)); }
	bool send() { return(send(requestACK)); }

	// requestACK mode won't be saved on EEPROM (we don't want to have inconsistent modes between gluons at start)
	void setAckMode(bool _mode) { requestACK = _mode; }
	bool getAckMode() { return(requestACK); }

	// message to send or receive (public so we can access easily their getters and setters)
	Message receivedMessage;
	Message senderMessage;

	// mySnifferMode mode won't be saved on EEPROM (we don't want to have inconsistent modes between gluons at start)
	void setSnifferMode(const SnifferMode& _mode) { mySnifferMode = _mode; }
	SnifferMode getSnifferMode() { return(mySnifferMode); }
	//void sendToSniffer(String _str);
	void sendToSniffer(const char* _str);

	// Other weird things:
	// The RFM module has a temperature sensor...
	uint8_t getTemperature() { return(myRadio.readTemperature(-1)); }   // -1 = user cal factor, adjust for correct ambient

private:

	RFM69 myRadio;      // Need an instance of the Radio Module (note: default constructor of RFM69 needed)
	byte  sendSize = 0;
	char  payload[61];  // note: max payload size for ONE packet is 61 bytes...
	bool  requestACK;   //=false;

	SnifferMode mySnifferMode;
};

extern ModuleRF myRF;

#endif
