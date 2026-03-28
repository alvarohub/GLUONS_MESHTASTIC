#pragma once
// ==========================================================================
//  LearningButton.h — Multi-level button for sensor calibration
//
//  Faithful port of V2's learningButton (which was a SensorDiscreteValues
//  on an analog pin). Detects multiple voltage levels via an analog input
//  with a resistor ladder. On the simplest (one-button) configuration,
//  pressing the button triggers learnConditions() on all sensors.
//
//  On ESP32, uses 12-bit ADC instead of V2's 10-bit.
// ==========================================================================

#include <Arduino.h>

namespace gluon {

class LearningButton {
public:
    LearningButton() = default;

    void init(int8_t pin, bool enabled = true) {
        pin_ = pin;
        enabled_ = enabled;
        if (pin_ >= 0) {
            // No need for explicit ADC mode on ESP32 — analogRead handles it
        }
    }

    void enable() { enabled_ = true; }
    void disable() { enabled_ = false; }
    bool isEnabled() const { return enabled_; }

    // Call every loop — detects button press transitions
    void update() {
        if (!enabled_ || pin_ < 0) return;

        uint16_t raw = analogRead(pin_);
        // Map 12-bit ADC to discrete levels (V2 used voltage dividers)
        // Level 0 = depressed (~0V = ADC < 500)
        // Level 1 = pressed   (~3.3V = ADC > 3500)
        uint8_t newLevel = 0;
        if (raw > 3500) newLevel = 1;
        // Can add more levels for multi-button ladder

        if (newLevel != currentLevel_) {
            // Debounce
            if (millis() - lastChangeTime_ < debounceMs_) return;
            lastChangeTime_ = millis();
            currentLevel_ = newLevel;
            event_ = true;
        }
    }

    bool checkEvent() {
        if (event_) {
            event_ = false;
            return true;
        }
        return false;
    }

    uint8_t level() const { return currentLevel_; }

private:
    int8_t pin_ = -1;
    bool enabled_ = false;
    bool event_ = false;
    uint8_t currentLevel_ = 0;
    uint32_t lastChangeTime_ = 0;
    uint32_t debounceMs_ = 100;
};

} // namespace gluon
