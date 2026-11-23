// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Kerem Kilic (Ellic Studio)
#include "melodee/shell/card_warm.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <deque>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <unordered_set>
#include <vector>

namespace melodee {
namespace {
// The driver's own read-ahead: the first time it reads a folder's cluster it asks for this much of it, a sector a
// request, unless every sector of that stretch is in the cache already. So a folder is read in stretches of this
// size from its cluster's start, each whole, up to the one that holds its end mark.
constexpr size_t stretch_bytes = 128 * 1024;
constexpr uint64_t fat_most_bytes = 32ull << 20;  // a FAT read whole up to this; past it, an entry as it is wanted
constexpr unsigned chain_most = 4096;             // clusters followed of one folder

uint32_t u32(const uint8_t* p) { return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24; }
uint64_t u64(const uint8_t* p) { return uint64_t(u32(p)) | uint64_t(u32(p + 4)) << 32; }

struct Volume {
    int fd = -1;
    uint64_t sector = 0, cluster = 0;   // bytes
    uint64_t fat_at = 0, fat_bytes = 0; // bytes from the volume's start
    uint64_t heap_at = 0;
    uint32_t clusters = 0, root = 0;
    CardWarmth* told = nullptr;
    bool read(void* into, size_t bytes, uint64_t at) const {
        size_t done = 0;
        while (done < bytes) {
            ssize_t got = pread(fd, static_cast<char*>(into) + done, bytes - done, off_t(at + done));
            if (got <= 0) return false;
            done += size_t(got);
        }
        ++told->requests;
        told->bytes += bytes;
        return true;
    }
    bool valid(uint32_t c) const { return c >= 2 && c - 2 < clusters; }
    uint64_t at(uint32_t c) const { return heap_at + uint64_t(c - 2) * cluster; }
    // The cluster after `c` in its chain; 0 at its end.
    uint32_t next(uint32_t c) const {
        uint8_t entry[4];
        if (uint64_t(c) * 4 + 4 > fat_bytes || !read(entry, 4, fat_at + uint64_t(c) * 4)) return 0;
        uint32_t n = u32(entry);
        return valid(n) ? n : 0;
    }
};

bool open_volume(const std::string& device, Volume& v) {
    v.fd = open(device.c_str(), O_RDONLY | O_CLOEXEC); // buffered: what is read stays in the page cache the driver looks in
    if (v.fd < 0) return false;
#if defined(__linux__)
    posix_fadvise(v.fd, 0, 0, POSIX_FADV_RANDOM); // what is asked for and no more
#endif
    uint8_t boot[512];
    if (!v.read(boot, sizeof(boot), 0) || std::memcmp(boot + 3, "EXFAT   ", 8)) return false;
    const unsigned sector_shift = boot[0x6c], cluster_shift = boot[0x6d];
    if (sector_shift < 9 || sector_shift > 12 || cluster_shift > 25 - sector_shift) return false;
    v.sector = uint64_t(1) << sector_shift;
    v.cluster = v.sector << cluster_shift;
    v.fat_at = uint64_t(u32(boot + 0x50)) * v.sector;
    v.fat_bytes = uint64_t(u32(boot + 0x54)) * v.sector;
    v.heap_at = uint64_t(u32(boot + 0x58)) * v.sector;
    v.clusters = u32(boot + 0x5c);
    v.root = u32(boot + 0x60);
    return v.fat_at && v.heap_at > v.fat_at && v.valid(v.root);
}

struct Folder {
    uint32_t first = 0;
    uint64_t bytes = 0;      // 0: not known (the root), its chain is followed to the end
    bool contiguous = false; // no chain in the FAT: its clusters follow one another
    uint8_t rank = 2;        // 0: under Melodee, 1: under Samples, 2: the rest of the card
};

bool named(const std::vector<uint16_t>& name, const char* word) {
    size_t n = std::strlen(word);
    if (name.size() != n) return false;
    for (size_t i = 0; i < n; ++i) {
        uint16_t c = name[i];
        if (c >= 'A' && c <= 'Z') c = uint16_t(c + 32);
        if (c != uint16_t(word[i])) return false;
    }
    return true;
}

// A folder's entries read up to its end mark; the folders in it handed to `found`.
template <class Found> void read_folder(const Volume& v, const Folder& folder, bool top, std::vector<uint8_t>& piece, Found found) {
    // An entry set: a file entry, its stream entry (where it starts, how long it is), then its name.
    bool open = false, is_folder = false, has_stream = false, contiguous = false;
    unsigned left = 0, name_length = 0;
    uint32_t first = 0;
    uint64_t length = 0;
    std::vector<uint16_t> name;
    uint32_t cluster = folder.first;
    uint64_t clusters = folder.bytes ? (folder.bytes + v.cluster - 1) / v.cluster : chain_most;
    for (uint64_t n = 0; n < std::min<uint64_t>(clusters, chain_most) && v.valid(cluster); ++n) {
        for (uint64_t pos = 0; pos < v.cluster; pos += piece.size()) {
            const size_t bytes = size_t(std::min<uint64_t>(piece.size(), v.cluster - pos));
            if (!v.read(piece.data(), bytes, v.at(cluster) + pos)) return;
            for (size_t e = 0; e + 32 <= bytes; e += 32) {
                const uint8_t* entry = piece.data() + e;
                const uint8_t type = entry[0];
                if (!type) return; // the end mark: nothing is in use past it
                if (type == 0x85) { // a file or a folder
                    open = true; has_stream = false;
                    left = entry[1];
                    is_folder = (entry[4] & 0x10) != 0;
                    name.clear();
                    continue;
                }
                if (!open || !left) { open = false; continue; }
                --left;
                if (type == 0xc0) {
                    has_stream = true;
                    contiguous = (entry[1] & 2) != 0;
                    name_length = entry[3];
                    first = u32(entry + 20);
                    length = u64(entry + 24);
                } else if (type == 0xc1 && top) { // names matter at the card's top only
                    for (unsigned c = 0; c < 15 && name.size() < name_length; ++c) name.push_back(uint16_t(entry[2 + c * 2] | entry[3 + c * 2] << 8));
                }
                if (!left) {
                    open = false;
                    if (is_folder && has_stream && v.valid(first)) found(Folder {first, length, contiguous, folder.rank}, name);
                }
            }
        }
        cluster = folder.contiguous ? cluster + 1 : v.next(cluster);
    }
}
} // namespace

CardWarmth warm_exfat(const std::string& device, const std::atomic<bool>& stop, unsigned most_folders) {
    CardWarmth out;
    const auto began = std::chrono::steady_clock::now();
    auto took = [&] { return unsigned(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - began).count()); };
    Volume v;
    v.told = &out;
    if (!open_volume(device, v)) { if (v.fd >= 0) close(v.fd); return out; }
    out.exfat = true;
    std::vector<uint8_t> piece(size_t(1) << 20);
    // The FAT: every chain the driver follows after this is in memory.
    if (v.fat_bytes <= fat_most_bytes)
        for (uint64_t at = 0; at < v.fat_bytes && !stop.load(std::memory_order_relaxed); at += piece.size())
            if (!v.read(piece.data(), size_t(std::min<uint64_t>(piece.size(), v.fat_bytes - at)), v.fat_at + at)) break;
    piece.resize(size_t(std::min<uint64_t>(v.cluster, stretch_bytes)));
    std::array<std::deque<Folder>, 3> waiting; // by rank, each a level at a time
    std::unordered_set<uint32_t> seen;
    waiting[2].push_back(Folder {v.root, 0, false, 2});
    seen.insert(v.root);
    bool top = true;
    out.whole = true;
    for (;;) {
        auto queue = std::find_if(waiting.begin(), waiting.end(), [](const auto& q) { return !q.empty(); });
        if (queue == waiting.end()) break;
        if (stop.load(std::memory_order_relaxed) || out.folders >= most_folders) { out.whole = false; break; }
        Folder folder = queue->front();
        queue->pop_front();
        read_folder(v, folder, top, piece, [&](Folder child, const std::vector<uint16_t>& name) {
            if (!seen.insert(child.first).second) return;
            if (top) child.rank = named(name, "melodee") ? 0 : named(name, "samples") ? 1 : 2;
            waiting[child.rank].push_back(child);
        });
        top = false;
        ++out.folders;
    }
    close(v.fd);
    out.ms = took();
    return out;
}

