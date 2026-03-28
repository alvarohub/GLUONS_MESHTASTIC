#pragma once
// ==========================================================================
//  GluonNode.h — The main Gluon node class
//
//  Faithful port of V2's NodeClass. Same composition pattern:
//    Inlets + Outlets + Sensors + Actuators + Logic + Transport
//    + Common interaction hardware (chirp, vibrator, tilt, LEDs, button)
//
//  Update loop faithfully follows V2's 10-step order:
//    0. Learning button + tilt sensor (common control sensors)
//    1. Transport receive → process messages
//    2. Sensor update
//    3. LED indicator update
//    4. Logic computation
//    5. Send outlet data (rate-limited)
//    6. Actuator update
//    7. Heartbeat (new in v3)
// ==========================================================================

#include "core/GluonTypes.h"
#include "core/Inlet.h"
#include "core/Outlet.h"
#include "core/BaseLogic.h"
#include "core/BaseSensor.h"
#include "core/BaseActuator.h"
#include "transport/MeshTransport.h"
#include "config/GluonConfig.h"
#include "interaction/ChirpSpeaker.h"
#include "interaction/Vibrator.h"
#include "interaction/TiltSensor.h"
#include "interaction/LedIndicator.h"
#include "interaction/LearningButton.h"
#include <Preferences.h>

namespace gluon {

class GluonNode {
public:
    GluonNode() = default;

    // =====================================================================
    //  Initialization
    // =====================================================================
    void init(const GluonConfig& config, MeshTransport* transport) {
        config_ = config;
        transport_ = transport;

        // Set identity
        transport_->init(config_.nodeId);
        nodeId_ = transport_->nodeId();

        // Create inlets and outlets from config
        inlets_.createInlets(config_.numInlets, config_.maxFanIn);
        outlets_.createOutlets(config_.numOutlets, config_.maxFanOut);

        // Initialize sensors and actuators (added externally via factories)
        sensors_.init();
        actuators_.init();

        // Attach logic module to I/O
        if (logic_) {
            logic_->attachTo(&inlets_, &outlets_, &sensors_, &actuators_);
            logic_->setUpdateMode(config_.updateMode);
            logic_->setPeriod(config_.periodicMs);
        }

        // Load persisted connections
        loadConnections();

        // Load logic state
        if (logic_) {
            Preferences prefs;
            prefs.begin("gluon", true); // read-only
            logic_->loadState(prefs);
            prefs.end();
        }

        // Initialize interaction hardware (V2: init common hardware in Node::init)
        ledIndicator_.init(ledPin_, &inlets_, &outlets_, ledEnabled_);
        chirpSpeaker_.init(chirpPin_, chirpEnabled_);
        vibrator_.init(vibratorPin_, vibratorLedcChannel_, vibratorEnabled_);
        tiltSensor_.init(tiltPin_);
        if (tiltEnabled_) tiltSensor_.enable();
        learningButton_.init(learningButtonPin_, learningButtonEnabled_);

        // V2 startup animation: chirp + blink each LED
        chirpSpeaker_.chirpUp();
        ledIndicator_.blinkInlet(0);
        ledIndicator_.blinkInlet(1);
        ledIndicator_.blinkInlet(2);
        ledIndicator_.blinkOutlet(0);
        delay(500);
        ledIndicator_.update();

        lastLatchTime_ = millis();
        log_i("GluonNode [%s] ID=0x%08X inlets=%d outlets=%d",
              config_.name.c_str(), nodeId_, inlets_.size(), outlets_.size());
    }

    // =====================================================================
    //  Component registration (called between config and init)
    // =====================================================================
    void setLogic(BaseLogic* logic) { logic_ = logic; }
    void addSensor(BaseSensor* sensor) { sensors_.add(sensor); }
    void addActuator(BaseActuator* actuator) { actuators_.add(actuator); }

    // =====================================================================
    //  Interaction hardware pin configuration (call before init)
    // =====================================================================
    void setChirpPin(int8_t pin, bool enabled = true) {
        chirpPin_ = pin; chirpEnabled_ = enabled;
    }
    void setVibratorPin(int8_t pin, uint8_t ledcCh = 4, bool enabled = true) {
        vibratorPin_ = pin; vibratorLedcChannel_ = ledcCh; vibratorEnabled_ = enabled;
    }
    void setTiltPin(int8_t pin, bool enabled = true) {
        tiltPin_ = pin; tiltEnabled_ = enabled;
    }
    void setLedPin(int8_t pin, bool enabled = true) {
        ledPin_ = pin; ledEnabled_ = enabled;
    }
    void setLearningButtonPin(int8_t pin, bool enabled = true) {
        learningButtonPin_ = pin; learningButtonEnabled_ = enabled;
    }

