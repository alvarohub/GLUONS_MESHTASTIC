#include "Outlet.h"

Port* Outlet::isPortActiveFromNode(const NodeID& _toNode)       // we retrieve only the FIRST match - we assume a max of one connection per inlet to another node!
{
	for (uint8_t j = 0; j < listPorts.maxSize(); j++) {     //  scan ALL the ports of of this inlet
		if ( listPorts[j].isActive() && listPorts[j].matchNode(_toNode) ) return (&listPorts[j]);
	}
	return (NULL); // otherwise return NULL pointer
}

bool Outlet::setNewLink(const Link& _link)
{
	if (listPorts.connect(_link)) {// the method check both space availability and no conflict (another similar link)

		// show momentary blink of the Inlet LED for the newly added link?
		// blinkLED((ledInde++)%MAX_NUM_INLETS);

		// SAVE TO EEPROM (we will resave all the links for THIS inlet - in the future we can have a less brutal method but it may imply a complex list implementation
		// on the EEPROMN MEMORY.. better to use the Array)
		//	myMemory.saveOutlet(*this); //NOTE: the index of the INLET maps the address in memory.

		return (true);
	}else
		return (false);
}

void Outlet::clearAllLinks()
{
	listPorts.disconnectAll();
	myMemory.clearOutletLinks(index);
}


// ==============================================================================================

bool OutletArray::addNewOutlet(const Outlet & _newOutlet)
{
	if (numOutletsUsed < MAX_NUM_OUTLETS) {
		arrayOutlets[numOutletsUsed++] = _newOutlet;
		return (true);
	} else
		return (false);
}

Outlet& OutletArray::operator[](const uint8_t _index)     // note: we return a reference (and not a const reference)
{
	if (_index < numOutletsUsed)
		return arrayOutlets[_index];
	else
		return arrayOutlets[numOutletsUsed - 1]; // let's return the last one
}

const Outlet& OutletArray::operator[](const uint8_t _index) const
{
	if (_index < numOutletsUsed)
		return arrayOutlets[_index];
	else
		return arrayOutlets[numOutletsUsed - 1]; // let's return the last one
}

// Disconnect all links from all inlets:
void OutletArray::disconnectEverything()   // this could mean setting these to IDLE, but it is simpler to set the number of active links to 0
{
	for (uint8_t i = 0; i < numOutletsUsed; i++) arrayOutlets[i].clearAllLinks();
	//LOG_PRINTLN(F("All Outlet links cleared."));
}
