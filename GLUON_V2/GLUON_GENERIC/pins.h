// ====================================== PINOUT ============================================
// Analog pins from A0 to A5 can be used as digital I/O, but A5 and A4 are used for I2C.
//  In general, I will try to use A6 and A7 for analog input, A1 for digital input, and A2 for digital output. A0 will depend on the hardware.
// Analog (PWM) are D5, D6 and D9
// Digital i/o for the led shift register are D7, D8 and A3

// ================================== "DIGITAL SIDE" ========================================
// D0 is RX
// D1 is TX
// D2 is for INT0, used by the RFM module

// D3:
#define PIN_RCV 3 // PD3, INT1: An IR detector/demodulator must be connected to this pin

// D4 :
#define tiltPin 4

// D5 (PWM):
#define PIN_VIBRATOR 5  // NOTE: this is a pity, it is the ONLY PWM pin accessible for analog actuators... I should put this on pin D8, but
// I don't have time to test now if it interferes with the SPI FLASH *** TODO!!! ***

// D6 (PWM):  CAN USE TONE (for the time being, I only use digitalWrite.. but can I use tone() method? check timers use)
#define PIN_CHIRP_SPEAKER 6

// D7:
#define PIN_DI_NEO	7

// D8:
// Used by the SPI FLASH CS, but I can use it for other things? what happens if I activate it? will it interfere with RFM // module? (D11 = MISO, D12 = MOSI, D13 = SCL)

// D9 (PWM):
#define PIN_SND 9 // PB1, D9, OC1A: the IR sender LED should be connected here. Moteinos have a built in LED on D9

// D10 to D13 are used to communicate with the RFM module (SPI interface)
// D10 = RFM CS (SS is the SPI slave select pin, for instance D10 on ATmega328)
// D11 = MISO, D12 = MOSI, D13 = SCL

// ================================== "ANALOG SIDE" ========================================

// A0:
//   Actuators:
// GLUON 1: ActuatorLedPulse *ledBlue = new ActuatorLedPulse("blue", A0);
// GLUON 2: ActuatorBuzzer* buzzer = new ActuatorBuzzer("BUZZER", A0);
//   Sensors:
// GLUON 3: SensorAnalogRotaryEncoder* mySensorPtr = new SensorAnalogRotaryEncoder("DIAL", A0);
// GLUON 5: SensorSwitch* mySensorPtr = new SensorSwitch("BUTTON", A0);
// GLUON 6: SensorSwitch* toggleMetro = new SensorSwitch("OnOff", A0);
// GLUON 7: ActuatorLedPulse *ledBlue = new ActuatorLedPulse("blue", A0);

// A1:
// GLUON 2: ActuatorLedPulse *ledWhite = new ActuatorLedPulse("white", A1);
// GLUON 5: ActuatorLedPulse *LED_red = new ActuatorLedPulse("red", A1);
// GLUON 6: ActuatorLedPulse *Beat_Led = new ActuatorLedPulse("beat", A1);  // PULSE MODE

// A2:
// GLUON 5: ActuatorLedPulse *LED_green = new ActuatorLedPulse("green", A2);
// GLUON 6: ActuatorLedNormal *ToogleMetro_Led = new ActuatorLedNormal("metro", A2);        // NORMAL MODE

// A3:
// SENSOR (general):
// GLUON 5: ActuatorLedPulse *LED_blue = new ActuatorLedPulse("blue", A3);
// GLUON 6: DisplaySevenSegment_MAX7221 *display = new DisplaySevenSegment_MAX7221(3, A3, A4, A5);

// I2C on the Arduino (or Moteino):
//   A4 (analog 4) is the data (SDA)
//   A5 (analog 5) is the clock (SCL)

// A6 and A7 are ONLY analog inputs
// A6:
// GLUON 1: SensorSharp* mySensorPtr = new SensorSharp("RANGE", A6);
// GLUON 7: SensorLDR* ldrSensor = new SensorLDR("LDR", A6);

// A7:
#define LEARNING_BUTTON_PIN A7
