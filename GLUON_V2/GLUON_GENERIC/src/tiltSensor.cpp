#include "tiltSensor.h"
#include <PCInterrupt.h>
#include <util/atomic.h>

// Pre-instantiated cross-file global variable:
TiltSensor myTiltSensor(tiltPin, DEFAULT_NUM_TICKS, DEFAULT_MEASURE_TICK_INTERVAL);

volatile uint8_t TiltSensor::indexTableTimes = 0;
volatile uint32_t TiltSensor::tableTimes[MAX_NUM_TICKS];
volatile uint8_t TiltSensor::times;

void TiltSensor::init(uint8_t _pin, uint8_t _times, uint32_t _interval) {
	pin = _pin;
	pinMode(pin, INPUT_PULLUP);
	times = _times;
	interval = _interval;
	indexTableTimes=0;
	for (uint8_t i=0; i< times; i++) tableTimes[i] = i*interval;
	enable(); // enabled by default...
}

void TiltSensor::enable() {sensorActive = true; PCattachInterrupt(pin, TiltSensor::tickAddTime, RISING);}//FALLING);} //CHANGE);}
void TiltSensor::disable() {sensorActive = false; PCdetachInterrupt(pin);}

// void TiltSensor::resetBuffer() {
// 	for (uint8_t i=0; i< times; i++) tableTimes[i] = i*interval;
// }

// the ISR routine:
// NOTE: perhaps we could do all this WITHOUT an ISR, but an update method instead. This way we also ensure
// that there is no repetition of the "shake event" (as the table continuously fill). The problem with this approach may be
// that the update (in the main NodeClass loop) may be too slow. On the other hand, an ISR is TOO FAST, and then we will
// detect an event because of the bouncing problem!
void TiltSensor::tickAddTime() {
	uint32_t auxTime = millis();
 	if (auxTime-tableTimes[indexTableTimes] > MINIMUM_TIME_BETWEEN_TICKS) {
		indexTableTimes = (indexTableTimes+1)%times;
		tableTimes[indexTableTimes] = auxTime;
	}
}

// bool TiltSensor::checkEvent() {
//     	if (!sensorActive) return(false);
//
// }

bool TiltSensor::checkEvent() {//checkLowLevelEvent() {
	//if (!sensorActive) return(false);

	bool shakeResult = false;
	uint32_t maxdelay;

	// NOTE: I checked using pin high/low that the main loop that calls this function does it every 1.3ms (700Hz)
	// digitalWrite(A0,HIGH);
	// digitalWrite(A0,LOW);

   ATOMIC_BLOCK (ATOMIC_RESTORESTATE) {//let's avoid being interrupted during the reading of the ISR variables

	// NOTE: the newest input in the table is in tableTimes[indexTableTimes], and the oldest is in tableTimes[(indexTableTimes+1)%times]
		 maxdelay = tableTimes[indexTableTimes] - tableTimes[(indexTableTimes+1)%times];

		if ( maxdelay < interval ) { // there is an event here! (shake)
			// we need to  avoid having a repeated event-shake. For this, we can put the older value to 0, OR if we want
			// to avoid detection for a longer time, we need to reset the table.
		    //tableTimes[(indexTableTimes+1)%times] = 0;
			for (uint8_t i=0; i< times; i++) tableTimes[i] = i*interval;
			shakeResult = true;
		}

	}

	return shakeResult;
}

/*
Another method (less elegant and precise, but consuming less memory):

volatile uint8_t TiltSensor::numDetectedTicks;
volatile bool event;

void TiltSensor::tickCount() {
	static uint8_t numDetectedTicks = 0;

	numDetectedTicks++;

	// NOTE: the use of millis() here is possible, but remember that its timer does not evolves inside the ISR.
	if (millis() - lastTimeCheck > interval) {
		if (numDetectedTicks>=times) event = true; else event = false;
		lastTimeCheck = millis();
		numDetectedTicks = 0;
	}

}

*/
