#pragma once
// ==========================================================================
//  Built-in sensor types for Gluon v3
//  Each sensor self-registers with the factory.
//
//  Ported from v2: SensorSwitch, SensorAnalog (was SensorSharp/LDR/Pot)
//  New: SensorI2C for Grove sensors with auto-detection
// ==========================================================================

#include "core/BaseSensor.h"
#include "factory/ComponentFactory.h"
#include <Wire.h>

namespace gluon {

// =========================================================================
//  SensorDigitalPin — Digital input (button, switch, tilt, PIR...)
//  Replaces v2: SensorSwitch, tiltSensor
// =========================================================================
class SensorDigitalPin : public BaseSensor {
public:
    SensorDigitalPin(const String& name, uint8_t pin, bool pullup = true, bool invert = false)
        : BaseSensor(name, Type::PIN_DIGITAL), pin_(pin), invert_(invert) {
        pinMode(pin_, pullup ? INPUT_PULLUP : INPUT);
        setEventMode(EventMode::ON_ENTERING_COND);
        setConditionValue(1.0f);
        setToleranceValue(0.5f);
        data_.label = name;
    }

    void update() override {
        if (!enabled_) return;
        bool raw = digitalRead(pin_);
        if (invert_) raw = !raw;

        // Debounce
        if (raw != lastRaw_) {
            lastChangeTime_ = millis();
            lastRaw_ = raw;
        }
        if (millis() - lastChangeTime_ < debounceMs_) return;

        detectEvent(raw ? 1.0f : 0.0f);
        data_.label = name_;
    }

    void setDebounce(uint32_t ms) { debounceMs_ = ms; }

private:
    uint8_t pin_;
    bool invert_;
    bool lastRaw_ = false;
    uint32_t lastChangeTime_ = 0;
    uint32_t debounceMs_ = 50;
};

// Self-register
REGISTER_SENSOR(switch, [](JsonObjectConst cfg) -> BaseSensor* {
    uint8_t pin = cfg["pin"] | 0;
    String name = cfg["name"] | "SW";
    bool pullup = cfg["pullup"] | true;
    bool invert = cfg["invert"] | false;
    auto* s = new SensorDigitalPin(name, pin, pullup, invert);
    if (!cfg["debounce"].isNull())
        s->setDebounce(cfg["debounce"] | 50);
    return s;
});

// =========================================================================
//  SensorAnalogPin — Analog input (potentiometer, LDR, thermistor, Sharp IR...)
//  Replaces v2: SensorSharp, SensorLDR, SensorAnalogPot,
//               SensorAnalogRotaryEncoder, SensorTemperature
// =========================================================================
class SensorAnalogPin : public BaseSensor {
public:
    SensorAnalogPin(const String& name, uint8_t pin, float minVal = 0.0f, float maxVal = 100.0f)
        : BaseSensor(name, Type::PIN_ANALOG), pin_(pin), minVal_(minVal), maxVal_(maxVal) {
        setEventMode(EventMode::ON_SIMPLE_CHANGE);
        setToleranceValue(1.0f);
        data_.label = name;
    }

    void init() override {
        analogReadResolution(12); // ESP32: 12-bit ADC (0-4095)
    }

    void update() override {
        if (!enabled_) return;
        uint16_t raw = analogRead(pin_);
        float mapped = mapFloat(raw, 0.0f, 4095.0f, minVal_, maxVal_);

        // Optional smoothing
        if (smoothing_ > 0) {
            smoothed_ = smoothed_ * smoothing_ + mapped * (1.0f - smoothing_);
            mapped = smoothed_;
        }

        detectEvent(mapped);
        data_.label = name_;
    }

    void learn(uint32_t durationMs) override {
        // V2-faithful: sample mean ± 2*stddev → conditionValue ± toleranceValue
        uint32_t start = millis();
        float sum = 0, sumSq = 0;
        uint32_t n = 0;
        while (millis() - start < durationMs) {
            float v = analogRead(pin_);
            float mapped = mapFloat(v, 0.0f, 4095.0f, minVal_, maxVal_);
            sum += mapped;
            sumSq += mapped * mapped;
            n++;
            delay(10);
        }
        if (n > 1) {
            float mean = sum / n;
            float variance = (sumSq / n) - (mean * mean);
            float stddev = sqrtf(fabs(variance));
            setConditionValue(mean);
            setToleranceValue(2.0f * stddev + 1.0f); // +1 to avoid zero tolerance
            log_i("Learned: pin %d mean=%.1f tol=%.1f (n=%lu)", pin_, mean, toleranceValue(), n);
        }
    }

