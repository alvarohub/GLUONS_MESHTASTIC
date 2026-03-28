#pragma once
// ==========================================================================
//  Built-in logic modules for Gluon v3
//
//  Ported from v2: Passthrough, OR-gate, Metro, Counter
//  New: LogicThreshold (Schmitt trigger), LogicMap (range mapping)
// ==========================================================================

#include "core/BaseLogic.h"
#include "factory/ComponentFactory.h"

namespace gluon {

// =========================================================================
//  LogicPassthrough — Sends sensor data (or first inlet) directly to outlet
//  Replaces v2: logicAnalogSensor, logicN3
// =========================================================================
class LogicPassthrough : public BaseLogic {
public:
    LogicPassthrough() : BaseLogic("PASS") {}

    void init(bool firstTime) override {
        setUpdateMode(UpdateMode::SENSOR_EVENT | UpdateMode::ANY_INLET);
    }

protected:
    void compute() override {
        // Priority: sensor data, then inlet data
        if (sensors_ && sensors_->hasAnyEvent()) {
            Data d = (*sensors_)[0].data();
            outlets_->setData(d);
            if (actuators_ && actuators_->size() > 0)
                actuators_->setData(d);
        } else if (inlets_ && inlets_->hasAnyNewData()) {
            for (uint8_t i = 0; i < inlets_->size(); i++) {
                if ((*inlets_)[i].hasNewData()) {
                    Data d = (*inlets_)[i].data();
                    outlets_->setData(d);
                    if (actuators_ && actuators_->size() > 0)
                        actuators_->setData(d);
                    break;
                }
            }
        }
    }
};

REGISTER_LOGIC(passthrough, [](JsonObjectConst) -> BaseLogic* {
    return new LogicPassthrough();
});

// =========================================================================
//  LogicOR — First inlet with data wins (fan-in merge)
//  Replaces v2: logicN2
// =========================================================================
class LogicOR : public BaseLogic {
public:
    LogicOR() : BaseLogic("OR") {}

    void init(bool firstTime) override {
        setUpdateMode(UpdateMode::ANY_INLET);
    }

protected:
    void compute() override {
        for (uint8_t i = 0; i < inlets_->size(); i++) {
            if ((*inlets_)[i].hasNewData()) {
                Data d = (*inlets_)[i].data();
                outlets_->setData(d);
                // Route to matching actuator if available
                if (actuators_ && i < actuators_->size())
                    (*actuators_)[i].setData(d);
                break;
            }
        }
    }
};

REGISTER_LOGIC(or_gate, [](JsonObjectConst) -> BaseLogic* {
    return new LogicOR();
});

// =========================================================================
//  LogicAND — Output only when ALL inlets have events
// =========================================================================
class LogicAND : public BaseLogic {
public:
    LogicAND() : BaseLogic("AND") {}

    void init(bool firstTime) override {
        setUpdateMode(UpdateMode::ANY_INLET);
    }

protected:
    void compute() override {
        bool allActive = true;
        float sum = 0;
        for (uint8_t i = 0; i < inlets_->size(); i++) {
            if (!(*inlets_)[i].hasNewData() || !(*inlets_)[i].data().event) {
                allActive = false;
                break;
            }
            sum += (*inlets_)[i].data().value;
        }
        if (allActive) {
            Data d("AND", sum / inlets_->size(), true);
            outlets_->setData(d);
        }
    }
};

REGISTER_LOGIC(and_gate, [](JsonObjectConst) -> BaseLogic* {
    return new LogicAND();
});

// =========================================================================
//  LogicCounter — Counts events, outputs overflow at threshold
//  Replaces v2: logicN4 (counter)
// =========================================================================
class LogicCounter : public BaseLogic {
public:
    LogicCounter() : BaseLogic("CNT") {}

    void init(bool firstTime) override {
        setUpdateMode(UpdateMode::FIRST_INLET);
        if (!firstTime) {
            Preferences prefs;
            prefs.begin("gluon", true);
            counter_ = prefs.getUShort("cnt_val", 0);
            counterTop_ = prefs.getUShort("cnt_top", 10);
            prefs.end();
        }
    }

