//
//  NodeClass.cpp
//  Node
//
//  Created by Alvaro Cassinelli on 9/16/15.
//  Copyright (c) 2015 Alvaro Cassinelli. All rights reserved.
//

#include "NodeClass.h"

// ==========================================================================================================================
// ==========================================================================================================================

void Node::init(bool firstTimeBuild)
{
	PRINTLN(F("Init... "));

	myIR.init();

	myRF.init(myNodeId);
	myRF.setSnifferMode(ModuleRF::SNIFFER_INACTIVE); // SNIFFER_INACTIVE = 0, SNIFFER_ACTIVE, SNIFFER_ACTIVE_VERBOSE

	mySensorArray.init(firstTimeBuild);
	myActuatorArray.init(firstTimeBuild);

	// NOTE: the logic should be set AFTER the inlets, outlets, sensors and actuators are initialized:
	if (myLogicPtr) myLogicPtr->init(firstTimeBuild);

	// Global actuators, sensors, switches, etc to indicate various things.
	// They could belong only to NodeClass, but I prefer to
	// have them as cross-file global objects, so I can use for many things (including debugging)
	// myLearningButton (no need to initialize - enable or disable will be decided by the nature of the module)
	// myTiltSensor (no need to initialize)

	myLedIndicator.init(&myInletArray, &myOutletArray, true); // last value is true for enabling the leds strip
	myChirpSpeaker.enable(); // (no need to initialize)
	//myVibrator (no need to initialize)

	// ==== OPTIONAL activity indicating initialization (or for tests )====


	// Vibration:
	//myVibrator.pulse(1000, 255);

	// Inlet/outlet indicators
	//myLedIndicator.showSequence();
	//delay(500);

    // Sound:
    myChirpSpeaker.chirpUp();

	myLedIndicator.blinkInlet(0);
	myLedIndicator.blinkInlet(1);
	myLedIndicator.blinkInlet(2);
	myLedIndicator.blinkOutlet(0);
	delay(500);

	myLedIndicator.update(); // update with current inlet/outlet status

	// Show this node data on serial port
	dumpNodeData();
	// Free RAM:
	PRINTLN(F("Free RAM: ")); PRINTLN(freeRam()); PRINTLN(F(" bytes"));

	// Finally, initialize the sending output timer (to create a controllable delay between the computation and the
	// sending of the result:
	setMinPeriodLatch(DEFAULT_MIN_PERIOD_LATCH);
	lastTimeLatch = millis();
}

void Node::dumpNodeData()
{
	LOG_PRINT(F("ID: ")); LOG_PRINTLN(getNodeId());
	LOG_PRINT(F("Name: ")); LOG_PRINTLN(getString());

	LOG_PRINT(getNumInlets()); LOG_PRINT(F(" IN (")); LOG_PRINT(getNumActiveInletPorts()); LOG_PRINTLN(F(" active)"));
	LOG_PRINT(getNumOutlets()); LOG_PRINT(F(" OUT (")); LOG_PRINT(getNumActiveOutletPorts()); LOG_PRINTLN(F(" active)"));

	//LOG_PRINT(F("Sniffer address: ")); LOG_PRINTLN(SNIFFER_ID);

	LOG_PRINT("Sniffer: ");
	if (myRF.getSnifferMode() == ModuleRF::SNIFFER_ACTIVE) LOG_PRINTLN("ACTIVE");
	else LOG_PRINTLN("INACTIVE");

	LOG_PRINT("RF ACK? ");
	if (myRF.getAckMode()) LOG_PRINTLN("YES");
	else LOG_PRINTLN(F("NO"));

	// Dump sensor data:
	if (mySensorArray.size() > 0) {
		mySensorArray.dump();
	}else {
		LOG_PRINTLN("No sensor.");
	}

	// Dump logic module data:
	if (myLogicPtr) {
		myLogicPtr->dump();
	}else {
		LOG_PRINTLN("No logic!?");
	}

	// Dump actuator data:
	if (myActuatorArray.size() > 0) {
		myActuatorArray.dump();
	}else {
		LOG_PRINTLN("No actuator.");
	}
}

// ====================================================================================================================================
// ====================================================================================================================================

void Node::createInlets(uint8_t _numInlets)   // The fan-in is fixed for the time being
{
	for (uint8_t i = 0; i < _numInlets; i++) {
		// Inlet newInlet(i);
		addInlet(Inlet(i)); // the index may be useful to identify the connexion, for instance the 0 for the "BANG" (or set a led...)
	}
}

void Node::createOutlets(uint8_t _numOutlets)    // The fan-in is fixed for the time being
{
	for (uint8_t i = 0; i < _numOutlets; i++) {
		// Inlet newInlet(i);
		addOutlet(Outlet(i)); // the index may be useful to identify the connexion, for instance the 0 for the "BANG" (or set a led...)
	}
}


void Node::loadConnexions()  // this will load all the data links for inlets and outlets.
{
	myMemory.loadInletArray(myInletArray);
	myMemory.loadOutletArray(myOutletArray);
}


void Node::saveConnexions()
{
	myMemory.saveInletArray(myInletArray);
	myMemory.saveOutletArray(myOutletArray);
}


//=====================================================================================================================================
//=====================================================================================================================================

