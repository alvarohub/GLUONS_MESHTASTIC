#include "moduleIR.h"

// Pre-instantiated cross-file global variable
ModuleIR myIR;

volatile bool ModuleIR::newData;
volatile uint8_t ModuleIR::NextBit;
volatile uint32_t ModuleIR::raw32BitCode;

void ModuleIR::init() {

	//0) Set volatile variables BEFORE setting the ISR that uses them:
	newData=false; NextBit=32; raw32BitCode=0;

	//1) Setting TIMER1 to generate PWM, 50% duty cycle, at 38kHz (we will enable/disable OC1A - PB1 to create the pulses).
	// NOTE: we will also use this TIMER1 for timing signals on reception (but top and prescalers needs to change; alternatively, // we could use the micros() function)
	// a) Set PB1 as output:
	pinMode(PIN_SND, OUTPUT); // DDRB  |= (1<<DDB1); // set OC1A(PB1) as output (this is just equal to pinMode(PIN_SND, OUTPUT))
	digitalWrite(PIN_SND, LOW);//PORTB &= ~(1<<PB1); // set it as LOW for a start (digitalWrite(PIN_SND, LOW))
	// b) SET TIMER1 in FAST PWM to generate a square wave at 38kHz:
	// b.1) MODE: Fast PWM, TOP= ICR1 by doing: [WGM13, WGM12, WGM11, WGM10] = [1 1 1 0]
	TCCR1B |= (_BV(WGM13) | _BV(WGM12)); TCCR1A |=_BV(WGM11); TCCR1A &= ~(_BV(WGM10));
	TCCR1B &= (~(_BV(CS12)) & ~(_BV(CS11))); TCCR1B |= _BV(CS10); // [CS12 CS11 CS10] = [0 0 1], set prescaler to 1
	ICR1 = 420 ; // TOP = IR_CARRIER_FREQ/(1xF_CKL)-1;
	OCR1A = 126; // MATCH = Compare match (30% of 420, since IR transmitters have between 25% and 33% duty cycle)
	// b.2) Compare Match Output A (for fast PWM):
	// Clear OC1A/OC1B on Compare Match, set OC1A/OC1B at BOTTOM (non-inverting mode):
	// 			[COM1A1/COM1B1 COM1A0/COM1B0] = [1 0]
	TCCR1A &= ~(_BV(COM1A1)); // we start with it OFF. Note: we can also do OFF PWM by setting OCR1A to TOP (ICR1)

	// 2) Prepare to set INT1 on falling edge:
	pinMode (PIN_RCV, INPUT_PULLUP); // I assume there is a pull-up in the IR sensor...but just for security.
	//attachInterrupt (1, detectCarrier, FALLING);
	// NOTE: The Timer/Counter Overflow Flag (TOV1) is set each time the counter reaches TOP (will be useful for timing
	// during reception, but top needs to be larger than the largest low state on the IR sensor for the NEC code.
	TIMSK1 &= ~(_BV(TOIE1));// Overflow Interrupt disabled (because otherwise an ISR on overflow, would clear the flag)
	interrupts(); // enable interrupts (just in case). Equal to sei();
	enableReception();


	// Random Jitter for the BEACON SENDING to avoid collision:
	resendBeacon = millis();
	randomDelay = 0;// random(-2, 2) * RANDOM_BLOCK_DELAY; // in ms

}


// NOTE: a very interesting math problem!! many strategies to test...
// TODO: PERHAPS the best is something like , i.e: DETECT if the channel is in use and use exponential backdrop!!!
// It is easy to know if the IR sensor is receiveing something: NextBit is different from 32!!!

void ModuleIR::updateBeacon(MessageIR& _msg) {
	if ( (millis() - resendBeacon) > (SEND_BEACON_PERIOD + randomDelay) ) {
		// the time randomizing is to avoid (probabilistically speaking) having both
		// nodes sending simultaneously their messages (this may happen sometimes... )
		sendBeacon(_msg);

		randomDelay = random(-1,4)*RANDOM_BLOCK_DELAY; // in ms. NOTE: random(a,b) gives an integer value between a (non inclusive) and b

		//resendBeacon = millis(); // i.e, resendBeacon += SEND_BEACON_PERIOD + randomDelay
		resendBeacon += SEND_BEACON_PERIOD;
	}
}

