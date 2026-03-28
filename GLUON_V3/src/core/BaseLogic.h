#pragma once
// ==========================================================================
//  BaseLogic.h — Abstract logic module base class
//
//  Preserves the brilliant update-mode bitmask from v2.
//  The compute()/evolve() split is kept: evolve() runs every loop,
//  compute() runs only when trigger conditions are met.
// ==========================================================================

#include "GluonTypes.h"
#include "Inlet.h"
#include "Outlet.h"
#include "BaseSensor.h"
#include "BaseActuator.h"
#include <Preferences.h>  // ESP32 NVS (replaces EEPROM)

namespace gluon {

class BaseLogic {
public:
    BaseLogic(const String& name) : name_(name) {}
    virtual ~BaseLogic() = default;

    // Called once at startup (replaces firstTimeBuild pattern)
    virtual void init(bool firstTime = false) {}

    // Attach to the node's I/O arrays (same pattern as v2)
    void attachTo(InletArray* inlets, OutletArray* outlets,
                  SensorArray* sensors, ActuatorArray* actuators) {
        inlets_ = inlets;
        outlets_ = outlets;
        sensors_ = sensors;
        actuators_ = actuators;
    }

    // Main update — called every loop iteration by the Node
    void update() {
        evolve(); // Always runs (timers, counters, etc.)

        bool shouldCompute = false;

        if ((updateMode_ & UpdateMode::SENSOR_EVENT) && sensors_ && sensors_->hasAnyEvent())
            shouldCompute = true;
        if ((updateMode_ & UpdateMode::FIRST_INLET) && inlets_ && inlets_->hasFirstInletData())
            shouldCompute = true;
        if ((updateMode_ & UpdateMode::ANY_INLET) && inlets_ && inlets_->hasAnyNewData())
            shouldCompute = true;
        if ((updateMode_ & UpdateMode::PERIODIC) && isPeriodElapsed())
            shouldCompute = true;
        // SYNC and HEARTBEAT_IN are triggered externally via forceCompute()

        if (shouldCompute) {
            compute();
            if (inlets_) inlets_->clearAllFlags();
            if (sensors_) sensors_->clearAllEvents();
        }
    }

    void forceCompute() {
        compute();
        if (inlets_) inlets_->clearAllFlags();
    }

    // Update mode management
    void setUpdateMode(uint8_t mode) { updateMode_ = mode; }
    uint8_t getUpdateMode() const { return updateMode_; }
    void toggleUpdateMode(uint8_t flag) { updateMode_ ^= flag; }

    // Periodic mode config
    void setPeriod(uint32_t ms) { periodMs_ = ms; }
    uint32_t period() const { return periodMs_; }

    const String& name() const { return name_; }

    // Persistence (ESP32 NVS instead of EEPROM)
    virtual void saveState(Preferences& prefs) {
        prefs.putUChar("logicMode", updateMode_);
    }
    virtual void loadState(Preferences& prefs) {
        updateMode_ = prefs.getUChar("logicMode", UpdateMode::ANY_INLET);
    }

protected:
    // Override these in subclasses:
    virtual void compute() = 0;  // Dataflow computation (pure virtual)
    virtual void evolve() {}     // Continuous evolution (optional)

    // Access to the node's I/O (same pointer pattern as v2, proven to work)
    InletArray* inlets_ = nullptr;
    OutletArray* outlets_ = nullptr;
    SensorArray* sensors_ = nullptr;
    ActuatorArray* actuators_ = nullptr;

    String name_;
    uint8_t updateMode_ = UpdateMode::ANY_INLET;
    uint32_t periodMs_ = 1000;

private:
    bool isPeriodElapsed() {
        uint32_t now = millis();
        if (now - lastPeriodTick_ >= periodMs_) {
            lastPeriodTick_ = now;
            return true;
        }
        return false;
    }
    uint32_t lastPeriodTick_ = 0;
};

} // namespace gluon
