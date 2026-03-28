#include "Inlet.h"


Port* Inlet::isPortActiveFromNode(const NodeID& _toNode) { // we retrieve only the FIRST match - we assume a max of one connection per inlet to another node!
	for (uint8_t j = 0; j < listPorts.maxSize(); j++) { //  scan ALL the ports of this inlet
		if ( listPorts[j].isActive() && listPorts[j].matchNode(_toNode) ) return (&listPorts[j]);
	}
	return (NULL); // otherwise return NULL pointer
}

bool Inlet::setNewLink(const Link& _link) {
	if (listPorts.connect(_link)) {// the method check both space availability and no conflict (another similar link)
		return (true);
	} else
		return (false);
}

bool Inlet::setNewLinkFromMessage(const Message& _msgLink) {
	Link newLink(_msgLink.getSenderId(), _msgLink.getTimeStamp()); // attn: time reception, not timestamp
	return setNewLink(newLink);
}

void Inlet::clearAllLinks() {
	listPorts.disconnectAll();
	// Also clear the links connections on EEPROM here? better not here, for having a clear separation between EEPROM snapshots and dynamic states
	// myMemory.clearInletLinks(index);
}

bool Inlet::isConnected(NodeID _toThisNode) {
	for (uint8_t i = 0; i < listPorts.maxSize(); i++) { // scan ALL the ports:
		// Search an active port with a "fixed" or "loose" link, matching the ID:
		if ( listPorts[i].isActive() && listPorts[i].matchNode(_toThisNode) ) { //&&  (listPorts[i].stateLink() == FIXED
			return(true);
		}
	}
	return(false);
}

bool Inlet::update(const Message& receivedMessage) {
	// Returns true if update was possible. New data carried by the received message-link is stored in the INLET
	// Check if this inlet has an active port with a "fixed" or "loose" link, matching the ID of the sender:
	if (isConnected(receivedMessage.getSenderId())) {
		// Then, update the INLET Data... HERE THIS IS CRUCIAL!!
		// The data STORED ON THIS INLET is the LATEST RECEIVED and will erase previous stored data.
		// Of course it would be possible to store data in each link, but then they would be INLETS, not links.
		// We could even have a Data buffer (Data[..]), sort of memory of the arrival times, and even SORT THEM BY TIMESTAMP. But I won't for now.
		// I REPEAT: THIS PIECE OF CODE IS ESSENTIAL to the working paradigm. So we just do:

		this->setNewData(receivedMessage.getData());
		return(true);
	}
	return(false);
}


// ========================================================================

bool InletArray::addNewInlet(const Inlet & _newInlet) {
	if (numInletsUsed < MAX_NUM_INLETS) {
		arrayInlets[numInletsUsed++] = _newInlet; // default copy constructor is ok because  ArrayPort <MAX_FAN_IN> listPorts is an standard array (not dynamically allocated)
		return (true);
	} else
		return (false);
}


Inlet& InletArray::operator[] (const uint8_t  _index) { // note: we return a reference (and not const)
	if (_index < numInletsUsed)
		return arrayInlets[_index];
	else
		return arrayInlets[numInletsUsed - 1]; // let's return the last one
}

const Inlet& InletArray::operator[] (const uint8_t  _index) const {
	if (_index < numInletsUsed)
		return arrayInlets[_index];
	else
		return arrayInlets[numInletsUsed - 1]; // let's return the last one
}

// Disconnect all links from all inlets:
void InletArray::disconnectEverything() { // this could mean setting these to IDLE, but it is simpler to set the number of active links to 0
	for (uint8_t i = 0; i < numInletsUsed; i++) arrayInlets[i].clearAllLinks();
	//LOG_PRINTLN(F("All Inlet links cleared."));
}

InletArray::ResultRequestConnection InletArray::requestForConnexion(const NodeID & _toNode) {
	// 1) First, if the node id is somewere in the list of links (in SOME inlet), then:
	// - if it was FIXED, then delete it.
	// - if it was LOOSE, then move it (if possible)
	ResultRequestConnection result = requestMoveDeleteInlet(_toNode);

	// 2) Otherwise, we need to CREATE it (if possible, otherwise do nothing):
	if (result.myAction == NO_ACTION) { // this means that the inlet could not be moved or deleted.
		// It means that there is no active port in any inlet, to this nodeID:  create a new link (IF POSSIBLE):
		for (uint8_t i = 0; i < numInletsUsed; i++) {
			if (arrayInlets[i].hasSpace()) {
				Link newLink(_toNode, millis()); // time creation
				arrayInlets[i].setNewLink(newLink); // this also saves on EEPROM the new list of ports
				result.inletIndex1=i;
				result.myAction=LINK_ADDED;
				return (result); // this exits the for loop - create a new inlet only once
			}
		}  // if there is no place... shoganai. No update was possible.
	}
	return (result); // can be NO_ACTION
}

