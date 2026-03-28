/*
Class for the ultrasonic rangefinder sensor SRF10 from Devantech, using I2C interface
Actually it should work for SRF02, 08 and 010.

Note: normally it should be powered with 5V... not sure it works with 3.7 or 3.3V

*/

#include <Wire.h>
#include "BaseSensor.h"

class SensorSRF10: public BaseSensor {
public:

	SensorSRF10() : BaseSensor("SRF10",ANALOG_OUTPUT) {}
	SensorSRF10(const char* _name) : BaseSensor (_name, ANALOG_OUTPUT) {}
	virtual ~SensorSRF10() override {}

	// Initializer: other things
	void init(bool firstTimeBuild) {
		if (firstTimeBuild) {
			// Save on memory the initial sensor(s) conditions (if needed!!):
			setConditions(15, 2); //15 cm, +/-2cm
			saveConditions();
		} else {
			loadConditions(); // in general, we will do READ FROM EEPROM (since the very first time they will be written - see Gluon setup)
		}

		setDetectionMode(ON_ENTERING_COND); // meaning that we go from a value not in range, to a value in range.
		enableLearning(); // we set default condition range, but enable learning (using IR or button)

		// SRF10 initialization:
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

uint16_t SensorSRF10::readAnalogOutput() { // return distance in cm
	uint16_t reading=0;

	// step 1: instruct sensor to read echoes
     Wire.beginTransmission(112); // transmit to device #112 (0x70)
     // the address specified in the datasheet is 224 (0xE0)
     // but i2c adressing uses the high 7 bits so it's 112 (0x70)
     Wire.write(byte(0x00));      // sets register pointer to the command register (0x00)
     Wire.write(byte(0x51));      // command sensor to measure in cm, inches or microsec
	 // use 0x50 for inches, 0x51 for centimeters and 0x52 for ping microseconds
     Wire.endTransmission();      // stop transmitting

     // step 2: wait for readings to happen
     delay(70); // datasheet suggests at least 65 milliseconds

     // step 3: instruct sensor to return a particular echo reading
     Wire.beginTransmission(112); // transmit to device #112
     Wire.write(byte(0x02));      // sets register pointer to echo #1 register (0x02)
     Wire.endTransmission();      // stop transmitting

     // step 4: request reading from sensor
     Wire.requestFrom(112, 2);    // request 2 bytes from slave device #112

     // step 5: receive reading from sensor
     if (2 <= Wire.available()) { // if two bytes were received
       reading = Wire.read();  // receive high byte (overwrites previous reading)
       reading = reading << 8;    // shift high byte to be high 8 bits
       reading |= Wire.read(); // receive low byte as lower 8 bits
     }

	 return(reading);
}

/*

// The following code changes the address of a Devantech Ultrasonic Range Finder (SRF10 or SRF08)
// usage: changeAddress(0x70, 0xE6);

void changeAddress(byte oldAddress, byte newAddress)
{
  Wire.beginTransmission(oldAddress);
  Wire.write(byte(0x00));
  Wire.write(byte(0xA0));
  Wire.endTransmission();

  Wire.beginTransmission(oldAddress);
  Wire.write(byte(0x00));
  Wire.write(byte(0xAA));
  Wire.endTransmission();

  Wire.beginTransmission(oldAddress);
  Wire.write(byte(0x00));
  Wire.write(byte(0xA5));
  Wire.endTransmission();

  Wire.beginTransmission(oldAddress);
  Wire.write(byte(0x00));
  Wire.write(newAddress);
  Wire.endTransmission();
}

*/