    void setSmoothing(float alpha) { smoothing_ = constrain(alpha, 0.0f, 0.99f); }

private:
    static float mapFloat(float x, float inMin, float inMax, float outMin, float outMax) {
        return (x - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
    }

    uint8_t pin_;
    float minVal_, maxVal_;
    float smoothing_ = 0.0f;
    float smoothed_ = 0.0f;
};

REGISTER_SENSOR(analog, [](JsonObjectConst cfg) -> BaseSensor* {
    uint8_t pin = cfg["pin"] | 0;
    String name = cfg["name"] | "ANA";
    float minVal = cfg["min"] | 0.0f;
    float maxVal = cfg["max"] | 100.0f;
    auto* s = new SensorAnalogPin(name, pin, minVal, maxVal);
    if (!cfg["smoothing"].isNull())
        s->setSmoothing(cfg["smoothing"] | 0.0f);
    if (!cfg["tolerance"].isNull())
        s->setToleranceValue(cfg["tolerance"] | 1.0f);
    if (!cfg["conditionValue"].isNull())
        s->setConditionValue(cfg["conditionValue"] | 512.0f);
    s->setLearnable(cfg["learnable"] | true);
    return s;
});

// =========================================================================
//  SensorRotaryEncoder — ISR-based quadrature encoder
//  Faithful port of V2's sensorRotaryEncoder, using ESP32 GPIO interrupts
//  instead of AVR PCInterrupt.
// =========================================================================
class SensorRotaryEncoder : public BaseSensor {
public:
    SensorRotaryEncoder(const String& name, uint8_t pinA, uint8_t pinB)
        : BaseSensor(name, Type::PIN_DIGITAL), pinA_(pinA), pinB_(pinB) {
        data_.label = name;
    }

    ~SensorRotaryEncoder() override { disable(); }

    void init() override {
        setToleranceValue(1.0f);
        setEventMode(EventMode::ON_SIMPLE_CHANGE);
        // Learning disabled — tolerance is fixed for step detection

        pinMode(pinA_, INPUT_PULLUP);
        pinMode(pinB_, INPUT_PULLUP);

        reset();
        enable();
    }

    void update() override {
        if (!enabled_) return;
        int32_t pos;
        portENTER_CRITICAL(&mux_);
        pos = position_;
        portEXIT_CRITICAL(&mux_);

        detectEvent((float)pos);
        data_.label = name_;
    }

    void enable() {
        enabled_ = true;
        instance_ = this;
        attachInterruptArg(digitalPinToInterrupt(pinA_), isrA, this, CHANGE);
        attachInterruptArg(digitalPinToInterrupt(pinB_), isrB, this, CHANGE);
    }

    void disable() {
        enabled_ = false;
        detachInterrupt(digitalPinToInterrupt(pinA_));
        detachInterrupt(digitalPinToInterrupt(pinB_));
    }

    void reset() {
        portENTER_CRITICAL(&mux_);
        position_ = 0;
        state_ = 0;
        if (digitalRead(pinA_)) state_ |= 4;
        if (digitalRead(pinB_)) state_ |= 8;
        portEXIT_CRITICAL(&mux_);
    }

    int32_t position() const {
        int32_t pos;
        portENTER_CRITICAL(&mux_);
        pos = position_;
        portEXIT_CRITICAL(&mux_);
        return pos;
    }

private:
    // V2-faithful quadrature state machine (in ISR context)
    static void IRAM_ATTR updatePosition(SensorRotaryEncoder* self) {
        uint8_t s = self->state_ & 3;
        if (digitalRead(self->pinA_)) s |= 4;
        if (digitalRead(self->pinB_)) s |= 8;
        switch (s) {
            case 0: case 5: case 10: case 15:
                break;
            case 1: case 7: case 8: case 14:
                self->position_++; break;
            case 2: case 4: case 11: case 13:
                self->position_--; break;
            case 3: case 12:
                self->position_ += 2; break;
            default:
                self->position_ -= 2; break;
        }
        self->state_ = (s >> 2);
    }

