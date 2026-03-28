#pragma once
// ==========================================================================
//  TiltSensor.h — Shake detection via GPIO interrupt
//
//  Faithful port of V2's tiltSensor. Detects N ticks within an interval
//  to fire a "shake" event. On ESP32, uses attachInterrupt (not PCInt).
//
//  V2 behavior: when shaken, sends PULL_INLET_PATCH_CHORD to all children
//  → they vibrate and show which inlets connect to the shaker.
// ==========================================================================

#include <Arduino.h>

namespace gluon {

class TiltSensor {
public:
    static constexpr uint8_t MAX_NUM_TICKS = 4;
    static constexpr uint32_t DEFAULT_INTERVAL = 2000;
    static constexpr uint32_t MIN_TIME_BETWEEN_TICKS = 500;

    TiltSensor() = default;

    void init(uint8_t pin, uint8_t requiredTicks = MAX_NUM_TICKS,
              uint32_t interval = DEFAULT_INTERVAL) {
        pin_ = pin;
        requiredTicks_ = min(requiredTicks, MAX_NUM_TICKS);
        interval_ = interval;
        indexTicks_ = 0;
        for (uint8_t i = 0; i < requiredTicks_; i++)
            tickTimes_[i] = i * interval_;

        if (pin_ >= 0) {
            pinMode(pin_, INPUT_PULLUP);
        }
    }

    void enable() {
        if (pin_ < 0) return;
        enabled_ = true;
        instance_ = this;
        attachInterrupt(digitalPinToInterrupt(pin_), isrTick, RISING);
    }

    void disable() {
        if (pin_ < 0) return;
        enabled_ = false;
        detachInterrupt(digitalPinToInterrupt(pin_));
    }

    bool isEnabled() const { return enabled_; }

    // Check if shake event occurred (call from main loop, not ISR)
    bool checkEvent() {
        if (!enabled_) return false;

        // Read ISR variables atomically
        portENTER_CRITICAL(&mux_);
        uint32_t newest = tickTimes_[indexTicks_];
        uint32_t oldest = tickTimes_[(indexTicks_ + 1) % requiredTicks_];
        portEXIT_CRITICAL(&mux_);

        uint32_t span = newest - oldest;
        if (span < interval_) {
            // Reset table to prevent repeated event
            portENTER_CRITICAL(&mux_);
            for (uint8_t i = 0; i < requiredTicks_; i++)
                tickTimes_[i] = i * interval_;
            portEXIT_CRITICAL(&mux_);
            return true;
        }
        return false;
    }

private:
    static void IRAM_ATTR isrTick() {
        if (!instance_) return;
        uint32_t now = millis();
        if (now - instance_->tickTimes_[instance_->indexTicks_] > MIN_TIME_BETWEEN_TICKS) {
            instance_->indexTicks_ = (instance_->indexTicks_ + 1) % instance_->requiredTicks_;
            instance_->tickTimes_[instance_->indexTicks_] = now;
        }
    }

    static DRAM_ATTR TiltSensor* instance_;  // Only one tilt sensor per node

    int8_t pin_ = -1;
    bool enabled_ = false;
    uint8_t requiredTicks_ = MAX_NUM_TICKS;
    uint32_t interval_ = DEFAULT_INTERVAL;

    volatile uint8_t indexTicks_ = 0;
    volatile uint32_t tickTimes_[MAX_NUM_TICKS] = {};

    portMUX_TYPE mux_ = portMUX_INITIALIZER_UNLOCKED;
};

// Static member — defined in a .cpp or here with inline for header-only
inline DRAM_ATTR TiltSensor* TiltSensor::instance_ = nullptr;

} // namespace gluon
