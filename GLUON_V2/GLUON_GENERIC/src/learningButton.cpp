
#include "learningButton.h"

LearningButton myLearningButton;

LearningButton::LearningButton() : SensorDiscreteValues("LEARN", NUM_LEVELS_LEARNING_BUTTON, 3.3, 0, LEARNING_BUTTON_PIN) {
	// We will read from 0 to numValues, and we want to detect a change every step - tolerance value is inclusive).

	SensorDiscreteValues::init(); // this is similar to the thing below:

	//setToleranceValue(1);  // Condition value not needed, only tolerance.
	//setDetectionMode(ON_SIMPLE_CHANGE);     // REM: on simple change with respect to the DISCRETIZED output of course.
	//disableLearning(); // disabled by default in the base class

    disable(); // Disabled by default (this is different from disableLearning: it disables the SENSOR). 
}
