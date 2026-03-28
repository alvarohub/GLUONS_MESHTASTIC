/*
* light weight WS2812 lib V2.1 - Arduino support
*
* Controls WS2811/WS2812/WS2812B RGB-LEDs
* Author: Matthias Riegler
* Modified for only 4 leds, without HSV control by Alvaro Cassinelli
*
*/

#ifndef NEO_STRIP_H_
#define NEO_STRIP_H_

#include <Arduino.h>

#include <avr/interrupt.h>
#include <avr/io.h>
#ifndef F_CPU
#define  F_CPU 16000000UL
#endif
#include <util/delay.h>
#include <stdint.h>


struct cRGB {
	uint8_t r;
	uint8_t g;
	uint8_t b;

	cRGB dim(uint8_t _percent) const {
		cRGB dimmedColor;
		dimmedColor.g = (uint8_t)(1.0*_percent/100*r);
		dimmedColor.r = (uint8_t)(1.0*_percent/100*g);
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

class NEO_STRIP {
public:
	NEO_STRIP();
	NEO_STRIP(uint16_t _numLeds, uint8_t pin);
	~NEO_STRIP();

	void init(uint16_t _numLeds, uint8_t pin);
	void setOutput(uint8_t pin);

	void setDimFactor(uint8_t _dimShiftBits);

	void set_crgb_at(uint16_t index, const cRGB& px_value);
	void setAll(const cRGB& px_value);

	cRGB get_crgb_at(uint16_t index);

	// an elementary state retrieval mechanism using double buffering:
	void dupBuffer() { // copy data in current buffer (pixelBuffer1) on the auxiliary buffer (pixelBuffer2)
		memcpy(pixelBuffer2, pixelBuffer1, 3*count_led);
	}
	void swapBuffer() {
		uint8_t *auxPixelBuffer = pixelBuffer2;
		pixelBuffer2=pixelBuffer1;
		pixelBuffer1 = auxPixelBuffer;
	}

	void display();

private:

	uint16_t count_led;
	uint8_t *pixelBuffer1, *pixelBuffer2;
	uint8_t dimShiftBits; // between 0 and 8

	void ws2812_sendarray_mask(uint8_t *array, uint16_t length, uint8_t pinmask, uint8_t *port, uint8_t *portreg);

	const volatile uint8_t *ws2812_port;
	volatile uint8_t *ws2812_port_reg;
	uint8_t pinMask;
};

#endif /* NEO_STRIP_H_ */
