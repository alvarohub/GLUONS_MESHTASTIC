#pragma once
// ==========================================================================
//  Built-in actuator types for Gluon v3
//
//  Faithful port of V2's actuatorBaseDigital.h modes:
//    NORMAL, TOGGLE, PULSE, BLINK — full FSM in update()
//  Plus: PWM output, Display actuator
// ==========================================================================

#include "core/BaseActuator.h"
#include "factory/ComponentFactory.h"

namespace gluon {

// =========================================================================
//  ActuatorDigitalPin — Digital output with V2's full mode FSM
//  Replaces v2: ActuatorLedNormal, ActuatorLedPulse, ActuatorLedToggle,
//               ActuatorBuzzer, ActuatorPowerMosfet
// =========================================================================
class ActuatorDigitalPin : public BaseActuator {
public:
    ActuatorDigitalPin(const String& name, uint8_t pin, Mode mode = Mode::MODE_PULSE)
        : BaseActuator(name, mode), pin_(pin) {
        pinMode(pin_, OUTPUT);
        digitalWrite(pin_, LOW);
    }

    void update() override {
        if (!enabled_) return;

        uint32_t now = millis();

        switch (mode_) {
            case Mode::MODE_PULSE:
                // Auto-OFF after duration
                if (isOn_ && (now - pulseStartMs_ >= durationMs_)) {
                    writePin(false);
                }
                break;

            case Mode::MODE_BLINK:
                // Blink sequence: ON-OFF-ON-OFF... for blinkRemaining_ pulses
                if (blinkRemaining_ > 0 && (now - pulseStartMs_ >= durationMs_)) {
                    pulseStartMs_ = now;
                    if (blinkPhaseOn_) {
                        writePin(false);
                        blinkPhaseOn_ = false;
                        blinkRemaining_--;
                    } else {
                        writePin(true);
                        blinkPhaseOn_ = true;
                    }
                }
                break;

            default:
                break;
        }
    }

protected:
    void onDataReceived() override {
        if (!enabled_) return;

        switch (mode_) {
            case Mode::MODE_NORMAL:
                // ON while event is true (V2: NORMAL mode)
                writePin(data_.event || (data_.value > 0));
                break;

            case Mode::MODE_TOGGLE:
                // Flip state on each event=true (V2: TOGGLE mode)
                if (data_.event) {
                    toggleState_ = !toggleState_;
                    writePin(toggleState_);
                }
                break;

            case Mode::MODE_PULSE:
                // ON on event=true, auto-OFF after durationMs (V2: PULSE mode)
                if (data_.event) {
                    writePin(true);
                    pulseStartMs_ = millis();
                }
                break;

            case Mode::MODE_BLINK:
                // Multiple pulses, count from data.value (V2: BLINK mode)
                if (data_.event) {
                    blinkRemaining_ = max(1, (int)data_.value);
                    blinkPhaseOn_ = true;
                    writePin(true);
                    pulseStartMs_ = millis();
                }
                break;

            default:
                break;
        }
    }

private:
    void writePin(bool on) {
        digitalWrite(pin_, on ? HIGH : LOW);
        isOn_ = on;
    }

    uint8_t pin_;
    bool isOn_ = false;
};

REGISTER_ACTUATOR(digital, [](JsonObjectConst cfg) -> BaseActuator* {
    uint8_t pin = cfg["pin"] | 0;
    String name = cfg["name"] | "OUT";
    String modeStr = cfg["mode"] | "pulse";

    BaseActuator::Mode mode = BaseActuator::Mode::MODE_PULSE;
    if (modeStr == "normal")      mode = BaseActuator::Mode::MODE_NORMAL;
    else if (modeStr == "toggle") mode = BaseActuator::Mode::MODE_TOGGLE;
    else if (modeStr == "pulse")  mode = BaseActuator::Mode::MODE_PULSE;
    else if (modeStr == "blink")  mode = BaseActuator::Mode::MODE_BLINK;

    auto* a = new ActuatorDigitalPin(name, pin, mode);
    if (!cfg["duration"].isNull())
        a->setDuration(cfg["duration"] | 200);
    return a;
});

// =========================================================================
//  ActuatorPWM — PWM output (LED brightness, motor speed, servo angle)
//  Replaces v2: ActuatorServo, analog actuators
// =========================================================================
class ActuatorPWM : public BaseActuator {
public:
    ActuatorPWM(const String& name, uint8_t pin, uint8_t channel = 0,
                float minVal = 0.0f, float maxVal = 255.0f)
        : BaseActuator(name, Mode::MODE_NORMAL), pin_(pin), channel_(channel),
          minVal_(minVal), maxVal_(maxVal) {}