    // =====================================================================
    //  Main update loop — call this from loop()
    //
    //  Faithful V2 order (see NodeClass.cpp::update()):
    //    0. Learning button + tilt sensor (common control)
    //    1. Transport receive → process messages
    //    2. Sensor update
    //    3. LED indicator update
    //    4. Logic computation
    //    5. Send outlet data (rate-limited)
    //    6. Actuator update
    //    7. Heartbeat
    // =====================================================================
    void update() {
        uint32_t now = millis();

        // 0a. Learning button (V2 step 0)
        learningButton_.update();
        if (learningButton_.checkEvent()) {
            if (learningButton_.level() == 0) {
                // Button pressed → learn sensor conditions (V2 behavior)
                learnAnalogConditions();
            }
        }

        // 0b. Tilt sensor — ISR-based, just check for event (V2 step 0)
        if (tiltSensor_.checkEvent()) {
            chirpSpeaker_.chirpShake();
            // Send PULL_PATCH_CHORD to all outlet children (V2 behavior)
            sendPullPatchChord();
        }

        // 1. Receive and process mesh messages (V2 steps 1-2)
        transport_->update();
        while (transport_->hasMessage()) {
            Message msg = transport_->receive();
            processMessage(msg);
        }

        // 2. Update all sensors (V2 step 3)
        sensors_.update();

        // 3. Update LED indicators (V2 step F)
        ledIndicator_.update();

        // 4. Logic computation (V2 step C)
        if (logic_) {
            logic_->update();
        }

        // 5. Send outlet data — rate-limited (V2 step D1)
        if (outlets_.hasAnyNewData() && (now - lastLatchTime_ >= config_.latchPeriodMs)) {
            sendOutletData();
            lastLatchTime_ = now;
        }

        // 6. Update actuators (V2 step D3)
        actuators_.update();

        // 7. Heartbeat (new in v3 — v2 had no keepalive)
        if (now - lastHeartbeat_ >= HEARTBEAT_INTERVAL_MS) {
            sendHeartbeat();
            lastHeartbeat_ = now;
        }
    }

    // =====================================================================
    //  Accessors
    // =====================================================================
    NodeID nodeId() const { return nodeId_; }
    const String& name() const { return config_.name; }
    const GluonConfig& config() const { return config_; }
    InletArray& inlets() { return inlets_; }
    OutletArray& outlets() { return outlets_; }
    SensorArray& sensors() { return sensors_; }
    ActuatorArray& actuators() { return actuators_; }
    BaseLogic* logic() { return logic_; }

    // Interaction hardware accessors (for external use / serial commands)
    ChirpSpeaker& chirp() { return chirpSpeaker_; }
    Vibrator& vibrator() { return vibrator_; }
    TiltSensor& tilt() { return tiltSensor_; }
    LedIndicator& leds() { return ledIndicator_; }
    LearningButton& button() { return learningButton_; }

    // For display/UI
    uint8_t totalInletLinks() const { return inlets_.totalActiveLinks(); }
    uint8_t totalOutletLinks() const { return outlets_.totalActiveLinks(); }

    // =====================================================================
    //  Connection management
    // =====================================================================
    void disconnectAll() {
        inlets_.disconnectAll();
        outlets_.disconnectAll();
        saveConnections();
        ledIndicator_.update();
    }

private:
    // =====================================================================
    //  V2-faithful sensor learning
    // =====================================================================
    void learnAnalogConditions() {
        log_i("LEARN");
        sensors_.learnConditions();
        sensors_.update();
    }

    // =====================================================================
    //  Send PULL_PATCH_CHORD to all outlet children (V2 behavior)
    //  When shaken, tell connected nodes to vibrate + show which inlets
    // =====================================================================
    void sendPullPatchChord() {
        for (uint8_t i = 0; i < outlets_.size(); i++) {
            for (const auto& link : outlets_[i].links()) {
                Message msg;
                msg.type = MessageType::PULL_PATCH_CHORD;
                msg.senderId = nodeId_;
                msg.receiverId = link.nodeId;
                msg.senderPort = i;
                msg.receiverPort = link.remotePort;
                transport_->send(msg);
            }
        }
    }

    // =====================================================================
    //  V2-faithful showPulledPatchChord — vibrate + show connected inlets
    // =====================================================================
    void showPulledPatchChord(NodeID fromNodeId) {
        ledIndicator_.showInletsMatchID(fromNodeId);
        vibrator_.manyPulses(3, 500, 255);
        ledIndicator_.update(); // restore normal display
    }