/*
// TODO!!!! With collision detection and exponential backoff algorithm:
void ModuleIR::updateBeacon(MessageIR& _msg) {
	// Is it time to send?
	if (millis() - resendBeacon) > SEND_BEACON_PERIOD ) {

		// Is the channel free?
		if (NextBit==32) {

			// Send the beacon and reschedule the timer for the next slot in SEND_BEACON_PERIOD
			sendBeacon(_msg);

			resendBeacon = millis();

		} else {
			// 1) Send a jamming signal (send something..)
			// 2) reschedule the sending ...
			// ...
		}
	}
}
*/

void ModuleIR::sendBeacon(MessageIR& _msg)  {
	// first, disable receiver (to avoid getting detecting our own message):
	enableSending();
	sendNEC(_msg.getRawCode());
	//LOG_PRINT(F("Sending RAW IR CODE: "));  LOG_PRINTLN(_msg.getRawCode(), HEX);
	// note: at the end of the pulse, OC1A will be LOW. No PWM is generated.
	enableReception(); // should be most of the time enabled, only disabled when sending (a mere 70ms)
}

void ModuleIR::pulse(uint16_t carrierDuration, uint16_t gapDuration) { // each unit of duration equal to 26.3us
	// note: we can measure the time using micros() or millis(), however and to avoid calls to other
	// functions, I will use TIMER1 OVERFLOW - knowing that the period now from 0 to TOP-1 is equal to 1/38kHz, i.e, 26.3us
	uint16_t count = carrierDuration;
	TCCR1A |= _BV(COM1A1); // Generate pulses at 38kHz (or we can do: OCR1A = 126)
	for (uint8_t i=0; i<2; i++) {
		for (uint16_t c=0; c<count; c++) {
			// The Timer/Counter Overflow Flag (TOV1, bit 0) on register TIFR1 (Timer/Counter1 Interrupt Flag Register)
			// is set each time the counter reaches TOP:
			do ; while ((TIFR1 & (1<<TOV1)) == 0);
			TIFR1 |= 1<<TOV1;  // Clear overflow (clear by writing a one to TOV1)
		}
		count = gapDuration;    // Generate gap
		TCCR1A &= ~(_BV(COM1A1)); // Disable carrier on OC1A (or we can do: OCR1A = 420 = ICR1 = TOP)
	}
	// note: at the end of the pulse, OC1A will be LOW. No PWM is generated.
}

void ModuleIR::enableSending() { // this disables reception, and enables sending
	// 1) Disable external interrupt INT1:
	detachInterrupt(1);

	// 2) Disable interrupts (if there were others appart from INT1) while setting the 16 bits register TOP, and other variables
	uint8_t oldSREG = SREG; cli(); // disable global interrupts
	TCCR1B &= (~(_BV(CS12)) & ~(_BV(CS11))); TCCR1B |= _BV(CS10); // [CS12 CS11 CS10] = [0 0 1], set prescaler to 1
	ICR1 = 420;// set an appropiate TOP, to be able to generate a PWM signal
	TCNT1=0;   // reset timer in case it was larger than ICR1
	TCCR1A |= _BV(COM1A1); // Enable PWM on OC1A
	SREG = oldSREG; // basically, if interrupts were enabled, they are back.
}

