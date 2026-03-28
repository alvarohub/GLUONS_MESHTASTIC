#pragma once
// ==========================================================================
//  BaseActuator.h — Abstract actuator base class
//
//  Faithful port of V2's actuator modes:
//    NORMAL:  output follows event (ON while event=true)
//    TOGGLE:  flip state on each event=true
//    PULSE:   ON on event=true, auto-OFF after durationMs
//    BLINK:   multiple pulses (count from data.value)
//    SCREEN:  data display (OLED, 7-seg)
// ==========================================================================

#include "GluonTypes.h"
#include <ArduinoJson.h>

namespace gluon {

class BaseActuator {
public:
    // V2-faithful mode names. Note: Arduino #defines NORMAL as 0x00, so we
    // prefix with MODE_ or use enum class to avoid conflicts.
    enum class Mode { MODE_NORMAL, MODE_TOGGLE, MODE_PULSE, MODE_BLINK, MODE_SCREEN };

    BaseActuator(const String& name, Mode mode = Mode::MODE_PULSE)
        : name_(name), mode_(mode) {}
    virtual ~BaseActuator() = default;

    virtual void init() {}

    // Called every loop — handles timed behaviors (pulse decay, blink sequence)
    virtual void update() {}

    // Receive data from logic — dispatches to mode-specific behavior
    virtual void setData(const Data& d) {
        data_ = d;
        onDataReceived();
    }

    const String& name() const { return name_; }
    Mode mode() const { return mode_; }
    void setMode(Mode m) { mode_ = m; }
    const Data& data() const { return data_; }
    void setEnabled(bool en) { enabled_ = en; }
    bool isEnabled() const { return enabled_; }

    void setDuration(uint32_t ms) { durationMs_ = ms; }
    uint32_t duration() const { return durationMs_; }

    virtual JsonDocument describe() const {
        JsonDocument doc;
        doc["name"] = name_;
        doc["mode"] = (int)mode_;
        return doc;
    }

protected:
    virtual void onDataReceived() {}

    Data data_;
    String name_;
    Mode mode_;
    bool enabled_ = true;
    uint32_t durationMs_ = 200;
    uint32_t pulseStartMs_ = 0;

    // V2 toggle/blink state
    bool toggleState_ = false;
    uint8_t blinkCount_ = 0;
    uint8_t blinkRemaining_ = 0;
    bool blinkPhaseOn_ = false;
};

// =========================================================================
//  ActuatorArray — Dynamic collection of actuators
// =========================================================================
class ActuatorArray {
public:
    void add(BaseActuator* actuator) { actuators_.push_back(actuator); }
    uint8_t size() const { return actuators_.size(); }

    BaseActuator& operator[](uint8_t idx) { return *actuators_[idx]; }
    const BaseActuator& operator[](uint8_t idx) const { return *actuators_[idx]; }

    void init() {
        for (auto* a : actuators_) a->init();
    }

    void update() {
        for (auto* a : actuators_) {
            if (a->isEnabled()) a->update();
        }
    }

    void setData(const Data& d) {
        for (auto* a : actuators_) a->setData(d);
    }

private:
    std::vector<BaseActuator*> actuators_;
};

} // namespace gluon
