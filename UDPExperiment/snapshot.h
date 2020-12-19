#pragma once

#include <cstdint>
#include <set>
#include <map>
#include <list>
#include <memory>
#include <vector>
#include <deque>

#include "object.h"
#include "packet.h"

constexpr uint32_t g_ServerMaxLen = 64;
constexpr uint32_t g_ClientMaxLen = 3;  // may increase if too tight

struct Snapshot {
    Snapshot(uint32_t seq, uint32_t tick) : id_table{}, seq{ seq }, tick{ tick } {};

    void reset(uint32_t newSeq, uint32_t newTick);

    Snapshot& operator=(const Snapshot&) = delete;
    Snapshot& operator=(Snapshot&&) = delete;


    std::map<uint32_t, std::unique_ptr<BaseStatus>> id_table;
    uint32_t seq;
    uint32_t tick;
};

/* 
* memory-efficient (cough cough) snapshot system
* sort of like a skip list */
class History {
public:
    History(uint32_t maxLen) : _snapshots{}, _top{}, _seq{ 0 }, _maxLen(maxLen), _activeObjects{} {
        _snapshots.reserve(_maxLen);
        _snapshots.push_back(Snapshot(_seq, 0)); // TODO: dummy tick
    }

    void addSnapshot(uint32_t newTick);

    // Server functions

    /* read status by type + insert to current snapshot */
    void insert(uint32_t id);

    /* serialize */
    PacketWriter serialize(uint32_t lastAckSeq);

    // END Server functions

    // Client functions

    /* multiple PacketReader may be requried 
    * If newer seq, add new snapshot 
    * If older than _seq, discard */
    void deserialize(PacketReader &reader, uint32_t lastAckSeq);

    // END client functions

private:
    std::vector<Snapshot> _snapshots;
    uint32_t _top; // rotated by _maxLen
    uint32_t _seq;
    uint32_t _maxLen;

    Snapshot& getMaster();
    Snapshot& getSnapshot(uint32_t seq);

    BaseStatus* getStatus(uint32_t id); //only latest
    BaseStatus* getStatus(uint32_t id, uint32_t seq);

    /* { id : [ lastSeq1, lastSeq2, ... (from new to old) ] }*/
    std::map<uint32_t, std::list<uint32_t>> _activeObjects;
    std::map<uint32_t, std::unique_ptr<BaseStatus>> _staleObjects;
};