void ModuleIR::enableReception(){ // this disables sending, and enables reception
	// Disable interrupts while setting the 16 bits register TOP, and other variables
	// (note that normally there should not be interrupts here, not at least from the INT1)
	uint8_t oldSREG = SREG; cli(); // disable global interrupts

	//1) Disable carrier on OC1A (perhaps not necessary, but this will ensure no interference)
	TCCR1A &= ~(_BV(COM1A1));
	// Set an appropiate TOP and prescaler, to be able to check OVERFLOW on timer counter TCNT1 while measuring periods (larger than 13.5ms)
	TCCR1B |= _BV(CS12); TCCR1B &= ~(_BV(CS11)); TCCR1B &= ~(_BV(CS10)); // [CS12 CS11 CS10]=[1 0 0], prescaler to 256
	ICR1 = 1000; // tick freq is 16MHz/256=62.5kHz, so one tick is 16us. 13500ms means about 844 ticks
	TCNT1=0;
	// "The Timer/Counter Overflow Flag (TOV1) is set each time the counter reaches TOP" (Atmega328p, p.124)
	//TIMSK1 &= ~(_BV(TOIE1));// Overflow Interrupt disabled (because otherwise an ISR on overflow, would clear the flag)
	TIFR1 |= 1<<TOV1; // Clear overflow - note: clear by writing a ONE to TOV1!! (see datasheet)

	//2) Reset the decoder parameters:
	newData=false; NextBit=32; raw32BitCode=0;

	SREG = oldSREG; // basically, if interrupts were enabled, they are back.

	//3) Enable external interrupt INT1:
	EIFR = bit (INTF1);  // clear flag for interrupt 1 (in case it was buffered)
	attachInterrupt(1, detectCarrier, FALLING);
}


bool ModuleIR::updateReceive() {
	// Here we will read the last data acquired by the IR module. It is either something or nothing. If it is something, then
	// there is either a COMMAND (address) directed to this gluon using a remote controller, or the sender NODEID in case of physical contact.
	uint32_t raw;
	bool received = readNEC(raw);
	if (received) {
		// Check data for any remote:
		LOG_PRINT(F("RAW IR: "));  LOG_PRINTLN(raw, HEX);
		// Set the data in the receivedMessage object (this will also compute its command and value fields)
		receivedMessage.setRawCode(raw);
		LOG_PRINT("IR:  COM "); LOG_PRINT(receivedMessage.getCommand(), HEX);
		LOG_PRINT(" VAL "); LOG_PRINTLN(receivedMessage.getValue(), HEX);
	}
	return (received);
}

bool ModuleIR::readNEC(uint32_t& _rawCode) {
	bool _newData = newData; // atomic operation

	if (_newData) {
		// Disable interrupts while we read the raw32BitCode or we might get an
		// inconsistent value (e.g. in the middle of a write to raw32BitCode):
		uint8_t oldSREG = SREG;cli();
		_rawCode = raw32BitCode;
		newData = false; // for checking next data only
		SREG = oldSREG;
	}
	return _newData;
}

// ISR interrupt routine called when there is a FALLING edge on pin PD3 (INT1)
void ModuleIR::detectCarrier() {

	// note: this would affect the PWM generation (we are chaning the values of the TCNT1), but before using the PWM, we
	// disable the carrier detection (no INT1)
	uint16_t Time = TCNT1; //  a time unit is 16us (=1/62.5KHZ, using prescaler 256)
	bool Overflow = TIFR1 & (1<<TOV1); //Timer/Counter Interrupt Flag Register TOV1 on TIFR1 (when TCNT1 reachs TOP = 1000)

	// Keep looking for AGC pulse and gap
	if (NextBit == 32) {
		//STARTING CODE (AGC) pulse and gap is within 10% of the required 13.5ms (844 ticks)
		if ((Time >= 760) && (Time <= 928) && (!Overflow)) {
		//  Start reading code:
		raw32BitCode = 0; NextBit = 0; newData=false;
	}
	//REPEAT CODE: pulse and gap is within 10% of the 11.25ms width (703 ticks)
	else if ((Time >= 632) && (Time <= 773) && (!Overflow)) { // 11250/16=703
		// raw32BitCode should not change, or we can use set it to CODE_REPEAT (0xFFFFFFFF)
		newData=true;
	}
}
// Data bit (1 is 2.25ms, 0 is 1.12ms - remember: we detect only the falling edges)
// A 0-bit has a length of 1.12ms (70 ticks), a 1-bit has a length of 2.25ms (141 ticks). The mean (threshold) = 105 ticks
else {
	if ( (Time > 200) || Overflow ) NextBit = 32; // Invalid - restart
	else {
		//hack for the time being: the older IRremote protocol sent data from MSB to LSB! we need to flip the 32 bit word:
		//if (Time > 105) raw32BitCode = raw32BitCode | ((uint32_t) 1<<NextBit);
		if (Time > 105) raw32BitCode = raw32BitCode | (uint32_t)1<<(31-NextBit) ;
		if (NextBit == 31) newData=true;
		NextBit++;
	}
}
TCNT1 = 0;        // Clear counter
TIFR1 |= 1<<TOV1; // Clear overflow - note: clear by writing a ONE to TOV1!! (see datasheet)
//EIFR – External Interrupt Flag Register = [– – – – – – INTF1 INTF0]
EIFR |= 1<<INTF1;  // Clear INT1 flag (is this really necessary? interrupt flag is cleared at the end of the ISR?)
// from Atmega328P datasheet (p.72): "The flag is cleared when the interrupt routine is executed. Alternatively,
// the flag can be cleared by writing a logical one to it.
}