    void saveState(Preferences& prefs) override {
        BaseLogic::saveState(prefs);
        prefs.putUShort("cnt_val", counter_);
        prefs.putUShort("cnt_top", counterTop_);
    }

protected:
    void compute() override {
        if ((*inlets_)[0].hasNewData()) {
            Data d = (*inlets_)[0].data();
            if (d.event) {
                counter_++;
                if (counter_ >= counterTop_) {
                    counter_ = 0;
                    Data overflow("OVF", (float)counterTop_, true);
                    outlets_->setData(overflow);
                }
            }
            // Show current count on actuator (display/7-seg)
            Data display("CNT", (float)counter_, false);
            if (actuators_ && actuators_->size() > 0)
                (*actuators_)[0].setData(display);
        }

        // Inlet 1: set counter top
        if (inlets_->size() > 1 && (*inlets_)[1].hasNewData()) {
            counterTop_ = max((uint16_t)1, (uint16_t)(*inlets_)[1].data().value);
        }

        // Inlet 2: reset counter
        if (inlets_->size() > 2 && (*inlets_)[2].hasNewData() && (*inlets_)[2].data().event) {
            counter_ = 0;
        }
    }

private:
    uint16_t counter_ = 0;
    uint16_t counterTop_ = 10;
};

REGISTER_LOGIC(counter, [](JsonObjectConst) -> BaseLogic* {
    return new LogicCounter();
});

// =========================================================================
//  LogicMetro — Clock/metronome generator (beloved from v2!)
//  Replaces v2: logicMetro
//  Inlet 0: toggle on/off
//  Inlet 1: set BPM
//  Outlet: bang at tempo
// =========================================================================
class LogicMetro : public BaseLogic {
public:
    LogicMetro() : BaseLogic("METRO") {}

    void init(bool firstTime) override {
        setUpdateMode(UpdateMode::ANY_INLET | UpdateMode::SENSOR_EVENT);
        if (firstTime) {
            bpm_ = 120;
            running_ = false;
        } else {
            Preferences prefs;
            prefs.begin("gluon", true);
            bpm_ = prefs.getUShort("metro_bpm", 120);
            running_ = prefs.getBool("metro_on", false);
            prefs.end();
        }
        lastBeat_ = millis();
    }

    void saveState(Preferences& prefs) override {
        BaseLogic::saveState(prefs);
        prefs.putUShort("metro_bpm", bpm_);
        prefs.putBool("metro_on", running_);
    }

protected:
    void evolve() override {
        if (!running_) return;

        uint32_t beatPeriod = 60000 / max((uint16_t)1, bpm_);
        uint32_t now = millis();

        if (now - lastBeat_ >= beatPeriod) {
            lastBeat_ = now;

            // Send beat to outlet
            Data beat("BEAT", (float)bpm_, true);
            outlets_->setData(beat);

            // Pulse the beat LED (actuator 0)
            if (actuators_ && actuators_->size() > 0) {
                Data pulse("beat", 255.0f, true);
                (*actuators_)[0].setData(pulse);
            }
        }
    }

    void compute() override {
        // Inlet 0 or Sensor 0: toggle metro on/off
        bool toggle = false;
        if (sensors_ && sensors_->size() > 0 && (*sensors_)[0].hasEvent()) {
            toggle = true;
        }
        if (inlets_->size() > 0 && (*inlets_)[0].hasNewData() && (*inlets_)[0].data().event) {
            toggle = true;
        }
        if (toggle) {
            running_ = !running_;
            lastBeat_ = millis();
            // Show state on actuator 1 (toggle LED)
            if (actuators_ && actuators_->size() > 1) {
                Data state("metro", running_ ? 255.0f : 0.0f, running_);
                (*actuators_)[1].setData(state);
            }
        }

        // Inlet 1: set BPM from input value
        if (inlets_->size() > 1 && (*inlets_)[1].hasNewData()) {
            bpm_ = constrain((uint16_t)(*inlets_)[1].data().value, 1, 999);
            lastBeat_ = millis();
        }

        // Show BPM on display actuator (actuator 2 if present)
        if (actuators_ && actuators_->size() > 2) {
            Data disp("BPM", (float)bpm_, running_);
            (*actuators_)[2].setData(disp);
        }
    }

private:
    uint16_t bpm_ = 120;
    bool running_ = false;
    uint32_t lastBeat_ = 0;
};

REGISTER_LOGIC(metro, [](JsonObjectConst) -> BaseLogic* {
    return new LogicMetro();
});

// =========================================================================
//  LogicThreshold — Schmitt trigger with hysteresis (new in v3)
//  Useful for: "if temperature > 30, output event"
// =========================================================================
class LogicThreshold : public BaseLogic {
public:
    LogicThreshold() : BaseLogic("THRESH") {}