void Node::update()
{
	// A) RECEIVING DATA ======================================= =======================================

	// 0) Special control buttons or sensors:
	// NOTE: this button could be used for something else than learning, considering that some modules do not have sensors.
	//       Therefore, we could have a more generic class, with a callback for each action. Or at least, the possiblity to
	// 	     disable the button to avoid random changes if no hardware learning button is present...
	myLearningButton.update();
	if (myLearningButton.checkEvent()) { // generated when there is a change on the value of the "discrete analog sensor" button, giving at least values 0 and 1.
		switch(myLearningButton.getData().numericData) {
			case 0: // end up pressed
				//myLedIndicator.allOn();
				learnAnalogConditions();
				//myLedIndicator.update(); // update with current inlet/outlet status
			break;
			case 1: // end up depressed
				// do nothing
			break;
			// ...
			default:
			break;
		}
	}

	// myTiltSensor does not need update (it works on an asynch ISR).
	// NOTE: however, it can be DISABLED if not used.
	if (myTiltSensor.checkEvent()) {

		//myChirpSpeaker.chirpUp();
        myChirpSpeaker.chirpShake();

		//Event is true when the number of shakes in a given time interval is larger than some threshold.
		// In this case, SEND a PULL_INLET_PATCH_CHORD message to all nodes connected outwards from this one.
		myRF.getSenderMessage().setType(PULL_INLET_PATCH_CHORD);
		sendSameMessageRF();
	}

	//1) IR data:
	if (myIR.updateReceive()) {  // this should be the only place where we call this method
		//Processes IR data stream (create new outlets if necessary, do some other actions too such as CLEARING the links...)
		processIRMessage();
		//LOG_PRINTLN("End receive IR test");
	}

	// 2) RF data:
	if (myRF.updateReceive()) {     // this should be the only place where we call this method
		// Processes the received RF stream (update inlets, and create new inlets if necessary):
		processRFMessage();
		//LOG_PRINTLN("End receive RF test");
	}

	// 3) Sensor Data:
	if (mySensorArray.size() > 0) {
		mySensorArray.update();
		//LOG_PRINTLN("End update sensor");
	}

    // F) Update LED indicators   ============================ =======================================
    // NOTE: this is suboptimal: we should better update ONLY when changing the connections. However, it may be handy to do
    // this everytime (to avoid incoherences and to be able to just BLINK leds when changing the connections)
    myLedIndicator.update();
    // LOG_PRINTLN("End update led indicators");

	// C) LOGIC         ======================================= =======================================
	// Perform logical operations between the INLETS and the SENSORS, and store the results in OUTLETS and ACTUATORS, DEPENDING on the
	// update mode (note that an IR "bang" will always force an update, whatever the mode)
	if (myLogicPtr) {
		myLogicPtr->update();
		//  LOG_PRINTLN("End logic operations (if needed)");
	}

	//D) SENDING DATA (OUTLET and ACTUATORS)  ============================ =======================================

	//1) OUTLETS (for the time being only one). Send RF messages through all the links that have new data to send.
	// NOTE: we will have a maximum rate of sending them (even if we don't queue computation) to avoid blocking loops
	if (millis()-lastTimeLatch > minPeriodLatch) {
		if (sendNewDataMessageRF()) { // I separate the conditions to highlight the importance of the ORDER of the checks...
		//myOutletArray.setAllOutletsDataUsed() ;  // not needed: this is done per-outlet in the sendNewDataMessageRF function
		//PRINTLN(F("Output data sent"));
		lastTimeLatch = millis();
		}
	}

	//2) IR BEACON
	if (!myOutletArray.isFullCapacity()) {
		//LOG_PRINTLN("Sending IR message: REQUEST_CREATION_OR_DELETE_INLET");
		MessageIR beaconMessage(REQUEST_CREATION_OR_DELETE_INLET, myNodeId); // COMMAND/VALUE comforms the whole IR CODE to send.
		myIR.updateBeacon(beaconMessage);
	}else {
		//LOG_PRINTLN("Sending IR message: REQUEST_MOVE_OR_DELETE_INLET");
		MessageIR beaconMessage(REQUEST_MOVE_OR_DELETE_INLET, myNodeId); // COMMAND/VALUE comforms the whole IR CODE to send.
		myIR.updateBeacon(beaconMessage);
	}

	// 3) ACTUATORS
	//Move the actuators as dictated by the logical function:
	if (myActuatorArray.size() > 0) {
		myActuatorArray.update();
		//LOG_PRINTLN("End update actuator");
	}


	// E) Process SERIAL COMMANDS?
	processSerialMessage();

	// LOG_PRINTLN("End Node update");
}


//=====================================================================================================================================
//=====================================================================================================================================

void Node::processSerialMessage()
{
	// ...
}

//=====================================================================================================================================
//====================================================================================================================================

