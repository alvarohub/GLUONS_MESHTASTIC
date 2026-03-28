#include "Arduino.h"

/*
	COMPILE: as ARDUINO UNO board
	VERSION 1.0:
        - using Moteino R4 / RFM69W (433MHz) / 4Mbit flash chip.
 */

#define GLUON 6
//#define FIRST_TIME_BUILD 1 // this should be 0, or 1 for the first time we load the program

const uint8_t FIRST_TIME_BUILD = 0;

// ========================================================
// Sensor, actuators and logic: for the time we can have #define CODES for populating them,
// but in the future we can use a JSON file on the external FLASH or something like that.

// Module for communication tests (NO SENORS OR ACTUATORS)
#if GLUON == 0
#define MY_NODE_ID          100
#define MY_NAME             "TEST"

#elif GLUON == 1
#define MY_NODE_ID          1
#define MY_NAME             "RANGE" // IR RANGEFINDER

#elif GLUON == 2
#define MY_NODE_ID          2
#define MY_NAME             "BUZZ" // LED + BUZZER + MOTOR

#elif GLUON == 3
#define MY_NODE_ID          3
#define MY_NAME             "DIAL" // DISCRETE POT

#elif GLUON == 4
#define MY_NODE_ID          4
#define MY_NAME             "COUNT" // DISPLAY

#elif GLUON == 5
#define MY_NODE_ID          5
#define MY_NAME             "SWITCH" // BUTTON SWITCH & RGB LED

#elif GLUON == 6
#define MY_NODE_ID          6
#define MY_NAME             "METRO" // TOGGLE SWITCH + SET SWITCHES & DISPLAY + BEAT LED + TOGGLE LED

// NOT YET PHYSICALLY IMPLEMENTED (but code ready)
#elif GLUON == 7
#define MY_NODE_ID          7
#define MY_NAME             "LDR" // light-dependent resistor

/*
Others (no physical nor software implementation)
	- typical digital /analog electronic elements: logical gates, flip flop, schmitt trigger, etc
	- delay
	- matrix
	- all kind of sensors (gas, humidity, accelerometer, gyro, etc)
	- GPS
	- "FAT" modules and complex functions: line follower, deep learning (check M3 micromote), FFT, signal analyser (edge triggers, oscilloscope...)
*/

#endif

// ========================================================

#include <Wire.h>
#include <EEPROM.h>
#include <Servo.h>
#include <SPI.h>
#include <RFM69.h>  // NOTE: the module used in this first batch is RFM69W (433MHz)

#include "utils.h"

// Create an instance of the class Gluon
#include "NodeClass.h"
Node myGluon; // could be preinstantiated, but I put it here for clarity.


// ======== SELECT SENSOR, ACTUATOR, and LOGIC =================
// NOTE: I can put here all the includes (in principle it won't take memory if the objects are not used)

// a) SENSORS:
#include "sensorSwitch.h"
#include "sensorSHARP.h"
#include "sensorSRF10.h"
#include "sensorMaxSonar.h"
#include "sensorLDR.h"
#include "sensorTemp.h"
#include "sensorAnalogPot.h"
#include "sensorAnalogRotaryEncoder.h"
//#include "sensorRotaryEncoder.h"
#include "sensorDiscreteValues.h"

// b) ACTUATORS:
#include "commonDigitalActuators.h"
#include "commonAnalogActuators.h"
#include "actuatorServo.h"
#include <Grove_LED_Bar.h>
#include "actuatorLedBar.h"
// #include "actuator4AlphaNum.h" <- not yet finished
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"
#include "actuatorSevenSeg.h"
#include "actuatorSevenSeg_MAX7221.h"

// c) LOGIC MODULE:
#include "testLogic.h" // <<-- Mainly for testing connections, etc. It is a kind of a logic code template
#include "logicAnalogSensor.h" // RANGE FINDER (N1), LDR (N7), etc
#include "logicN2.h" // BUZZER
#include "logicN3.h" // DIAL
#include "logicN4.h" // COUNTER
#include "logicN5.h" // SWITCH
#include "logicMetro.h" // METRO

