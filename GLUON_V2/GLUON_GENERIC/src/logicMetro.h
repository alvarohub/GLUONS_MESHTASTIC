
#include "BaseLogic.h"

class logicMetro: public BaseLogic {

public:
	logicMetro() : BaseLogic("M") {}

	void init(bool firstTimeBuild)
	{
		if (firstTimeBuild) {
			beatsPerMin = 60; // 1 per second
			metroOnOff = false;

			setUpdateMode(ANY_INLET_BANG);  // Saved in EEPROM
			setMetroBMP(beatsPerMin);       // this also saves on EEPROM
			setMetro(metroOnOff);           // this also saves on EEPROM

		}else  {
			loadUpdateMode();
			beatsPerMin = myMemory.loadData(0);
			metroOnOff = (myMemory.loadData(1) > 0);
		}

		lastBeat = millis();
	}

	virtual void compute() override;
	virtual void evolve() override;

private:

	void setMetroBMP(uint16_t _bmp)
	{
		beatsPerMin = constrain(_bmp, 0, 999);;

		// Save on EEPROM:
		myMemory.saveData(0, beatsPerMin);

		// Set beat period on the beat LED (actuator 0)
		(*actuatorArrayPtr)[0].setDuration(60000.0/beatsPerMin/2); // duration of the pulse led
		//ActuatorLedPulse* myLedPulse = dynamic_cast<ActuatorLedPulse*>( &( (*actuatorArrayPtr)[0]) );
		//myLedPulse->setDuration(60000.0/beatsPerMin/2); // duration of the pulse led

		// Set the 7 seg display:
		//Data outputData;
		//outputData.set("bpm", beatsPerMin, true, millis());
		//(*actuatorArrayPtr)[2].setNewData(outputData);

		lastBeat = millis();
	}

	void setMetro(bool _mode)
	{
		metroOnOff = _mode;

		// Save on EEPROM:
		myMemory.saveData(1, (metroOnOff ? 1 : 0));

		lastBeat = millis(); // set the beat timer

		// set toogle LED (actuator 1), which needs to be a led (digital actuator) in NORMAL mode, not PULSE mode
		Data outputData;
		outputData.set("metro", (metroOnOff ? 255 : 0),  metroOnOff, millis());
		(*actuatorArrayPtr)[1].setNewData(outputData);
	}

	void toggleMetro()
	{
		setMetro(!metroOnOff);
		delay(100); // to avoid fast toggling
	}

private:
	bool metroOnOff;        // Metro ON/OFF (toggle on metro switch)
	uint16_t beatsPerMin;   // metro beats per minute (from 0 to 255?). Note: sampling rate may need to be faster, but this is a test (btw, 255BPM is 4.25Hz)
	unsigned long lastBeat;
};

void logicMetro::evolve()
{
	Data outputData;

	// 1) check modification of parameters from switches:
	//if (sensorArrayPtr->checkEvent()) {

		// a) Check the TOOGLE switch (METRO ON/OFF) (check connections!)
		if ( (*sensorArrayPtr)[0].checkEvent() ) toggleMetro();

/*
		// b) Set BMP using two switches (note: to reduce the number of used pins, I am using an "analog" switch with three states). No pressed means level 2, left pressed means level 1, and right is level 0.
		if ( (*sensorArrayPtr)[1].checkEvent() ) {
			switch ((*sensorArrayPtr)[1].getData().numericData) {
				case 2:
					// nothing
				break;
				case 1:
					beatsPerMin+=10; //beatsPerMin++;
					setMetroBMP(beatsPerMin);
				break;
				case 0:
					if (beatsPerMin>9) beatsPerMin-=10; else beatsPerMin = 0 ;//beatsPerMin--;
					setMetroBMP(beatsPerMin);
				break;
			}
		}
*/
		// // +1 BMP
		// else if ((*sensorArrayPtr)[1].checkEvent()) {
		//      beatsPerMin++; if (beatsPerMin>999) beatsPerMin=999; // set toogle LED (actuator 0)
		//      setMetroBMP(beatsPerMin);
		// }
		// //-1 BMP
		// else if ((*sensorArrayPtr)[2].checkEvent()) {
		//      beatsPerMin--; if (beatsPerMin<0) beatsPerMin=0;
		//      setMetroBMP(beatsPerMin);
		// }
	//}


	// 2) Generate beat IF the metro is ON:
	if ( metroOnOff && ( millis() - lastBeat > 60000.0 / beatsPerMin ) ) {
		// Set new data on the outlet:
		outputData.set("beat", 128, true, millis()); // this set the output as new, so it will be sent
		outletArrayPtr->setNewData(outputData);

		// Show beat on the beat LED, which is a digital actuator in PULSE mode (actuator 0)
		(*actuatorArrayPtr)[0].setNewData(outputData);

		lastBeat = millis();
	}

}

void logicMetro::compute()   // executed if there is data on ANY inlet (NOT on switches - check setUpdateMode)
{
	// 1) Use the first inlet to toogle the metro (on/off):
	if (inletArrayPtr->isBang(0)) {// this means that there is something new there:
		// NOTE: I could  just ignore the event or numerical value and use the BANG as a toogle signal,
		// but just as in MAX/MSP, we can STOP the metro if the value or event is 0.
		// I will use the event field (the binary field), instead of the numeric value (OR use some sort of priority over those
		// fields).
		//(*inletArrayPtr)[0].checkEvent();
		//(*inletArrayPtr)[0].getData().numericData;

		// NOTE: for some reason (memory overload? if I check the event it does not ALWAYS work...)
		//if ((*inletArrayPtr)[0].checkEvent())

		toggleMetro();
	}

	// 2) Use the second inlet numericData field to set the BMP:
	if (inletArrayPtr->isBang(1)) {
		setMetroBMP((*inletArrayPtr)[1].getData().numericData);
	}

	// 3) reset the new data flag (this is to reset the BANG)
	inletArrayPtr->setAllInletsDataUsed();

	// Note: the METRO OUTPUT is generated in the evolve() method, not here, but here we could just "pass" the bang or the numerical value on the
	// inlet that has new data...
}