// hack from the time being (old IRRemote seems to flip the whole 32 bit word:)
void ModuleIR::sendNEC(uint32_t _rawCode) {
	uint16_t command, value, data;
	uint8_t bitMask;
	command = _rawCode & 0xFFFF;
	value = (_rawCode >> 16) & 0xFFFF;
	// 1) First send AGC tone:
	pulse(342, 171); //9ms carrier, 4.5ms silence (each tick: 26.3us, hence 9000/26.3=342)
	// 2) Then the address and the command:
	data = value;
	for (uint8_t i=0; i<2; i++) {
		bitMask = 16;
		do {
			// hack from the time being (old IRRemote seems to flip the whole 32 bit word:)
			if (data & (1<<(bitMask-1))) pulse(21, 64); else pulse(21, 21); // note: 21 ->553us, 64 -> 1684us
			//if (data & bitMask) pulse(21, 64); else pulse(21, 21); // note: 21 ->553us, 64 -> 1684us
			bitMask-- ;
		} while (bitMask>0);
		data = command;
	}
	// Last, an extra pulse at the end to terminate the last bit:
	pulse(21, 0);
}

/* Explanation of init() function:
Set the INT1 as falling edge:
1)  External Interrupt Control Register A (EICRA)
EICRA = [– – – – ISC11 ISC10 ISC01 ISC00]
The falling edge of INT1 generates an interrupt request: [ISC11 ISC10] = [1 0]

EICRA|=_BV(ISC11);

2) EIMSK – External Interrupt Mask Register: EIMSK = [– – – – – – INT1 INT0]
When the INT1 bit is set (one) and the I-bit in the Status Register (SREG) is set (one), the external pin //interrupt is enabled:

EIMSK |= _BV(INT1);

(note: interrupts should be enabled by default: sei, SREG|=(1<<7))
Another method:

pinMode (PIN_RCV, INPUT_PULLUP);
attachInterrupt (1, detectedCarrier, FALLING);
interrupts (); // enable interrupts (just in case). Equal to sei();

Note: for disabling: noInterrupts () or cli (); // clear interrupts flag

Setting TIMER1 to generate PWM, 50% duty cycle, at 38kHz (we will enable/disable OC1A - PB1 to create the pulses).
note: We can do without using interrupts, by using CTC mode or PWM mode.
**For generating a waveform output in CTC mode, the OC1A output can be set to toggle its logical level on each compare
match by setting the Compare Output mode bits to toggle mode (COM1A1:0 = 1). The OC1A value will not be visible on the
port pin unless the data direction for the pin is set to output (DDR_OC1A = 1)
The resulting PWM frequency is:
IR_CARRIER_FREQ = F_CLK / (2xNx(1+OCR1A)), N= prescaler (1, 8, 64, 256, or 1024)
** If using PWM mode, we can change the PWM cycle of OC1B (PB2), but remember that now this pin cannot be used (is on RFM69 module).

Reminder: TCCR1A and TCCR1B need to be set to:
- Waveform Generation Mode bits (WGM): these control the overall mode of the timer. (These bits are split between TCCRnA and TCCRnB.)
- Clock Select bits (CS): these control the clock prescaler
- Compare Match Output A Mode bits (COMnA): these enable/disable/invert output A
- Compare Match Output B Mode bits (COMnB): these enable/disable/invert output B

// I will use fast PWM mode, by setting ICR1 as TOP:
// The resulting PWM frequency is:
//  IR_CARRIER_FREQ = F_CLK / (Nx(1+TOP)), N= prescaler (1, 8, 64, 256, or 1024)
//   hence: TOP = IR_CARRIER_FREQ/(NxF_CKL)-1

// 1) MODE: Fast PWM, TOP= ICR1, and TOV1 Flag set on TOP, by doing: [WGM13, WGM12, WGM11, WGM10] = [1 1 1 0] (mode 14)
// TCCR1A = [COM1A1 COM1A0 COM1B1 COM1B0     –    –   WGM11 WGM10]
// TCCR1B = [ ICNC1  ICES1     –   WGM13 WGM12 CS12    CS11  CS10]
TCCR1A |=_BV(WGM11); TCCR1B |= (_BV(WGM13) | _BV(WGM12));
TCCR1B |= _BV(CS10); // [CS12 CS11 CS10] = [0 0 1], set prescaler to 1 (remember, TIMER1 is a 16 bit counter! ICR1 can count up to 0xFFFF or 65535)
ICR1 = 420; // = IR_CARRIER_FREQ/(1xF_CKL)-1;
OCR1A = 210;// Compare match
DDRB  |= (1<<DDB1); // set OC1A(PB1) as output (this is just equal to pinMode(PIN_SND, OUTPUT))
PORTB &= ~(1<<PB1); // set it as LOW for a start (digitalWrite(PIN_SND, LOW))
// 2) Compare Match Output A (for fast PWM):
// Normal port operation, OC1A/OC1B disconnected:
//          [COM1A1/COM1B1 COM1A0/COM1B0] = [0 0]
// Clear OC1A/OC1B on Compare Match, set OC1A/OC1B at BOTTOM (non-inverting mode):
// 			[COM1A1/COM1B1 COM1A0/COM1B0] = [1 0]
TCCR1A &=  ~(_BV(COM1A1)) & ~(_BV(COM1A0));

*/

