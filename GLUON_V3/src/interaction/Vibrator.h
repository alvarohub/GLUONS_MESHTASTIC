#pragma once
// ==========================================================================
//  Vibrator.h — Motor vibrator for "pulling" connections
//
//  Faithful port of V2's vibrator. On ESP32, uses LEDC PWM.
//  When a connected node is shaken, it sends PULL_PATCH_CHORD,
//  and the receiving node vibrates to show the physical connection.
// ==========================================================================

#include <Arduino.h>

namespace gluon {

class Vibrator {
public:
    Vibrator() = default;

    void init(uint8_t pin, uint8_t ledcChannel = 4, bool enabled = true) {
        pin_ = pin;
        channel_ = ledcChannel;
        enabled_ = enabled;
        if (pin_ >= 0) {
            ledcSetup(channel_, 1000, 8); // 1kHz, 8-bit resolution
            ledcAttachPin(pin_, channel_);
            ledcWrite(channel_, 0);
        }
    }

    void enable() { enabled_ = true; }
    void disable() { enabled_ = false; }
    bool isEnabled() const { return enabled_; }

    // Single pulse (faithful V2 port)
    void pulse(uint32_t timeMs, uint8_t strength = 255) {
        if (!enabled_ || pin_ < 0) return;
        ledcWrite(channel_, strength);
        delay(timeMs);
        ledcWrite(channel_, 0);
        delay(timeMs);
    }

    // Multiple pulses (faithful V2 port — used for PULL_PATCH_CHORD)
    void manyPulses(uint8_t times, uint32_t timeMs, uint8_t strength = 255) {
        if (!enabled_ || pin_ < 0) return;
        for (uint8_t i = 0; i < times; i++) {
            pulse(timeMs, strength);
        }
    }

private:
    int8_t pin_ = -1;
    uint8_t channel_ = 4;
    bool enabled_ = false;
};

} // namespace gluon
