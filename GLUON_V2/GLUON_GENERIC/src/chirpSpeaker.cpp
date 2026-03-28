
#include "chirpSpeaker.h"

// Pre-instantiated cross-file global variable:
ChirpSpeaker myChirpSpeaker(PIN_CHIRP_SPEAKER);


ChirpSpeaker::ChirpSpeaker(uint8_t _pin, bool _state) {
	init(_pin, _state);
}

void ChirpSpeaker::init(uint8_t _pin, bool _state) {
	state = _state;
	pinSpeaker=_pin;
	pinMode(pinSpeaker, OUTPUT);
}

void ChirpSpeaker::chirpShake() {
    if (!state) return;
    bool level = true;
    for (uint8_t i = 0; i < 200; i++) {
        digitalWrite(pinSpeaker, level);
        level = !level;
        delayMicroseconds(random(100));
    }
    digitalWrite(pinSpeaker, LOW);
}

void ChirpSpeaker::chirpDown()
{
	if (state) {
		bool level = true;
		for (uint8_t i = 0; i < 255; i++) {
			digitalWrite(pinSpeaker, level);
			level = !level;
			delayMicroseconds(i);
		}
		digitalWrite(pinSpeaker, LOW);
	}
}

void ChirpSpeaker::chirpUp()
{
	if (state) {
		bool level = true;
		for (uint8_t i = 255; i > 0; i--) {
			digitalWrite(pinSpeaker, level);
			level = !level;
			delayMicroseconds(i);
		}
		digitalWrite(pinSpeaker, LOW);
	}
}

void ChirpSpeaker::chirpDownUp()
{
	if (state) {
		boolean level = true;
		for (uint8_t i = 0; i < 200; i++) {
			digitalWrite(pinSpeaker, level);
			level = !level;
			delayMicroseconds(i);
		}
		for (uint8_t i = 200; i > 0; i--) {
			digitalWrite(pinSpeaker, level);
			level = !level;
			delayMicroseconds(i);
		}
	digitalWrite(pinSpeaker, LOW);
	}
}
