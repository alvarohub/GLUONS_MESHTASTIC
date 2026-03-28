#ifndef _learning_button_H
#define _learning_button_H

#include "Arduino.h"
#include "sensorDiscreteValues.h"

#define NUM_LEVELS_LEARNING_BUTTON 3
#define LEARNING_BUTTON_PIN A7

class LearningButton : public SensorDiscreteValues {
public:
	LearningButton();

	// More specific methods:
	// TODO: other methods could for instance trigger callback functions depending on the value of the button.
	// TODO: make a class for all the "common" sensors.

private:

};

extern LearningButton myLearningButton;

#endif
