#include "sensorRotaryEncoder.h"
#include <PCInterrupt.h>
#include <util/atomic.h>

//volatile int32_t SensorRotaryEncoder::position = 0;
//volatile uint8_t SensorRotaryEncoder::state=0;
//static uint8_t SensorRotaryEncoder::pinA = ENC_PINA, SensorRotaryEncoder::pinB = ENC_PINB;

void SensorRotaryEncoder::init(bool firstTimeBuild) {
	setToleranceValue(1);   // Condition value not needed, only tolerance (we will read from 0 to 11, and we want to detect a change every step - tolerance value is inclusive). Also, no need to save on EEPROM (learning is disabled)

	setDetectionMode(ON_SIMPLE_CHANGE);
	//disableLearning();    // disabled by default in the base sensor (this way, the tolerance is fixed)

	pinMode(pinA, INPUT_PULLUP);
	pinMode(pinB, INPUT_PULLUP);

	reset();
	enable(); // enabled by default

}

void SensorRotaryEncoder::enable() {
	BaseSensor::enable();
	PCattachInterrupt(pinA, SensorRotaryEncoder::tickA, CHANGE);
	PCattachInterrupt(pinB, SensorRotaryEncoder::tickB, CHANGE);
}
void SensorRotaryEncoder::disable() {
	BaseSensor::disable();
	PCdetachInterrupt(pinA);
	PCdetachInterrupt(pinB);
}

void SensorRotaryEncoder::reset() {
	ATOMIC_BLOCK (ATOMIC_RESTORESTATE) {
	position=0;
	// also init state:
	state=0;
	if (digitalRead(pinA)) state |= 4;
	if (digitalRead(pinB)) state |= 8;
	}
}

// The ISR routines:
void SensorRotaryEncoder::tickA(void* arg) {
	//bool auxStateA = digitalRead (this_->pinA);
	updatePosition(arg);
}

void SensorRotaryEncoder::tickB(void* arg) {
	//bool auxStateB = digitalRead (this_->pinB);
	updatePosition(arg);
}

void SensorRotaryEncoder::updatePosition(void* arg) {
	SensorRotaryEncoder* this_ = (SensorRotaryEncoder*) arg;
	uint8_t s = this_->state & 3;
	if (digitalRead(this_->pinA)) s |= 4;
	if (digitalRead(this_->pinB)) s |= 8;
	switch (s) {
		case 0: case 5: case 10: case 15:
			break;
		case 1: case 7: case 8: case 14:
			this_->position++; break;
		case 2: case 4: case 11: case 13:
			this_->position--; break;
		case 3: case 12:
			this_->position += 2; break;
		default:
			this_->position -= 2; break;
	}
	this_->state = (s >> 2);
}

uint16_t SensorRotaryEncoder::readAnalogOutput() {
	uint16_t readout;
	ATOMIC_BLOCK (ATOMIC_RESTORESTATE) {//let's avoid being interrupted during the reading of the ISR variables
		readout = position;
	}
	// TODO: do more treatment if necessary (for instance, real angle)
	return(readout);
}


//                           _______         _______
//               Pin1 ______|       |_______|       |______ Pin1
// negative <---         _______         _______         __      --> positive
//               Pin2 __|       |_______|       |_______|   Pin2

		//	new		new		old		old
		//	pin2	pin1	pin2	pin1		Result
		//	----	----	----	----		------
		//		0		0		0		0		no movement
		//		0		0		0		1		+1
		//		0		0		1		0		-1
		//		0		0		1		1		+2  (assume pin1 edges only)
		//		0		1		0		0		-1
		//		0		1		0		1		no movement
		//		0		1		1		0		-2  (assume pin1 edges only)
		//		0		1		1		1		+1
		//		1		0		0		0		+1
		//		1		0		0		1		-2  (assume pin1 edges only)
		//		1		0		1		0		no movement
		//		1		0		1		1		-1
		//		1		1		0		0		+2  (assume pin1 edges only)
		//		1		1		0		1		-1
		//		1		1		1		0		+1
		//		1		1		1		1		no movement
