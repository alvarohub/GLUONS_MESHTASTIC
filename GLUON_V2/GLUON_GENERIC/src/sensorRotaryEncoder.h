#ifndef _rotary_encoder_H
#define _rotary_encoder_H

#include "BaseSensor.h"

#define ENC_PINA A0
#define ENC_PINB A1


class SensorRotaryEncoder: public BaseSensor {
public:

	SensorRotaryEncoder() : BaseSensor("ROT_ENC", ANALOG_OUTPUT), pinA(ENC_PINA), pinB(ENC_PINB) {init();}
	SensorRotaryEncoder(const char* _name, uint8_t _pinA, uint8_t pinB) : BaseSensor(_name, ANALOG_OUTPUT) {init();}

	virtual ~SensorRotaryEncoder() override {}

	void init(bool firstTimeBuild = false);

	void enable();
	void disable();
	void reset();

	virtual uint16_t readAnalogOutput() override;

private:
	//static inline int bin2sign (const boolean var) {return 2 * var - 1;}

	// ISR routines:
	static void tickA(void* arg);
	static void tickB(void* arg);
	static void updatePosition(void* arg);

	//pins for A and B interrupts:
	uint8_t pinA, pinB;

	volatile int32_t position;
	volatile uint8_t state;
};

#endif