void Node::processRFMessage()
{
	char typeString[MAX_LENGTH_TYPE_STRING];
	strcpy(typeString, myRF.getReceivedMessage().getType()); // getType return is a const char* (with termination, i.e., a c-string)

	LOG_PRINT(F("RF: ")); LOG_PRINTLN(typeString);
	//PRINT_CSTRING(typeString);

	// (1) Check if it is a request for update data on some INLET:
	if (!strcmp(typeString, UPDATE)) {
		LOG_PRINT("From ");
		int8_t result = myInletArray.requestForUpdate(myRF.getReceivedMessage());
		// if result is -1, then it means we could not update; otherwise the return value is the inlet where we put the data.
		if (result != -1) {
			PRINT(myRF.getReceivedMessage().getSenderId());
			PRINT(" to inlet ");
			PRINTLN(result);

			//myLedIndicator.update(); // or:
			//myLedIndicator.blinkInlet(result);

			// If required, send to the sniffer the payload (in this case without ACK):
			// - Outlet number: ?
			// - To inlet number: result
			if (myRF.getSnifferMode() == ModuleRF::SNIFFER_ACTIVE_VERBOSE) {
				sendSniffedDataPacket(myRF.getReceivedMessage(), -1, result); // -1 means we don't know the actual index of the sender OUTLET
			}
		}else {
			LOG_PRINT(" unknown ");
			LOG_PRINTLN(myRF.getReceivedMessage().getSenderId());
		}
	}
	// (2) Check if it's an acknowlegment of new connection, i.e. a request for the creation of a new OUTLET link in THIS gluon:
	else if (!strcmp(typeString, ACK_LINK)) {
		if (!myOutletArray[0].isFullCapacity()) {  // otherwise abort (note: this should not really happen if messages are not lost)
			Link newLink(myRF.getReceivedMessage().getSenderId(), millis());
			if (myOutletArray[0].setNewLink(newLink)) {     // first index is the outlet index (0 for now, always... ).
				// note: there should be no need to check the return value of setNewLink because we already checked capacity...

				// PRINT("Out to "); PRINT(myRF.getReceivedMessage().getSenderId()); PRINTLN(" added.");

				// Output link added:
				myChirpSpeaker.chirpUp();

				// Blink the outlet 0 briefly:
				myLedIndicator.blinkOutlet(0);
				//myLedIndicator.update(); // update with current inlet/outlet status

				saveConnexions(); // to avoid mystakes, we just resave the WHOLE interconection pattern

				if (myRF.getSnifferMode() != ModuleRF::SNIFFER_INACTIVE) {
					sendSnifferNO(myRF.getReceivedMessage(), 0);
				}
			}
		}
	}
	// (3) Check if it's an acknowledgment of disconnection, i.e. a request for the deleting an OUTLET link:
	else if (!strcmp(typeString, ACK_LINK_DELETED)) {
		for (uint8_t i = 0; i < myOutletArray.size(); i++) {
			//1) Check if the node ID is in the list of links (in SOME outlet - for the time being only one).
			Port *auxPort = myOutletArray[i].isPortActiveFromNode(myRF.getReceivedMessage().getSenderId());

			if (auxPort != NULL) {                                          //There SHOULD be... but we check anyway.
				myOutletArray[i].listPorts.disconnect(auxPort->myLink); // disconnect the port... and MOVE if LOOSE
				LOG_PRINT("Out to");
				LOG_PRINT(myRF.getReceivedMessage().getSenderId());
				LOG_PRINTLN(" deleted");

				// Output link deleted:
				myChirpSpeaker.chirpDown();

				// Blink the outlet 0 briefly:
				myLedIndicator.blinkOutlet(0);
				//myLedIndicator.update(); // update with current inlet/outlet status

				saveConnexions(); // to avoid mystakes, we just resave the WHOLE interconection pattern

				if (myRF.getSnifferMode() != ModuleRF::SNIFFER_INACTIVE) {
					sendSnifferDO(myRF.getReceivedMessage(), 0);
				}
			}
		}
	}
	// (4) Set sniffer mode by an RF message:
	else if (!strcmp(typeString, SNIFFER_MODE_OFF)) {              // ATTENTION!!! address 255 is reserved as the BROADCAST address, and is the one used to put the gluon on SNIFFER MODE ACTIVE (if not using the IR REMOTE).
		myRF.setSnifferMode(ModuleRF::SNIFFER_INACTIVE);

	}else if (!strcmp(typeString, SNIFFER_MODE_ON)) {              // ATTENTION!!! address 255 is reserved as the BROADCAST address, and is the one used to put the gluon on SNIFFER MODE ACTIVE (if not using the IR REMOTE).
		myRF.setSnifferMode(ModuleRF::SNIFFER_ACTIVE);

	}else if (!strcmp(typeString, SNIFFER_MODE_ON_VERBOSE)) {      // ATTENTION!!! address 255 is reserved as the BROADCAST address, and is the one used to put the gluon on SNIFFER MODE ACTIVE (if not using the IR REMOTE).
		myRF.setSnifferMode(ModuleRF::SNIFFER_ACTIVE_VERBOSE);

	}else if (!strcmp(typeString, SYNCH_CLK)) {
		Data::offsetTime = millis(); // this set the "zero" time coordinate
		// ...or we can also synch using any value sent by the computer:
		//setOffsetTime(myRF.getReceivedMessage().getData().numericData);

	}else if (!strcmp(typeString, SCAN_NET)) {     // SCAN THE CONNECTION NETWORK (response is data on the network connections of THIS gluon)
		// Note: if a gluon in the network is not connected to annything, it will report itself too
		sendSniffedConnections();
	}

	else if (!strcmp(typeString, RESET_CON)) {
		disconnectEverything();
		sendSnifferClearAllConnections(); // we send back to the gluon to confirm it was deleted.
	}

	// Testing the connection "physically" by shaking one module will send a PULL_PATH_CHORD message to all the
	// nodes that are connected to its outlet. Then, when receiving this message, the module will vibrate and show the
	// inlet where it connects to.
	else if (!strcmp(typeString, PULL_INLET_PATCH_CHORD)) {
		showPulledPatchChord(myRF.getReceivedMessage().getSenderId());
	}

	else {                                 //...there cannot be any other case.. for now.
		LOG_PRINTLN(F(" ? "));
	}
}

