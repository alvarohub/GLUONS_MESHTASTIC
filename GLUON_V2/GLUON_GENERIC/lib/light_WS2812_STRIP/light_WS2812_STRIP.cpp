
#include "light_WS2812_STRIP.h"

NEO_STRIP::NEO_STRIP() {}

NEO_STRIP::NEO_STRIP(uint16_t _num_leds, uint8_t _pin) {
	init(_num_leds, _pin);
}

void NEO_STRIP::init(uint16_t _num_leds, uint8_t _pin) {
	count_led = _num_leds;
	pixelBuffer1 = new uint8_t[3 * count_led];
	pixelBuffer2 = new uint8_t[3 * count_led];
	setOutput(_pin);
	dimShiftBits = 2; // intensity will be divided by 2^dimShiftBits
}

NEO_STRIP::~NEO_STRIP() {
	delete[] pixelBuffer1;
	delete[] pixelBuffer2;
}

void NEO_STRIP::setDimFactor(uint8_t _dimShiftBits) { // from 0 to 8 (powers of two)
	if (_dimShiftBits<=8) dimShiftBits = _dimShiftBits;
}

cRGB NEO_STRIP::get_crgb_at(uint16_t index) {
	cRGB px_value;
	if(index < count_led) {
		index*=3;
		px_value.r = pixelBuffer1[index++];
		px_value.g = pixelBuffer1[index++];
		px_value.b = pixelBuffer1[index];
	}
	return px_value;
}

void NEO_STRIP::set_crgb_at(uint16_t index, const cRGB& px_value) {
	if (index < count_led) {
		index*=3;
		pixelBuffer1[index++] = px_value.r;
		pixelBuffer1[index++] = px_value.b; // NOTE: looks like the correct order for the WS2812 is RBG and not RGB
		pixelBuffer1[index] = px_value.g;
	}
}

void NEO_STRIP::setAll(const cRGB& px_value) {
	for (uint8_t i=0; i<count_led; i++) {
		set_crgb_at(i, px_value);
	}
}

void NEO_STRIP::display() {
	// First, dim the buffer:
	*ws2812_port_reg |= pinMask; // Enable DDR
	ws2812_sendarray_mask(pixelBuffer1, 3*count_led, pinMask ,(uint8_t*) ws2812_port,(uint8_t*) ws2812_port_reg );
}

void NEO_STRIP::setOutput(uint8_t pin) {
	pinMask = digitalPinToBitMask(pin);
	ws2812_port = portOutputRegister(digitalPinToPort(pin));
	ws2812_port_reg = portModeRegister(digitalPinToPort(pin));
}

/*
	MAIN ROUTINE:

	void  ws2812_sendarray_mask(uint8_t *data,uint16_t datlen,uint8_t maskhi,uint8_t *port, uint8_t *portreg)

	This routine writes an array of bytes with RGB values to the Dataout pin using the fast 800kHz clockless WS2811/2812 protocol.
*/

// Timing in ns
#define w_zeropulse   350
#define w_onepulse    900
#define w_totalperiod 1250

// Fixed cycles used by the inner loop
#define w_fixedlow    3
#define w_fixedhigh   6
#define w_fixedtotal  10

// Insert NOPs to match the timing, if possible
#define w_zerocycles    (((F_CPU/1000)*w_zeropulse          )/1000000)
#define w_onecycles     (((F_CPU/1000)*w_onepulse    +500000)/1000000)
#define w_totalcycles   (((F_CPU/1000)*w_totalperiod +500000)/1000000)

// w1 - nops between rising edge and falling edge - low
#define w1 (w_zerocycles-w_fixedlow)
// w2   nops between fe low and fe high
#define w2 (w_onecycles-w_fixedhigh-w1)
// w3   nops to complete loop
#define w3 (w_totalcycles-w_fixedtotal-w1-w2)

#if w1>0
#define w1_nops w1
#else
#define w1_nops  0
#endif

// The only critical timing parameter is the minimum pulse length of the "0"
// Warn or throw error if this timing can not be met with current F_CPU settings.
#define w_lowtime ((w1_nops+w_fixedlow)*1000000)/(F_CPU/1000)
#if w_lowtime>550
#error "Light_ws2812: Sorry, the clock speed is too low. Did you set F_CPU correctly?"
#elif w_lowtime>450
#warning "Light_ws2812: The timing is critical and may only work on NEO_STRIPB, not on NEO_STRIP(S)."
#warning "Please consider a higher clockspeed, if possible"
#endif

#if w2>0
#define w2_nops w2
#else
#define w2_nops  0
#endif

#if w3>0
#define w3_nops w3
#else
#define w3_nops  0
#endif

#define w_nop1  "nop      \n\t"
#define w_nop2  "rjmp .+0 \n\t"
#define w_nop4  w_nop2 w_nop2
#define w_nop8  w_nop4 w_nop4
#define w_nop16 w_nop8 w_nop8

void  NEO_STRIP::ws2812_sendarray_mask(uint8_t *data,uint16_t datlen,uint8_t maskhi,uint8_t *port, uint8_t *portreg)
{
	uint8_t curbyte,ctr,masklo;
	uint8_t sreg_prev;

	masklo = ~maskhi & *port;
	maskhi |= *port;
	sreg_prev=SREG;
	cli();

	while (datlen--) {
		curbyte = (*data++) >> dimShiftBits; // dimShiftBits is the number of bits to shift to the right: 1 to 8
		//curbyte = (*data++);

		asm volatile(
			"       ldi   %0,8  \n\t"
			"loop%=:            \n\t"
			"       st    X,%3 \n\t"    //  '1' [02] '0' [02] - re
			#if (w1_nops&1)
			w_nop1
			#endif
			#if (w1_nops&2)
			w_nop2
			#endif
			#if (w1_nops&4)
			w_nop4
			#endif
			#if (w1_nops&8)
			w_nop8
			#endif
			#if (w1_nops&16)
			w_nop16
			#endif
			"       sbrs  %1,7  \n\t"    //  '1' [04] '0' [03]
			"       st    X,%4 \n\t"     //  '1' [--] '0' [05] - fe-low
			"       lsl   %1    \n\t"    //  '1' [05] '0' [06]
			#if (w2_nops&1)
			w_nop1
			#endif
			#if (w2_nops&2)
			w_nop2
			#endif
			#if (w2_nops&4)
			w_nop4
			#endif
			#if (w2_nops&8)
			w_nop8
			#endif
			#if (w2_nops&16)
			w_nop16
			#endif
			"       brcc skipone%= \n\t"    //  '1' [+1] '0' [+2] -
			"       st   X,%4      \n\t"    //  '1' [+3] '0' [--] - fe-high
			"skipone%=:               "     //  '1' [+3] '0' [+2] -

			#if (w3_nops&1)
			w_nop1
			#endif
			#if (w3_nops&2)
			w_nop2
			#endif
			#if (w3_nops&4)
			w_nop4
			#endif
			#if (w3_nops&8)
			w_nop8
			#endif
			#if (w3_nops&16)
			w_nop16
			#endif

			"       dec   %0    \n\t"    //  '1' [+4] '0' [+3]
			"       brne  loop%=\n\t"    //  '1' [+5] '0' [+4]
			:	"=&d" (ctr)
			//    :	"r" (curbyte), "I" (_SFR_IO_ADDR(ws2812_PORTREG)), "r" (maskhi), "r" (masklo)
			:	"r" (curbyte), "x" (port), "r" (maskhi), "r" (masklo)
		);
	}

	SREG=sreg_prev;
}
