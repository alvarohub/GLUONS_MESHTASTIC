
#include "ledIndicators.h"

// Pre-instantiated cross-file global variable (note that we will need to init it with the inlet and outlet arrays)
LedIndicator myLedIndicator;

LedIndicator::LedIndicator(): enabled(false) {}

void LedIndicator::init(InletArray* inPtr, OutletArray* outPtr, bool _mode) {
	enabled = _mode;
	inletArrayPtr = inPtr; // the number of inlets is given by inletArrayPtr->size()
	outletArrayPtr = outPtr; // the number of outlets is given by outletArrayPtr->size()

	// 1) Init the led strip
	// numInletLEDs = inletArrayPtr->size();
	// numOutletLEDs= outletArrayPtr->size();

	//myNeoStrip.init(numInletLEDs + numOutletLEDs, PIN_DI_NEO);
	//NOTE: using simplified library with only 4 leds I don't need to do any initialization of the Neostrip.

	// 2) at the start, we can either show the (EEPROM saved) connections, or do something else to
	// indicate the module has been powered up and is working correctly:
	// We can either do allOff(), display() or showSequence():
	allOff();
}

void LedIndicator::setLed(uint8_t _index, const cRGB& _color) {
	myNeoStrip.set_crgb_at(_index, _color);
	display();
}

void LedIndicator::allColor(const cRGB& _color) {
	myNeoStrip.setAll(_color);
	display();
}

void LedIndicator::allOff() {
	allColor(black);
}

void LedIndicator::allOn() {
	allColor(white.dim(60));
}

void LedIndicator::oscillate(const cRGB& _color) {
    uint8_t percent = 50.0*cos(2.0*PI*millis()/1000)+50;
	allColor(_color.dim(percent));
}

void LedIndicator::blinkAll(const cRGB& _color) {
	for (uint8_t i = 0; i < 4; i++) {
        allColor(_color.dim(50));
        delay(100);
        allColor(black);
        delay(100);
    }
}


void LedIndicator::updateInletLeds()  {
	for (uint8_t i = 0; i < inletArrayPtr->size(); i++) updateInletLeds((*inletArrayPtr)[i]);
}

void LedIndicator::updateInletLeds(Inlet& _inlet)  {
	// Show differently Data and only Connectivity:
	uint8_t inletIndex = _inlet.getIndex();
	if (_inlet.getNumActivePorts() > 0) {
		if (_inlet.isNewData()) {
			//setLed(inletIndex, colorPalette[inletIndex].dim(80));
			setLed(inletIndex, white.dim(80)); delay(15);
        }
		else
			setLed(inletIndex, colorPalette[inletIndex].dim(50));
	}
	else
		setLed(inletIndex, black);
}

void LedIndicator::updateOutletLeds()  {
	for (uint8_t i = 0; i < outletArrayPtr->size(); i++) updateOutletLeds((*outletArrayPtr)[i]);
}

void LedIndicator::updateOutletLeds(Outlet& _outlet)  {
	uint8_t outletIndex = _outlet.getIndex() + inletArrayPtr->size();
	if (_outlet.getNumActivePorts() > 0) {
		if (_outlet.isNewData()) {
			setLed(outletIndex, colorPalette[outletIndex].dim(80)); delay(15);
        }
		else
			setLed(outletIndex, colorPalette[outletIndex].dim(50));
	}
	else
		setLed(outletIndex, black);
}

void LedIndicator::update()  {
	// This updates ALL led indicators to the current state of the inlet/outlets:
	updateInletLeds();
	updateOutletLeds();
}

void LedIndicator::showSequence() {
		allOff();
		//for (uint8_t j=0; j<2; j++) {
			//for (uint8_t i = 0; i < numInletLEDs + numOutletLEDs; i++) {
			for (uint8_t i = 0; i < NUM_LEDS_STRIP; i++) {

				setLed(i, colorPalette[i].dim(50));
				delay(200);

				setLed(i, black);
				delay(200);
			}
	//	}
	// update(); // this reverts to the current state of input/output show
}

void LedIndicator::blinkInlet(uint8_t _index) {
	blinkLed(_index);
}

void LedIndicator::blinkOutlet(uint8_t _index) {
	blinkLed(_index + inletArrayPtr->size()); // add offset in the strip
}

void LedIndicator::blinkLed(uint8_t _index) {
	for (uint8_t i = 0; i < 2; i++) {
        setLed(_index, black);//white.dim(50));
        delay(100);
		setLed(_index, colorPalette[_index].dim(60)) ;//black);
		delay(100);
	}
	//update(); // refresh display using data from the inlet/outlet array display.
}


void LedIndicator::showInletsMatchID(InletArray& _inletArray, NodeID _toThisID) {
	int indexInletConnected = _inletArray.isConnected(_toThisID); // returns -1 if there is NO inlet connected to the ID,
	// NOTE: when the method is called when THIS node receives a check pull request, there SHOULD be always a match)
	if (indexInletConnected>=0) {
		blinkInlet(indexInletConnected);
	}
}


/* MISC (for the future, using double buffer)

void LedIndicator::showSequence() {
		myNeoStrip.swapBuffer(); 	// save the current state (swap buffers in the NEO_STRIP)
		allOff();
		for (uint8_t j=0; j<2; j++) {
			//for (uint8_t i = 0; i < numInletLEDs + numOutletLEDs; i++) {
			for (uint8_t i = 0; i < NUM_LEDS_STRIP; i++) {
				setLed(i, colorPalette[i].dim(50));
				delay(200);
				setLed(i, black.dim(50));
				delay(200);
			}
		}
		myNeoStrip.swapBuffer(); // retrieve the old state
		myNeoStrip.display()
}

Blink unique led and back to normal using double buffer:
void LedIndicator::blinkLed(uint8_t _index) {
	dupBuffer();
	swapBuffer();
	for (uint8_t i = 0; i < 3; i++) {
		setLed(_index, white.dim(50));
		display();
		delay(100);
		setLed(_index, colorPalette[_index].dim(50)) ;//black);
		display();
		delay(100);
	}
	swapBuffer(); // get back to current state of input/outputs indication
	display();
}
*/