// =============================================================================================================================================
// =============================================================================================================================================

bool Node::sendMessageToChildren(Outlet& fromOutlet) {
	// This method sends a message in senderMessage (= RF.getSenderMessage()) to all the children of a *given* outlet.
		bool couldSendThisOutlet = true;
		for (uint8_t j = 0; j < fromOutlet.listPorts.maxSize(); j++) {
			Port portToSend = fromOutlet.listPorts[j];
			if (portToSend.isActive()) {
				// Set port to send:
				myRF.getSenderMessage().setReceiverId(portToSend.myLink.nodeId);
				//myRF.getSenderMessage().setSenderId(myNodeId); // no need, it never changes.

				// Send the message:
				bool couldSendThisPort = myRF.send(); // send() alone sends the senderMessage by default
				couldSendThisOutlet &= couldSendThisPort;

				// If required, send to the sniffer the payload (in this case without ACK):
				// Outlet number: i
				// To inlet number: unkown - this should be seek at the receiver side (otherwise it would be a waste of memory)
				if (couldSendThisPort && (myRF.getSnifferMode() != ModuleRF::SNIFFER_INACTIVE)) {
					sendSniffedDataPacket(myRF.getSenderMessage(), fromOutlet.getIndex(), -1); // -1 means we don't know the actual index (inlet or outlet)
				}
			}
		}
	return(couldSendThisOutlet);
}

bool Node::sendNewDataMessageRF() //This method sends the data from all the outlets in the array outletArray that have data "ready to send"
{
	bool couldSendAll = true;
	for (uint8_t i = 0; i < myOutletArray.size(); i++) {
		if (myOutletArray[i].isNewData()) {
			// Set message type, here UPDATE, since it will send DATA to the next module:
			myRF.getSenderMessage().setType(UPDATE);
			// Set data fields since this is a request for update:
			myRF.getSenderMessage().createDataPayload(myOutletArray[i].getData());

			if (sendMessageToChildren(myOutletArray[i])) {

				myOutletArray[i].setNewDataFlag(false);  // this sets the new data flag to false
				//LedIndicator.update();

			} else
				couldSendAll=false;
		}
	}
	//PRINTLN("here");
	return(couldSendAll);
}

bool Node::sendSameMessageRF() {
	// For instance, a payload-less PULL_INLET_PATCH_CHORD message to all the nodes connected to this node outlets
		bool couldSendAll = true;
		for (uint8_t i = 0; i < myOutletArray.size(); i++) couldSendAll&=sendMessageToChildren(myOutletArray[i]);
		return(couldSendAll);
}


// =============================================================================================================================================
// =============================================================================================================================================