std::string exfat_device(const std::string& mount) {
    std::ifstream mounts("/proc/mounts");
    std::string line, found;
    while (std::getline(mounts, line)) {
        std::istringstream words(line);
        std::string device, where, kind;
        if (!(words >> device >> where >> kind)) continue;
        if (where == mount && kind == "exfat" && device.rfind("/dev/", 0) == 0) found = device; // the last mounted there
    }
    return found;
}

void CardWarm::start(const std::string& mount) {
    if (busy.load(std::memory_order_acquire)) return;
    const std::string device = exfat_device(mount);
    if (device.empty()) return;
    if (thread.joinable()) thread.join();
    stopping.store(false);
    busy.store(true, std::memory_order_release);
    thread = std::thread([this, device] {
        const CardWarmth w = warm_exfat(device, stopping);
        packed.store((w.whole ? uint64_t(1) << 63 : 0) | uint64_t(w.folders & 0x7fffffff) << 32 | w.ms, std::memory_order_relaxed);
        if (w.exfat) {
            std::printf("card: %u folders read ahead in %u ms (%u requests, %.1f MB)%s\n", w.folders, w.ms, w.requests, double(w.bytes) / (1 << 20),
                        w.whole ? "" : ", not all");
            std::fflush(stdout);
        }
        busy.store(false, std::memory_order_release);
    });
}

void CardWarm::stop() {
    stopping.store(true);
    if (thread.joinable()) thread.join();
}

CardWarmth CardWarm::last() const {
    const uint64_t p = packed.load(std::memory_order_relaxed);
    CardWarmth w;
    w.whole = (p >> 63) != 0;
    w.folders = unsigned((p >> 32) & 0x7fffffff);
    w.ms = unsigned(p & 0xffffffff);
    w.exfat = p != 0;
    return w;
}

} // namespace melodee
