
#include "vibrator.h"

// Pre-instantiated cross-file global variable:
Vibrator myVibrator(PIN_VIBRATOR);


Vibrator::Vibrator(uint8_t _pin, bool _state) {
	init(_pin, _state);
}

void Vibrator::init(uint8_t _pin, bool _state) {
	state = _state;
	pinVibrator=_pin;
	//pinMode(pinVibrator, OUTPUT);
}

void Vibrator::manyPulses(uint8_t times, uint32_t _timeMs, uint8_t strength) {
	for (uint8_t i=0; i<times; i++) pulse(_timeMs, strength);
}

void Vibrator::pulse(uint32_t _timeMs, uint8_t strength) {
	//digitalWrite(pinVibrator, HIGH);
	analogWrite(pinVibrator, strength);
	delay(_timeMs);
	//digitalWrite(pinVibrator, LOW);
	analogWrite(pinVibrator, 0);
	delay(_timeMs);
}