    void init() override {
        // ESP32 Arduino Core 2.x API
        ledcSetup(channel_, freq_, resolution_);
        ledcAttachPin(pin_, channel_);
    }

protected:
    void onDataReceived() override {
        if (!enabled_) return;
        float normalized = constrain((data_.value - minVal_) / (maxVal_ - minVal_), 0.0f, 1.0f);
        uint32_t duty = (uint32_t)(normalized * ((1 << resolution_) - 1));
        ledcWrite(channel_, duty);
    }

private:
    uint8_t pin_;
    uint8_t channel_;
    float minVal_, maxVal_;
    uint32_t freq_ = 5000;
    uint8_t resolution_ = 8;
};

REGISTER_ACTUATOR(pwm, [](JsonObjectConst cfg) -> BaseActuator* {
    uint8_t pin = cfg["pin"] | 0;
    String name = cfg["name"] | "PWM";
    uint8_t ch = cfg["channel"] | 0;
    float minV = cfg["min"] | 0.0f;
    float maxV = cfg["max"] | 255.0f;
    return new ActuatorPWM(name, pin, ch, minV, maxV);
});

// =========================================================================
//  ActuatorDisplay — Shows data on the node's screen
// =========================================================================
class ActuatorDisplay : public BaseActuator {
public:
    ActuatorDisplay(const String& name = "DISP")
        : BaseActuator(name, Mode::MODE_SCREEN) {}

    const String& displayLabel() const { return data_.label; }
    float displayValue() const { return data_.value; }
    bool displayEvent() const { return data_.event; }
};

REGISTER_ACTUATOR(display, [](JsonObjectConst cfg) -> BaseActuator* {
    String name = cfg["name"] | "DISP";
    return new ActuatorDisplay(name);
});

// =========================================================================
//  ActuatorServo — Servo motor via ESP32 LEDC PWM
//  Faithful port of V2's ServoMotor.
//  Uses 50Hz LEDC channel with 500-2400µs pulse width for 0-180°.
//  No external library needed — pure ESP32 LEDC.
// =========================================================================
class ActuatorServo : public BaseActuator {
public:
    ActuatorServo(const String& name, uint8_t pin, uint8_t channel = 1,
                  uint16_t minAngle = 0, uint16_t maxAngle = 180)
        : BaseActuator(name, Mode::MODE_NORMAL), pin_(pin), channel_(channel),
          minAngle_(minAngle), maxAngle_(maxAngle) {}

    void init() override {
        // Servo: 50Hz PWM, 16-bit resolution for fine positioning
        ledcSetup(channel_, 50, 16);
        ledcAttachPin(pin_, channel_);
        writeAngle(0);
    }

    void update() override {
        // No timed behavior needed for basic servo
    }

protected:
    void onDataReceived() override {
        if (!enabled_) return;
        // Constrain input value to angle range
        uint16_t angle = constrain((uint16_t)data_.value, minAngle_, maxAngle_);
        writeAngle(angle);
    }

private:
    void writeAngle(uint16_t angle) {
        // Map angle (0-180) to pulse width (500-2400µs)
        // At 50Hz with 16-bit resolution: 1 tick = 1000000/(50*65536) ≈ 0.305µs
        // 500µs  = ~1638 ticks (0°)
        // 2400µs = ~7864 ticks (180°)
        uint32_t pulseUs = map(angle, 0, 180, 500, 2400);
        uint32_t duty = (uint32_t)(pulseUs * 65536.0f / 20000.0f); // 20000µs = 1/50Hz
        ledcWrite(channel_, duty);
    }

    uint8_t pin_;
    uint8_t channel_;
    uint16_t minAngle_, maxAngle_;
};

REGISTER_ACTUATOR(servo, [](JsonObjectConst cfg) -> BaseActuator* {
    uint8_t pin = cfg["pin"] | 0;
    String name = cfg["name"] | "SERVO";
    uint8_t ch = cfg["channel"] | 1;
    uint16_t minA = cfg["minAngle"] | 0;
    uint16_t maxA = cfg["maxAngle"] | 180;
    return new ActuatorServo(name, pin, ch, minA, maxA);
});

// =========================================================================
//  ActuatorMotorDC — DC motor with H-bridge (direction pin + PWM speed)
//  Faithful port of V2's ActuatorMotorDC.
//  Positive values = forward, negative = reverse.
// =========================================================================
class ActuatorMotorDC : public BaseActuator {
public:
    ActuatorMotorDC(const String& name, uint8_t pwmPin, uint8_t dirPin,
                    uint8_t channel = 2)
        : BaseActuator(name, Mode::MODE_NORMAL), pwmPin_(pwmPin),
          dirPin_(dirPin), channel_(channel) {}

