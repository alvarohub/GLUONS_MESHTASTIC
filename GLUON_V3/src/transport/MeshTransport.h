#pragma once
// ==========================================================================
//  MeshTransport.h — Abstraction layer for mesh communication
//
//  Replaces v2's ModuleRF (RFM69-specific). Transport-agnostic interface
//  with an initial implementation for Meshtastic Serial Module.
//
//  Architecture: The Gluon app talks to a Meshtastic node over Serial.
//  Meshtastic handles all LoRa radio, mesh routing, encryption, ACKs.
//  We just send/receive application-level messages.
//
//  Future: swap this implementation for native Meshtastic Module API,
//  or direct RadioLib, or even WiFi/MQTT — without changing the Node.
// ==========================================================================

#include "core/GluonTypes.h"

namespace gluon {

// =========================================================================
//  Abstract transport interface
// =========================================================================
class MeshTransport {
public:
    virtual ~MeshTransport() = default;

    virtual void init(NodeID myNodeId) = 0;
    virtual void update() = 0;  // Poll for incoming messages

    // Send a Message to a specific node (mesh handles routing)
    virtual bool send(const Message& msg) = 0;

    // Send to broadcast (all nodes)
    virtual bool broadcast(const Message& msg) = 0;

    // Check for received messages
    virtual bool hasMessage() const = 0;
    virtual Message receive() = 0;

    // Node identity
    NodeID nodeId() const { return nodeId_; }

    // Sniffer mode (preserved from v2)
    enum class SnifferMode { OFF, ON, VERBOSE };
    void setSnifferMode(SnifferMode mode) { snifferMode_ = mode; }
    SnifferMode snifferMode() const { return snifferMode_; }

protected:
    NodeID nodeId_ = NODE_ID_INVALID;
    SnifferMode snifferMode_ = SnifferMode::OFF;
};

// =========================================================================
//  Meshtastic Serial Bridge — talks to Meshtastic via UART
//
//  Uses Meshtastic's Serial Module: text/binary messages sent over
//  serial are forwarded to the mesh on a configured channel.
//
//  For the initial prototype, we use a simple framed binary protocol:
//    [0x94][0xC3][len_hi][len_lo][msgpack_payload...]
//  This matches Meshtastic's streaming protocol framing.
//
//  Alternatively, in text mode, Meshtastic Serial Module can forward
//  plain text messages to/from the mesh (simpler for debugging).
// =========================================================================
class MeshtasticSerialTransport : public MeshTransport {
public:
    explicit MeshtasticSerialTransport(HardwareSerial& serial, uint32_t baud = 115200)
        : serial_(serial), baud_(baud) {}

    void init(NodeID myNodeId) override {
        nodeId_ = myNodeId;
        serial_.begin(baud_);
        log_i("Gluon transport: Meshtastic serial bridge on NodeID 0x%08X", myNodeId);
    }

    void update() override {
        // Read from serial, accumulate bytes, try to parse messages
        while (serial_.available()) {
            uint8_t b = serial_.read();
            rxBuf_[rxPos_++] = b;

            // Simple framing: look for msgpack start, try to deserialize
            if (rxPos_ >= 4 && rxBuf_[0] == 0x94 && rxBuf_[1] == 0xC3) {
                uint16_t expectedLen = ((uint16_t)rxBuf_[2] << 8) | rxBuf_[3];
                if (rxPos_ >= 4 + expectedLen) {
                    // We have a complete frame
                    Message msg;
                    if (msg.deserialize(rxBuf_ + 4, expectedLen)) {
                        if (pendingCount_ < MAX_PENDING) {
                            pending_[pendingCount_++] = msg;
                        }
                    }
                    rxPos_ = 0; // Reset buffer
                }
            }

            // Prevent buffer overflow
            if (rxPos_ >= sizeof(rxBuf_)) {
                rxPos_ = 0;
            }
        }
    }

    bool send(const Message& msg) override {
        uint8_t payload[256];
        size_t len = msg.serialize(payload, sizeof(payload));
        if (len == 0) return false;

        // Frame it
        uint8_t header[4] = { 0x94, 0xC3, (uint8_t)(len >> 8), (uint8_t)(len & 0xFF) };
        serial_.write(header, 4);
        serial_.write(payload, len);
        serial_.flush();

        if (snifferMode_ != SnifferMode::OFF) {
            log_i("TX -> 0x%08X type=%d len=%d", msg.receiverId, (int)msg.type, len);
        }
        return true;
    }

    bool broadcast(const Message& msg) override {
        Message bcast = msg;
        bcast.receiverId = NODE_ID_BROADCAST;
        return send(bcast);
    }

    bool hasMessage() const override { return pendingCount_ > 0; }

    Message receive() override {
        if (pendingCount_ == 0) return Message{};
        Message msg = pending_[0];
        // Shift queue
        for (uint8_t i = 1; i < pendingCount_; i++)
            pending_[i - 1] = pending_[i];
        pendingCount_--;
        return msg;
    }

private:
    HardwareSerial& serial_;
    uint32_t baud_;

    // RX buffer
    uint8_t rxBuf_[512] = {};
    uint16_t rxPos_ = 0;

    // Simple message queue
    static constexpr uint8_t MAX_PENDING = 8;
    Message pending_[MAX_PENDING];
    uint8_t pendingCount_ = 0;
};

// =========================================================================
//  Loopback transport — for testing without radio hardware
// =========================================================================
class LoopbackTransport : public MeshTransport {
public:
    void init(NodeID myNodeId) override {
        nodeId_ = myNodeId;
        log_i("Gluon transport: loopback (no radio)");
    }

    void update() override {} // Nothing to poll

    bool send(const Message& msg) override {
        // In loopback, messages sent to self are received back
        if (msg.receiverId == nodeId_ || msg.receiverId == NODE_ID_BROADCAST) {
            if (pendingCount_ < MAX_PENDING) {
                pending_[pendingCount_++] = msg;
            }
        }
        return true;
    }

    bool broadcast(const Message& msg) override { return send(msg); }
    bool hasMessage() const override { return pendingCount_ > 0; }
    Message receive() override {
        if (pendingCount_ == 0) return Message{};
        Message msg = pending_[0];
        for (uint8_t i = 1; i < pendingCount_; i++)
            pending_[i - 1] = pending_[i];
        pendingCount_--;
        return msg;
    }

private:
    static constexpr uint8_t MAX_PENDING = 8;
    Message pending_[MAX_PENDING];
    uint8_t pendingCount_ = 0;
};

} // namespace gluon
