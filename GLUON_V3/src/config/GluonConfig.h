#pragma once
// ==========================================================================
//  GluonConfig.h — JSON-based runtime configuration
//
//  Replaces v2's compile-time #define GLUON system.
//  Config is stored as /config.json on LittleFS and can be:
//    - Pre-loaded at flash time (via PlatformIO data/ folder)
//    - Modified over serial/mesh at runtime
//    - Auto-generated from hardware detection (future: I2C scan)
// ==========================================================================

#include <ArduinoJson.h>
#include <LittleFS.h>

namespace gluon {

struct GluonConfig {
    // Node identity
    String name = "GLUON";
    uint32_t nodeId = 0;       // 0 = auto-assign from Meshtastic
    uint8_t numInlets = 3;
    uint8_t numOutlets = 1;
    uint8_t maxFanIn = 3;
    uint8_t maxFanOut = 8;

    // Logic
    String logicType = "passthrough";
    uint8_t updateMode = 0x04;  // ANY_INLET by default
    uint32_t periodicMs = 1000;

    // Sensors (array of {type, name, pin, ...})
    JsonDocument sensorsDoc;

    // Actuators (array of {type, name, pin, mode, ...})
    JsonDocument actuatorsDoc;

    // Transport
    String transportType = "meshtastic_serial";
    uint32_t serialBaud = 115200;
    uint32_t latchPeriodMs = 200;  // Min interval between outgoing messages

    // Display
    bool displayEnabled = true;
    String displayDriver = "ssd1306_128x64";  // XIAO Expansion Board OLED

    // Load from LittleFS
    bool load(const char* path = "/config.json") {
        if (!LittleFS.begin(true)) {
            log_e("LittleFS mount failed");
            return false;
        }
        File f = LittleFS.open(path, "r");
        if (!f) {
            log_w("Config file not found: %s (using defaults)", path);
            return false;
        }

        JsonDocument doc;
        auto err = deserializeJson(doc, f);
        f.close();
        if (err) {
            log_e("Config parse error: %s", err.c_str());
            return false;
        }

        fromJson(doc.as<JsonObjectConst>());
        log_i("Config loaded from %s: name=%s, logic=%s", path, name.c_str(), logicType.c_str());
        return true;
    }

    // Save to LittleFS
    bool save(const char* path = "/config.json") const {
        if (!LittleFS.begin(true)) return false;
        File f = LittleFS.open(path, "w");
        if (!f) return false;
        JsonDocument doc;
        toJson(doc.to<JsonObject>());
        serializeJsonPretty(doc, f);
        f.close();
        return true;
    }

    void fromJson(JsonObjectConst obj) {
        name = obj["name"] | name;
        nodeId = obj["nodeId"] | nodeId;
        numInlets = obj["inlets"] | numInlets;
        numOutlets = obj["outlets"] | numOutlets;
        maxFanIn = obj["maxFanIn"] | maxFanIn;
        maxFanOut = obj["maxFanOut"] | maxFanOut;
        logicType = obj["logic"] | logicType;
        updateMode = obj["updateMode"] | updateMode;
        periodicMs = obj["periodicMs"] | periodicMs;
        transportType = obj["transport"] | transportType;
        serialBaud = obj["serialBaud"] | serialBaud;
        latchPeriodMs = obj["latchPeriodMs"] | latchPeriodMs;
        displayEnabled = obj["displayEnabled"] | displayEnabled;
        displayDriver = obj["displayDriver"] | displayDriver;

        if (!obj["sensors"].isNull()) {
            sensorsDoc.clear();
            sensorsDoc.set(obj["sensors"]);
        }
        if (!obj["actuators"].isNull()) {
            actuatorsDoc.clear();
            actuatorsDoc.set(obj["actuators"]);
        }
    }

    void toJson(JsonObject obj) const {
        obj["name"] = name;
        obj["nodeId"] = nodeId;
        obj["inlets"] = numInlets;
        obj["outlets"] = numOutlets;
        obj["maxFanIn"] = maxFanIn;
        obj["maxFanOut"] = maxFanOut;
        obj["logic"] = logicType;
        obj["updateMode"] = updateMode;
        obj["periodicMs"] = periodicMs;
        obj["transport"] = transportType;
        obj["serialBaud"] = serialBaud;
        obj["latchPeriodMs"] = latchPeriodMs;
        obj["displayEnabled"] = displayEnabled;
        obj["displayDriver"] = displayDriver;
        obj["sensors"] = sensorsDoc;
        obj["actuators"] = actuatorsDoc;
    }
};

} // namespace gluon