    // =====================================================================
    //  Message processing (evolved from v2's processRFMessage)
    // =====================================================================
    void processMessage(const Message& msg) {
        switch (msg.type) {
            case MessageType::DATA_UPDATE:
                inlets_.routeMessage(msg);
                break;

            case MessageType::LINK_REQUEST:
                handleLinkRequest(msg);
                break;

            case MessageType::LINK_ACK:
                handleLinkAck(msg);
                break;

            case MessageType::LINK_DELETE:
                handleLinkDelete(msg);
                break;

            case MessageType::PULL_PATCH_CHORD:
                showPulledPatchChord(msg.senderId);
                break;

            case MessageType::NODE_QUERY:
                sendNodeAnnounce();
                break;

            case MessageType::SYNC_CLOCK:
                // V2: Data::offsetTime = millis();
                break;

            case MessageType::HEARTBEAT:
                // Update last-seen table for neighbor tracking
                break;

            case MessageType::NETWORK_RESET:
                disconnectAll();
                break;

            case MessageType::NETWORK_SCAN:
                sendNodeAnnounce(); // respond with our info
                break;

            default:
                break;
        }
    }

    // =====================================================================
    //  Connection handlers — with V2-faithful chirp/LED feedback
    // =====================================================================
    void handleLinkRequest(const Message& msg) {
        auto result = inlets_.requestConnection(msg.senderId);

        Message reply;
        reply.senderId = nodeId_;
        reply.receiverId = msg.senderId;
        reply.senderPort = result.inletIndex;
        reply.receiverPort = msg.senderPort;

        switch (result.action) {
            case InletArray::ConnectionResult::ADDED:
                reply.type = MessageType::LINK_ACK;
                transport_->send(reply);
                // V2 feedback: chirpUp + blink the inlet
                chirpSpeaker_.chirpUp();
                ledIndicator_.blinkInlet(result.inletIndex);
                saveConnections();
                log_i("Link ADDED: 0x%08X -> inlet[%d]", msg.senderId, result.inletIndex);
                break;

            case InletArray::ConnectionResult::MOVED:
                // V2: no RF reply needed for MOVED (same node, just inlet change)
                // V2 feedback: chirpDownUp + blink destination inlet
                chirpSpeaker_.chirpDownUp();
                ledIndicator_.blinkInlet(result.inletIndex);
                saveConnections();
                log_i("Link MOVED: 0x%08X inlet[%d] -> inlet[%d]",
                       msg.senderId, result.fromInletIndex, result.inletIndex);
                break;

            case InletArray::ConnectionResult::DELETED:
                reply.type = MessageType::LINK_DELETE;
                transport_->send(reply);
                // V2 feedback: chirpDown + blink the inlet
                chirpSpeaker_.chirpDown();
                ledIndicator_.blinkInlet(result.inletIndex);
                saveConnections();
                log_i("Link DELETED: 0x%08X from inlet[%d]", msg.senderId, result.inletIndex);
                break;

            default:
                // No space — no action
                break;
        }
    }

    void handleLinkAck(const Message& msg) {
        // Remote node accepted our connection — create outlet link
        if (msg.receiverPort < outlets_.size()) {
            outlets_[msg.receiverPort].addLink(Link(msg.senderId, msg.senderPort));
            // V2 feedback: chirpUp + blink outlet
            chirpSpeaker_.chirpUp();
            ledIndicator_.blinkOutlet(0);
            saveConnections();
            log_i("Outlet[%d] -> 0x%08X confirmed", msg.receiverPort, msg.senderId);
        }
    }

    void handleLinkDelete(const Message& msg) {
        // Remote node deleted our link — remove from all outlets
        for (uint8_t i = 0; i < outlets_.size(); i++) {
            if (outlets_[i].removeLink(msg.senderId)) {
                // V2 feedback: chirpDown + blink outlet
                chirpSpeaker_.chirpDown();
                ledIndicator_.blinkOutlet(i);
            }
        }
        saveConnections();
    }

    // =====================================================================
    //  Data transmission
    // =====================================================================
    void sendOutletData() {
        for (uint8_t oi = 0; oi < outlets_.size(); oi++) {
            if (!outlets_[oi].hasNewData()) continue;
            Data d = outlets_[oi].consumeData();

            for (const auto& link : outlets_[oi].links()) {
                Message msg;
                msg.type = MessageType::DATA_UPDATE;
                msg.senderId = nodeId_;
                msg.receiverId = link.nodeId;
                msg.senderPort = oi;
                msg.receiverPort = link.remotePort;
                msg.data = d;
                transport_->send(msg);
            }
        }
    }

