#pragma once
// ==========================================================================
//  Inlet.h — Data input port for a Gluon node
//
//  Evolved from v2: now uses std::vector<Link> (no MAX_FAN_IN limit),
//  and the LOOSE/FIXED link lifecycle is preserved.
// ==========================================================================

#include "GluonTypes.h"
#include <vector>

namespace gluon {

class Inlet {
public:
    explicit Inlet(uint8_t index, uint8_t maxFanIn = 3)
        : index_(index), maxFanIn_(maxFanIn) {
        links_.reserve(maxFanIn);
    }

    uint8_t index() const { return index_; }
    uint8_t numLinks() const { return links_.size(); }
    uint8_t maxLinks() const { return maxFanIn_; }
    bool hasSpace() const { return links_.size() < maxFanIn_; }
    bool isFullCapacity() const { return !hasSpace(); }

    // --- Data access ---
    bool hasNewData() const { return newDataFlag_; }
    const Data& data() const { return data_; }
    Data consumeData() { newDataFlag_ = false; return data_; }
    void setData(const Data& d) { data_ = d; newDataFlag_ = true; timeUpdated_ = millis(); }
    void clearNewFlag() { newDataFlag_ = false; }
    uint32_t age() const { return millis() - timeUpdated_; }

    // --- Link management ---
    bool isConnectedTo(NodeID nodeId) const {
        for (const auto& link : links_)
            if (link.nodeId == nodeId) return true;
        return false;
    }

    const Link* findLink(NodeID nodeId) const {
        for (const auto& link : links_)
            if (link.nodeId == nodeId) return &link;
        return nullptr;
    }

    bool addLink(const Link& link) {
        if (isFullCapacity()) return false;
        if (isConnectedTo(link.nodeId)) return false; // no duplicates
        links_.push_back(link);
        return true;
    }

    bool removeLink(NodeID nodeId) {
        for (auto it = links_.begin(); it != links_.end(); ++it) {
            if (it->nodeId == nodeId) {
                links_.erase(it);
                return true;
            }
        }
        return false;
    }

    // Remove the oldest LOOSE link (for re-patching). Returns the removed nodeId.
    NodeID removeOldestLoose() {
        for (auto it = links_.begin(); it != links_.end(); ++it) {
            if (it->state() == LinkState::LOOSE) {
                NodeID id = it->nodeId;
                links_.erase(it);
                return id;
            }
        }
        return NODE_ID_INVALID;
    }

    void clearAllLinks() { links_.clear(); }

    const std::vector<Link>& links() const { return links_; }

    // Update data if message comes from a connected node
    bool update(const Message& msg) {
        if (isConnectedTo(msg.senderId)) {
            setData(msg.data);
            return true;
        }
        return false;
    }

private:
    uint8_t index_;
    uint8_t maxFanIn_;
    std::vector<Link> links_;
    Data data_;
    bool newDataFlag_ = false;
    uint32_t timeUpdated_ = 0;
};

// =========================================================================
//  InletArray — Collection of inlets for a node
// =========================================================================
class InletArray {
public:
    void addInlet(uint8_t maxFanIn = 3) {
        inlets_.emplace_back(inlets_.size(), maxFanIn);
    }

    void createInlets(uint8_t count, uint8_t maxFanIn = 3) {
        inlets_.clear();
        inlets_.reserve(count);
        for (uint8_t i = 0; i < count; i++)
            inlets_.emplace_back(i, maxFanIn);
    }

    uint8_t size() const { return inlets_.size(); }

    Inlet& operator[](uint8_t idx) { return inlets_[idx]; }
    const Inlet& operator[](uint8_t idx) const { return inlets_[idx]; }

    // Check if any inlet has new data (for ANY_INLET update mode)
    bool hasAnyNewData() const {
        for (const auto& in : inlets_)
            if (in.hasNewData()) return true;
        return false;
    }

    // Check if first inlet has new data (for FIRST_INLET update mode)
    bool hasFirstInletData() const {
        return !inlets_.empty() && inlets_[0].hasNewData();
    }

    void clearAllFlags() {
        for (auto& in : inlets_) in.clearNewFlag();
    }

    void disconnectAll() {
        for (auto& in : inlets_) in.clearAllLinks();
    }

    uint8_t totalActiveLinks() const {
        uint8_t total = 0;
        for (const auto& in : inlets_) total += in.numLinks();
        return total;
    }

    // Route an incoming message to the correct inlet
    bool routeMessage(const Message& msg) {
        if (msg.receiverPort < inlets_.size()) {
            return inlets_[msg.receiverPort].update(msg);
        }
        // Fallback: try all inlets
        for (auto& in : inlets_) {
            if (in.update(msg)) return true;
        }
        return false;
    }

    // Connection request — faithful port of V2's requestForConnexion + requestMoveDeleteInlet.
    // Two-phase approach:
    //   Phase 1: If fromNode already has a link somewhere:
    //            - LOOSE → move round-robin to next inlet with space
    //            - FIXED → delete it
    //   Phase 2: If no existing link, create on first inlet with space.
    struct ConnectionResult {
        enum Action { NONE, ADDED, MOVED, DELETED } action = NONE;
        uint8_t inletIndex = 0;
        uint8_t fromInletIndex = 0; // only for MOVED: original inlet
        NodeID displacedNode = NODE_ID_INVALID;
    };

    ConnectionResult requestConnection(NodeID fromNode) {
        ConnectionResult result;

        // Phase 1: Check if fromNode already has a link (move or delete)
        for (uint8_t i = 0; i < inlets_.size(); i++) {
            const Link* existing = inlets_[i].findLink(fromNode);
            if (existing) {
                if (existing->state() == LinkState::LOOSE) {
                    // Move round-robin to next inlet with space (V2 pattern)
                    for (uint8_t k = 1; k < inlets_.size(); k++) {
                        uint8_t next = (i + k) % inlets_.size();
                        if (inlets_[next].hasSpace()) {
                            inlets_[i].removeLink(fromNode);
                            inlets_[next].addLink(Link(fromNode, 0)); // fresh timestamp
                            result.action = ConnectionResult::MOVED;
                            result.fromInletIndex = i;
                            result.inletIndex = next;
                            return result;
                        }
                    }
                    // No space on any other inlet — do nothing
                    result.action = ConnectionResult::NONE;
                    return result;
                } else {
                    // FIXED link — delete it
                    inlets_[i].removeLink(fromNode);
                    result.action = ConnectionResult::DELETED;
                    result.inletIndex = i;
                    return result;
                }
            }
        }

        // Phase 2: No existing link — create on first inlet with space
        for (uint8_t i = 0; i < inlets_.size(); i++) {
            if (inlets_[i].hasSpace()) {
                inlets_[i].addLink(Link(fromNode, 0));
                result.action = ConnectionResult::ADDED;
                result.inletIndex = i;
                return result;
            }
        }

        return result; // NONE — no space
    }

    const std::vector<Inlet>& all() const { return inlets_; }

private:
    std::vector<Inlet> inlets_;
};

} // namespace gluon
