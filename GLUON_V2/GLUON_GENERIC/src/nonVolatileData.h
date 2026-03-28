#ifndef _nonVolatileData_h
#define _nonVolatileData_h

#include "Arduino.h"
#include <EEPROM.h> // Note: 1024 bytes on the ATmega328

#include "utils.h"
#include "Inlet.h"
#include "Outlet.h"

/*
*    This class is a wrapper for the non volatile NonVolatileMemory functions.
*    I have several options: use EEPROM, ATMEGA FLASH (PROGMEM), or the 4MB FLASH on the Moteino.
*	 For the time being, I will implement this using EEPROM, but the IMPLEMENTATION of this class could be different. In particular, it
*    would be good to use the external FLASH to save/load ALL the data (including the TRANSIENT data on the inlets/outlets), this
*    way we can save SRAM space (however the external flash is for wireless programming)
*
*   NOTES:
*    - I will make NonVolatileMemory object a GLOBAL (cross file) because it is used by most modules in different ways.
*	 - saving of inlets and outlets is done in the NodeClass implementation, and NOT on the Inlet/Outlet classes, the idea being to
*	have a clear separation between the routines that use the state variables in dynamic memory, and the occurrence of "snapshots" taken on the EEPROM.
*/

// ================= DEFINITION OF THE TABLE ADDRESSES ==========================
const uint8_t ADDRESS_MYNODE = 0;

// For the time being, only SCALAR conditions, and a max of TWO SENSORS (note: switches do not need to save/load anything)
const uint8_t ADDRESS_EVENT_CONDITIONVALUE_LSB_A = 1;
const uint8_t ADDRESS_EVENT_CONDITIONVALUE_MSB_A = 2;
const uint8_t ADDRESS_EVENT_TOLERANCEVALUE_LSB_A = 3;
const uint8_t ADDRESS_EVENT_TOLERANCEVALUE_MSB_A = 4;

const uint8_t ADDRESS_EVENT_CONDITIONVALUE_LSB_B = 5;
const uint8_t ADDRESS_EVENT_CONDITIONVALUE_MSB_B = 6;
const uint8_t ADDRESS_EVENT_TOLERANCEVALUE_LSB_B = 7;
const uint8_t ADDRESS_EVENT_TOLERANCEVALUE_MSB_B = 8;

const uint8_t ADDRESS_UPDATE_MODE = 9; // LOGIC update mode is a BYTE (bit flags set - non exclusive - updating methods)

// address to store number of intlet/outlets (maybe not used - instead this will be fixed during the construction of the gluon)
const uint8_t ADDRESS_NUM_INLETS = 10;
const uint8_t ADDRESS_NUM_OUTLETS = 11;

// We will fix the map for the memory, but the actual number of inlets or outlets will be set while "building" the gluon
// The total memory used assuming 6 inlets, 6 outlets and fan in/out of 8 is therefore:
//  1+2+2*6*(8+1) = 111 bytes (Atmega328 has up to 1024 bytes of EEPROM)
const uint8_t ADDRESS_NUM_INPUTS_INLET1 = 12;
// ....  follow MAX_FAN_IN nodeIDs, exactly the content of [ADDRESS_NUM_INPUTS_INLET1] links...
// I provide here space for up to THREE inlets and ONE outlet.
// Each one will be characterised by FIRST the number of ports, and then their addresses. Only the ACTIVE links are saved.
const uint8_t ADDRESS_NUM_INPUTS_INLET2 = (ADDRESS_NUM_INPUTS_INLET1 + MAX_FAN_IN + 1);
const uint8_t ADDRESS_NUM_INPUTS_INLET3 = (ADDRESS_NUM_INPUTS_INLET2 + MAX_FAN_IN + 1);
// const uint8_t ADDRESS_NUM_INPUTS_INLET4 = (ADDRESS_NUM_INPUTS_INLET3 + MAX_FAN_IN + 1);
// const uint8_t ADDRESS_NUM_INPUTS_INLET5 = (ADDRESS_NUM_INPUTS_INLET4 + MAX_FAN_IN + 1);
// const uint8_t ADDRESS_NUM_INPUTS_INLET6 = (ADDRESS_NUM_INPUTS_INLET5 + MAX_FAN_IN + 1);

const uint8_t ADDRESS_NUM_OUTPUTS_OUTLET1 = (ADDRESS_NUM_INPUTS_INLET3 + MAX_FAN_IN + 1);
// const uint8_t ADDRESS_NUM_OUTPUTS_OUTLET2 = (ADDRESS_NUM_OUTPUTS_OUTLET1 + MAX_FAN_OUT + 1);
// const uint8_t ADDRESS_NUM_OUTPUTS_OUTLET3 = (ADDRESS_NUM_OUTPUTS_OUTLET2 + MAX_FAN_OUT + 1);
// const uint8_t ADDRESS_NUM_OUTPUTS_OUTLET4 = (ADDRESS_NUM_OUTPUTS_OUTLET3 + MAX_FAN_OUT + 1);
// const uint8_t ADDRESS_NUM_OUTPUTS_OUTLET5 = (ADDRESS_NUM_OUTPUTS_OUTLET4 + MAX_FAN_OUT + 1);
// const uint8_t ADDRESS_NUM_OUTPUTS_OUTLET6 = (ADDRESS_NUM_OUTPUTS_OUTLET5 + MAX_FAN_OUT + 1);

