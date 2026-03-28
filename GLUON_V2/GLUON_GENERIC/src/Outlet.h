#ifndef _Outlet_h_
#define _Outlet_h_

#include "Arduino.h"
#include "utils.h"
#include "nonVolatileData.h"
#include "moduleRF.h"

using namespace std;
//using std::list;

#define MAX_NUM_OUTLETS 1

class Outlet {

public:
	Outlet(): index(0), newDataFlag(false) {listPorts.disconnectAll();} // use default copy constructor of struct
	Outlet(uint8_t _index): index(_index), newDataFlag(false) {listPorts.disconnectAll();} // use default copy constructor of struct

	uint8_t getIndex() {return (index);}
	void setIndex(uint8_t _index) {index = _index;}

	uint8_t getNumPorts() {return (listPorts.maxSize());} // total number of links (this is, MAX_FAN_IN)
	uint8_t getNumActivePorts() {return (listPorts.getNumActivePorts());} // num of active links

	bool setNewLink(const Link& _link);

	Port* isPortActiveFromNode(const NodeID& _toNode);

	bool isFullCapacity() {return (listPorts.isFull());}
	bool hasSpace() {return (!listPorts.isFull());}

	bool isNewData() {return (newDataFlag);}
	void setNewDataFlag(bool flg) {newDataFlag = flg;}
	void setNewData(Data& _data) {data = _data; newDataFlag = true;}

	Data getData() { return (data); } // if not using "checkOutData()" method, do not forget to set data as "used" (this will depend on how it is used... )
	Data checkOutData() {newDataFlag = false; return(data);}

	void clearAllLinks();

	// better to make this private, but for the time being I need to compile quickly and the nonVolatileMemory "saveOutlet" function needs to access link data
	ArrayPort <MAX_FAN_OUT> listPorts; // MAX_FAN_OUT would be 4 for the time being...

private:

	uint8_t index;
	Data data;
	bool newDataFlag;

};

// =======================================================================

class OutletArray {
public:

	OutletArray() : numOutletsUsed(0) {}

	uint8_t size() {return (numOutletsUsed);}

	void clear() {numOutletsUsed = 0;}

	bool addNewOutlet(const Outlet & _newOutlet); // only when building the gluon (note: no delete outlet method - this won't happen during normal operation)

	bool isFullCapacity() {
		bool isfull=true;
		for (uint8_t i = 0; i < numOutletsUsed; i++) isfull &= arrayOutlets[i].isFullCapacity();
		return (isfull);
	}

	// Easy access to outlets:
	Outlet& operator[] (const uint8_t  _index);
	const Outlet& operator[] (const uint8_t  _index) const;

	void disconnectEverything();

	void setNewData(Data& _data) {
		for (uint8_t i = 0; i < numOutletsUsed; i++) arrayOutlets[i].setNewData(_data); // this also sets the flag of the outlet data as new!
	}

	void setAllOutletsDataUsed() {
		for (uint8_t i = 0; i < numOutletsUsed; i++) {arrayOutlets[i].setNewDataFlag(false); }
	} // to use after processing the Band or the Synch check


private:
	Outlet arrayOutlets[MAX_NUM_OUTLETS];
	uint8_t numOutletsUsed;
};


#endif