// Daniel Modules:
// #include "logicD1.h"
// #include "logicD2.h"
// #include "logicD6.h"
// #include "logicD7.h"

//#include <avr/pgmspace.h>
// ************************************************************************************************************************

void setup()
{
#if defined(DEBUG_MODE_FULL) || defined(DEBUG_MODE)
	Serial.begin(57600);
#endif

	// ====================== Prepare THIS gluon =======================

	// 1) First, set THIS node ID and a its defining name (really no need to do this from EEPROM)
	myGluon.setNodeId(MY_NODE_ID); // from 0 to 253 (255 is for broadcast in RFM module, and 254 reserved for the SNIFFER)
	myGluon.setString(MY_NAME);    // this will be used in the future to pass complex messages (type of gluon, etc)


#if (FIRST_TIME_BUILD)
	// The first time we build the gluon we need to save on EEPROM the some basic things: number of inlets and outlets:
	myGluon.createInlets(3);        // inlets (for the time being, the fan-in is fixed to 1 - see utils.h)
	myGluon.createOutlets(1);       // outlets (for the time being, the fan-out is fixed to 4 - see utils.h)
	myGluon.disconnectEverything(); // we could have methods to start with particular links active.
	myGluon.saveConnexions();       // Save interconnection structure to EEPROM
#else
	myGluon.loadConnexions();       // this will automatically populate the inlets/outlets arrays, and also remember the previous active links.
#endif

	// ==========================================================================================================
	// A) SETTING SENSORS(S)
	// ONLY ANALOG PINS: A6 and A7. A7 will generally be used for the GENERAL sensor control of the gluons (such
	// as the learning button). However, in the future it would be better to use A5 for control (so we can take
	// advantage of the internal pullups)
	// ==========================================================================================================

#if GLUON == 0	// MODULE 0: **TEST** CONNECTION MODULE
	myLearningButton.disableLearning();
	myTiltSensor.enable();

#elif GLUON == 1 // MODULE 1: RANGEFINDER (ANALOG) & LED
	SensorSharp* mySensorPtr = new SensorSharp("RANG", A6); //"RANGE", A6);
	myGluon.addSensor(mySensorPtr);
    // Analog sensor: we need to enable the learning button (it is disabled by default). NOTE: perhaps this can be done in he Gluon init method, after checking the type of sensor (sensor class could indicate the need to enable/disable the button in case learning is needed - the variable exist)
    myLearningButton.enable(); // enable button sensor (disabled by default)

	// tilt sensor can be enabled or disabled per module or before the global init. It is enabled by default
    //myTiltSensor.disable();

#elif  GLUON == 2 // MODULE 2: LED + BUZZER + MOTOR (no sensor)
	// Also, we need to disable the learning button (it is disabled by default)
	// myLearningButton.disable();
	// NOTE: learning button could be disabled every time there is NO SENSOR on the module, but since
	// this button can be used for something else, it is better to do it case by case.

#elif GLUON == 3 // MODULE 3: ANALOG ROTARY ENCODER:
	// Using discrete steps potentiometer:
	SensorAnalogRotaryEncoder* rotarySensor = new SensorAnalogRotaryEncoder("DIAL", A0);
	myGluon.addSensor(rotarySensor);

	// Using real ISR based rotary encoder, with pushbutton (not fully tested)
	//SensorRotaryEncoder* rotarySensor = new SensorRotaryEncoder("DIAL", A0, A1);
	//myGluon.addSensor(rotarySensor);
	//SensorSwitch* pushButtonSensor = new SensorSwitch("BUTTON", A2); // potentiometer with pushbutton!
	//myGluon.addSensor(pushButtonSensor);

    //myTiltSensor.disable();

#elif GLUON == 4 // MODULE 4: COUNTER (no analog sensor)
	//myLearningButton.disable(); // disabled by default
    //myTiltSensor.disable();

#elif GLUON == 5	// MODULE 5: DIGITAL BUTTON (no analog sensor)
	SensorSwitch* mySensorPtr = new SensorSwitch("SW", A0);
	myGluon.addSensor(mySensorPtr);
	//myLearningButton.disable(); // disabled by default
	//myTiltSensor.disable(); // enabled by default

#elif GLUON == 6	//MODULE 6: DIGITAL ON/OFF METRO BUTTON (the threeSwitch is analog, but we don't need to enable the learning BUTTON)
	SensorSwitch* toggleMetro = new SensorSwitch("SW1", A0);
	myGluon.addSensor(toggleMetro);

	//SensorDiscreteValues* threeSwitch = new SensorDiscreteValues("SW", 3, 3.3, 0, A6);
	//myGluon.addSensor(threeSwitch);

	//myLearningButton.disable(); // disabled by default
	//myTiltSensor.disable(); // enabled by default

#elif GLUON == 7	//MODULE 7: LIGHT SENSOR (LDR)
		SensorLDR* ldrSensor = new SensorLDR("LDR", A6);
		myGluon.addSensor(ldrSensor);
#endif

	// OTHERS:
	// SensorAnalogPot* mySensorPtr = new SensorAnalogPot();
	// SensorSRF10* mySensorPtr = new SensorSRF10();
	// SensorMaxSonar* mySensorPtr = new SensorMaxSonar();
	// SensorLDR* mySensorPtr = new SensorLDR();
	// SensorTemperature* mySensorPtr = new SensorTemperature();                 // ANY (ex: module N2)

	//=========================================================================================================
	// B) ACTUATOR(S)
	//=========================================================================================================
#if GLUON == 0	// MODULE 0: **TEST** CONNECTION MODULE

#elif GLUON == 1 // MODULE 1: RANGEFINDER & LED
    BaseActuatorDigitalPin *ledBlue = new BaseActuatorDigitalPin("B", A0, PULSE);
    //ActuatorLedPulse *ledBlue = new ActuatorLedPulse("B", A0);
	myGluon.addActuator(ledBlue);

#elif GLUON==2	// MODULE 2: LED + BUZZER + MOTOR

    BaseActuatorDigitalPin  *buzzer = new BaseActuatorDigitalPin("B", A1, PULSE);
    myGluon.addActuator(buzzer);

    BaseActuatorDigitalPin  *ledWhite = new BaseActuatorDigitalPin("W", A0, PULSE);
    myGluon.addActuator(ledWhite);

    // MOTOR: let's make it an ANALOG actuator (using numeric data field for the strength) or just a digital one:
    //BaseActuatorAnalogPin *motor = new BaseActuatorAnalogPin("M",PULSE_ON_TRUE_EVENT); // <-- ONLY ON PIN 9!!! but cannot be used
    BaseActuatorDigitalPin  *motor = new BaseActuatorDigitalPin("M", A2, PULSE);
    motor->setDuration(3000); // TODO: child class specific for a motor. Also: put a transistor...

    myGluon.addActuator(motor);

    // ActuatorLedPulse *ledWhite = new ActuatorLedPulse("W", A2);
    // myGluon.addActuator(ledWhite);
    // ActuatorBuzzer* buzzer = new ActuatorBuzzer("BUZZ", A1);
	// myGluon.addActuator(buzzer);
    // ActuatorBuzzer* motor = new ActuatorBuzzer("MOT", A0);
	// myGluon.addActuator(motor);

#elif GLUON==3 // MODULE 3: ROTARY ENCODER

	// * DIGITAL: no actuator!

	// * ANALOG: RG or RGB leds on the potentiometer (not tested)
	// ActuatorLedPulse *LED_red = new ActuatorLedPulse("red", A3);
	// myGluon.addActuator(LED_red);
	// ActuatorLedPulse *LED_blue = new ActuatorLedPulse("blue", A4);
	// myGluon.addActuator(LED_blue);

#elif GLUON==4	// MODULE 4: COUNTER (uses I2C pins: A5 and A4)
	DisplaySevenSegment_I2C* myActuatorPtr = new DisplaySevenSegment_I2C("DISP");     // MODULE N4
	myGluon.addActuator(myActuatorPtr);

#elif GLUON==5	// MODULE 5: BUTTON with RGB leds
    BaseActuatorDigitalPin *LED_red = new BaseActuatorDigitalPin("R", A1, PULSE);
	myGluon.addActuator(LED_red);
	BaseActuatorDigitalPin *LED_green = new BaseActuatorDigitalPin("G", A2, PULSE);
	myGluon.addActuator(LED_green);
	BaseActuatorDigitalPin *LED_blue = new BaseActuatorDigitalPin("B", A3, PULSE);
	myGluon.addActuator(LED_blue);

	// ActuatorLedPulse *LED_red = new ActuatorLedPulse("R", A1);
	// myGluon.addActuator(LED_red);
	// ActuatorLedPulse *LED_green = new ActuatorLedPulse("G", A2);
	// myGluon.addActuator(LED_green);
	// ActuatorLedPulse *LED_blue = new ActuatorLedPulse("B", A3);
	// myGluon.addActuator(LED_blue);

#elif GLUON==6	// MODULE 6: METRO
    BaseActuatorDigitalPin *Beat_Led = new BaseActuatorDigitalPin("SW1", A1, PULSE);
	//ActuatorLedPulse *Beat_Led = new ActuatorLedPulse("SW1", A1);  // PULSE MODE
	Beat_Led->setDuration(7); // problematic: this should be changeable as BMP changes (but I need to do a dynamic_cast, and it seems not allowed on ARM toolchain ('dynamic_cast' not permitted with -fno-rtti)
	myGluon.addActuator(Beat_Led);

    //BaseActuatorDigitalPin *ToogleMetro_Led = new BaseActuatorDigitalPin("SW1", A2, NORMAL);
    //ActuatorLedNormal *ToogleMetro_Led = new ActuatorLedNormal("SW2", A2);        // NORMAL MODE
	//myGluon.addActuator(ToogleMetro_Led);

// **********  not adding the display to see if this is the cause of the memory overlaod:
	//DisplaySevenSegment_MAX7221 *display = new DisplaySevenSegment_MAX7221(3, A3, A4, A5);
	//rem: DisplaySevenSegment_MAX7221(uint8_t _numDigits, uint8_t _toDIN, uint8_t _toCLK, uint8_t _toCS);
	//myGluon.addActuator(display);

#elif GLUON==7
// MODULE 7: LDR & LED
BaseActuatorDigitalPin *ledBlue = new BaseActuatorDigitalPin("B", A0, PULSE);
//ActuatorLedPulse *ledBlue = new ActuatorLedPulse("blue", A0);
myGluon.addActuator(ledBlue);

#endif

	// OTHERS:
	//ledBar* myActuatorPtr = new ledBar();
	// AnalogBeeper* myActuatorPtr = new AnalogBeeper();

	// TO DO:
	//ServoMotor* myActuatorPtr= new ServoMotor();

	// ==========================================================================================================
	// C) LOGIC
	// IMPORTANT: setLogic should be called AFTER setting sensors and actuators!!!
	//===============================================================================================================

#if GLUON == 0
	TestLogic* myLogic = new TestLogic();
#elif GLUON == 1
	logicAnalogSensor* myLogic = new logicAnalogSensor();
#elif GLUON ==2
	logicN2* myLogic = new logicN2();
#elif GLUON ==3
	logicN3* myLogic = new logicN3();
#elif GLUON ==4
	logicCounter* myLogic = new logicCounter();   // MODULE N4 (counter)
#elif GLUON ==5
	logicN5* myLogic = new logicN5();
#elif GLUON ==6
	logicMetro* myLogic = new logicMetro();
#elif GLUON == 7
	logicAnalogSensor* myLogic = new logicAnalogSensor();
#endif

	myGluon.setLogic(myLogic);      // can do too myGluon.setLogic(new SynchLogicAnd())

	//===============================================================================================================
	//D) INITIALIZATION:
	myGluon.init(FIRST_TIME_BUILD); // <- this calls init for IR module, RF module, LedIndicator, all the sensors (set or load conditions) and actuators.
	//===============================================================================================================

}

//===============================================================================================================
// Infinite loop:
void loop()
{
	myGluon.update();
}