// OTHER GENERIC VALUES for the logic function (ex: beats-per-minute on the METRO gluon, etc)
const uint8_t ADDRESS_DATA0 = (ADDRESS_NUM_OUTPUTS_OUTLET1 + MAX_FAN_OUT + 1);

class Inlet;
class Outlet;
class InletArray;
class OutletArray;

class NonVolatileMemory {

public:
//softPWM::softPWM() {}
//NonVolatileMemory


// Inlets:
  	void loadInletArray(InletArray& _inletArray);
  	void saveInletArray(InletArray& _inletArray);
	Inlet getInlet(uint8_t index);
	uint8_t getNumInlets() {return (EEPROM.read(ADDRESS_NUM_INLETS));}
	void setNumInlets(uint8_t _numInlets) {EEPROM.update(ADDRESS_NUM_INLETS, _numInlets);}
	void saveInlet(Inlet& _inlet); //I would put "const Inlet&..." but it seems that the EEPROM function inside does not use const and the compiler throws error
	void clearInletLinks(uint8_t _index);

// Outlets:
  	void loadOutletArray(OutletArray& _outletArray);
  	void saveOutletArray(OutletArray& _outletArray);
	Outlet getOutlet(uint8_t index);
	uint8_t getNumOutlets() {return (EEPROM.read(ADDRESS_NUM_OUTLETS));}
	void setNumOutlets(uint8_t _numOutlets) {EEPROM.update(ADDRESS_NUM_OUTLETS, _numOutlets);}
	void saveOutlet(Outlet& _outlet);
	void clearOutletLinks(uint8_t _index);

// Sensor(s):
	uint16_t loadConditionValue(const uint8_t _index) {
		return (((uint16_t)EEPROM.read(getAddressSensor(_index)+1)) << 8) + EEPROM.read(getAddressSensor(_index)); // ATTN! "+" has precedence over << ...
	}
	uint16_t loadToleranceValue(const uint8_t _index) {
		return (((uint16_t)EEPROM.read(getAddressSensor(_index)+3)) << 8) + EEPROM.read(getAddressSensor(_index)+2); // ATTN! "+" has precedence over << ...
	}
	void saveConditionValue(const uint8_t _index, const uint16_t _eventCondValue) {
		EEPROM.update(getAddressSensor(_index),   _eventCondValue & 0xFF); // LSB
		EEPROM.update(getAddressSensor(_index)+1, _eventCondValue >> 8);   // MSB
	}
	void saveToleranceValue(const uint8_t _index, const uint16_t _eventTolValue) {
		EEPROM.update(getAddressSensor(_index)+2, _eventTolValue & 0xFF); // LSB
    	EEPROM.update(getAddressSensor(_index)+3, _eventTolValue >> 8);   // MSB
	}

	void saveUpdateMode(const uint8_t& _updateMode) {EEPROM.update(ADDRESS_UPDATE_MODE, _updateMode);}
	uint8_t loadUpdateMode() {return EEPROM.read(ADDRESS_UPDATE_MODE); }

	// General data table as list of uint16_t (starting from ADDRESS_DATA0)
	uint16_t loadData(const uint8_t _index) {
		return (((uint16_t)EEPROM.read(getAddressData(_index)+1)) << 8) + EEPROM.read(getAddressData(_index));
	}
	void saveData(const uint8_t _index, const uint16_t _data) {
		EEPROM.update(getAddressData(_index),   _data & 0xFF); // LSB
		EEPROM.update(getAddressData(_index)+1, _data >> 8);   // MSB
	}

private:
	uint16_t getAddressSensor(uint8_t _index) {return (ADDRESS_EVENT_CONDITIONVALUE_LSB_A + _index * 4);} // condition + tolerance are SCALAR (LSB and MSB)

	uint16_t getAddressInlet(uint8_t _index) {return (ADDRESS_NUM_INPUTS_INLET1 + _index * (MAX_FAN_IN + 1));} // the +1 is because the fist value is the actual number of active inlets
	uint16_t getAddressOutlet(uint8_t _index) {return (ADDRESS_NUM_OUTPUTS_OUTLET1 + _index * (MAX_FAN_OUT + 1));} // the +1 is because the fist value is the actual number of active outlets

	// Generic data (each entry is 16 bytes)
	uint16_t getAddressData(uint8_t _index) {return (ADDRESS_DATA0 + _index * 2);}
};

// Cross file, preinstantiated object:
extern NonVolatileMemory myMemory;

#endif
