#pragma once
// ==========================================================================
//  GluonTypes.h — Core data structures for the Gluon dataflow engine
//
//  Evolved from Gluons v2.0 utils.h (2015) for ESP32-S3.
//  Key changes from v2:
//    - Dynamic sizing (no MAX_FAN_IN/OUT compile-time limits)
//    - float instead of int16_t for numeric data
//    - Binary serialization via msgpack (ArduinoJson)
//    - std::vector, std::function, String instead of C arrays/strings
// ==========================================================================

#include <Arduino.h>
#include <vector>
#include <functional>
#include <ArduinoJson.h>

namespace gluon {

// =========================================================================
//  NodeID — Network-wide node identifier
//  In Meshtastic, nodes have 32-bit IDs. We use that directly.
// =========================================================================
using NodeID = uint32_t;
constexpr NodeID NODE_ID_INVALID = 0;
constexpr NodeID NODE_ID_BROADCAST = 0xFFFFFFFF;

// =========================================================================
//  Data — The universal data packet flowing through the network
//
//  Preserved from v2: label (was stringData), value (was numericData),
//  event flag, timestamp. Added: float precision and optional blob.
// =========================================================================
struct Data {
    String label;           // Descriptor (e.g., "button", "temp", "bpm")
    float value = 0.0f;     // Numeric payload (float, not int16_t)
    bool event = false;     // Boolean event flag (bang)
    uint32_t timestamp = 0; // millis() at creation

    Data() = default;
    Data(const String& lbl, float val, bool evt)
        : label(lbl), value(val), event(evt), timestamp(millis()) {}

    void set(const String& lbl, float val, bool evt) {
        label = lbl;
        value = val;
        event = evt;
        timestamp = millis();
    }

    // Serialize into a JsonObject (for transport)
    void toJson(JsonObject obj) const {
        obj["l"] = label;
        obj["v"] = value;
        obj["e"] = event;
        obj["t"] = timestamp;
    }

    // Deserialize from a JsonObject
    bool fromJson(JsonObjectConst obj) {
        if (obj["v"].isNull()) return false;
        label = obj["l"] | "";
        value = obj["v"] | 0.0f;
        event = obj["e"] | false;
        timestamp = obj["t"] | (uint32_t)millis();
        return true;
    }
};

// =========================================================================
//  Link — A connection to/from another node
//  Preserves the LOOSE->FIXED lifecycle from v2.
// =========================================================================
enum class LinkState { LOOSE, FIXED };

constexpr uint32_t LOOSE_PERIOD_MS = 2000;

struct Link {
    NodeID nodeId = NODE_ID_INVALID;
    uint8_t remotePort = 0;     // Which inlet/outlet on the remote node
    uint32_t timeCreated = 0;

    Link() = default;
    Link(NodeID id, uint8_t port)
        : nodeId(id), remotePort(port), timeCreated(millis()) {}

    bool isValid() const { return nodeId != NODE_ID_INVALID; }

    LinkState state() const {
        if (millis() - timeCreated < LOOSE_PERIOD_MS) return LinkState::LOOSE;
        return LinkState::FIXED;
    }
};

// =========================================================================
//  Message — What travels over the mesh transport
//
//  In v2 this was an ASCII string like "UP+button/128/1/45230".
//  In v3 we use a lightweight binary format (msgpack via ArduinoJson).
// =========================================================================
enum class MessageType : uint8_t {
    DATA_UPDATE = 0x01,       // Payload data from outlet to inlet
    LINK_REQUEST = 0x10,      // Request to create a connection
    LINK_ACK = 0x11,          // Acknowledge connection created
    LINK_DELETE = 0x12,       // Delete a connection
    LINK_MOVE = 0x13,         // Move a connection to another inlet
    PULL_PATCH_CHORD = 0x14,  // Shake → vibrate connected nodes (from v2)
    NODE_ANNOUNCE = 0x20,     // Node presence/capability announcement
    NODE_QUERY = 0x21,        // Query node capabilities
    SYNC_CLOCK = 0x30,        // Clock synchronization
    HEARTBEAT = 0x31,         // Keepalive (new in v3!)
    NETWORK_SCAN = 0x40,      // Request topology dump
    NETWORK_RESET = 0x41,     // Clear all connections
};

struct Message {
    MessageType type;
    NodeID senderId;
    NodeID receiverId;
    uint8_t senderPort;       // Outlet index on sender
    uint8_t receiverPort;     // Inlet index on receiver
    Data data;                // Payload (for DATA_UPDATE)

    // Serialize to buffer for mesh transport (returns bytes written)
    size_t serialize(uint8_t* buf, size_t maxLen) const {
        JsonDocument doc;
        doc["ty"] = (uint8_t)type;
        doc["si"] = senderId;
        doc["ri"] = receiverId;
        doc["sp"] = senderPort;
        doc["rp"] = receiverPort;
        if (type == MessageType::DATA_UPDATE) {
            JsonObject d = doc["d"].to<JsonObject>();
            data.toJson(d);
        }
        return serializeMsgPack(doc, buf, maxLen);
    }

    // Deserialize from buffer
    bool deserialize(const uint8_t* buf, size_t len) {
        JsonDocument doc;
        if (deserializeMsgPack(doc, buf, len) != DeserializationError::Ok)
            return false;
        type = (MessageType)(doc["ty"] | 0);
        senderId = doc["si"] | NODE_ID_INVALID;
        receiverId = doc["ri"] | NODE_ID_INVALID;
        senderPort = doc["sp"] | 0;
        receiverPort = doc["rp"] | 0;
        if (!doc["d"].isNull()) {
            data.fromJson(doc["d"].as<JsonObjectConst>());
        }
        return true;
    }
};

// =========================================================================
//  UpdateMode — Logic trigger modes (preserved from v2, great design)
// =========================================================================
namespace UpdateMode {
    constexpr uint8_t MANUAL        = 0x00;
    constexpr uint8_t SENSOR_EVENT  = 0x01;
    constexpr uint8_t FIRST_INLET   = 0x02;
    constexpr uint8_t ANY_INLET     = 0x04;
    constexpr uint8_t SYNC          = 0x08;
    constexpr uint8_t PERIODIC      = 0x10;
    constexpr uint8_t HEARTBEAT_IN  = 0x20; // New: trigger on heartbeat
}

} // namespace gluon
