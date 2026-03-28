//
//  nonVolatileData.cpp
//  gluon
//
//  Created by Alvaro Cassinelli on 9/15/15.
//  Copyright (c) 2015 Alvaro Cassinelli. All rights reserved.
//

#include "nonVolatileData.h"

// Pre-instantiated cross-file global variable
NonVolatileMemory myMemory;

Inlet NonVolatileMemory::getInlet(uint8_t _index)
{
	Inlet newInlet(_index);
	uint16_t address = getAddressInlet(_index); // addressesInlet[_index];
	uint8_t numActiveLinks = EEPROM.read(address++);

	for (uint8_t i = 0; i < numActiveLinks; i++, address++) {
		Link newLink(EEPROM.read(address));     // the read data is the NodeId; note that at creation, the Link is ACTIVE because we only saved ACTIVE links to EEPROM.
		// and the time of creation is 0, meaning it will certainly be FIXED (but we can change that here)
		newInlet.listPorts.connect(newLink);    // note: I don't use newInlet.setNewLink(newLink) because it would RESAVE into EEPROM
		// we could also do:
		// newInlet.listPorts[i].myLink=newLink;
		// newInlet.listPorts[i].setActive();
		// newInlet.listPorts.numActivePorts++;
	}
	return (newInlet);
}

void NonVolatileMemory::saveInlet(Inlet& _inlet)
{
	// we will use the index as the address in memory:
	uint16_t address = getAddressInlet(_inlet.getIndex()); // the content of this address is the number of active links of the inlet

	// The only thing that needs to be saved in the EEPROM (for the time being) is the number of ACTIVE links, and their IDs (we ommit time of creation)
	EEPROM.update(address++, _inlet.getNumActivePorts());   // attention postfix not prefix
	for (uint8_t i = 0; i < _inlet.getNumPorts(); i++) {    // attention: we need to scan the WHOLE array for the  _inlet.getNumActiveLinks() links:
		if (_inlet.listPorts[i].isActive()) EEPROM.update(address++, _inlet.listPorts[i].myLink.nodeId);
	}
}

void NonVolatileMemory::clearInletLinks(uint8_t _index)
{
	uint16_t address = getAddressInlet(_index);

	EEPROM.update(address, 0);
}

Outlet NonVolatileMemory::getOutlet(uint8_t _index)
{
	Outlet newOutlet(_index);
	uint16_t address = getAddressOutlet(_index); // addressesOutlet[_index];
	uint8_t numActiveLinks = EEPROM.read(address++);

	for (uint8_t i = 0; i < numActiveLinks; i++, address++) {
		Link newLink(EEPROM.read(address));     // the read data is the NodeId; note that at creation, the Link is ACTIVE,
		newOutlet.listPorts.connect(newLink);   // note: I don't use newOutlet.setNewLink(newLink) because it would RESAVE into EEPROM
	}
	return (newOutlet);
}

void NonVolatileMemory::saveOutlet(Outlet & _outlet)
{
	uint16_t address = getAddressOutlet(_outlet.getIndex());        // addressesOutlet[_index];

	EEPROM.update(address++, _outlet.getNumActivePorts());          // attention postfix not prefix
	for (uint8_t i = 0; i < _outlet.getNumPorts(); i++ ) {          // attention: we need to scan the WHOLE array for the  _outlet.getNumActiveLinks() links:
		if (_outlet.listPorts[i].isActive()) EEPROM.update(address++, _outlet.listPorts[i].myLink.nodeId);
	}
}

void NonVolatileMemory::clearOutletLinks(uint8_t _index)
{
	uint16_t address = getAddressOutlet(_index);

	EEPROM.update(address, 0);
}

// =============================================== Operations on the arrays of inlets/outlets ===============================================
void NonVolatileMemory::saveInletArray(InletArray& _inletArray)
{
	// Save Inlet data to EEPROM, including the ACTIVE links in the inlets:
	myMemory.setNumInlets(_inletArray.size());
	for (uint8_t i = 0; i < _inletArray.size(); i++) myMemory.saveInlet(_inletArray[i]); // the index map the address in EEPROM memory
}

void NonVolatileMemory::loadInletArray(InletArray& _inletArray)
{
	// Populate the inlet array from EEPROM memory:
	_inletArray.clear();                            // just sets numInletsUsed to zero
	for (uint8_t i = 0; i < myMemory.getNumInlets(); i++) {
		Inlet newInlet(myMemory.getInlet(i));   // the method getInlet returns an Inlet object.
		//Here default copy constructor is used, which is ok because there is no dynamically allocated arrays.
		_inletArray.addNewInlet(newInlet);
	}
}

void NonVolatileMemory::loadOutletArray(OutletArray& _outletArray)
{
	_outletArray.clear();
	// LOG_PRINTLN("Adding from eeprom nb outlets: "); LOG_PRINTLN(myMemory.getNumOutlets());
	for (uint8_t i = 0; i < myMemory.getNumOutlets(); i++) {
		Outlet newOutlet(myMemory.getOutlet(i)); // the method getOutlet returns an Outlet object. Here default copy constructor is used !
		_outletArray.addNewOutlet(newOutlet);
	}
	//numOutletsUsed = myMemory.getNumOutlets(); // no need because the index grows as we add things
}

void NonVolatileMemory::saveOutletArray(OutletArray& _outletArray)
{
	// Save Inlet data to EEPROM, including the ACTIVE links in the inlets:
	myMemory.setNumOutlets(_outletArray.size());
	for (uint8_t i = 0; i < _outletArray.size(); i++) myMemory.saveOutlet(_outletArray[i]); // the index map the address in EEPROM memory
}
