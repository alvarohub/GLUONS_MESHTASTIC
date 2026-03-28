/*
Class for the ultrasonic rangefinder sensor I2C-XL from MaxSonar/MaxBotix, using I2C interface
Check here: http://www.maxbotix.com/articles/028.htm#arduino
Pin 5 to SCL
Pin 4 to SDA
Requires pull‑ups for SCL and SDA connected to 5V to work reliably
*/

//The Arduino Wire library uses the 7-bit version of the address, so the code example uses 0x70 instead of the 8‑bit 0xE0
#define SensorAddress byte(0x70)
#define RangeCommand byte(0x51)
//These are the two commands that need to be sent in sequence to change the sensor address
#define ChangeAddressCommand1 byte(0xAA)
#define ChangeAddressCommand2 byte(0xA5)

#include <Wire.h>
#include "BaseSensor.h"

class SensorMaxSonar: public BaseSensor {
public:

	SensorMaxSonar() : BaseSensor("I2CXL",ANALOG_OUTPUT) {}
	SensorMaxSonar(const char* _name) : BaseSensor (_name, ANALOG_OUTPUT) {}
	virtual ~SensorMaxSonar() override {}

	// Initializer: other things
	void init(bool firstTimeBuild) {
		if (firstTimeBuild) {
			// Save on memory the initial sensor(s) conditions (if needed!!):
			setConditions(100, 10); //100 cm, +/-10cm
			saveConditions();
		} else {
			loadConditions(); // in general, we will do READ FROM EEPROM (since the very first time they will be written - see Gluon setup)
		}

		setDetectionMode(ON_ENTERING_COND); // meaning that we go from a value not in range, to a value in range.
		enableLearning(); // we set default condition range, but enable learning (using IR or button)

		// Wire initialization:
		Wire.begin();    // join i2c bus (address optional for master)
	}

	// Overriding implementation of the base class virtual function:
	//virtual void updateData() override ;
	virtual uint16_t readAnalogOutput() override;

private:
	//pins (I2C pins)
	//static const uint8_t clockPin=A5;
	//static const uint8_t dataPin=A4;

	uint16_t distanceCm();

};

uint16_t SensorMaxSonar::readAnalogOutput() { // return distance in cm
	uint16_t reading=0;

	// step 1: start reading:
	Wire.beginTransmission(SensorAddress);     //Start addressing
    Wire.write(RangeCommand);                  //send range command
	Wire.endTransmission();                    //Stop and do something else now

     // step 2: wait for readings to happen
     delay(100);

     // step 3: instruct sensor to return a particular echo reading
	 Wire.requestFrom(SensorAddress, byte(2));

	if (Wire.available() >= 2) {                       //Sensor responded with the two bytes
           byte HighByte = Wire.read();                  //Read the high byte back
           byte LowByte = Wire.read();                   //Read the low byte back
        	reading = word(HighByte, LowByte);         //Make a 16-bit word out of the two bytes for the range
	  } // otherwise return 0

	 return(reading);
}

/*
Commands a sensor at oldAddress to change its address to newAddress
oldAddress must be the 7-bit form of the address that is used by Wire
7BitHuh determines whether newAddress is given as the new 7 bit version or the 8 bit version of the address
\ If true, if is the 7 bit version, if false, it is the 8 bit version

void changeAddress(byte oldAddress, byte newAddress, boolean SevenBitHuh){
       Wire.beginTransmission(oldAddress);                 //Begin addressing
       Wire.write(ChangeAddressCommand1);              //Send first change address command
       Wire.write(ChangeAddressCommand2);              //Send second change address command

       byte temp;
       if(SevenBitHuh){ temp = newAddress << 1; }     //The new address must be written to the sensor
       else     { temp = newAddress;         }               //in the 8bit form, so this handles automatic shifting
       Wire.write(temp);                                          //Send the new address to change to
       Wire.endTransmission();
}

As a minor note, in old Arduino IDE's
Wire.read() should be substituted by Wire.receive()
and
Wire.write() should be substituted by Wire.send()

*/
