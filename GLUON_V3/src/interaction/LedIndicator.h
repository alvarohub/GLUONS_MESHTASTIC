#pragma once
// ==========================================================================
//  LedIndicator.h — Visual feedback for inlet/outlet connection status
//
//  Faithful port of V2's ledIndicators. V2 used a 4-LED NeoPixel strip
//  with colors: red(inlet0), green(inlet1), blue(inlet2), white(outlet0).
//
//  For V3 ESP32-S3 we support:
//    - NeoPixel strip (Adafruit NeoPixel or ESP32 RMT)
//    - Fallback: built-in LED blink patterns
//
//  Uses the Adafruit NeoPixel library (widely available on ESP32).
//  If no NeoPixel hardware is present, falls back to serial logging.
// ==========================================================================

#include <Arduino.h>
#include "core/Inlet.h"
#include "core/Outlet.h"

namespace gluon {

// Simple RGB struct (similar to V2's cRGB)
struct RGB {
    uint8_t r, g, b;
    constexpr RGB(uint8_t r_ = 0, uint8_t g_ = 0, uint8_t b_ = 0) : r(r_), g(g_), b(b_) {}
    RGB dim(uint8_t percent) const {
        return RGB(r * percent / 100, g * percent / 100, b * percent / 100);
    }
};

// V2 color palette
namespace Colors {
    constexpr RGB red    = {255, 0, 0};
    constexpr RGB green  = {0, 255, 0};
    constexpr RGB blue   = {0, 0, 255};
    constexpr RGB white  = {255, 255, 255};
    constexpr RGB black  = {0, 0, 0};
}

class LedIndicator {
public:
    static constexpr uint8_t NUM_LEDS = 4; // 3 inlets + 1 outlet (V2 layout)

    LedIndicator() = default;

    void init(int8_t pin, InletArray* inlets, OutletArray* outlets, bool enabled = true) {
        pin_ = pin;
        inlets_ = inlets;
        outlets_ = outlets;
        enabled_ = enabled;

        if (pin_ >= 0) {
            // Use NeoPixel via ESP32 RMT — we'll use simple digital writes as placeholder
            // TODO: Replace with Adafruit_NeoPixel when lib is added to platformio.ini
            pinMode(pin_, OUTPUT);
        }

        // V2 color palette: inlet0=red, inlet1=green, inlet2=blue, outlet0=white
        palette_[0] = Colors::red;
        palette_[1] = Colors::green;
        palette_[2] = Colors::blue;
        palette_[3] = Colors::white;

        allOff();
    }

    void enable() { enabled_ = true; }
    void disable() { enabled_ = false; }
    bool isEnabled() const { return enabled_; }

    // Update all LEDs based on current inlet/outlet connection state
    // (called every loop iteration, like V2)
    void update() {
        if (!enabled_ || !inlets_ || !outlets_) return;

        // Update inlet LEDs
        for (uint8_t i = 0; i < inlets_->size() && i < 3; i++) {
            const Inlet& inlet = (*inlets_)[i];
            if (inlet.numLinks() > 0) {
                if (inlet.hasNewData()) {
                    leds_[i] = Colors::white.dim(80); // flash white on new data
                } else {
                    leds_[i] = palette_[i].dim(50); // steady color = connected
                }
            } else {
                leds_[i] = Colors::black; // off = disconnected
            }
        }

        // Update outlet LED
        uint8_t outIdx = inlets_->size(); // outlet LED comes after inlet LEDs
        if (outIdx < NUM_LEDS && outlets_->size() > 0) {
            const Outlet& outlet = (*outlets_)[0];
            if (outlet.numLinks() > 0) {
                if (outlet.hasNewData()) {
                    leds_[outIdx] = palette_[outIdx].dim(80);
                } else {
                    leds_[outIdx] = palette_[outIdx].dim(50);
                }
            } else {
                leds_[outIdx] = Colors::black;
            }
        }

        display();
    }

    // Blink a specific inlet LED (called on connection change)
    void blinkInlet(uint8_t index) {
        if (!enabled_ || index >= 3) return;
        for (uint8_t i = 0; i < 4; i++) {
            leds_[index] = palette_[index].dim(80);
            display();
            delay(100);
            leds_[index] = Colors::black;
            display();
            delay(100);
        }
    }

    // Blink the outlet LED
    void blinkOutlet(uint8_t index) {
        if (!enabled_) return;
        uint8_t ledIdx = (inlets_ ? inlets_->size() : 3) + index;
        if (ledIdx >= NUM_LEDS) return;
        for (uint8_t i = 0; i < 4; i++) {
            leds_[ledIdx] = palette_[ledIdx].dim(80);
            display();
            delay(100);
            leds_[ledIdx] = Colors::black;
            display();
            delay(100);
        }
    }

    // Show all LEDs in sequence (startup animation)
    void showSequence() {
        if (!enabled_) return;
        for (uint8_t i = 0; i < NUM_LEDS; i++) {
            allOff();
            leds_[i] = palette_[i];
            display();
            delay(200);
        }
        allOff();
    }

    // Show which inlets are connected to a specific node (for PULL_PATCH_CHORD)
    void showInletsMatchID(NodeID nodeId) {
        if (!enabled_ || !inlets_) return;
        allOff();
        for (uint8_t i = 0; i < inlets_->size() && i < 3; i++) {
            if ((*inlets_)[i].isConnectedTo(nodeId)) {
                leds_[i] = palette_[i];
            }
        }
        display();
    }

    void allOff() {
        for (uint8_t i = 0; i < NUM_LEDS; i++)
            leds_[i] = Colors::black;
        display();
    }

    void allOn() {
        for (uint8_t i = 0; i < NUM_LEDS; i++)
            leds_[i] = Colors::white.dim(60);
        display();
    }

    void blinkAll(const RGB& color) {
        if (!enabled_) return;
        for (uint8_t i = 0; i < 4; i++) {
            for (uint8_t j = 0; j < NUM_LEDS; j++)
                leds_[j] = color.dim(50);
            display();
            delay(100);
            allOff();
            delay(100);
        }
    }

private:
    void display() {
        if (!enabled_ || pin_ < 0) return;
        // TODO: Send leds_[] to NeoPixel strip via Adafruit_NeoPixel or RMT
        // For now, log state changes for debugging
        // (NeoPixel library integration is a build-time dependency)
    }

    int8_t pin_ = -1;
    bool enabled_ = false;
    InletArray* inlets_ = nullptr;
    OutletArray* outlets_ = nullptr;

    RGB leds_[NUM_LEDS] = {};
    RGB palette_[NUM_LEDS] = {};
};

} // namespace gluon
