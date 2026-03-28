#pragma once
// ==========================================================================
//  Outlet.h — Data output port for a Gluon node
//
//  Each outlet holds links to remote inlets and queues outgoing data.
//  The transport layer reads from outlets and sends over the mesh.
// ==========================================================================

#include "GluonTypes.h"
#include <vector>

namespace gluon {

class Outlet {
public:
    explicit Outlet(uint8_t index, uint8_t maxFanOut = 8)
        : index_(index), maxFanOut_(maxFanOut) {
        links_.reserve(maxFanOut);
    }

    uint8_t index() const { return index_; }
    uint8_t numLinks() const { return links_.size(); }
    uint8_t maxLinks() const { return maxFanOut_; }
    bool hasSpace() const { return links_.size() < maxFanOut_; }
    bool isFullCapacity() const { return !hasSpace(); }

    // --- Data ---
    bool hasNewData() const { return newDataFlag_; }
    const Data& data() const { return data_; }
    Data consumeData() { newDataFlag_ = false; return data_; }
    void setData(const Data& d) { data_ = d; newDataFlag_ = true; }

    // --- Link management ---
    bool isConnectedTo(NodeID nodeId) const {
        for (const auto& link : links_)
            if (link.nodeId == nodeId) return true;
        return false;
    }

    bool addLink(const Link& link) {
        if (isFullCapacity()) return false;
        if (isConnectedTo(link.nodeId)) return false;
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

    void clearAllLinks() { links_.clear(); }

    const std::vector<Link>& links() const { return links_; }

private:
    uint8_t index_;
    uint8_t maxFanOut_;
    std::vector<Link> links_;
    Data data_;
    bool newDataFlag_ = false;
};

// =========================================================================
//  OutletArray — Collection of outlets for a node
// =========================================================================
class OutletArray {
public:
    void addOutlet(uint8_t maxFanOut = 8) {
        outlets_.emplace_back(outlets_.size(), maxFanOut);
    }

    void createOutlets(uint8_t count, uint8_t maxFanOut = 8) {
        outlets_.clear();
        outlets_.reserve(count);
        for (uint8_t i = 0; i < count; i++)
            outlets_.emplace_back(i, maxFanOut);
    }

    uint8_t size() const { return outlets_.size(); }

    Outlet& operator[](uint8_t idx) { return outlets_[idx]; }
    const Outlet& operator[](uint8_t idx) const { return outlets_[idx]; }

    // Set data on all outlets (broadcast from logic)
    void setData(const Data& d) {
        for (auto& out : outlets_) out.setData(d);
    }

    bool hasAnyNewData() const {
        for (const auto& out : outlets_)
            if (out.hasNewData()) return true;
        return false;
    }

    void disconnectAll() {
        for (auto& out : outlets_) out.clearAllLinks();
    }

    uint8_t totalActiveLinks() const {
        uint8_t total = 0;
        for (const auto& out : outlets_) total += out.numLinks();
        return total;
    }

    const std::vector<Outlet>& all() const { return outlets_; }

private:
    std::vector<Outlet> outlets_;
};

} // namespace gluon