    void init(bool firstTime) override {
        setUpdateMode(UpdateMode::ANY_INLET | UpdateMode::SENSOR_EVENT);
    }

protected:
    void compute() override {
        float inputVal = 0;
        if (sensors_ && sensors_->hasAnyEvent()) {
            inputVal = (*sensors_)[0].data().value;
        } else if (inlets_->hasAnyNewData()) {
            inputVal = (*inlets_)[0].data().value;
        }

        bool wasAbove = isAbove_;
        if (!isAbove_ && inputVal > (threshold_ + hysteresis_)) {
            isAbove_ = true;
        } else if (isAbove_ && inputVal < (threshold_ - hysteresis_)) {
            isAbove_ = false;
        }

        // Output on transition
        if (isAbove_ != wasAbove) {
            Data d("THRESH", inputVal, isAbove_);
            outlets_->setData(d);
        }
    }

private:
    float threshold_ = 50.0f;
    float hysteresis_ = 5.0f;
    bool isAbove_ = false;
};

REGISTER_LOGIC(threshold, [](JsonObjectConst cfg) -> BaseLogic* {
    return new LogicThreshold();
});

// =========================================================================
//  LogicLoop — Countdown counter with reset
//  Faithful port of V2's Loop logic.
//  Inlet 0: decrement → bang at zero + auto-reset to counterTop
//  Inlet 1: set counterTop from value
// =========================================================================
class LogicLoop : public BaseLogic {
public:
    LogicLoop() : BaseLogic("LOOP") {}

    void init(bool firstTime) override {
        setUpdateMode(UpdateMode::ANY_INLET);
        counter_ = counterTop_;
    }

protected:
    void compute() override {
        // Inlet 1: set counter top
        if (inlets_->size() > 1 && (*inlets_)[1].hasNewData()) {
            counterTop_ = max(1, (int)(*inlets_)[1].data().value);
        }

        // Inlet 0: decrement
        if ((*inlets_)[0].hasNewData() && (*inlets_)[0].data().event) {
            counter_--;

            // Pulse actuator on every decrement
            if (actuators_ && actuators_->size() > 0) {
                Data pulse("DEC", (float)counter_, true);
                (*actuators_)[0].setData(pulse);
            }

            if (counter_ <= 0) {
                counter_ = counterTop_;
                // Fire output bang on reaching zero
                Data overflow("ZERO", (float)counterTop_, true);
                outlets_->setData(overflow);
            }
        }
    }

private:
    int counter_ = 10;
    int counterTop_ = 10;
};

REGISTER_LOGIC(loop, [](JsonObjectConst cfg) -> BaseLogic* {
    return new LogicLoop();
});

// =========================================================================
//  LogicLineFollower — Two line sensors → DC motor control
//  Faithful port of V2's LineFollower.
//  Inlet 0: right line sensor event
//  Inlet 1: left line sensor event
//  Output to actuator: signed PWM value (-255..+255)
// =========================================================================
class LogicLineFollower : public BaseLogic {
public:
    LogicLineFollower() : BaseLogic("FOLLOW") {}

    void init(bool firstTime) override {
        setUpdateMode(UpdateMode::ANY_INLET);
    }

protected:
    void compute() override {
        bool lineRight = (*inlets_)[0].hasNewData() && (*inlets_)[0].data().event;
        bool lineLeft = inlets_->size() > 1 &&
                        (*inlets_)[1].hasNewData() && (*inlets_)[1].data().event;

        float motorValue = 0;
        if (lineRight && !lineLeft) motorValue = -255.0f;   // steer left
        else if (!lineRight && lineLeft) motorValue = 255.0f; // steer right
        // both or neither: stop

        Data output("MOTOR", motorValue, (motorValue != 0));
        if (actuators_ && actuators_->size() > 0)
            (*actuators_)[0].setData(output);
        outlets_->setData(output);
    }
};

REGISTER_LOGIC(line_follower, [](JsonObjectConst cfg) -> BaseLogic* {
    return new LogicLineFollower();
});

// =========================================================================
//  LogicRouter — Route each inlet to its matching actuator
//  Generalized from V2's D1 (RGB LEDs), D7 (Button+RGB), N5 (Switch+RGB).
//  Pattern: inlet[i] → actuator[i], sensor[0] → outlet
//  Also forwards first inlet data to outlet (if no sensor event).
// =========================================================================
class LogicRouter : public BaseLogic {
public:
    LogicRouter() : BaseLogic("ROUTE") {}

