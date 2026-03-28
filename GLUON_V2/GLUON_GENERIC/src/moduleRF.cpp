#include "moduleRF.h"

// Pre-instantiated cross-file global variable
ModuleRF myRF;

ModuleRF::ModuleRF(): sendSize(0) {
	/*
	NOTE: for the time being, I won't make the module sleep (I need to check if they can be waken up by packet arrival)
	However, it is possible to put the RADIO in sleep mode (it will anyway wake up when doing receiveDone(), but can it
	wake up upon packet reception? TODO: search about that)
	myRadio.sleep(); //sleep right away to save power
	*/
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

	setAckMode(DEFAULT_ACK_MODE);
}

// Generic sender method:
bool ModuleRF::sendTo(const Message& msgToSend, const NodeID& toNode, bool _ackMode) {

	//1)  Build the string to send from the msgToSend:
	// String stringToSend = msgToSend.getStringToSend();
	// stringToSend.toCharArray(payload, stringToSend.length() + 1);

	char buffer[62];
	msgToSend.getStringToSend(buffer); // NOTE: this will produce a string with only the type of the message, or with
	// payload too (the method getStringToSend check the type - for instance, UPDATE has payload, but PULL_INLET_PATCH_CHORD or ACK_LINK don't)

	// note: the senderMessage contains the senderID (fixed):
	// PRINT(F("Snd RF: ")); PRINT(senderMessage.getSenderId()); PRINT(F(" -> ")); PRINTLN(toNode);
	// PRINT(F("Snd: ")); for (int j=0; j< strlen(buffer); j++) PRINT(buffer[j]); PRINTLN(); //PRINTLN(strlen(buffer));

	// 2) Send the data:
	bool worked = true;
	if (_ackMode) {

		if ( myRadio.sendWithRetry(toNode, buffer, strlen(buffer), NUM_RETRIES, ACK_WAIT_TIME) )
		{
			PRINTLN(F("sent: OK")); // this means that we could send the packet, perhaps after several retries (we did receive an acknowledgment...)
		} else {
			worked = false;
			PRINTLN(F("sent: FAIL"));
		}
	} else { // send without ack request:
		//myRadio.send(toNode, payload, stringToSend.length());
		myRadio.send(toNode,  buffer, strlen(buffer));
	}
	return (worked);
}

bool ModuleRF::updateReceive() { // note: we always receive in the receivedMessage
	bool newData = false;
	char rawstring[MAX_LENGTH_PAYLOAD_RF]; // a packet on the RFM69 can be 61 bytes, but we want a string - so we need to include the NULL character
	//String rawstring = "";

	if (myRadio.receiveDone()) { // this wakes up from radio sleep mode (if it was in sleep mode)
		newData = true;

		//PRINTLN("Rec RF:");
		//PRINT("   [RX_RSSI:");PRINT(myRadio.RSSI);PRINTLN("]");
		// Get the data string to parse into a message:
		for (uint8_t i = 0; i<myRadio.DATALEN; i++) { // can also use myRadio.GetDataLen() if not using pointer
			rawstring[i] = (char)myRadio.DATA[i];
			//rawstring += String((char)myRadio.DATA[i]);
		}

		rawstring[myRadio.DATALEN] = '\0'; // string terminator (not needed, myRadio does it for you?)
		//PRINT_CSTRING(rawstring);

		// Fill the fields of receivedMessage:
		receivedMessage.setTimeStamp(millis()); // note that the received time stamp may differ from the time stamp of data (this is interesting)
		receivedMessage.setSenderId(myRadio.SENDERID); // note: the receiver node (myNodeID) is set once and for all in the init()
		newData = receivedMessage.setMessageFromString(rawstring);// newData will be true if the message could be parsed correctly:

		if (newData) {
			PRINTLN("PARSE OK: ");
			PRINT(F("Rec RF: ")); PRINT(receivedMessage.getReceiverId()); PRINT(F(" <- ")); PRINTLN(myRadio.SENDERID);
			PRINT(F("Msg: ")); PRINT_CSTRING(rawstring);
			//receivedMessage.dump();
		} else 	PRINTLN("PARSE FAIL");

		// Send Acknowledgment if requested:
		// note: sending the ACK must be done AFTER retrieving the data on from the radio (otherwise data seems lost)
		if (myRadio.ACKRequested()) {
			myRadio.sendACK();
			LOG_PRINTLN(F("ACK SENT"));
		}

	}
	return (newData);
}

// void ModuleRF::sendToSniffer(String _str) { // the way to send things to the the sniffer is always through a string.
//  _str.toCharArray(payload, _str.length() + 1);
//  myRadio.send(SNIFFER_ID, payload, _str.length()); // without retry!
// }

void ModuleRF::sendToSniffer(const char* _str) { // the way to send things to the the sniffer is always through a string.
	//_str.toCharArray(payload, _str.length() + 1);
	myRadio.send(SNIFFER_ID, _str, strlen(_str)); // without retry!
}
