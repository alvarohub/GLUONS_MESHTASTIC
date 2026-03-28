// LOGIC FOR THE COUNTER MODULE

/* DESCRIPTION:
*
* INPUTS:
*    INLET 1 : compute (increment/decrement counter, depending on the event value, or stringData (inc/dec)
*             INLET 2 : (to do) reset counter to the numericData
*   INLET 3 : (to do) configure TOP (this can also be configured by the sensor on the gluon, which MUST be some sort of encoder or potentiometer)
*
* OPERATION:
*  Counter Register with bang at overflow (<TOP)
*  Update mode: ANY_INLET (but we will do different things with the data on different inlets)
*
* OUTPUT (for the time being, only one).
*          Data Fields are: {stringData, numericData, event, timeStamp}
*  stringData: "overflow"
*  numericData: TOP
*    event: true
*    timeStamp: millis()
*
************************************* Associated Actuator and Sensor ************************************
* REQUIREMENT: OPTIONAL
*
* ACTUATOR:
*  Function : to display the current counter)
*  Type: Two digit display? Led bar?
*
* SENSOR:
*  Function: for setting the TOP value
*  Type: rotary encoder, potentiometer, dip switch, push buttons?
*
*/

#include "BaseLogic.h"

class logicCounter : public BaseLogic {

public:
    logicCounter() : BaseLogic("COUNT") {}
    //logicCounter(uint8_t _top) : BaseLogic("COUNT"), counterTOP(_top) {}

    void init(bool firstTimeBuild) {

        if (firstTimeBuild)
            setUpdateMode(ANY_INLET_BANG);
        else
            loadUpdateMode();

        counterTOP = 10;//10; // default value...
        reset();
    }


    virtual void compute() override;

    // Methods specific to this logic module:
    void reset() {
        counter = counterTOP;
        Data outputData;
        outputData.set("set", counter, true, millis());
        actuatorArrayPtr->setNewData(outputData);
    }

    void setCounterTop(uint8_t _top) {
        counterTOP = _top;
        reset();
    }

private:
    uint8_t counterTOP, counter;

};

void logicCounter::compute() {
    // Note: if we are here, it means that there was a BANG on first inlet.
    // Let's check the data on the data string on the first inlet to see if we need to increment or decrement:
    // ... to do  Data firstInlet=(*inletArrayPtr)[i].getData();

    Data outputData;

    // Do different things depending on the inlet:
    if (inletArrayPtr->isBang(0)) { // if is the first inlet, increment the counter.
        // Inrement COUNTER and overflow:
        if (counter == counterTOP) {
            counter = 0;
            //In overflow case, set the outlet as TRUE EVENT
            outputData.set("OFW+", 0, true, millis());
            // Set the new data to send on overflow?
            //outletArrayPtr->setNewData(outputData);
        }
        else {
            counter++;
            if (counter == counterTOP) {
                outputData.set("+TOP", counterTOP, true, millis());
                outletArrayPtr->setNewData(outputData);
            }
            else outputData.set("INC", counter, false, millis()); // false event...
        }
    }
    else if (inletArrayPtr->isBang(2)) {  // this is the third inlet, decrement the counter. NOTE: equivalent to isBangFirstInlet()
        // DECREMENT COUNTER and overflow:
        if (counter == 0) {
            counter = counterTOP;
            // In overflow case, set the outlet as TRUE EVENT
            outputData.set("OFW-", counterTOP, true, millis());
            // Set the new data to send on overflow?
            //outletArrayPtr->setNewData(outputData);
        }
        else {
            counter--;
            if (counter == 0) {
                outputData.set("-ZERO", 0, true, millis());
                outletArrayPtr->setNewData(outputData);
            }
            else outputData.set("DECR", counter, false, millis()); // false event...
        }
    }
    else { // This means, there was a BANG on SECOND inlet (no event) // else if (inletArrayPtr->isBang(1)) {
        // SET the value of the counterTOP using the data on the SECOND inlet
        // also, would be nice to flash the data so it start from this value next time the module is powered on.. but this is kind of complicated because I am not using flash memory to store data of actuators yet - and only a few sensors by the way.

        PRINTLN((*inletArrayPtr)[1].getData().numericData);

        setCounterTop((*inletArrayPtr)[1].getData().numericData);
        reset();
        outputData.set("SET", counter, false, millis());

        // NOTE: we don't set new data to send in this case?
        // outletArrayPtr->setNewData(outputData);
    }

    // Actuator (display): always set to the last value of the counter:
    actuatorArrayPtr->setNewData(outputData); // in all actuators... here only one, the display.
}
