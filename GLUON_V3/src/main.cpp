// ==========================================================================
//  GLUON V3 — main.cpp
//
//  Entry point for a Gluon node on ESP32-S3 (XIAO or Heltec).
//  Reads /config.json from LittleFS, builds the node from factories,
//  configures interaction hardware from BoardPins, and runs the main loop.
//
//  Compare with v2's GLUON_GENERIC.cpp which had 400+ lines of
//  #if GLUON==X blocks. Now it's ~80 lines regardless of module type.
// ==========================================================================

#include <Arduino.h>
#include "core/GluonNode.h"
#include "config/GluonConfig.h"
#include "transport/MeshTransport.h"
#include "factory/ComponentFactory.h"
#include "boards/BoardPins.h"

// Including these headers registers all built-in types with the factories
#include "modules/BuiltinSensors.h"
#include "modules/BuiltinActuators.h"
#include "modules/BuiltinLogics.h"
#include "modules/BuiltinDisplays.h"  // Optional display actuators (conditional on libs)

using namespace gluon;

// Forward declaration
void processSerial();

// ---- Globals ----
GluonConfig config;
GluonNode node;

// Transport (choose one at compile time for now, or from config later)
#ifdef USE_MESHTASTIC_SERIAL
MeshtasticSerialTransport transport(Serial1);
#else
LoopbackTransport transport;
#endif

void setup() {
    Serial.begin(115200);
    delay(1000); // Let USB CDC stabilize on ESP32-S3
    Serial.println("\n=== GLUON V3 ===");
    Serial.printf("Board: %s\n", board::BOARD_NAME);

    // 1. Power control (Heltec Vext for OLED/sensors)
    if (board::VEXT_PIN >= 0) {
        pinMode(board::VEXT_PIN, OUTPUT);
        digitalWrite(board::VEXT_PIN, LOW); // LOW = power ON
    }

    // 2. Load config from LittleFS
    if (!config.load("/config.json")) {
        Serial.println("Using default config (no /config.json found)");
        Serial.println("Upload a config via: pio run -t uploadfs");
    }

    Serial.printf("Node: %s | Logic: %s | Transport: %s\n",
                  config.name.c_str(), config.logicType.c_str(),
                  config.transportType.c_str());

    // 3. Build sensors from config
    NodeBuilder::buildSensors(node.sensors(),
        config.sensorsDoc.as<JsonArrayConst>());
    Serial.printf("  Sensors: %d registered\n", node.sensors().size());

    // 4. Build actuators from config
    NodeBuilder::buildActuators(node.actuators(),
        config.actuatorsDoc.as<JsonArrayConst>());
    Serial.printf("  Actuators: %d registered\n", node.actuators().size());

    // 5. Build logic module from config
    BaseLogic* logic = NodeBuilder::buildLogic(config.logicType);
    if (logic) {
        node.setLogic(logic);
        Serial.printf("  Logic: %s\n", logic->name().c_str());
    } else {
        Serial.printf("  WARNING: Unknown logic type '%s'\n", config.logicType.c_str());
    }

    // 6. Configure interaction hardware pins from BoardPins (V2 common objects)
    node.setChirpPin(board::CHIRP_PIN);
    node.setVibratorPin(board::VIBRATOR_PIN, board::VIBRATOR_LEDC_CH);
    node.setTiltPin(board::TILT_PIN);
    node.setLedPin(board::NEOPIXEL_PIN);
    node.setLearningButtonPin(board::LEARNING_BTN_PIN,
                              board::LEARNING_BTN_PIN >= 0); // enable only if wired
    node.setIRPins(board::IR_SEND_PIN, board::IR_RECV_PIN,
                   board::IR_SEND_PIN >= 0 || board::IR_RECV_PIN >= 0);

    // 7. Initialize the node (sets up inlets/outlets, loads connections,
    //    initializes interaction hardware, startup animation, etc.)
    node.init(config, &transport);

    // 8. Print registered factory types (useful for debugging/discovery)
    Serial.println("Available types:");
    Serial.print("  Sensors: ");
    for (const auto& t : SensorFactory::instance().registeredTypes())
        Serial.printf("%s ", t.c_str());
    Serial.println();
    Serial.print("  Actuators: ");
    for (const auto& t : ActuatorFactory::instance().registeredTypes())
        Serial.printf("%s ", t.c_str());
    Serial.println();
    Serial.print("  Logic: ");
    for (const auto& t : LogicFactory::instance().registeredTypes())
        Serial.printf("%s ", t.c_str());
    Serial.println();

    Serial.println("=== READY ===\n");
}

void loop() {
    node.update();

    // Optional: serial command interface for debugging
    processSerial();
}

// =========================================================================
//  Simple serial console (replaces v2's sniffer serial commands)
// =========================================================================
void processSerial() {
    if (!Serial.available()) return;

    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd == "status") {
        Serial.printf("Node: %s (0x%08X)\n", node.name().c_str(), node.nodeId());
        Serial.printf("Inlets: %d (links: %d)\n", node.inlets().size(), node.totalInletLinks());
        Serial.printf("Outlets: %d (links: %d)\n", node.outlets().size(), node.totalOutletLinks());
        if (node.logic())
            Serial.printf("Logic: %s (mode: 0x%02X)\n", node.logic()->name().c_str(), node.logic()->getUpdateMode());
    }
    else if (cmd == "connections") {
        for (uint8_t i = 0; i < node.inlets().size(); i++) {
            Serial.printf("  IN[%d]: ", i);
            for (const auto& link : node.inlets()[i].links())
                Serial.printf("0x%08X ", link.nodeId);
            Serial.println(node.inlets()[i].numLinks() == 0 ? "(none)" : "");
        }
        for (uint8_t i = 0; i < node.outlets().size(); i++) {
            Serial.printf("  OUT[%d]: ", i);
            for (const auto& link : node.outlets()[i].links())
                Serial.printf("0x%08X ", link.nodeId);
            Serial.println(node.outlets()[i].numLinks() == 0 ? "(none)" : "");
        }
    }
    else if (cmd == "reset") {
        node.disconnectAll();
        Serial.println("All connections cleared.");
    }
    else if (cmd == "learn") {
        Serial.println("Learning sensor conditions...");
        node.sensors().learnConditions();
        Serial.println("Done.");
    }
    else if (cmd == "chirp") {
        node.chirp().chirpUp();
        Serial.println("Chirp!");
    }
    else if (cmd == "vibrate") {
        node.vibrator().pulse(200, 255);
        Serial.println("Vibrate!");
    }
    else if (cmd == "types") {
        Serial.println("Registered types:");
        for (const auto& t : SensorFactory::instance().registeredTypes())
            Serial.printf("  sensor: %s\n", t.c_str());
        for (const auto& t : ActuatorFactory::instance().registeredTypes())
            Serial.printf("  actuator: %s\n", t.c_str());
        for (const auto& t : LogicFactory::instance().registeredTypes())
            Serial.printf("  logic: %s\n", t.c_str());
    }
    else if (cmd == "help") {
        Serial.println("Commands: status, connections, reset, learn, chirp, vibrate, types, help");
    }
}