/*
Alternative way to do the ISR (there would be still another one to try, using micros())
//ISR interrupt routine called when there is a FALLING edge on pin PD3 (INT1)
void ModuleIR::detectCarrier() {
	static boolean idleState = true;
	if (idleState) {
		idleState=false;
		NextBit = 32; newData=false;
		// raw32BitCode = 0; not changed, in case we use the repeat code
	} else {
		uint16_t Time = TCNT1; //  a time unit is 16us
		// Keep looking for AGC pulse and gap
		if (NextBit == 32) {

			// test:
			//raw32BitCode = Time; newData=true; idleState=true;

			//STARTING CODE (AGC) pulse and gap is within 10% of the required 13.5ms (844)
			if ((Time >= 760) && (Time <= 928) ) {
				// Start reading code:
				raw32BitCode = 0; NextBit = 0;
			}
			//REPEAT CODE: pulse and gap is within 10% of the 11.25ms width (703 ticks)
			else if ((Time >= 632) && (Time <= 773) ) {
				// raw32BitCode should not change...
				newData=true;
				NextBit = 32;idleState=true;
			}
		}
		else { // Data bit:
			// Data bit (1 is 2.25ms, 0 is 1.12ms - remember: we detect only the falling edges)
			// A 0-bit has a length of 1.12ms (70 ticks), a 1-bit has a length of 2.25ms (141 ticks).
			// The mean (105 ticks) is used as a threshold.
			if (Time > 200) {NextBit = 32; newData=false; idleState=true;} // Invalid - restart
			else {
				if (Time > 105) raw32BitCode = raw32BitCode | ((uint32_t)(1<<(31-NextBit)));
				if (NextBit == 31) {newData=true; idleState=true;}
				NextBit++;
			}
		}
	}
	// And always:
	TCNT1 = 0;        // Clear counter
	TIFR1 |= 1<<TOV1; // Clear overflow - note: clear by writing a ONE to TOV1!! (see datasheet)
	//EIFR – External Interrupt Flag Register = [– – – – – – INTF1 INTF0]
	EIFR |= 1<<INTF1;  // Clear INT1 flag (is this really necessary? interrupt flag is cleared at the end of the ISR?)
	// from Atmega328P datasheet (p.72): "The flag is cleared when the interrupt routine is executed. Alternatively,
	// the flag can be cleared by writing a logical one to it.
}
*/
