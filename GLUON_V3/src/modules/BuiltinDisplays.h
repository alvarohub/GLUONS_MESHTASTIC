#pragma once
// ==========================================================================
//  Optional display/LED-bar actuators for Gluon v3
//
//  These require external libraries. To use, add them to platformio.ini:
//    lib_deps =
//        adafruit/Adafruit LED Backpack Library@^1.4.1
//        wayoda/LedControl@^1.0.6
//
//  Then #include this header in main.cpp AFTER BuiltinActuators.h.
//
//  Faithful ports of V2:
//    - actuator4AlphaNum.h    → ActuatorAlphaNum4
//    - actuatorSevenSeg.h     → ActuatorSevenSegI2C
//    - actuatorSevenSeg_MAX7221.h → ActuatorSevenSegMAX
//    - actuatorLedBar.h       → ActuatorLedBar (needs Grove_LED_Bar in lib/)
// ==========================================================================

#include "core/BaseActuator.h"
#include "factory/ComponentFactory.h"
#include <Wire.h>

namespace gluon {

// =========================================================================
//  ActuatorAlphaNum4 — I2C 4-character alphanumeric LED (Adafruit HT16K33)
//  Faithful port of V2's DisplayAlphaNum4 with scrolling text.
//  Requires: Adafruit LED Backpack Library
// =========================================================================
#if __has_include("Adafruit_LEDBackpack.h")
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"

class ActuatorAlphaNum4 : public BaseActuator {
public:
    ActuatorAlphaNum4(const String& name, uint8_t i2cAddr = 0x70,
                      uint16_t scrollDelay = 500)
        : BaseActuator(name, Mode::MODE_SCREEN), i2cAddr_(i2cAddr),
          scrollDelay_(scrollDelay) {}

    ~ActuatorAlphaNum4() override { delete display_; }

    void init() override {
        display_ = new Adafruit_AlphaNum4();
        display_->begin(i2cAddr_);
        display_->setBrightness(8);
        memset(scrollBuf_, ' ', sizeof(scrollBuf_));
        scrollBuf_[0] = '\0';
    }

    void update() override {
        if (!enabled_ || !display_) return;
        // Scroll if string is longer than 4 chars
        if (strlen(scrollBuf_) > 4 && millis() - lastScroll_ > scrollDelay_) {
            scrollIndex_++;
            if (scrollIndex_ >= strlen(scrollBuf_)) scrollIndex_ = 0;
            displayShifted(scrollIndex_);
            lastScroll_ = millis();
        }
    }

protected:
    void onDataReceived() override {
        if (!enabled_ || !display_) return;
        // Use label as display string
        setDisplayString(data_.label.c_str());
        scrollIndex_ = 0;
        displayShifted(0);
    }

private:
    void setDisplayString(const char* str) {
        size_t len = strlen(str);
        if (len > sizeof(scrollBuf_) - 4) len = sizeof(scrollBuf_) - 4;
        memcpy(scrollBuf_, str, len);
        // Add trailing spaces for circular scroll
        scrollBuf_[len] = ' ';
        scrollBuf_[len + 1] = ' ';
        scrollBuf_[len + 2] = ' ';
        scrollBuf_[len + 3] = '\0';
    }

    void displayShifted(uint8_t shift) {
        size_t slen = strlen(scrollBuf_);
        for (uint8_t i = 0; i < 4; i++) {
            uint8_t charAt = (i + shift) % slen;
            display_->writeDigitAscii(i, scrollBuf_[charAt]);
        }
        display_->writeDisplay();
    }

    Adafruit_AlphaNum4* display_ = nullptr;
    uint8_t i2cAddr_;
    char scrollBuf_[20] = {};
    uint16_t scrollDelay_;
    uint8_t scrollIndex_ = 0;
    uint32_t lastScroll_ = 0;
};

REGISTER_ACTUATOR(alphanum4, [](JsonObjectConst cfg) -> BaseActuator* {
    String name = cfg["name"] | "ALPHA";
    uint8_t addr = cfg["address"] | 0x70;
    uint16_t scroll = cfg["scrollDelay"] | 500;
    return new ActuatorAlphaNum4(name, addr, scroll);
});

// =========================================================================
//  ActuatorSevenSegI2C — I2C 4-digit 7-segment (Adafruit HT16K33)
//  Faithful port of V2's DisplaySevenSegment_I2C.
//  Requires: Adafruit LED Backpack Library
// =========================================================================
class ActuatorSevenSegI2C : public BaseActuator {
public:
    ActuatorSevenSegI2C(const String& name, uint8_t i2cAddr = 0x70)
        : BaseActuator(name, Mode::MODE_SCREEN), i2cAddr_(i2cAddr) {}

    ~ActuatorSevenSegI2C() override { delete display_; }

