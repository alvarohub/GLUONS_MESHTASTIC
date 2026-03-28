#pragma once
// ==========================================================================
//  ChirpSpeaker.h — Audible feedback for connection events
//
//  Faithful port of V2's chirpSpeaker. On ESP32, uses direct GPIO toggling
//  (same technique as V2 — bit-banging a square wave at varying frequency).
//
//  V2 sounds:
//    chirpUp()     — ascending tone  (link created)
//    chirpDown()   — descending tone (link deleted)
//    chirpDownUp() — down then up    (link moved)
//    chirpShake()  — random noise    (tilt/shake detected)
// ==========================================================================

#include <Arduino.h>

namespace gluon {

class ChirpSpeaker {
public:
    ChirpSpeaker() = default;

    void init(uint8_t pin, bool enabled = true) {
        pin_ = pin;
        enabled_ = enabled;
        if (pin_ >= 0) pinMode(pin_, OUTPUT);
    }

    void enable() { enabled_ = true; }
    void disable() { enabled_ = false; }
    bool isEnabled() const { return enabled_; }

    // Ascending tone — connection created (faithful V2 port)
    void chirpUp() {
        if (!enabled_ || pin_ < 0) return;
        bool level = true;
        for (int i = 255; i > 0; i--) {
            digitalWrite(pin_, level);
            level = !level;
            delayMicroseconds(i);
        }
        digitalWrite(pin_, LOW);
    }

    // Descending tone — connection deleted (faithful V2 port)
    void chirpDown() {
        if (!enabled_ || pin_ < 0) return;
        bool level = true;
        for (int i = 0; i < 255; i++) {
            digitalWrite(pin_, level);
            level = !level;
            delayMicroseconds(i);
        }
        digitalWrite(pin_, LOW);
    }

    // Down then up — connection moved (faithful V2 port)
    void chirpDownUp() {
        if (!enabled_ || pin_ < 0) return;
        bool level = true;
        for (int i = 0; i < 200; i++) {
            digitalWrite(pin_, level);
            level = !level;
            delayMicroseconds(i);
        }
        for (int i = 200; i > 0; i--) {
            digitalWrite(pin_, level);
            level = !level;
            delayMicroseconds(i);
        }
        digitalWrite(pin_, LOW);
    }

    // Random noise — shake detected (faithful V2 port)
    void chirpShake() {
        if (!enabled_ || pin_ < 0) return;
        bool level = true;
        for (int i = 0; i < 200; i++) {
            digitalWrite(pin_, level);
            level = !level;
            delayMicroseconds(random(100));
        }
        digitalWrite(pin_, LOW);
    }

private:
    int8_t pin_ = -1;
    bool enabled_ = false;
};

} // namespace gluon
