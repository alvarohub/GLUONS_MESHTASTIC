/*
  light weight WS2812 for only 4 LEDS, without double buffering
*/

#ifndef NEO_STRIP_SIMPLE_H_
#define NEO_STRIP_SIMPLE_H_

#include <Arduino.h>

#include <avr/interrupt.h>
#include <avr/io.h>
#ifndef F_CPU
#define  F_CPU 16000000UL
#endif
#include <util/delay.h>
#include <stdint.h>

//Pin connected to the neopixel string (with four leds)
#define PIN_DI_NEO 7
#define NUM_LEDS_STRIP 4
#define PIXEL_BUFFER_SIZE 12 // (3 x NUM_LEDS_STRIP)

struct cRGB {
	uint8_t r, g, b;

	cRGB dim(uint8_t _percent) const {
		cRGB dimmedColor;
		dimmedColor.r = (uint8_t)(1.0*_percent/100*r);
		dimmedColor.g = (uint8_t)(1.0*_percent/100*g);
		dimmedColor.b = (uint8_t)(1.0*_percent/100*b);
		return(dimmedColor);
	}

	bool operator!=( const cRGB& col ) {return( (col.r!=r)||(col.g!=g)||(col.b!=b)); }
};

const cRGB black = {0,0,0};
const cRGB white = {255, 255, 255};
const cRGB red = {255, 0, 0};
const cRGB green = {0, 255, 0};
const cRGB blue = {0, 0, 255};

class NEO_STRIP_SIMPLE {
public:
	NEO_STRIP_SIMPLE();

	void set_crgb_at(uint8_t index, const cRGB& px_value);
	void setAll(const cRGB& px_value);

	void setDimFactor(uint8_t _dimShiftBits);
	void display();

private:

	uint8_t pixelBuffer[PIXEL_BUFFER_SIZE];
	uint8_t dimShiftBits; // between 0 and 8

	void ws2812_sendarray_mask(uint8_t *array, uint8_t length, uint8_t pinmask, uint8_t *port, uint8_t *portreg);

	const volatile uint8_t *ws2812_port;
	volatile uint8_t *ws2812_port_reg;
	uint8_t pinMask;
};

#endif