    void init() override {
        display_ = new Adafruit_7segment();
        display_->begin(i2cAddr_);
        display_->setBrightness(15);
        writeValue(0);
    }

protected:
    void onDataReceived() override {
        if (!enabled_ || !display_) return;
        int16_t val = constrain((int16_t)data_.value, -999, 999);
        writeValue(val);
    }

private:
    void writeValue(int16_t val) {
        display_->print(val, DEC);
        display_->writeDisplay();
    }

    Adafruit_7segment* display_ = nullptr;
    uint8_t i2cAddr_;
};

REGISTER_ACTUATOR(seven_seg_i2c, [](JsonObjectConst cfg) -> BaseActuator* {
    String name = cfg["name"] | "7SEG";
    uint8_t addr = cfg["address"] | 0x70;
    return new ActuatorSevenSegI2C(name, addr);
});

#endif // __has_include("Adafruit_LEDBackpack.h")

// =========================================================================
//  ActuatorSevenSegMAX — MAX7221 SPI 7-segment (up to 8 digits)
//  Faithful port of V2's DisplaySevenSegment_MAX7221.
//  Uses software SPI via LedControl library (any 3 pins).
//  Requires: LedControl library
// =========================================================================
#if __has_include("LedControl.h")
#include "LedControl.h"

class ActuatorSevenSegMAX : public BaseActuator {
public:
    ActuatorSevenSegMAX(const String& name, uint8_t dinPin, uint8_t clkPin,
                        uint8_t csPin, uint8_t numDigits = 8)
        : BaseActuator(name, Mode::MODE_SCREEN), dinPin_(dinPin),
          clkPin_(clkPin), csPin_(csPin), numDigits_(numDigits) {}

    ~ActuatorSevenSegMAX() override { delete display_; }

    void init() override {
        display_ = new LedControl(dinPin_, clkPin_, csPin_, 1);
        display_->shutdown(0, false);
        display_->setIntensity(0, 15);
        display_->clearDisplay(0);
        writeValue(0);
    }

protected:
    void onDataReceived() override {
        if (!enabled_ || !display_) return;
        int16_t halfRange = (int16_t)pow(10, numDigits_) - 1;
        int16_t val = constrain((int16_t)data_.value, -halfRange, halfRange);
        writeValue(val);
    }

private:
    void writeValue(int16_t val) {
        bool sign = (val < 0);
        if (sign) val = -val;

        display_->clearDisplay(0);
        uint8_t digit = 0;
        while (val > 0 && digit < numDigits_ - 1) {
            display_->setDigit(0, digit, val % 10, false);
            val /= 10;
            digit++;
        }
        if (val > 0) {
            display_->setDigit(0, digit, val % 10, sign);
        } else if (sign) {
            display_->setDigit(0, digit, 11, false); // 11 = '-'
        }
    }

    LedControl* display_ = nullptr;
    uint8_t dinPin_, clkPin_, csPin_;
    uint8_t numDigits_;
};

REGISTER_ACTUATOR(seven_seg_max, [](JsonObjectConst cfg) -> BaseActuator* {
    String name = cfg["name"] | "7SEG_M";
    uint8_t din = cfg["dinPin"] | 0;
    uint8_t clk = cfg["clkPin"] | 0;
    uint8_t cs = cfg["csPin"] | 0;
    uint8_t digits = cfg["digits"] | 8;
    return new ActuatorSevenSegMAX(name, din, clk, cs, digits);
});

#endif // __has_include("LedControl.h")

// =========================================================================
//  ActuatorLedBar — Grove 10-LED bar
//  Faithful port of V2's ledBar.
//  Uses Grove_LED_Bar library with serial protocol on 2 digital pins.
//  Requires: Grove_LED_Bar library in lib/ folder
// =========================================================================
#if __has_include("Grove_LED_Bar.h")
#include <Grove_LED_Bar.h>

class ActuatorLedBar : public BaseActuator {
public:
    ActuatorLedBar(const String& name, uint8_t clkPin, uint8_t dataPin)
        : BaseActuator(name, Mode::MODE_NORMAL), clkPin_(clkPin),
          dataPin_(dataPin) {}

    ~ActuatorLedBar() override { delete bar_; }

    void init() override {
        pinMode(clkPin_, OUTPUT);
        pinMode(dataPin_, OUTPUT);
        bar_ = new Grove_LED_Bar(clkPin_, dataPin_, 1);
        bar_->begin();
        bar_->setLevel(5);
    }

protected:
    void onDataReceived() override {
        if (!enabled_ || !bar_) return;
        uint8_t level = constrain((uint8_t)data_.value, 0, 10);
        bar_->setLevel(level);
    }

private:
    Grove_LED_Bar* bar_ = nullptr;
    uint8_t clkPin_;
    uint8_t dataPin_;
};

REGISTER_ACTUATOR(led_bar, [](JsonObjectConst cfg) -> BaseActuator* {
    String name = cfg["name"] | "LBAR";
    uint8_t clk = cfg["clkPin"] | 0;
    uint8_t data = cfg["dataPin"] | 0;
    return new ActuatorLedBar(name, clk, data);
});

#endif // __has_include("Grove_LED_Bar.h")

} // namespace gluon