    void init(bool firstTime) override {
        setUpdateMode(UpdateMode::ANY_INLET | UpdateMode::SENSOR_EVENT);
    }

protected:
    void compute() override {
        Data outputData;
        bool hasOutput = false;

        // Route sensor event to outlet (priority)
        if (sensors_ && sensors_->hasAnyEvent()) {
            outputData = (*sensors_)[0].data();
            hasOutput = true;
        }

        // Route each inlet to its matching actuator
        for (uint8_t i = 0; i < inlets_->size(); i++) {
            if ((*inlets_)[i].hasNewData()) {
                Data d = (*inlets_)[i].data();
                if (actuators_ && i < actuators_->size())
                    (*actuators_)[i].setData(d);
                // First inlet with data also goes to outlet (if no sensor event)
                if (!hasOutput) {
                    outputData = d;
                    hasOutput = true;
                }
            }
        }

        if (hasOutput) {
            outlets_->setData(outputData);
        }
    }
};

REGISTER_LOGIC(router, [](JsonObjectConst cfg) -> BaseLogic* {
    return new LogicRouter();
});

// =========================================================================
//  LogicServoControl — Servo with min/max override from secondary inlets
//  Faithful port of V2's D2 (servo module).
//  Inlet 0: angle value
//  Inlet 1: go to min (0°)
//  Inlet 2: go to max (180°)
// =========================================================================
class LogicServoControl : public BaseLogic {
public:
    LogicServoControl() : BaseLogic("SRV_CTL") {}

    void init(bool firstTime) override {
        setUpdateMode(UpdateMode::ANY_INLET);
    }

protected:
    void compute() override {
        Data outputData;

        // Check which inlet triggered
        if (inlets_->size() > 2 && (*inlets_)[2].hasNewData()) {
            // Inlet 2: max position
            outputData.set("MAX", 180.0f, true);
        } else if (inlets_->size() > 1 && (*inlets_)[1].hasNewData()) {
            // Inlet 1: min position
            outputData.set("MIN", 0.0f, true);
        } else if ((*inlets_)[0].hasNewData()) {
            // Inlet 0: angle from value
            outputData = (*inlets_)[0].data();
        }

        // Drive actuator (servo) and outlet
        if (actuators_ && actuators_->size() > 0)
            (*actuators_)[0].setData(outputData);
        outlets_->setData(outputData);
    }
};

REGISTER_LOGIC(servo_control, [](JsonObjectConst cfg) -> BaseLogic* {
    return new LogicServoControl();
});

// =========================================================================
//  LogicRangefinderBeep — Rangefinder + beeper
//  Faithful port of V2's D6.
//  Sensor event or inlet bang → beep actuator + sensor data to outlet
// =========================================================================
class LogicRangefinderBeep : public BaseLogic {
public:
    LogicRangefinderBeep() : BaseLogic("RANGE_BEEP") {}

    void init(bool firstTime) override {
        setUpdateMode(UpdateMode::ANY_INLET | UpdateMode::SENSOR_EVENT);
    }

protected:
    void compute() override {
        Data outputData;

        if (sensors_ && sensors_->hasAnyEvent()) {
            outputData = (*sensors_)[0].data();
        } else {
            // Event from inlet: pass first inlet's numeric data, event = false
            for (uint8_t i = 0; i < inlets_->size(); i++) {
                if ((*inlets_)[i].hasNewData()) {
                    outputData.set("IN", (*inlets_)[i].data().value, false);
                    break;
                }
            }
        }

        // Always beep actuator when compute triggered
        Data beepData("BEEP", 128.0f, true);
        if (actuators_ && actuators_->size() > 0)
            (*actuators_)[0].setData(beepData);

        outlets_->setData(outputData);
    }
};

REGISTER_LOGIC(rangefinder_beep, [](JsonObjectConst cfg) -> BaseLogic* {
    return new LogicRangefinderBeep();
});

} // namespace gluon
