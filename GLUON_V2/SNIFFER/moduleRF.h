

/* 
 *  This is a simplified version of the ModuleRF class used in the other gluons (this is the sniffer: we don't need sniffer mode)
 *  By default, and for the SNIFFER gluons, the ack is set to false not to interfere too much with the rest of the network
 *  
 *  
 */


#ifndef _moduleRF_h_
#define _moduleRF_h_

#include "Arduino.h"
#include <SPI.h>
#include <RFM69.h>
#include <RFM69_ATC.h> 
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
#define ACK_TIME   50  // # of ms to wait for an ack
#define NETWORKID 99 //the network ID we are on - the same on all nodes that talk to each other (will be fixed for the time being)
#define FREQUENCY     RF69_433MHZ
//#define FREQUENCY    RF69_868MHZ
//#define FREQUENCY    RF69_915MHZ
#define ENCRYPTKEY    "sampleEncryptKey" //exactly the same 16 characters/bytes on all nodes!

// The address of the sniffer for the whole system:
#define SNIFFER_ID 254 // ATTENTION!!! address 255 is reserved as the BROADCAST address!!!

// when using sendWithRetry:
#define RETRY_WAIT_TIME 40 // in ms
#define NUM_RETRIES 2

class ModuleRF { 
public:
	ModuleRF();

	void init(uint8_t _mynodeid);

	bool updateReceive();

	Message& getReceivedMessage() {return(receivedMessage);} // not a const& because we may want to modify the message
	Message& getSenderMessage() {return(senderMessage);}
	//void setMessage(const Message& _msg) {receivedMessage=_msg;}

	// Send generic message:
	bool sendTo(const Message& msgToSend, const NodeID& toNode, bool _ackMode);
	bool sendTo(const Message& msgToSend, const NodeID& toNode) {return(sendTo(msgToSend, toNode, requestACK));}

	// Send the current "senderMessage":
	bool send(bool _ackMode) {return(sendTo(senderMessage, senderMessage.getReceiverId(), _ackMode));};
	bool send() {return(send(requestACK));}

	void setAckMode(bool _mode) {requestACK = _mode;}
	bool getAckMode() {return (requestACK);}

	// message to send or receive (public so we can access easily their getters and setters)
	Message receivedMessage;
	Message senderMessage;

	// Other weird things:
	// The RFM module has a temperature sensor...
	uint8_t getTemperature() {return(myRadio.readTemperature(-1));}// -1 = user cal factor, adjust for correct ambient

  String rawstring;
private:

	#ifdef AUTO_SET_RF_POWER // use RFM69_ATC instead of RFM69 if one wants automatic power adjustement.
  RFM69_ATC myRadio;
  #else
  RFM69 myRadio;      // Need an instance of the Radio Module (note: default constructor of RFM69 needed)
  #endif

	byte sendSize = 0;
	char payload[61];// note: max payload size for ONE packet is 61 bytes...
	bool requestACK;

};

extern ModuleRF myRF;

#endif
