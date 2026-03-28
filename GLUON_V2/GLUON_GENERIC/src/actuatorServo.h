/*
 SERVO MOTORS (using servo library). The servo can be attached to any pin, not just pwm.
  By the way, on boards other than the Mega, use of the library disables analogWrite() (PWM) functionality on pins 9 and 10, whether or not there is a Servo on those pins. This is ok for pin 10 (since it is used by the RFM module), and since pin 9 pwm capabilities are disabled, we better use it so as not to generate confusion.
*/


#include "Arduino.h"
#include "actuatorBaseAnalog.h"

#include <Servo.h>

class ServoMotor : public BaseActuatorAnalog {
public:

	// Parameter-less constructor:
	ServoMotor() : BaseActuatorAnalog("Servo", UPDATE_ALWAYS) {
		myServo = new Servo();
	}
	virtual ~ServoMotor() override { delete myServo;}

	void init(bool firstTimeBuild) {
		BaseActuatorAnalog::init(firstTimeBuild);
		myServo->attach(9); // attach always on pin 9
		setMinMax(0, 180);
		setActuator(0);
	}

	virtual void setActuator(int16_t value) override {myServo->write(value);}

protected:

	Servo* myServo;
};