    static void IRAM_ATTR isrA(void* arg) {
        updatePosition(static_cast<SensorRotaryEncoder*>(arg));
    }
    static void IRAM_ATTR isrB(void* arg) {
        updatePosition(static_cast<SensorRotaryEncoder*>(arg));
    }

    uint8_t pinA_, pinB_;
    volatile int32_t position_ = 0;
    volatile uint8_t state_ = 0;
    mutable portMUX_TYPE mux_ = portMUX_INITIALIZER_UNLOCKED;
    static SensorRotaryEncoder* instance_; // for reference if needed
};

// Static init
inline SensorRotaryEncoder* SensorRotaryEncoder::instance_ = nullptr;

REGISTER_SENSOR(rotary_encoder, [](JsonObjectConst cfg) -> BaseSensor* {
    uint8_t pinA = cfg["pinA"] | 0;
    uint8_t pinB = cfg["pinB"] | 0;
    String name = cfg["name"] | "ROT_ENC";
    return new SensorRotaryEncoder(name, pinA, pinB);
});

// =========================================================================
//  SensorDiscreteValues — Analog pin with linear discretization
//  Faithful port of V2's sensorDiscreteValues.
//  Also replaces V2's sensorAnalogRotaryEncoder (DFRobot 12-pos pot).
//  Outputs integer levels 0 .. numLevels-1
// =========================================================================
class SensorDiscreteValues : public BaseSensor {
public:
    SensorDiscreteValues(const String& name, uint8_t pin, uint8_t numLevels,
                         float minVolts = 0.0f, float maxVolts = 3.3f)
        : BaseSensor(name, Type::PIN_ANALOG), pin_(pin),
          numLevels_(numLevels), minVolts_(minVolts), maxVolts_(maxVolts) {
        data_.label = name;
    }

    void init() override {
        analogReadResolution(12); // ESP32: 12-bit (0-4095)
        setToleranceValue(1.0f);
        setEventMode(EventMode::ON_SIMPLE_CHANGE);
        // Learning disabled — step detection is deterministic
    }

    void update() override {
        if (!enabled_) return;

        float readVolts = 3.3f * analogRead(pin_) / 4095.0f;
        float intervalVolts = (maxVolts_ - minVolts_) / numLevels_;

        uint8_t level = 0;
        while (level < (numLevels_ - 1) &&
               readVolts > ((level + 1) * intervalVolts + minVolts_)) {
            level++;
        }

        detectEvent((float)level);
        data_.label = name_;
    }

private:
    uint8_t pin_;
    uint8_t numLevels_;
    float minVolts_, maxVolts_;
};

REGISTER_SENSOR(discrete, [](JsonObjectConst cfg) -> BaseSensor* {
    uint8_t pin = cfg["pin"] | 0;
    String name = cfg["name"] | "DISC";
    uint8_t levels = cfg["levels"] | 12;
    float minV = cfg["minVolts"] | 0.0f;
    float maxV = cfg["maxVolts"] | 3.3f;
    return new SensorDiscreteValues(name, pin, levels, minV, maxV);
});

// =========================================================================
//  SensorUltrasonicI2C — I2C ultrasonic rangefinder (cm)
//  Faithful port of V2's sensorMaxSonar + sensorSRF10.
//  Covers: MaxBotix I2C-XL (addr 0x70), Devantech SRF10/02/08 (addr 0x70)
//  Configurable I2C address and protocol variant.
// =========================================================================
class SensorUltrasonicI2C : public BaseSensor {
public:
    enum class Protocol { MAXSONAR, SRF10 };

    SensorUltrasonicI2C(const String& name, uint8_t addr = 0x70,
                        Protocol proto = Protocol::SRF10)
        : BaseSensor(name, Type::BUS_I2C), addr_(addr), proto_(proto) {
        data_.label = name;
    }

    void init() override {
        Wire.begin();
        setConditionValue(15.0f);
        setToleranceValue(2.0f);
        setEventMode(EventMode::ON_ENTERING_COND);
        setLearnable(true);
    }

    void update() override {
        if (!enabled_) return;
        // Rate-limit I2C reads (sensor needs settling time)
        uint32_t now = millis();
        if (now - lastReadMs_ < readIntervalMs_) return;
        lastReadMs_ = now;

        uint16_t cm = (proto_ == Protocol::MAXSONAR) ? readMaxSonar() : readSRF10();
        detectEvent((float)cm);
        data_.label = name_;
    }