    void init() override {
        pinMode(dirPin_, OUTPUT);
        digitalWrite(dirPin_, LOW);
        ledcSetup(channel_, 5000, 8); // 5kHz, 8-bit resolution
        ledcAttachPin(pwmPin_, channel_);
        ledcWrite(channel_, 0);
    }

protected:
    void onDataReceived() override {
        if (!enabled_) return;
        int16_t speed = (int16_t)data_.value;
        if (speed >= 0) {
            digitalWrite(dirPin_, LOW);
            ledcWrite(channel_, constrain(speed, 0, 255));
        } else {
            digitalWrite(dirPin_, HIGH);
            ledcWrite(channel_, constrain(-speed, 0, 255));
        }
    }

private:
    uint8_t pwmPin_;
    uint8_t dirPin_;
    uint8_t channel_;
};

REGISTER_ACTUATOR(motor_dc, [](JsonObjectConst cfg) -> BaseActuator* {
    uint8_t pwmPin = cfg["pwmPin"] | 0;
    uint8_t dirPin = cfg["dirPin"] | 0;
    String name = cfg["name"] | "MOTOR";
    uint8_t ch = cfg["channel"] | 2;
    return new ActuatorMotorDC(name, pwmPin, dirPin, ch);
});

// =========================================================================
//  ActuatorAnalogOut — Generic analog output with V2's 4 modes
//  Faithful port of V2's BaseActuatorAnalog modes:
//    UPDATE_ALWAYS, UPDATE_ON_TRUE_EVENT, PULSE_ON_TRUE_EVENT,
//    UPDATE_ON_TRUE_EVENT_ZERO_ON_FALSE_EVENT
//  Uses ESP32 LEDC for PWM output.
// =========================================================================
class ActuatorAnalogOut : public BaseActuator {
public:
    enum class AnalogMode { ALWAYS, ON_EVENT, PULSE, EVENT_OR_ZERO };

    ActuatorAnalogOut(const String& name, uint8_t pin, uint8_t channel = 3,
                      AnalogMode aMode = AnalogMode::ALWAYS)
        : BaseActuator(name, Mode::MODE_NORMAL), pin_(pin), channel_(channel),
          aMode_(aMode) {}

    void init() override {
        ledcSetup(channel_, 5000, 8);
        ledcAttachPin(pin_, channel_);
        ledcWrite(channel_, 0);
    }

    void update() override {
        if (!enabled_) return;
        // Auto-off for pulse mode
        if (aMode_ == AnalogMode::PULSE && pulseActive_ &&
            millis() - pulseStartMs_ > pulseDurationMs_) {
            ledcWrite(channel_, (uint32_t)minVal_);
            pulseActive_ = false;
        }
    }

    void setPulseDuration(uint32_t ms) { pulseDurationMs_ = ms; }
    void setRange(float minV, float maxV) { minVal_ = minV; maxVal_ = maxV; }

protected:
    void onDataReceived() override {
        if (!enabled_) return;
        uint8_t val = constrain((uint8_t)data_.value, (uint8_t)minVal_, (uint8_t)maxVal_);

        switch (aMode_) {
            case AnalogMode::ALWAYS:
                ledcWrite(channel_, val);
                break;
            case AnalogMode::ON_EVENT:
                if (data_.event) ledcWrite(channel_, val);
                break;
            case AnalogMode::PULSE:
                if (data_.event) {
                    ledcWrite(channel_, val);
                    pulseStartMs_ = millis();
                    pulseActive_ = true;
                }
                break;
            case AnalogMode::EVENT_OR_ZERO:
                ledcWrite(channel_, data_.event ? val : (uint32_t)minVal_);
                break;
        }
    }

private:
    uint8_t pin_;
    uint8_t channel_;
    AnalogMode aMode_;
    float minVal_ = 0, maxVal_ = 255;
    uint32_t pulseDurationMs_ = 1000;
    bool pulseActive_ = false;
};

REGISTER_ACTUATOR(analog_out, [](JsonObjectConst cfg) -> BaseActuator* {
    uint8_t pin = cfg["pin"] | 0;
    String name = cfg["name"] | "AOUT";
    uint8_t ch = cfg["channel"] | 3;
    String modeStr = cfg["analogMode"] | "always";
    auto mode = ActuatorAnalogOut::AnalogMode::ALWAYS;
    if (modeStr == "on_event") mode = ActuatorAnalogOut::AnalogMode::ON_EVENT;
    else if (modeStr == "pulse") mode = ActuatorAnalogOut::AnalogMode::PULSE;
    else if (modeStr == "event_or_zero") mode = ActuatorAnalogOut::AnalogMode::EVENT_OR_ZERO;
    auto* a = new ActuatorAnalogOut(name, pin, ch, mode);
    if (!cfg["pulseDuration"].isNull())
        a->setPulseDuration(cfg["pulseDuration"] | 1000);
    if (!cfg["min"].isNull() || !cfg["max"].isNull())
        a->setRange(cfg["min"] | 0.0f, cfg["max"] | 255.0f);
    return a;
});

} // namespace gluon
