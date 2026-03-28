#ifndef _vibrator_h_
#define _vibrator_h_

/*
* ********************************************************************************************************************
*                               Motor vibrator - used to explore connections by "pulling"
* ********************************************************************************************************************/

#include "Arduino.h"

//Pin connected to the motor vibrator:
#define PIN_VIBRATOR 5 // PWM

class Vibrator {
public:
	Vibrator() {}
	Vibrator(uint8_t _pin,  bool _state=true);
	void init(uint8_t _pin, bool _state=true);

	void enable() {state=true;}
	void disable() {state=false;}

	void manyPulses(uint8_t times, uint32_t _timeMs, uint8_t strength);
	void pulse(uint32_t _timeMs,  uint8_t strength);
private:
	bool state;
	uint8_t pinVibrator;
};

// Pre-instantiated object:
extern Vibrator myVibrator;

#endif
