#include "moduleRF.h"

// Pre-instantiated cross-file global variable
ModuleRF myRF;

ModuleRF::ModuleRF(): sendSize(0) {
	/*
	NOTE: for the time being, I won't make the module sleep (I need to check if they can be waken up by packet arrival)
	myRadio.Sleep(); //sleep right away to save power (ATTN: can it be made to wake up upon data receival??)
	*/
  rawstring="";
}

void ModuleRF::init(uint8_t _mynodeid) {

	myRadio.initialize(FREQUENCY, _mynodeid, NETWORKID); //  radio.initialize(FREQUENCY,NODEID,NETWORKID);
	//myRadio.encrypt(null);
	myRadio.encrypt(ENCRYPTKEY);
	myRadio.promiscuous(false); //set to 'true' to sniff all packets on the same network. To test in the future for the SNIFFER gluon...

  // Power level: this function implements 2 modes as follows:
  //       - for RFM69W the range is from 0-31 [-18dBm to 13dBm] (PA0 only on RFIO pin)
  //       - for RFM69HW the range is from 0-31 [5dBm to 20dBm]  (PA1 & PA2 on PA_BOOST pin & high Power PA settings - see section 3.3.7 in datasheet, p22)
  myRadio.setPowerLevel(30);

	receivedMessage.setReceiverId(_mynodeid);
	senderMessage.setSenderId(_mynodeid);

	setAckMode(false); // by default, and for the SNIFFER gluons, the ack is set to false not to interfere too much with the rest of the network
}

// Generic sender method:
bool ModuleRF::sendTo(const Message& msgToSend, const NodeID& toNode, bool _ackMode) {

	//1)  Build the string to send from the msgToSend:
	String stringToSend = msgToSend.getStringToSend();
	stringToSend.toCharArray(payload, stringToSend.length() + 1);

	// note: the senderMessage contains the senderID (fixed):
	LOG_PRINT(F("Snd RF: ")); LOG_PRINT(senderMessage.getSenderId()); LOG_PRINT(F(" -> ")); LOG_PRINTLN(toNode);
	LOG_PRINT(F("Msg: ")); LOG_PRINTLN(stringToSend);

	// 2) Send the data:
	bool worked = true;
	if (_ackMode) {
		//virtual bool sendWithRetry(uint8_t toAddress, const void* buffer, uint8_t bufferSize, uint8_t retries=2, uint8_t retryWaitTime=40); // 40ms roundtrip req for 61byte packets
		if ( myRadio.sendWithRetry(toNode, payload, stringToSend.length(), NUM_RETRIES, RETRY_WAIT_TIME) )
		LOG_PRINTLN(F("sent: OK")); // this means that we could send the packet, perhaps after several retries (we did receive an acknowledgment...)
		else {
			worked = false;
			LOG_PRINTLN(F("sent: FAIL"));
		}
	} else // send without ack request:
	myRadio.send(toNode, payload, stringToSend.length());

	// Also, if required, send to the sniffer the payload (in this case without ACK): 
//	  if (worked && mySnifferMode == SNIFFER_ACTIVE)  {
//		  // Create sniffer payload, which will be an extended version of the payload just sent:
//		  stringToSend = msgToSend.getExtendedHeaderToSend() + F("+") +  msgToSend.getPayload();
//		  //EX:			 "UPDATE/ID1/ID2/TIME(packet)             +      NAME/VAL/EVENT/TIME(data)"
//		  stringToSend.toCharArray(payload, stringToSend.length() + 1);
//		  myRadio.send(SNIFFER_ID, payload, stringToSend.length()); // without retry
//	  }

	return (worked);
}

bool ModuleRF::updateReceive() { // note: we always receive in the receivedMessage
	bool newData = false;
  rawstring="";

	if (myRadio.receiveDone())
	{
		newData = true;

		// Get the data string to parse into a message:
		for (uint8_t i = 0; i < myRadio.DATALEN; i++) { //can also use radio.GetDataLen() if you don't like pointers
		rawstring += String((char)myRadio.DATA[i]);
		//Serial.print("   [RX_RSSI:");Serial.print(myRadio.RSSI);Serial.print("]");
	}

	// Fill the fields of receivedMessage:
	receivedMessage.setTimeStamp(millis()); // note that the received time stamp may differ from the time stamp of data (this is interesting)
	receivedMessage.setSenderId(myRadio.SENDERID); // note: the receiver node (myNodeID) is set once and for all in the init()
	//newData = receivedMessage.setMessageFromString(rawstring);// newData will be true if the message could be parsed correctly:

//	LOG_PRINT(F("Rec RF: ")); LOG_PRINT(receivedMessage.getReceiverId()); LOG_PRINT(F(" <- ")); LOG_PRINTLN(myRadio.SENDERID);
//	LOG_PRINT(F("Msg: ")); LOG_PRINTLN(rawstring);
	//receivedMessage.dump();

	// Send Acknowledgment if requested:
	// note: sending the acknowlwdgement must be done AFTER retrieving the data on from the radio (otherwise data seems lost)
	if (myRadio.ACKRequested())
	{
		myRadio.sendACK();
		LOG_PRINTLN(F("ACK SENT"));
	}

}

return (newData);
}