InletArray::ResultRequestConnection InletArray::requestMoveDeleteInlet(const NodeID & _toNode) {
	ResultRequestConnection result;

	for (uint8_t i = 0; i < numInletsUsed; i++) { // numInletsUsed or we can call .size()

		//1) Check if the node ID is in the list of links (in SOME inlet), and if it is, check if we can move it.
		Port* auxPort = arrayInlets[i].isPortActiveFromNode(_toNode); //Port* Inlet::isPortActiveFromNode(const NodeID& _toNode) {

		if (auxPort != NULL) {
			if (auxPort->stateLink() == Link::LOOSE) { // then move it (if possible) to the next INLET:
				// Find an available place on any Inlet (NOT starting from the current one - if there is no place, it will be set to here again)
				for (uint8_t k = 1; k < numInletsUsed ; k++) { // not until numInletsUsed + 1 because in this case it means no action is needed
					uint8_t next = (i + k) % numInletsUsed;
					if (arrayInlets[next].hasSpace()) {
						// First, disconnect from the inlet i:
						arrayInlets[i].listPorts.disconnect(auxPort->myLink);// disconnect the port to _toNode on inlet i
						// Then attach _toNode to index "next":
						Link newLink(_toNode, millis()); // note that we UPDATE the time creation
						arrayInlets[next].setNewLink(newLink); // all INLET data is re-saved on EEPROM

						// Save details of the modified inlet connection to send to the sniffer:
						result.myAction=LINK_MOVED;
						result.inletIndex1=i;
						result.inletIndex2=next;
						return (result);
					}
				} // If we get here, it means that there is no available space: do nothing:
				result.myAction=NO_ACTION;
				return (result);
			} else {  // if it was fixed instead, it needs to be deleted:
				arrayInlets[i].listPorts.disconnect(auxPort->myLink);// disconnect the port
				result.inletIndex1=i;
				result.myAction=LINK_DELETED;
				return (result);
			}
			// break; // we treat only the first match (auxPort!=NULL), This break exit the loop over the inlets
		}
	} // end loop inlets.
	// If we reached here it means no link could be moved/deleted:
	result.myAction=NO_ACTION;
	return (result);
}

int8_t InletArray::requestForUpdate(const Message& _newMessage) { // -1 means no update was possible; otherwise the return value is the updated inlet
	/*
	This means that the RF message was NOT generated by an IR request for connection, which in normal circumstances implies
	there IS one link with the same nodeID (and "fixed", i.e., old enough. Note that if this is NOT old enough, then there is a problem, it cannot be)
	We need to update the data in all of thems, but for the time being we will limit to only one inlet for the same outlet
	*/
	for (uint8_t i = 0; i < numInletsUsed; i++) if (arrayInlets[i].update(_newMessage)) return(i); // for the time being, we assume only one inlet from the same outlet
	return (-1);
}

int8_t InletArray::isConnected(NodeID _toThisNode) {
	for (uint8_t i = 0; i < numInletsUsed; i++) if (arrayInlets[i].isConnected(_toThisNode)) return(i); // for the being, we assume only one inlet from the same outlet...
	return (-1);
}

// Synch NEW data on ALL inlets?
bool InletArray::isSynchData() {
  // First, check that we have simultaneoulsy NEW data on all inlets (NOTE: there may be other interesting "synch" modes, like PARTIAL synch data...)
  boolean allNewData=this->isAllBang();

  // Then, compute the largest time interval between the update time stamps:
	bool synchDataNew = false;
	unsigned long earliest = arrayInlets[0].getAge(), latest = earliest;
	for (uint8_t i = 0; i < numInletsUsed ; i++) {
		unsigned long timestamp = arrayInlets[i].getAge();
		if (earliest > timestamp) earliest = timestamp;
		if (latest < timestamp) latest = timestamp;
	}
	if ((latest - earliest) < synchInterval) synchDataNew = true;

	return (allNewData && synchDataNew);
}