    void learn(uint32_t durationMs) override {
        uint32_t start = millis();
        float sum = 0, sumSq = 0;
        uint32_t n = 0;
        while (millis() - start < durationMs) {
            uint16_t cm = (proto_ == Protocol::MAXSONAR) ? readMaxSonar() : readSRF10();
            sum += cm;
            sumSq += (float)cm * cm;
            n++;
            delay(100);
        }
        if (n > 1) {
            float mean = sum / n;
            float variance = (sumSq / n) - (mean * mean);
            float stddev = sqrtf(fabs(variance));
            setConditionValue(mean);
            setToleranceValue(2.0f * stddev + 1.0f);
            log_i("Learned ultrasonic: mean=%.1f tol=%.1f (n=%lu)", mean, toleranceValue(), n);
        }
    }

    void setReadInterval(uint32_t ms) { readIntervalMs_ = ms; }

private:
    // MaxBotix I2C-XL protocol
    uint16_t readMaxSonar() {
        Wire.beginTransmission(addr_);
        Wire.write(0x51); // range command (cm)
        Wire.endTransmission();
        delay(100);
        Wire.requestFrom(addr_, (uint8_t)2);
        if (Wire.available() >= 2) {
            uint8_t hi = Wire.read();
            uint8_t lo = Wire.read();
            return (uint16_t)word(hi, lo);
        }
        return 0;
    }

    // Devantech SRF10/SRF02/SRF08 protocol
    uint16_t readSRF10() {
        Wire.beginTransmission(addr_);
        Wire.write(0x00);
        Wire.write(0x51); // command: range in cm
        Wire.endTransmission();
        delay(70);
        Wire.beginTransmission(addr_);
        Wire.write(0x02); // echo register
        Wire.endTransmission();
        Wire.requestFrom(addr_, (uint8_t)2);
        if (Wire.available() >= 2) {
            uint16_t reading = Wire.read();
            reading = (reading << 8) | Wire.read();
            return reading;
        }
        return 0;
    }

    uint8_t addr_;
    Protocol proto_;
    uint32_t lastReadMs_ = 0;
    uint32_t readIntervalMs_ = 100;
};

REGISTER_SENSOR(ultrasonic_i2c, [](JsonObjectConst cfg) -> BaseSensor* {
    uint8_t addr = cfg["address"] | 0x70;
    String name = cfg["name"] | "SONAR";
    String proto = cfg["protocol"] | "srf10";
    auto p = (proto == "maxsonar") ? SensorUltrasonicI2C::Protocol::MAXSONAR
                                   : SensorUltrasonicI2C::Protocol::SRF10;
    auto* s = new SensorUltrasonicI2C(name, addr, p);
    if (!cfg["readInterval"].isNull())
        s->setReadInterval(cfg["readInterval"] | 100);
    if (!cfg["conditionValue"].isNull())
        s->setConditionValue(cfg["conditionValue"] | 15.0f);
    if (!cfg["tolerance"].isNull())
        s->setToleranceValue(cfg["tolerance"] | 2.0f);
    return s;
});

// =========================================================================
//  SensorI2C — Generic I2C sensor (for Grove modules)
//  This is new in v3 — auto-detects common I2C devices by address.
//  TODO: Implement specific drivers for common Grove sensors:
//    - 0x38: DHT20 (temp/humidity) — Grove DHT20
//    - 0x76/0x77: BME280 (temp/humidity/pressure)
//    - 0x29: VL53L0X (distance) — Grove Time of Flight
//    - 0x48: ADS1115 (ADC)
//    - etc.
// =========================================================================
class SensorI2CGeneric : public BaseSensor {
public:
    SensorI2CGeneric(const String& name, uint8_t addr)
        : BaseSensor(name, Type::BUS_I2C), addr_(addr) {
        data_.label = name;
    }

    void init() override {
        // TODO: Wire.begin() and probe address
    }

    void update() override {
        // TODO: Read from specific I2C driver based on addr_
    }

private:
    uint8_t addr_;
};

REGISTER_SENSOR(i2c, [](JsonObjectConst cfg) -> BaseSensor* {
    uint8_t addr = cfg["address"] | 0x00;
    String name = cfg["name"] | "I2C";
    return new SensorI2CGeneric(name, addr);
});

} // namespace gluon
