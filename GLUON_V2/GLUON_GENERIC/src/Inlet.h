#ifndef _Inlet_h
#define _Inlet_h

#include "Arduino.h"
#include "utils.h"
#include "nonVolatileData.h"

using namespace std;
//using std::list;

#define MAX_NUM_INLETS 3 // just to secure against memory overload on arduino
#define DEFAULT_SYNCH_INTERVAL 1000 // in ms


class Inlet {

public:
	Inlet(): index(0), newDataFlag(false), timeUpdated(0) {listPorts.disconnectAll();}
	Inlet(uint8_t _index): index(_index), newDataFlag(false), timeUpdated(0) {listPorts.disconnectAll();}
	//~Inlet(); // default constructor is ok, since I am not doing dynamic allocation.

	// index of the outlet:
	uint8_t getIndex() {return (index);}
	void setIndex(uint8_t _index) {index = _index;}

	uint8_t getNumPorts() {return (listPorts.maxSize());} // total number of links (this is, MAX_FAN_IN)
	uint8_t getNumActivePorts() {return (listPorts.getNumActivePorts());} // num of active links

	bool isFullCapacity() {return (listPorts.isFull());}
	bool hasSpace() {return (!listPorts.isFull());}

	Port* isPortActiveFromNode(const NodeID& _toNode);

	bool setNewLink(const Link& _link);
	bool setNewLinkFromMessage(const Message& _msgLink);

	bool isConnected(NodeID _toThisNode);
	bool update(const Message& _msgLink);

	// Checking if there is new data on this inlet:
	bool isNewData() {return (newDataFlag);}
	void setNewDataFlag(bool flg) {newDataFlag = flg;}
	void setNewData(const Data& _data) {data = _data; newDataFlag = true;}

	// Getting the data on this inlet:
	bool checkEvent() {return(data.event);}
	Data getData() { return (data); } // do not forget to set data as "used" (this will depend on how it is used... )
	Data checkOutData() {return(data); newDataFlag = false;}

	unsigned long getAge() {return (timeUpdated);}

	void clearAllLinks();

	ArrayPort <MAX_FAN_IN> listPorts; // MAX_FAN_IN would be 1 for the time being...

private:

	uint8_t index; // index of the inlet

	Data data;
	bool newDataFlag;
	unsigned long timeUpdated;

};


class InletArray {
public:
	InletArray():synchInterval(DEFAULT_SYNCH_INTERVAL), numInletsUsed(0) {};

	uint8_t size() {return (numInletsUsed);}

	void clear() {numInletsUsed = 0;}

	bool addNewInlet(const Inlet&); // only when building the gluon (note: no delete inlet method - this won't happen during normal operation)

	bool isFullCapacity() {bool isfull=true;  for (uint8_t i = 0; i < numInletsUsed; i++) isfull &= arrayInlets[i].isFullCapacity(); return (isfull); }

	// overloaded operators for getter and setter using simple [] notation:
	const Inlet& operator[] (const uint8_t  _index) const;
	Inlet& operator[] (const uint8_t  _index);

	void disconnectEverything();

	enum Action {NO_ACTION=0, LINK_DELETED, LINK_ADDED, LINK_MOVED};
	struct ResultRequestConnection {
		Action myAction;
		uint8_t inletIndex1, inletIndex2; // details on the changed inlet (deleted, moved, created... )
	};

	ResultRequestConnection requestMoveDeleteInlet(const NodeID & _toNode);
	ResultRequestConnection requestForConnexion(const NodeID & _toNode);

	//In the following two methods, -1 means we could not find a corresponding inlet,
	// otherwise it returs the index (we assume only one):
	int8_t isConnected(NodeID _toThisNode);
	int8_t requestForUpdate(const Message& _newMessage);

	// Getting the data on the inlet:
	Data getData(uint8_t _index) { return (arrayInlets[_index].getData()); }

	bool isSynchData();
	void setSynchInterval(unsigned long _synchInterval) {synchInterval = _synchInterval;} // tolerance of the "simultanety"
	bool checkEvent() { bool event=false; for (uint8_t i = 0; i < numInletsUsed; i++) event |= arrayInlets[i].checkEvent(); return(event); }

	// Perhaps the following methods should remain only in the BaseLogic class and addressed using [i]...
	//bool checkEvent(uint8_t _index) {return(arrayInlets[_index].checkEvent());}
	bool isBangFirstInlet() {return (arrayInlets[0].isNewData());}
	bool isBang(uint8_t index) {return (arrayInlets[index].isNewData());}
	bool isSomeBang()  {bool isbang = false; for (uint8_t i = 0; i < numInletsUsed; i++) isbang |= arrayInlets[i].isNewData(); return (isbang); }
  	bool isAllBang()   {bool isbang = true; for (uint8_t i = 0; i < numInletsUsed; i++) isbang &= arrayInlets[i].isNewData(); return (isbang); }

	void setAllInletsDataUsed() {for (uint8_t i = 0; i < numInletsUsed; i++) arrayInlets[i].setNewDataFlag(false); } // to use after processing the Bang or the Synch check

private:
	// vector<Inlet> arrayInlets; // no need to use a list: after gluon is built, the array does not change
	Inlet arrayInlets[MAX_NUM_INLETS];
	unsigned long synchInterval;
	uint8_t numInletsUsed;

};

#endif
