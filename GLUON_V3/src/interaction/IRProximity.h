#pragma once
// ==========================================================================
//  IRProximity.h — IR-based proximity connection discovery
//
//  Faithful port of V2's moduleIR to ESP32-S3, using the IRremote library
//  (RMT peripheral — no bit-banging, no timer hijacking).
//
//  V2 flow preserved:
//    1. Each node periodically sends an IR NEC beacon with its NodeID
//    2. Nearby node receives beacon → requestConnection() on inlets
//    3. Result (ADDED/DELETED/MOVED) triggers RF ACK over mesh
//    4. Chirp + LED feedback on both sides
//
//  The beacon type depends on outlet capacity:
//    - Outlets not full → REQUEST_CREATE (0xAAAA): "I want to connect"
//    - Outlets full     → REQUEST_MOVE   (0xBBBB): "I'll displace a LOOSE link"
//
//  NEC encoding (32-bit raw code):
//    [ command (16 bits) ][ nodeId lower 16 bits ]
//    This is identical to V2's MessageIR encoding.
//
//  On ESP32-S3, the IRremote library uses the RMT peripheral:
//    - TX: RMT channel for 38kHz carrier generation
//    - RX: RMT channel for demodulated signal timing
//    Both are non-blocking and interrupt-driven.
// ==========================================================================

// IRremote must be configured BEFORE including its header
#define DECODE_NEC          1  // Only enable NEC protocol (saves flash)
#define NO_LED_FEEDBACK_CODE 1 // Don't blink any LED on IR activity
#define RAW_BUFFER_LENGTH   68 // NEC needs 68 entries (standard)
#define EXCLUDE_EXOTIC_PROTOCOLS 1

#include <IRremote.hpp>
#include "core/GluonTypes.h"

namespace gluon {

// V2 IR commands — same encoding
constexpr uint16_t IR_CMD_REQUEST_CREATE = 0xAAAA;  // Outlet has space
constexpr uint16_t IR_CMD_REQUEST_MOVE   = 0xBBBB;  // Outlet is full

// V2 timing constants
constexpr uint32_t IR_BEACON_PERIOD_MS   = 700;     // Beacon interval
constexpr int32_t  IR_RANDOM_JITTER_MS   = 15;      // Random jitter unit (ms)
constexpr int8_t   IR_JITTER_RANGE_LOW   = -1;      // Multiplier low
constexpr int8_t   IR_JITTER_RANGE_HIGH  = 4;       // Multiplier high

// V2 remote control codes (for IR remote configuration — optional)
constexpr uint32_t IR_CODE_FORCE_BANG        = 0xFFC23D;
constexpr uint32_t IR_CODE_SET_STANDBY       = 0xFF6897;
constexpr uint32_t IR_CODE_SENSOR_EVENT      = 0xFF30CF;
constexpr uint32_t IR_CODE_FIRST_INLET       = 0xFF18E7;
constexpr uint32_t IR_CODE_ANY_INLET         = 0xFF7A85;
constexpr uint32_t IR_CODE_SYNCH             = 0xFF10EF;
constexpr uint32_t IR_CODE_PERIODIC          = 0xFF38C7;
constexpr uint32_t IR_CODE_CLEAR_ALL         = 0xFF629D;
constexpr uint32_t IR_CODE_CLEAR_INLETS      = 0xFFA25D;
constexpr uint32_t IR_CODE_CLEAR_OUTLETS     = 0xFFE21D;
constexpr uint32_t IR_CODE_LEARN             = 0xFF906F;
constexpr uint32_t IR_CODE_REPEAT            = 0xFFFFFFFF;

// ==========================================================================
//  IR beacon result — what happened when we received a beacon
// ==========================================================================
struct IRBeaconEvent {
    bool received = false;      // Did we get a valid beacon?
    uint16_t command = 0;       // IR_CMD_REQUEST_CREATE or IR_CMD_REQUEST_MOVE
    NodeID senderNodeId = 0;    // Lower 16 bits of sender's NodeID
    bool isRemoteControl = false; // Was it from an IR remote instead?
    uint32_t remoteCode = 0;    // Raw code if from remote
};

// ==========================================================================
//  IRProximity — IR beacon sender/receiver for connection discovery
// ==========================================================================
class IRProximity {
public:
    IRProximity() = default;

    // -----------------------------------------------------------------
    //  Initialization
    // -----------------------------------------------------------------
    void init(int8_t sendPin, int8_t recvPin, bool enabled = true) {
        sendPin_ = sendPin;
        recvPin_ = recvPin;
        enabled_ = enabled && (sendPin >= 0 || recvPin >= 0);

        if (!enabled_) {
            log_i("IR proximity disabled (no pins assigned)");
            return;
        }

        // Initialize IR receiver (uses RMT on ESP32-S3)
        if (recvPin_ >= 0) {
            IrReceiver.begin(recvPin_, false); // false = no LED feedback
            log_i("IR receiver on GPIO %d", recvPin_);
        }

        // Initialize IR sender (uses RMT on ESP32-S3)
        if (sendPin_ >= 0) {
            IrSender.begin(sendPin_, false, 0); // pin, enable LED feedback, LED pin
            log_i("IR sender on GPIO %d (38kHz NEC)", sendPin_);
        }

        lastBeaconTime_ = millis();
        randomJitter_ = 0;
    }

