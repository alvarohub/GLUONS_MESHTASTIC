#pragma once
// ==========================================================================
//  BaseSensor.h — Abstract sensor base class
//
//  Faithful port of V2's BaseSensor with all 6 event detection modes,
//  condition range (conditionValue ± toleranceValue), and learn().
// ==========================================================================

#include "GluonTypes.h"
#include <ArduinoJson.h>

namespace gluon {

class BaseSensor {
public:
    // NOTE: Arduino ESP32 #defines ANALOG as 0x0C, so we avoid that name
    enum class Type { PIN_DIGITAL, PIN_ANALOG, BUS_I2C, SOFT };

    // Event detection modes — faithful port of V2's EventDetectionMode
    enum class EventMode {
        ON_SIMPLE_CHANGE,   // |new - old| > toleranceValue  (any change)
        ON_TRUE_COND,       // |value - conditionValue| < toleranceValue  (value IN range)
        ON_FALSE_COND,      // |value - conditionValue| >= toleranceValue  (value NOT in range)
        ON_CHANGE_COND,     // crossed boundary either direction
        ON_LEAVING_COND,    // was in range, now out
        ON_ENTERING_COND,   // was out of range, now in  (most common)
    };

    BaseSensor(const String& name, Type type)
        : name_(name), type_(type) {}
    virtual ~BaseSensor() = default;

    // Core interface
    virtual void init() {}
    virtual void update() = 0;

    // Learn/calibrate: samples sensor for durationMs, sets conditionValue = mean, toleranceValue = 2*stddev
    virtual void learn(uint32_t durationMs = 3000) {}

    // Identity
    const String& name() const { return name_; }
    Type type() const { return type_; }

    // Data access
    const Data& data() const { return data_; }
    bool hasEvent() const { return data_.event; }
    void clearEvent() { data_.event = false; }

    // Configuration — V2 condition range: conditionValue ± toleranceValue
    void setEventMode(EventMode mode) { eventMode_ = mode; }
    EventMode eventMode() const { return eventMode_; }
    void setConditionValue(float val) { conditionValue_ = val; }
    float conditionValue() const { return conditionValue_; }
    void setToleranceValue(float val) { toleranceValue_ = val; }
    float toleranceValue() const { return toleranceValue_; }
    void setEnabled(bool en) { enabled_ = en; }
    bool isEnabled() const { return enabled_; }
    void setLearnable(bool l) { learnable_ = l; }
    bool isLearnable() const { return learnable_; }

    virtual JsonDocument describe() const {
        JsonDocument doc;
        doc["name"] = name_;
        doc["type"] = (int)type_;
        doc["eventMode"] = (int)eventMode_;
        doc["conditionValue"] = conditionValue_;
        doc["toleranceValue"] = toleranceValue_;
        return doc;
    }

protected:
    // V2-faithful event detection — uses condition range (conditionValue ± toleranceValue)
    void detectEvent(float newValue) {
        bool inRange = fabs(newValue - conditionValue_) < toleranceValue_;
        bool wasInRange = fabs(lastValue_ - conditionValue_) < toleranceValue_;

        bool evt = false;
        switch (eventMode_) {
            case EventMode::ON_SIMPLE_CHANGE:
                evt = fabs(newValue - lastValue_) > toleranceValue_;
                break;
            case EventMode::ON_TRUE_COND:
                evt = inRange;
                break;
            case EventMode::ON_FALSE_COND:
                evt = !inRange;
                break;
            case EventMode::ON_CHANGE_COND:
                evt = (wasInRange != inRange);
                break;
            case EventMode::ON_LEAVING_COND:
                evt = (wasInRange && !inRange);
                break;
            case EventMode::ON_ENTERING_COND:
                evt = (!wasInRange && inRange);
                break;
        }
        lastValue_ = newValue;

        data_.value = newValue;
        data_.event = evt;
        data_.timestamp = millis();
    }

    Data data_;
    String name_;
    Type type_;
    EventMode eventMode_ = EventMode::ON_SIMPLE_CHANGE;
    float conditionValue_ = 512.0f;   // V2 default: mid-range for 10-bit ADC
    float toleranceValue_ = 10.0f;    // V2 default
    float lastValue_ = 0.0f;
    bool enabled_ = true;
    bool learnable_ = false;          // V2: only some sensors support learn()
};

// =========================================================================
//  SensorArray — Dynamic collection of sensors
// =========================================================================
class SensorArray {
public:
    void add(BaseSensor* sensor) { sensors_.push_back(sensor); }
    uint8_t size() const { return sensors_.size(); }

    BaseSensor& operator[](uint8_t idx) { return *sensors_[idx]; }
    const BaseSensor& operator[](uint8_t idx) const { return *sensors_[idx]; }

    void init() {
        for (auto* s : sensors_) s->init();
    }

    void update() {
        for (auto* s : sensors_) {
            if (s->isEnabled()) s->update();
        }
    }

    bool hasAnyEvent() const {
        for (const auto* s : sensors_)
            if (s->isEnabled() && s->hasEvent()) return true;
        return false;
    }

    void clearAllEvents() {
        for (auto* s : sensors_) s->clearEvent();
    }

    // V2: learnConditions() — learn all learnable sensors
    void learnConditions(uint32_t durationMs = 3000) {
        for (auto* s : sensors_) {
            if (s->isLearnable()) s->learn(durationMs);
        }
    }

private:
    std::vector<BaseSensor*> sensors_;
};

} // namespace gluon