void Node::processIRMessage()
{
	bool notCommand = false;

	LOG_PRINT("RCV IR msg: ");

	//1) First, check if it's a COMMAND:
	switch (myIR.getCommand()) {
	// ========= COMMANDS (with associated VALUE) ==================================================================================
	case REQUEST_CREATION_OR_DELETE_INLET: // this message is sent by a node that did not saturate all the outlet links.
		LOG_PRINTLN("CREA/DEL link");
		processRequest(myIR.getValue());   //myIR.getValue() contains the node ID to where it has to connect
		break;

	case REQUEST_MOVE_OR_DELETE_INLET:    // this IR message is sent by a node that has saturated the number of possible outlet links
		LOG_PRINTLN("MOV/DEL link");
		processRequest(myIR.getValue());
		break;

	default:
		LOG_PRINTLN("Not COMM");
		notCommand = true;
		break;
	}

	// ========= NOT COMMANDS (check if it is a  RAW CODE sent by a REMOTE CONTROLLER, not another Node) ==========================
	if (notCommand) {
		switch (myIR.getRawCode()) {
		case CODE_REPEAT: // this means that the NEC Remote controlled just repeated the code (it actually sends a repeat code).
			// for the time being, just ignore this...
			break;

		case CODE_FORCE_BANG:
			if (myLogicPtr) {
				LOG_PRINTLN("FORCE BANG");
				myLogicPtr->forceUpdate();
			}
			break;

		// Change update mode:
		case CODE_SET_STANDBY:
			if (myLogicPtr) {
				LOG_PRINTLN("STANDBY");
				myLogicPtr->setUpdateMode(ONLY_MANUAL_BANG); // this will also CLEAR all the flags, and save on EEPROM
			}
			break;

		case CODE_ON_SENSOR_EVENT:
			if (myLogicPtr) {
				LOG_PRINTLN("SENSOR BANG");
				myLogicPtr->toggleUpdateMode(SENSOR_EVENT_BANG);
			}
			break;

		case CODE_SET_UPDATE_FIRST_INLET_BANG:
			if (myLogicPtr) {
				LOG_PRINTLN("1st BANG");
				myLogicPtr->toggleUpdateMode(FIRST_INLET_BANG);
			}
			break;

		case CODE_SET_UPDATE_ANY_INLET_BANG:
			if (myLogicPtr) {
				LOG_PRINTLN("ANY BANG");
				myLogicPtr->toggleUpdateMode(ANY_INLET_BANG);
			}
			break;

		case CODE_SET_UPDATE_SYNCH:
			if (myLogicPtr) {
				LOG_PRINTLN("SYNCH BANG");
				myLogicPtr->toggleUpdateMode(SYNCH_BANG);
			}
			break;

		case CODE_SET_UPDATE_PERIODIC:
			if (myLogicPtr) {
				LOG_PRINTLN("PERIODIC BANG");
				myLogicPtr->toggleUpdateMode(PERIODIC_BANG);
			}
			break;

		case CODE_CLEAR_ALL_CONNEXIONS:
			LOG_PRINTLN("CLR ALL CONN");
			disconnectEverything(); // disconnects outlets and inlets links
			saveConnexions();       // save on EEPROM the new state of all connections (input and output)
			if (myRF.getSnifferMode() != ModuleRF::SNIFFER_INACTIVE) sendSnifferClearAllConnections();
			break;

		case CODE_CLEAR_ALL_INLET_LINKS:
			LOG_PRINTLN("CLR IN");
			myInletArray.disconnectEverything(); // only inlets are disconnected
			// Finally, save in EEPROM:
			myMemory.saveInletArray(myInletArray);
			// saveConnexions(); // to avoid mystakes, we could just resave the WHOLE interconection pattern
			if (myRF.getSnifferMode() != ModuleRF::SNIFFER_INACTIVE) sendSnifferClearAllInConnections();
			break;

		case CODE_CLEAR_ALL_OUTLET_LINKS:
			LOG_PRINTLN("CLR OUT");
			myOutletArray.disconnectEverything(); // only outlet are disconnected
			myMemory.saveOutletArray(myOutletArray);
			// saveConnexions(); // to avoid mystakes, we could just resave the WHOLE interconection pattern
			if (myRF.getSnifferMode() != ModuleRF::SNIFFER_INACTIVE) sendSnifferClearAllOutConnections();
			break;

		case CODE_LEARN_SENSOR_THRESHOLD:
			learnAnalogConditions();
			break;

		//...
		default:
			LOG_PRINTLN("?");
			break;
		}
	}
}

void Node::learnAnalogConditions() {
	// Force learning conditions for ALL the current sensors (for which learning is ENABLED only) on this gluon (digital sensors are not concerned, neither the "common control sensors")
	PRINTLN(F("LEARN"));
	mySensorArray.learnConditions();  // here the conditions are NOT saved on EEPROM,
	mySensorArray.saveConditions();   // but here there are saved (note that if the sensor has not enabled learning, saving occurs all the same - of random values...)
	mySensorArray.update();
	mySensorArray.dump();
}

// ========================================================================================================================
// ========================================================================================================================