    void enable()  { enabled_ = true; }
    void disable() { enabled_ = false; }
    bool isEnabled() const { return enabled_; }

    // -----------------------------------------------------------------
    //  Send beacon — call from the main update loop
    //
    //  V2 behavior: send periodically with random jitter to avoid
    //  collision. The command depends on outlet capacity:
    //    - Not full: REQUEST_CREATE (new connection)
    //    - Full:     REQUEST_MOVE   (displace LOOSE link)
    // -----------------------------------------------------------------
    void updateBeacon(NodeID myNodeId, bool outletsAtCapacity) {
        if (!enabled_ || sendPin_ < 0) return;

        uint32_t now = millis();
        if ((now - lastBeaconTime_) >= (IR_BEACON_PERIOD_MS + randomJitter_)) {
            // Choose command based on outlet capacity (V2 behavior)
            uint16_t command = outletsAtCapacity
                ? IR_CMD_REQUEST_MOVE
                : IR_CMD_REQUEST_CREATE;

            // Encode: [ command (16 bits) ][ nodeId lower 16 bits ]
            uint16_t nodeId16 = (uint16_t)(myNodeId & 0xFFFF);
            sendBeacon(command, nodeId16);

            // V2-faithful random jitter for next beacon
            randomJitter_ = (int32_t)(random(IR_JITTER_RANGE_LOW, IR_JITTER_RANGE_HIGH))
                            * IR_RANDOM_JITTER_MS;
            lastBeaconTime_ = now;
        }
    }

    // -----------------------------------------------------------------
    //  Check for received beacon — call from the main update loop
    //
    //  Returns an IRBeaconEvent describing what was received.
    //  Handles both Gluon beacons (command + nodeId) and standard
    //  IR remote codes (for configuration, V2-faithful).
    // -----------------------------------------------------------------
    IRBeaconEvent checkReceive() {
        IRBeaconEvent event;
        if (!enabled_ || recvPin_ < 0) return event;

        if (!IrReceiver.decode()) return event;

        // We have a decoded IR frame
        uint32_t rawCode = IrReceiver.decodedIRData.decodedRawData;

        // Resume receiver immediately (V2: re-enable reception ASAP)
        IrReceiver.resume();

        // Ignore repeat codes (NEC remote holding a button)
        if (rawCode == 0 || rawCode == IR_CODE_REPEAT) return event;

        // Check if it's a Gluon beacon (command in upper 16 bits)
        uint16_t command = (uint16_t)(rawCode >> 16);
        uint16_t value = (uint16_t)(rawCode & 0xFFFF);

        if (command == IR_CMD_REQUEST_CREATE || command == IR_CMD_REQUEST_MOVE) {
            // Gluon beacon: proximity connection request
            event.received = true;
            event.command = command;
            event.senderNodeId = (NodeID)value;
            log_d("IR beacon from node 0x%04X (cmd=0x%04X)", value, command);
        } else {
            // Standard IR remote code (for configuration)
            event.received = true;
            event.isRemoteControl = true;
            event.remoteCode = rawCode;
            log_d("IR remote code: 0x%08X", rawCode);
        }

        return event;
    }

private:
    // -----------------------------------------------------------------
    //  Send a NEC IR beacon
    //
    //  Encodes command and nodeId into a 32-bit NEC raw code.
    //  The IRremote library handles 38kHz carrier, timing, etc.
    // -----------------------------------------------------------------
    void sendBeacon(uint16_t command, uint16_t nodeId16) {
        // Temporarily pause receiver to avoid self-reception (V2 behavior)
        if (recvPin_ >= 0) IrReceiver.stop();

        // NEC raw code: upper 16 = command, lower 16 = nodeId
        uint32_t rawCode = ((uint32_t)command << 16) | nodeId16;

        // Send as NEC using the raw 32-bit value
        // IRremote's sendNECRaw sends the raw 32-bit MSB-first
        IrSender.sendNECRaw(rawCode, 0); // 0 repeats

        // Re-enable receiver after sending (V2: enableReception)
        if (recvPin_ >= 0) {
            IrReceiver.start();
        }

        log_v("IR beacon sent: cmd=0x%04X nodeId=0x%04X", command, nodeId16);
    }

    // Configuration
    int8_t sendPin_ = -1;
    int8_t recvPin_ = -1;
    bool enabled_ = false;

    // Beacon timing (V2-faithful)
    uint32_t lastBeaconTime_ = 0;
    int32_t randomJitter_ = 0;
};

} // namespace gluon
