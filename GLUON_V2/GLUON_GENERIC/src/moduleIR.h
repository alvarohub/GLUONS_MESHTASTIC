/*
	moduleIR.h

	Simplified IR receiver/sender for NEC codes, using INT1 (PD3)

	  * TIMER0 shoud not be touched (delay, micros...). TIMER0 generates PWM on PD5 and PD6. PD6 is used as digital output by the register LEDs. PD5 is therefore the only "classic" PWM for using on the gluon.
	  * TIMER1: controls PWM on pins PB1 and PB2. PB2 is used by the RFM69 module. PB1 can be used for generating the 38kHz signal for transmission of IR messages. Note that there is a LED connected to that pin on the Moteino.
	  * TIMER2: controls PWM on pins PD3 and PB3. PB3 is used by the RFM module; PD3 is the INT0, so I won't use it as PWM.

	  Conclusion:
	  	- The only "generic" PWM pin available is on PD5, and I cannot change it's period (490Hz)
	  	- Use TIMER1, PB1/OC1A to generate 38kHz (send)
		- Set INT1 on FALLING EDGE on PD3 to detect IR messages, use core micros() function to time the signals.

*/


#ifndef _moduleIR_h_
#define _moduleIR_h_

#include "Arduino.h"
#include "utils.h"
// Definition of interrupt names
//#include < avr/io.h >
// ISR interrupt service routine
//#include < avr/interrupt.h >

#define SEND_BEACON_PERIOD 700 // in ms [700 was not bad]
#define RANDOM_BLOCK_DELAY 15 // in ms. Note:  NEC IR code duration is about 67ms [15 was not bad]

// CPU Frequency
#ifdef F_CPU
#	define F_CLK  F_CPU     // main Arduino clock
#else
#	define F_CLK  16000000  // main Arduino clock
#endif

#define IR_CARRIER_FREQ    38000L

#define PIN_RCV 3 // PD3, D3, INT1: An IR detector/demodulator must be connected to this pin
#define PIN_SND 9 // PB1, D9, OC1A: the IR sender LED should be connected here

// IR COMMANDS: 2 bytes (associates values: 2 bytes). Total rawcode is 32 bis (NEC code)
#define REQUEST_CREATION_OR_DELETE_INLET 0xAAAA   // should NOT be a standard IR code. The rest (2 bytes) is the VALUE (the NodeId)
#define REQUEST_MOVE_OR_DELETE_INLET 0xBBBB   // should NOT be a standard IR code. The rest (2 bytes) is the VALUE, and it will be the NodeId.

// Other RAW CODES from a standard remote controller:
// 0) This is the standard code sent by a NEC REMOTE when holding a key down is 0xFFFFFFFF (I will just ignore for now, but we could REPEAT the action...)
#define CODE_REPEAT 0xFFFFFFFF

// 1) Update of the output (logic update)
#define CODE_FORCE_BANG 0xFFC23D //  ">||". This FORCES a logic update, whatever the logicMode.

#define CODE_SET_STANDBY 0xFF6897 //  "0" . This stops every auto-update (only update  using the "forced" IR code). It will be very useful for testing.
#define CODE_ON_SENSOR_EVENT 0xFF30CF // "1"
#define CODE_SET_UPDATE_FIRST_INLET_BANG 0xFF18E7 // "2"
#define CODE_SET_UPDATE_ANY_INLET_BANG 0xFF7A85 // "3"
#define CODE_SET_UPDATE_SYNCH 0xFF10EF // "4"
#define CODE_SET_UPDATE_PERIODIC 0xFF38C7 //"5"
// 2) Reset connexions:
#define CODE_CLEAR_ALL_CONNEXIONS 0xFF629D //  "CH"
#define CODE_CLEAR_ALL_INLET_LINKS 0xFFA25D    //  "CH-"
#define CODE_CLEAR_ALL_OUTLET_LINKS 0xFFE21D    //"CH+"
// 3) LEARN THRESHOLD:
#define CODE_LEARN_SENSOR_THRESHOLD 0xFF906F //  "EQ"
// 4) FORCE NO LEARNING:
//#define CODE_STOP_LEARNING

// ...


// PUT IN PROGMEM?!!!
/*
const uint32_t codesRemote [21]  =
{ 0xFFA25D, 0xFF629D, 0xFFE21D,
  0xFF22DD, 0xFF02FD, 0xFFC23D,
  0xFFE01F, 0xFFA857, 0xFF906F,
  0xFF6897, 0xFF9867, 0xFFB04F,

  0xFF30CF, 0xFF18E7, 0xFF7A85,
  0xFF10EF, 0xFF38C7, 0xFF5AA5,
  0xFF42BD, 0xFF4AB5, 0xFF52AD
};


const String buttonsRemote[21]  =
{ "CH-", "CH", "CH+",
  "<<" , ">>", ">||",
  "-", "+", "EQ",
  "0", "100+", "200+",
  "1", "2", "3",
  "4", "5", "6",
  "7", "8", "9"
};
*/

class ModuleIR  {

public:
	ModuleIR() {}
	void init();

	bool updateReceive();
	void sendBeacon(MessageIR&) ;
	void updateBeacon(MessageIR&);

	MessageIR getMessage() {return (receivedMessage);}
	uint32_t getRawCode() {return (receivedMessage.getRawCode());}
	uint16_t getCommand() {return (receivedMessage.getCommand());}
	uint16_t getValue() {return (receivedMessage.getValue());}

private:

	static void detectCarrier(); // this is the ISR. Must be declared as STATIC.
	bool readNEC(uint32_t& _rawCode);
	void sendNEC(uint32_t _rawCode);

	void pulse(uint16_t carrierDuration, uint16_t gapDuration);
	void enableSending();
	void enableReception();

	static volatile bool newData;
	static volatile uint8_t NextBit;
	static volatile uint32_t raw32BitCode;

	// static const uint16_t top  = 420;
	// static const uint16_t match = 126;

	//Message receivedMessage; // in the future, let's unify all the messages in this type, and use payload or MessageIR as an intermediate layer.
	MessageIR receivedMessage; // this is the equivalent of the  payload in case of the RF method..

	unsigned long resendBeacon, randomDelay;
};

extern ModuleIR myIR;

#endif
