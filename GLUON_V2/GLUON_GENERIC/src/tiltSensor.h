#ifndef _tilt_sensor_H
#define _tilt_sensor_H

// TODO: in the future, make this a derived class of BaseSensor... For now, this is a special piece of code.

#include "Arduino.h"
//#include "PinChangeInterrupt.h"
//#include <PCInterrupt.h>
//The following pins are usable for PinChangeInterrupt:
//Arduino Uno/Nano/Mini: All pins are usable
//Arduino Mega: 10, 11, 12, 13, 50, 51, 52, 53, A8 (62), A9 (63), A10 (64),
//			  A11 (65), A12 (66), A13 (67), A14 (68), A15 (69)
//Arduino Leonardo/Micro: 8, 9, 10, 11, 14 (MISO), 15 (SCK), 16 (MOSI)

#define tiltPin 4
#define DEFAULT_MEASURE_TICK_INTERVAL 2000 // in ms
#define MINIMUM_TIME_BETWEEN_TICKS 500 // in ms

#define MAX_NUM_TICKS 4
#define DEFAULT_NUM_TICKS 4

class TiltSensor {
public:
	TiltSensor() {};
	TiltSensor(uint8_t _pin, uint8_t _times = DEFAULT_NUM_TICKS, uint32_t _interval = DEFAULT_MEASURE_TICK_INTERVAL) {
		init(_pin,  _times,  _interval);
	}

	void init(uint8_t _pin, uint8_t _times = DEFAULT_NUM_TICKS, uint32_t interval = DEFAULT_MEASURE_TICK_INTERVAL);

	void enable();
	void disable();

	bool checkEvent();

private:
	uint8_t pin; // the sensing pin, triggering an interrupt (using pin change interrupt)
	static volatile uint8_t times; // the number of times it has to tick during the interval to produce an event
	uint32_t interval; // the measuring interval in ms

	static void tickAddTime();
	static volatile uint8_t indexTableTimes;
	static volatile uint32_t tableTimes[MAX_NUM_TICKS];

	bool sensorActive;// enable/disable are needed in case there is no hardware for the sensor or during tests

};

// Pre-instantiated object:
extern TiltSensor myTiltSensor;

#endif
