#pragma once
// ==========================================================================
//  ComponentFactory.h — Runtime factory for sensors, actuators, and logic
//
//  Replaces v2's compile-time #define GLUON + #if/#elif blocks.
//  Modules self-register via static initializers. New module types
//  can be added by simply including a header file.
//
//  This is the key to making Gluons extensible by the community:
//  anyone can write a new sensor/actuator/logic .h file and register it.
// ==========================================================================

#include "core/BaseSensor.h"
#include "core/BaseActuator.h"
#include "core/BaseLogic.h"
#include <ArduinoJson.h>
#include <map>
#include <functional>

namespace gluon {

// =========================================================================
//  Factory template — one factory per component type
// =========================================================================
template <typename T>
class ComponentFactory {
public:
    using CreateFn = std::function<T*(JsonObjectConst config)>;

    static ComponentFactory& instance() {
        static ComponentFactory inst;
        return inst;
    }

    void registerType(const String& type, CreateFn fn) {
        registry_[type] = fn;
        log_i("Registered component type: %s", type.c_str());
    }

    T* create(JsonObjectConst config) {
        String type = config["type"] | "unknown";
        auto it = registry_.find(type);
        if (it == registry_.end()) {
            log_e("Unknown component type: %s", type.c_str());
            return nullptr;
        }
        return it->second(config);
    }

    bool hasType(const String& type) const {
        return registry_.count(type) > 0;
    }

    // List all registered types (for discovery/UI)
    std::vector<String> registeredTypes() const {
        std::vector<String> types;
        for (const auto& pair : registry_)
            types.push_back(pair.first);
        return types;
    }

private:
    ComponentFactory() = default;
    std::map<String, CreateFn> registry_;
};

// Convenience aliases
using SensorFactory = ComponentFactory<BaseSensor>;
using ActuatorFactory = ComponentFactory<BaseActuator>;
using LogicFactory = ComponentFactory<BaseLogic>;

// =========================================================================
//  Registration helper macro
//  Usage in a module header:
//    REGISTER_SENSOR("switch", [](JsonObjectConst cfg) {
//        return new SensorSwitch(cfg);
//    });
// =========================================================================
#define REGISTER_SENSOR(type, fn) \
    static bool _reg_sensor_##type = [] { \
        gluon::SensorFactory::instance().registerType(#type, fn); \
        return true; \
    }()

#define REGISTER_ACTUATOR(type, fn) \
    static bool _reg_actuator_##type = [] { \
        gluon::ActuatorFactory::instance().registerType(#type, fn); \
        return true; \
    }()

#define REGISTER_LOGIC(type, fn) \
    static bool _reg_logic_##type = [] { \
        gluon::LogicFactory::instance().registerType(#type, fn); \
        return true; \
    }()

// =========================================================================
//  Node Builder — Constructs a complete node from JSON config
// =========================================================================
class NodeBuilder {
public:
    // Populate a GluonNode's sensors from the config's sensor array
    static void buildSensors(SensorArray& sensors, JsonArrayConst arr) {
        if (arr.isNull()) return;
        for (JsonObjectConst sensorCfg : arr) {
            BaseSensor* s = SensorFactory::instance().create(sensorCfg);
            if (s) sensors.add(s);
        }
    }

    // Populate actuators
    static void buildActuators(ActuatorArray& actuators, JsonArrayConst arr) {
        if (arr.isNull()) return;
        for (JsonObjectConst actCfg : arr) {
            BaseActuator* a = ActuatorFactory::instance().create(actCfg);
            if (a) actuators.add(a);
        }
    }

    // Create the logic module
    static BaseLogic* buildLogic(const String& type) {
        JsonDocument doc;
        doc["type"] = type;
        return LogicFactory::instance().create(doc.as<JsonObjectConst>());
    }
};

} // namespace gluon