void Node::processRequest(const NodeID& _toNode) // called from the IR initial transaction
{
	LOG_PRINT("Action: ");
	InletArray::ResultRequestConnection result = myInletArray.requestForConnexion(_toNode);

	switch (result.myAction) {
	case InletArray::LINK_ADDED:
		// IMPORTANT: in principle, it should be in a "candidate" state until we receive an acknoledgment of the creation of the OUTPUT - this happens often if the antena is NOT long enough: the IR message can be received, but
		// then the RF message is not received. Anyway, this won't happen (I hope) in the real situation when the modules will be very close anyway so that the IR message is received. But for more robustness, it would be good
		// to implement this handshake...

		// Send an acknowledgment to the other node so it can ADD AN OUTLET LINK TO THIS INLET:
		myRF.getSenderMessage().setType(ACK_LINK);
		myRF.getSenderMessage().setReceiverId(_toNode); //_toNode = myIR.getValue() is the node ID of the sender outlet
		myRF.getSenderMessage().setSenderId(myNodeId);  // not really needed, this was set once and for all

		if (myRF.send()) {
			LOG_PRINT("Link from node ");
			LOG_PRINT(_toNode);
			LOG_PRINT("CREA on IN ");
			LOG_PRINT(result.inletIndex1);

			// Link has been ADDED:
			myChirpSpeaker.chirpUp();

			// Blink the inlet result.inletIndex1) briefly:
			myLedIndicator.blinkInlet(result.inletIndex1);
		//	myLedIndicator.update(); // update with current inlet/outlet status

			saveConnexions(); // to avoid mystakes, we just resave the WHOLE interconection pattern

			if (myRF.getSnifferMode() == ModuleRF::SNIFFER_ACTIVE_VERBOSE) {
				sendSnifferNI(myRF.getSenderMessage(), result.inletIndex1);
			}
		}else {
			loadConnexions(); // otherswise, we have to go back to the previous state of the inputs
		}
		break;

	case InletArray::LINK_DELETED:

		// Send an ACK_LINK_DELETED to the other node so it can DELETE AN OUTLET LINK TO THIS INLET:
		myRF.getSenderMessage().setType(ACK_LINK_DELETED);
		myRF.getSenderMessage().setReceiverId(_toNode); //_toNode = myIR.getValue() is the node ID of the sender outlet
		myRF.getSenderMessage().setSenderId(myNodeId);  // not really needed, this was set once and for all

		if (myRF.send()) {
			LOG_PRINT("Link from node ");
			LOG_PRINT(_toNode);
			LOG_PRINT(" at IN ");
			LOG_PRINT(result.inletIndex1);
			LOG_PRINTLN(" DEL");

			// Link deleted:
			myChirpSpeaker.chirpDown();

			// Blink the inlet result.inletIndex1) briefly:
			myLedIndicator.blinkInlet(result.inletIndex1);
			//myLedIndicator.update(); // update with current inlet/outlet status

			saveConnexions(); // to avoid mystakes, we just resave the WHOLE interconection pattern

			if (myRF.getSnifferMode() == ModuleRF::SNIFFER_ACTIVE_VERBOSE) {
				sendSnifferDI(myRF.getSenderMessage(), result.inletIndex1);
			}
		}else {
			loadConnexions(); // otherswise, we have to go back to the previous state of the inputs
		}
		break;

	case InletArray::LINK_MOVED:
		// No need to send anything to the other gluon.

		LOG_PRINT("Link from node ");
		LOG_PRINT(_toNode);
		LOG_PRINT(" MOVED from IN ");
		LOG_PRINT(result.inletIndex1);
		LOG_PRINT(" to IN ");
		LOG_PRINTLN(result.inletIndex2);

		// Link Moved
		myChirpSpeaker.chirpDownUp();

		// Blink the inlet result.inletIndex2) briefly:
		myLedIndicator.blinkInlet(result.inletIndex2);
		//myLedIndicator.update(); // update with current inlet/outlet status

		saveConnexions(); // to avoid mystakes, we just resave the WHOLE interconection pattern

		//... but send to the sniffer:
		if (myRF.getSnifferMode() != ModuleRF::SNIFFER_INACTIVE) {
			sendSnifferMOV(myNodeId, _toNode, result.inletIndex1, result.inletIndex2);
		}

		break;

	case InletArray::NO_ACTION:
		LOG_PRINTLN("None");
		break;
	}
}


// ====================================================================================================================================
/* ====================================================================================================================================

SNIFFER MESSAGES:

There are of two kinds: (a) normal packets and (b) connection transactions.
For easier parsing, I am using this RPN convention using a binary or ternary OPERATOR:

		 			X,Y,...,COMMAND (followed by EOL)

Where the arguments can be either a number, a pair of numbers, a combination of both, or a string (payload data)
Example sniffed strings for a typical communication between gluon 3 (sender) and gluon 4 (receiver):

- Creation of an outlet link: 		[3,0],4,NO				New link created on outlet 0 of gluon 3, TO gluon 4.
- Deletion of an outlet link:		[3,0],4,DO				Deleting link on outlet 0 of gluon 3, that was connected TO gluon 4
- Creation of an inlet link:		3,[4,2],NI				New link created on inlet 2 of gluon 4, FROM gluon 3
- Deletion of an inlet link:		3,[4,2],DI				Deleting link on inlet 2 of gluon 4, that was connected FROM gluon 3
- Move of INLET link:			 	3,[4,2],DI [NEW LINE] 3,[4,1],NI	As you can see, there is no need of a new code and the parser
 																		will work.
- A SENT packet:       				[3,0],4,UPDATE+4536+ROT/10/1/15500,S	Note that parsing the payload is different (I want to use
 																			OSC like packets)
- A RECEIVED packet:   				3,[4,2],UPDATE+4536+ROT/10/1/15500,R

- Connection data:					3,[4,5,..],[X,6,7],NET	This means that the message refers to gluon with ID 2, and:
 															- The (unique) output connecting to the nodes 4 and 5 (there can be more
															connections - up to four - or none, in that case it will read []).
															- The inlets (from 0 to 2) connect to: nothing (X), node 6, and node 7.

- Disconnecting ALL inlets/outlets:	3,CLR_CON	(first the index of the gluon)
- Disconnecting all OUTLETS:		3,CLR_CON_OUT
- Disconnecting all INLETS:			3,CLR_CON_IN

=====================================================================================================================================*/
// Signal the CREATION of a an OUTLET link ("NO" stands for New Outlet)
// On module myNodeId (myRF.getReceivedMessage().getReceiverId()), create link on Outlet outletIndex
// Towards module myRF.getReceivedMessage().getSenderId(), on Inlet UNKNOWN
// Example: [3,0],4,NO
void Node::sendSnifferNO(Message& msgToSniff, uint8_t outletIndex ) {
	//String strToSniffer="["+String(msgToSniff.getReceiverId())+","+String(outletIndex)+"],"+String(msgToSniff.getSenderId())+",NO";

	// other methods (note: they don't give less FLASH memory, but more. However, perhaps they are less prone to heap fragmentation?)
	char strToSniffer[MAX_LENGTH_PAYLOAD_RF];
	sprintf(strToSniffer, "[%d,%d],%d,NO", msgToSniff.getReceiverId(), outletIndex, msgToSniff.getSenderId());
	// or:
	// strcat(stringData2, String(msgToSniff.getReceiverId()).c_str());
	// strcat(stringData2, ",");
	// strcat(stringData2, String(outletIndex).c_str());
	// strcat(stringData2, "],");
	// strcat(stringData2, String(msgToSniff.getSenderId()).c_str());
	// strcat(stringData2, ",NO");

	delay(20); // this delay is for avoiding collisions with the sendSnifferNI on the other gluon!
	myRF.sendToSniffer(strToSniffer);
}

