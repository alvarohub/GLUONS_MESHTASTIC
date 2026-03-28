#include "Arduino.h"
#include "BaseLogic.h"



void BaseLogic::setUpdateMode(uint8_t _updateModeMask)
{
	// ATTN: the way the packing of bitfields is done is compiler dependent (little/big endianness...!), so just doing an assignement, like:
	//  updateModeMask = SENSOR_EVENT_BANG |  SYNCH_BANG, may not work properly.

	//To be completely sure, we need then to set things one by one (and I will declare updateModeMask private to avoid possible errors)
//  uint8_t sensorEventBang:1, firstInletBang:1, anyInletBang:1, synchBang:1, periodicBang:1;
	updateModeMask.bitValue.sensorEventBang = ( _updateModeMask & SENSOR_EVENT_BANG ) > 0;
	updateModeMask.bitValue.firstInletBang = (_updateModeMask & FIRST_INLET_BANG) > 0;
	updateModeMask.bitValue.anyInletBang = ( _updateModeMask & ANY_INLET_BANG) > 0;
	updateModeMask.bitValue.synchBang = ( _updateModeMask & SYNCH_BANG) > 0;
	updateModeMask.bitValue.periodicBang = ( _updateModeMask & PERIODIC_BANG) > 0;

	// Saved in EEPROM:
	myMemory.saveUpdateMode(updateModeMask.byteValue);

}

void BaseLogic::toggleUpdateMode(uint8_t _mode) // again, since we cannot be sure of the packing of bit fields, we need to do this horror:
{
	if (_mode == SENSOR_EVENT_BANG) updateModeMask.bitValue.sensorEventBang = 1 - updateModeMask.bitValue.sensorEventBang;
	else if (_mode == FIRST_INLET_BANG) updateModeMask.bitValue.firstInletBang = 1 - updateModeMask.bitValue.firstInletBang;
	else if (_mode == ANY_INLET_BANG) updateModeMask.bitValue.anyInletBang = 1 - updateModeMask.bitValue.anyInletBang;
	else if (_mode == SYNCH_BANG) updateModeMask.bitValue.synchBang = 1 - updateModeMask.bitValue.synchBang;
	else if (_mode == PERIODIC_BANG) updateModeMask.bitValue.periodicBang = 1 - updateModeMask.bitValue.periodicBang;

	dump();

	// Save in EEPROM:
	myMemory.saveUpdateMode(updateModeMask.byteValue);

	delay(200); // this is just to avoid toggling to fast

}

void BaseLogic::update()
{

	/* 1) Evolve (no event-based update - check sensors for parameter change, or generate metro signals, increment delay update, etc). If the specific sensor logic does not define the method, the base class method does... nothing. */
	evolve();

	// Serial.print("sensor event bang bit: "); Serial.println(updateModeMask.bitValue.sensorEventBang, BIN);
	// if (sensorArrayPtr->checkEvent()) {Serial.print("Sensor Event: ");Serial.println(sensorArrayPtr->checkEvent(), BIN);}

	// 2) EVENT BASED UPDATE:
	if (   ( updateModeMask.bitValue.sensorEventBang && sensorArrayPtr->checkEvent() )
	       || ( updateModeMask.bitValue.firstInletBang  && inletArrayPtr->isBangFirstInlet() )
	       || ( updateModeMask.bitValue.anyInletBang && inletArrayPtr->isSomeBang() )
	       || ( updateModeMask.bitValue.synchBang  && inletArrayPtr->isSynchData() )
	       || ( updateModeMask.bitValue.periodicBang) ) {
		// note: we can do also: (updateModeMask.b0 == SENSOR_EVENT_BANG)...

		/* NOTE: it would be nice to DELAY the computation to avoid blocking network "loops". The problem with this is that
		 in the next cycles, all the values (on sensors or inlets) would have changed. What we need to do instead, is to
		 delay the SENDING of the result on the outlet. Of course, we have to take into account the fact that IF during that
		 cycle, another computation is triggered, there will be some conflicting results.

		 What we can do is that we send the LATEST computation anyway, but never at full speed, but the best alternative would
		 be to have a QUEUE with computed data to send with a programmable delay. This implies storing an array of DATA elements in the Outlet class (the data elements already have a time, but we need to add the time of computation!), and consume it adding some delay. Which of course will mean the queue can SATURATE if for instance the sensors or inlets receive data too fast. How to deal with this is interesting and a REAL problem in the world of networking (TODO), but for our purpose, we can just use another strategy (IDEA):
		 HAVE A MINIMUM PERIOD FOR SENDING THE OUTLET PACKETS. Not ideal of course, but if the period is small enough, it will be ok and at the same time will avoid loop "dead-locks".

		 This period (I prefer to call it period instead of delay because we won't add the delay to the time of computation) should be a variable of Outlet or NodeClass (and changeable by NodeClass perhaps), not BaseLogic. (In case of using a queue, the queue of course should be in the Outlet class. But then the sending control is a matter of the NodeClass).
		 This mistake took me hours to understand - the "event" value of a rotary encoder was never sent as true (it triggered a scheduling of a computation, but when it was done, the sensor itself had a "false" value on the Data field).
		 */

		compute();
		inletArrayPtr->setAllInletsDataUsed();// very important to set the inlet data as used!
	}
}

void BaseLogic::forceUpdate() // can be called by the IR remote or the SNIFFER.
{
	compute();
	inletArrayPtr->setAllInletsDataUsed(); // very important to set the inlet data as used!
}

void BaseLogic::dump()
{
	LOG_PRINTLN(F("Logic"));
	LOG_PRINT(F("Name ")); LOG_PRINTLN(myPersonalString);
	LOG_PRINT(F("Bang mode: "));
	if (updateModeMask.byteValue == ONLY_MANUAL_BANG) LOG_PRINT(F("MANUAL"));
	else {
		if ( updateModeMask.bitValue.sensorEventBang) LOG_PRINT(F("SENSOR"));
		if (  updateModeMask.bitValue.firstInletBang ) LOG_PRINT(F(" | 1st INLET"));
		if (updateModeMask.bitValue.anyInletBang ) LOG_PRINT(F(" | ANY INLET"));
		if (updateModeMask.bitValue.synchBang) LOG_PRINT(F(" | SYNCH"));
		if (updateModeMask.bitValue.periodicBang) LOG_PRINT(F(" | PERIODIC"));
	}
	LOG_PRINTLN("");
 // LOG_PRINTLN(F("---------- End logic unit dump ---------------"));
}