    void sendHeartbeat() {
        Message msg;
        msg.type = MessageType::HEARTBEAT;
        msg.senderId = nodeId_;
        msg.data.set(config_.name, (float)millis() / 1000.0f, true);
        transport_->broadcast(msg);
    }

    void sendNodeAnnounce() {
        Message msg;
        msg.type = MessageType::NODE_ANNOUNCE;
        msg.senderId = nodeId_;
        msg.data.label = config_.name;
        msg.data.value = inlets_.size() * 100 + outlets_.size(); // Compact encoding
        msg.data.event = true;
        transport_->broadcast(msg);
    }

    // =====================================================================
    //  Persistence (ESP32 NVS replaces EEPROM)
    // =====================================================================
    void saveConnections() {
        Preferences prefs;
        prefs.begin("gluon_links", false);

        // Save inlet links
        for (uint8_t i = 0; i < inlets_.size(); i++) {
            String key = "in" + String(i) + "_n";
            prefs.putUChar(key.c_str(), inlets_[i].numLinks());
            for (uint8_t j = 0; j < inlets_[i].numLinks(); j++) {
                String lkey = "in" + String(i) + "_" + String(j);
                prefs.putUInt(lkey.c_str(), inlets_[i].links()[j].nodeId);
            }
        }

        // Save outlet links
        for (uint8_t i = 0; i < outlets_.size(); i++) {
            String key = "out" + String(i) + "_n";
            prefs.putUChar(key.c_str(), outlets_[i].numLinks());
            for (uint8_t j = 0; j < outlets_[i].numLinks(); j++) {
                String lkey = "out" + String(i) + "_" + String(j);
                prefs.putUInt(lkey.c_str(), outlets_[i].links()[j].nodeId);
            }
        }

        prefs.end();
    }

    void loadConnections() {
        Preferences prefs;
        prefs.begin("gluon_links", true); // read-only

        for (uint8_t i = 0; i < inlets_.size(); i++) {
            String key = "in" + String(i) + "_n";
            uint8_t n = prefs.getUChar(key.c_str(), 0);
            for (uint8_t j = 0; j < n; j++) {
                String lkey = "in" + String(i) + "_" + String(j);
                NodeID nid = prefs.getUInt(lkey.c_str(), NODE_ID_INVALID);
                if (nid != NODE_ID_INVALID) {
                    inlets_[i].addLink(Link(nid, 0));
                }
            }
        }

        for (uint8_t i = 0; i < outlets_.size(); i++) {
            String key = "out" + String(i) + "_n";
            uint8_t n = prefs.getUChar(key.c_str(), 0);
            for (uint8_t j = 0; j < n; j++) {
                String lkey = "out" + String(i) + "_" + String(j);
                NodeID nid = prefs.getUInt(lkey.c_str(), NODE_ID_INVALID);
                if (nid != NODE_ID_INVALID) {
                    outlets_[i].addLink(Link(nid, 0));
                }
            }
        }

        prefs.end();
    }

    // =====================================================================
    //  Members
    // =====================================================================
    GluonConfig config_;
    NodeID nodeId_ = NODE_ID_INVALID;

    // Dataflow components
    InletArray inlets_;
    OutletArray outlets_;
    SensorArray sensors_;
    ActuatorArray actuators_;
    BaseLogic* logic_ = nullptr;
    MeshTransport* transport_ = nullptr;

    // Interaction hardware (V2 common objects)
    ChirpSpeaker chirpSpeaker_;
    Vibrator vibrator_;
    TiltSensor tiltSensor_;
    LedIndicator ledIndicator_;
    LearningButton learningButton_;

    // Interaction hardware pin config (set before init)
    int8_t chirpPin_ = -1;
    bool chirpEnabled_ = false;
    int8_t vibratorPin_ = -1;
    uint8_t vibratorLedcChannel_ = 4;
    bool vibratorEnabled_ = false;
    int8_t tiltPin_ = -1;
    bool tiltEnabled_ = false;
    int8_t ledPin_ = -1;
    bool ledEnabled_ = false;
    int8_t learningButtonPin_ = -1;
    bool learningButtonEnabled_ = false;

    // Timing
    uint32_t lastLatchTime_ = 0;
    uint32_t lastHeartbeat_ = 0;
    static constexpr uint32_t HEARTBEAT_INTERVAL_MS = 30000; // Every 30s
};

} // namespace gluon
