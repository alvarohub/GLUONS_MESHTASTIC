#ifndef _ChirpSpeaker_h_
#define _ChirpSpeaker_h_

/*
* ********************************************************************************************************************
*                               Small speaker to indicate creation/deletion of connexions
* ********************************************************************************************************************/

#include "Arduino.h"

//Pin connected to the piezo:
#define PIN_CHIRP_SPEAKER 6

class ChirpSpeaker {
public:
	ChirpSpeaker() {}
	ChirpSpeaker(uint8_t _pin,  bool _state=true);
	void init(uint8_t _pin, bool _state=true);

	void enable() {state=true;}
	void disable() {state=false;}

	void chirpDown();
	void chirpUp();
	void chirpDownUp();
    void chirpShake();
    //void chirpLearn();
private:
	bool state;
	uint8_t pinSpeaker;
};

// Pre-instantiated object:
extern ChirpSpeaker myChirpSpeaker;

#endif