// Signal the CREATION of a an INLET link ("NI" stands for New Inlet)
// sendSnifferNI( myRF.getSenderMessage(), result.inletIndex1 );
// Example: 3,[4,2],NI
void Node::sendSnifferNI(Message& msgToSniff, uint8_t inletIndex ) {
	//String strToSniffer=String(msgToSniff.getReceiverId())+",["+String(msgToSniff.getSenderId())+","+String(inletIndex)+"],NI";
	char strToSniffer[MAX_LENGTH_PAYLOAD_RF];
	sprintf(strToSniffer, "%d,[%d,%d],NI", msgToSniff.getReceiverId(), msgToSniff.getSenderId(),inletIndex);
	myRF.sendToSniffer(strToSniffer);
}

// Signal the DELETION of a an OUTLET link ("DO" stands for Delete Outlet)
// On module myNodeId (myRF.getReceivedMessage().getReceiverId()), delete link on Outlet outletIndex
// Towards module myRF.getReceivedMessage().getSenderId(), on Inlet UNKNOWN
// Example: [3,0],4,DO
void Node::sendSnifferDO(Message& msgToSniff, uint8_t outletIndex ) {
	//String strToSniffer="["+String(msgToSniff.getReceiverId())+","+String(outletIndex)+"],"+String(msgToSniff.getSenderId())+",DO";
	char strToSniffer[MAX_LENGTH_PAYLOAD_RF];
	sprintf(strToSniffer, "[%d,%d],%d,DO", msgToSniff.getReceiverId(), outletIndex, msgToSniff.getSenderId());
	delay(15); // this delay is for avoiding collisions with the sendSnifferDI on the other gluon!
	myRF.sendToSniffer(strToSniffer);
}

// Signal the DELETION of a an INLET link ("DI" stands for Delete Inlet)
// sendSnifferDI( myRF.getSenderMessage(), result.inletIndex1 );
// Example: 3,[4,2],NI
void Node::sendSnifferDI(Message& msgToSniff, uint8_t inletIndex ) {
	//String strToSniffer=String(msgToSniff.getReceiverId())+",["+String(msgToSniff.getSenderId())+","+String(inletIndex)+"],DI";
	char strToSniffer[MAX_LENGTH_PAYLOAD_RF];
	sprintf(strToSniffer, "%d,[%d,%d],DI", msgToSniff.getReceiverId(), msgToSniff.getSenderId(), inletIndex);
	myRF.sendToSniffer(strToSniffer);
}

// Example: 3,[4,2],DI\n3,[4,1],NI
void Node::sendSnifferMOV( uint8_t _myNodeId, uint8_t _toNode, uint8_t fromInlet, uint8_t toInlet) {
	//String strToSniffer=String(_toNode)+",["+String(_myNodeId)+","+String(fromInlet)+"],DI\n"; // 3,[4,2],DI [EOF]
	//strToSniffer+=String(_toNode)+",["+String(_myNodeId)+","+String(toInlet)+"],NI";		  // 3,[4,1],NI

	char strToSniffer[MAX_LENGTH_PAYLOAD_RF];
	sprintf(strToSniffer, "%d,[%d,%d],DI\n%d,[%d,%d],NI", _toNode, _myNodeId, fromInlet, _toNode, _myNodeId, toInlet);
	myRF.sendToSniffer(strToSniffer);
}

void Node::sendSniffedDataPacket(Message& msgToSniff, int outletIndex, int inletIndex) {
	// Notes:
	// - msgToSniff contains both the sender and receiver node ID, interpreted differently depending if this was received or sent packet.
	// - outletIndex and inletIndex are the respective outlet/inlet indexes of the sender and receiver (both cannot be know simultaneously)
	// - When it is not known, we will send -1. If the outletIndex is -1, this means that this is a RECEIVED packet...
	//String strToSniffer;
	char strToSniffer[MAX_LENGTH_PAYLOAD_RF];

	if (outletIndex<0) { // msgToSniff is is a RECEIVED packet:
		// Example output: 3,[4,2],UPDATE/4536+ROT/10/1/15500,R
		//strToSniffer = String(msgToSniff.getSenderId()) + ",[" + String(msgToSniff.getReceiverId()) + "," + String(inletIndex) + "]," + msgToSniff.getStringToSend()+",R";
		char stringtosend[MAX_LENGTH_PAYLOAD_RF]; msgToSniff.getStringToSend(stringtosend);
		sprintf(strToSniffer, "%d,[%d,%d],%s,R",msgToSniff.getSenderId(), msgToSniff.getReceiverId(), inletIndex, stringtosend);
		delay(20); // this delay is for avoiding collisions with the sendSniffedDataPacket on the sender gluon!
	} else { // this is a sent packet:
		// Example output: [3,0],4,UPDATE/4536+ROT/10/1/15500,S
		//strToSniffer = "["+String(msgToSniff.getSenderId()) + "," + String(outletIndex) + "]," + String(msgToSniff.getReceiverId()) + "," + msgToSniff.getStringToSend()+",S";
		char stringtosend[MAX_LENGTH_PAYLOAD_RF]; msgToSniff.getStringToSend(stringtosend);
		sprintf(strToSniffer, "[%d,%d],%d,%s,S",msgToSniff.getSenderId(), outletIndex, msgToSniff.getReceiverId(),  stringtosend);
	}
	myRF.sendToSniffer(strToSniffer);
}

