#include "BaseActuator.h"

  // ... not much for the time being. But we can have more or less complex functions, like timers for actuating during a certain amount of time, etc.

void BaseActuator::dump() {
	LOG_PRINT(F("Name ")); LOG_PRINT_CSTRING(myPersonalString);
	myData.dataDump();
	//LOG_PRINTLN(F("---------- End actuator data dump ---------------"));
}
