// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <string>

namespace melodee::web {
// One producer (audio), one consumer (HTTP). The producer never allocates,
// locks, waits, reads a clock or writes files. A slow viewer loses blocks,
// never delays the instrument. Only a recent explicit viewer session captures.
class AudioFeed {
public:
    static constexpr unsigned frames = 128, capacity = 512;
    static_assert(std::atomic<uint64_t>::is_always_lock_free, "audio counters must be lock-free");
    struct Stamp { uint64_t sample; unsigned rate; };
    void format(unsigned rate) { sample_rate.store(rate); } // audio thread, before rendering
    Stamp stamp() const { return {sample_clock.load(), sample_rate.load()}; }
    uint32_t start() { // HTTP thread only
        read.store(write.load(std::memory_order_acquire), std::memory_order_release);
        const auto id = ++serial;
        active.store(id, std::memory_order_release);
        lease.store(block_clock.load() + 2048);
        return id;
    }
    bool stop(uint32_t id) {
        if (!id || id != active.load()) return false;
        lease.store(0); active.store(0);
        return true;
    }
    void push(const int32_t* stereo, unsigned count) { // audio thread only
        const auto at = sample_clock.fetch_add(count, std::memory_order_relaxed);
        const auto block = block_clock.fetch_add(1, std::memory_order_relaxed);
        const auto id = active.load(std::memory_order_acquire);
        if (!id || block >= lease.load(std::memory_order_relaxed) || count != frames) return;
        const auto next = write.load(std::memory_order_relaxed);
        if (next - read.load(std::memory_order_acquire) >= capacity) {
            dropped.fetch_add(1, std::memory_order_relaxed); return;
        }
        auto& item = queue[next % capacity];
        item.id = id; item.rate = sample_rate.load(std::memory_order_relaxed); item.sample = at;
        for (unsigned n=0; n<frames*2; ++n) item.pcm[n] = int16_t(stereo[n] / 65536);
        write.store(next + 1, std::memory_order_release);
    }
    // TWAC + rate:u32 + first-sample:u64 + frames:u32 + drops:u32 + session:u32 + reserved:u32;
    // little-endian signed 16-bit stereo PCM follows. At most ~100 KB per request.
    std::string packet(uint32_t id, unsigned recent_ms = 0) {
        if (!id || id != active.load()) return {};
        lease.store(block_clock.load() + 2048);
        std::string pcm;
        pcm.reserve(192 * frames * 4);
        unsigned rate = 0, blocks = 0;
        uint64_t first = 0;
        auto cursor = read.load(std::memory_order_relaxed);
        const auto end = write.load(std::memory_order_acquire);
        // Monitoring follows the newest blocks; recording keeps the original
        // contiguous queue. HTTP only: no additional audio callback work.
        const unsigned keep=std::max(8u,std::min(128u,(sample_rate.load(std::memory_order_relaxed)*std::min(recent_ms,100u)+127999)/128000));
        if(recent_ms && end-cursor>keep){const auto skipped=end-cursor-keep;cursor+=skipped;read.store(cursor,std::memory_order_release);dropped.fetch_add(unsigned(skipped),std::memory_order_relaxed);}
        while (cursor < end && blocks < 192) {
            const auto& item = queue[cursor % capacity];
            if (item.id != id) { read.store(++cursor, std::memory_order_release); continue; }
            if (blocks && (item.rate != rate || item.sample != first + uint64_t(blocks)*frames)) break;
            if (!blocks) { rate = item.rate; first = item.sample; }
            for (auto sample : item.pcm) append(pcm, uint16_t(sample), 2);
            ++blocks;
            read.store(++cursor, std::memory_order_release);
        }
        std::string out = "TWAC";
        append(out, rate, 4); append(out, first, 8); append(out, blocks * frames, 4);
        append(out, dropped.load(), 4); append(out, id, 4); append(out, 0, 4);
        return out + pcm;
    }
private:
    static void append(std::string& out, uint64_t value, unsigned bytes) {
        for (unsigned n=0; n<bytes; ++n) out += char((value >> (n*8)) & 255);
    }
    struct Block { std::array<int16_t, frames*2> pcm; uint64_t sample; unsigned rate; uint32_t id; };
    std::array<Block, capacity> queue;
    alignas(64) std::atomic<uint64_t> write {0};
    alignas(64) std::atomic<uint64_t> read {0};
    std::atomic<uint64_t> sample_clock {0}, block_clock {0}, lease {0};
    std::atomic<unsigned> sample_rate {44100}, dropped {0};
    std::atomic<uint32_t> active {0};
    uint32_t serial = 0; // HTTP thread only
};
}