// Example: 3,[4,5,..],[1,6],[],[4,7,8,1],NET
void Node::sendSniffedConnections() {
	// Send CONNECTION DATA STRING for this gluon:
	char strToSniffer[MAX_LENGTH_PAYLOAD_RF];
	sprintf(strToSniffer,"%d,[", getNodeId()); // we start the lists of connections for the outlets.
	//for (unit8_t i=0; i<getNumOutlets(); i++) { // actually it is supposed to be only ONE OUTLET for the time being...
	for (uint8_t j = 0; j < myOutletArray[0].listPorts.maxSize(); j++) { // we need to scan the whole array of ports, even
		// though the number of active ports may be smaller (they are not in order!)
		Port portChecked = myOutletArray[0].listPorts[j];
		if (portChecked.isActive()) {
			//strToSniffer+=String(portChecked.myLink.nodeId);
			char stringId[4]; itoa(portChecked.myLink.nodeId, stringId, 10);
			strcat(strToSniffer,stringId);
			if (j<myOutletArray[0].listPorts.getNumActivePorts()-1) strcat(strToSniffer,","); //strToSniffer+=",";
		}
	}
	// close list for the (unique) outlet:
	strcat(strToSniffer,"],");
	// Create lists of connected nodes for the inlets:
	for (uint8_t i=0; i<getNumInlets(); i++) {
		strcat(strToSniffer,"[");
		for (uint8_t j = 0; j < myInletArray[i].listPorts.maxSize(); j++) {
			Port portChecked = myInletArray[i].listPorts[j];
			if (portChecked.isActive()) {
				//strToSniffer+=String(portChecked.myLink.nodeId);
				char stringId[4]; itoa(portChecked.myLink.nodeId, stringId, 10);
				strcat(strToSniffer,stringId);
			}
			if (j<myInletArray[i].listPorts.maxSize()-1) strcat(strToSniffer,",");
		}
		strcat(strToSniffer,"]");
		if (i<getNumInlets()-1) strcat(strToSniffer,",");
	}
	strcat(strToSniffer,"],NET");

	/*
	 NOW, THIS IS IMPORTANT: to avoid having all the gluons sending at the same time to the sniffer gluon and loosing data
	 (note that on top of multiple collisions, we are sending without ack), I will adopt a very simple strategy: a delay before
	 sending that is proportional to the nodeID number. Of course, this will BLOCK the execution of the whole data-flow network,
	 but I suppose this is not a problem since this kind of message is only sent rarely (when scanning the network for the first time
	 for instance...). Since sending a 61 byte packet takes roughly 10-15ms, we may safely use this formula for the sending times:
	                        Tk = 15 * k, with k from 0 to 253 (the nodeID). Assuming then than there is in the network
	 a node whose ID is 253, this means that a full SCAN_NET will complete after 15*253 = 3795ms.
	 */
	delay(15*myNodeId);
	myRF.sendToSniffer(strToSniffer);
	//... and to avoid having some gluons working while the others are still waiting their turn to send to the sniffer, we wait:
	delay(15*(253-myNodeId));

}

void Node::sendSnifferClearAllConnections() {
	//String strToSniffer=String(getNodeId())+",CLR_CON";
	char strToSniffer[MAX_LENGTH_PAYLOAD_RF];
	sprintf(strToSniffer,"%d,CLR_CON", getNodeId());
	delay(15*getNodeId());
	myRF.sendToSniffer(strToSniffer);
}

void Node::sendSnifferClearAllOutConnections() {
	//String strToSniffer=String(getNodeId())+",CLR_CON_OUT";
	char strToSniffer[MAX_LENGTH_PAYLOAD_RF];
	sprintf(strToSniffer,"%d,CLR_CON_OUT", getNodeId());
	delay(15*getNodeId());
	myRF.sendToSniffer(strToSniffer);
}

void Node::sendSnifferClearAllInConnections() {
	//String strToSniffer=String(getNodeId())+",CLR_CON_IN";
	char strToSniffer[MAX_LENGTH_PAYLOAD_RF];
	sprintf(strToSniffer,"%d,CLR_CON_IN", getNodeId());
	delay(15*getNodeId());
	myRF.sendToSniffer(strToSniffer);
}

void Node::showPulledPatchChord(NodeID _toThisID) {
	// Vibrate and show with colors the inlets that are connected:


	// (a) show only the connected inlets:
	myLedIndicator.showInletsMatchID(myInletArray, _toThisID);
	//myLedIndicator.update(); // update with current inlet/outlet status

    // (b) vibrate
	//myVibrator.pulse(500, 255);
    myVibrator.manyPulses(3, 500, 255);

}
