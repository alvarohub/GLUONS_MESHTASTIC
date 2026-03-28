#ifndef _ledIndicators_h
#define _ledIndicators_h

/*
	Class to control STATUS LEDS (show the state of the inlets/outlets, etc.), using a 4-LED NEOPIXEL STRIP
*/

#include "Arduino.h"
#include "Inlet.h"
#include "Outlet.h"

#include "stripNeoLedsSimple.h"

class LedIndicator {
public:
	LedIndicator();
	void init(InletArray* inPtr, OutletArray* outPtr, bool _mode);

	// Enable/disable updating led indicator:
	void enable() {enabled=true;}
	void disable() {enabled=false;}

	// Setting led buffer but NOT displaying yet:
	void setLed(uint8_t _index, const cRGB& color);

	// Double buffer copy and swap:
	// void dupBuffer() {myNeoStrip.dupBuffer();}
	// void swapBuffer() {myNeoStrip.swapBuffer();}

	// Actual display (sending data to the strip, IF enabled):
	void display() {
		if (!enabled) return; // <-- only place where I will test for enable/disable
		myNeoStrip.display();
	}

	// functions that also *automatically* update the display:
    void allColor(const cRGB& _color);
    void allOff();
	void allOn();
    void oscillate(const cRGB& _color);
    void blinkAll(const cRGB& _color);
	void showSequence();
	void blinkLed(uint8_t _index);
	void blinkInlet(uint8_t _index);
	void blinkOutlet(uint8_t _index);
	void showInletsMatchID(InletArray& _inletArray, NodeID _toThisID);
	void update(); // updates ALL led indicators using the populated inlet and outlet arrays

private:

	NEO_STRIP_SIMPLE myNeoStrip;

	// We will use POINTERS to point to the relevant arrays (of inlets, outlets, sensors and actuators)
	InletArray* inletArrayPtr;
	OutletArray* outletArrayPtr;

	void updateInletLeds( Inlet& _inlet);
	void updateOutletLeds( Outlet& _outlet);
	// overloading for the whole array:
	void updateInletLeds();
	void updateOutletLeds();

	// The color palette corresponding to the three inlets + outlet;
	const cRGB colorPalette[4] = {red, green, blue, white};

	bool enabled;
};

// Pre-instantiated object with FOUR Leds.
extern LedIndicator myLedIndicator; // disabled at init. It MUST be activated by hand if we want to use it.

#endif
