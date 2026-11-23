// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Kerem Kilic (Ellic Studio)
#pragma once
#include <atomic>
#include <cstdint>
#include <string>
#include <thread>

// The card's folders read ahead of the kernel. The MPC's exFAT driver reads a
// folder a 512-byte sector a request, and the first time it touches one it
// asks for the whole cluster that way: 256 requests, about 150 ms a folder on
// the MPC's card, however few files it holds. A plug-in that walks thousands
// of files (Surge XT's data) queues some hundred thousand of them, and every
// other read of the card (a sample, an instrument's frames) waits behind them:
// a project that loads in seconds takes most of a minute after a start.
//
// The driver looks for a sector in the block device's page cache before it
// asks the card. So this reads the same sectors first, through the partition's
// device node, in a few large requests: the FAT, then each folder's clusters
// up to its end mark (128 KB a folder on the MPC's card), the folders Melodee
// uses first. Read only; nothing of the card is written or held once it is done.
namespace melodee {

struct CardWarmth {
    bool exfat = false;       // the card is exFAT and its device could be read
    unsigned folders = 0;     // folders read
    unsigned requests = 0;    // reads asked of the device
    uint64_t bytes = 0;       // read in all
    unsigned ms = 0;
    bool whole = false;       // every folder was reached (not stopped, nor past the most allowed)
};

// The exFAT volume on `device` (a partition's node): its FAT and its folders' entries read into the page cache.
// `most_folders`: where to stop on a card of very many; `stop` set from another thread ends it at the next folder.
CardWarmth warm_exfat(const std::string& device, const std::atomic<bool>& stop, unsigned most_folders = 6000);
// The device node mounted at `mount` when it is exFAT ("" otherwise), from /proc/mounts.
std::string exfat_device(const std::string& mount);

// The same on a thread of its own: begun when the card is found (and again when it comes back from being lent),
// stopped and joined before the card is let go.
class CardWarm {
public:
    ~CardWarm() { stop(); }
    void start(const std::string& mount); // nothing when one is running, or the card is not exFAT
    void stop();
    bool running() const { return busy.load(std::memory_order_acquire); }
    CardWarmth last() const; // of the run that ended last
private:
    std::thread thread;
    std::atomic<bool> busy {false}, stopping {false};
    std::atomic<uint64_t> packed {0}; // folders << 32 | ms, whole in bit 63
};

} // namespace melodee
