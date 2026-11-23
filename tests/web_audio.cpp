// SPDX-License-Identifier: AGPL-3.0-only
#include "melodee/web/audio_feed.h"
#include "melodee/web/screen_feed.h"
#include <cassert>
#include <cstdlib>
#include <memory>
#include <new>
#include <thread>
#include <vector>
static thread_local bool in_audio = false;
void* operator new(std::size_t n) { assert(!in_audio); if (auto p=std::malloc(n)) return p; throw std::bad_alloc(); }
void operator delete(void* p) noexcept { std::free(p); }
static uint64_t read_le(const std::string& p, unsigned at, unsigned n) {
    uint64_t value=0; for (unsigned i=0;i<n;++i) value |= uint64_t(uint8_t(p[at+i])) << (i*8); return value;
}
int main() {
    using melodee::web::AudioFeed;
    auto feed=std::make_unique<AudioFeed>();
    std::array<int32_t,256> signal {};
    for (unsigned n=0;n<128;++n) { signal[n*2]=INT32_MAX; signal[n*2+1]=INT32_MIN; }
    auto push=[&] { in_audio=true; feed->push(signal.data(),128); in_audio=false; };
    push(); assert(feed->stamp().sample==128);
    auto id=feed->start(); assert(id && feed->packet(id+1).empty());
    push(); auto packet=feed->packet(id);
    assert(packet.substr(0,4)=="TWAC" && read_le(packet,4,4)==44100);
    assert(read_le(packet,8,8)==128 && read_le(packet,16,4)==128);
    assert(packet.size()==32+128*4 && read_le(packet,32,2)==32767 && read_le(packet,34,2)==32768);
    assert(feed->stop(id)); push(); assert(feed->packet(id).empty());
    feed->format(96000); id=feed->start();
    for (unsigned n=0;n<600;++n) push();
    packet=feed->packet(id); assert(read_le(packet,4,4)==96000 && read_le(packet,20,4)==88);
    assert(read_le(packet,16,4)==192*128);
    unsigned total=192;
    while (true) { packet=feed->packet(id); unsigned count=unsigned(read_le(packet,16,4))/128; total+=count; if (!count) break; }
    assert(total==512); // bounded queue; never overwrites a reader
    id=feed->start(); for (unsigned n=0;n<3000;++n) push();
    assert(read_le(feed->packet(id),20,4)==88+1536); // expires after 2048 callbacks, not 3000
    while (read_le(feed->packet(id),16,4)) {} // consume completed data
    id=feed->start();
    std::thread producer([&] { for (unsigned n=0;n<10000;++n) push(); });
    uint64_t last=0;
    for (unsigned n=0;n<1000;++n) {
        packet=feed->packet(id);
        if (read_le(packet,16,4)) { auto at=read_le(packet,8,8); assert(at>=last); last=at; }
    }
    producer.join();
    auto monitor=std::make_unique<AudioFeed>();auto monitor_id=monitor->start();for(unsigned n=0;n<100;++n)monitor->push(signal.data(),128);
    auto latest=monitor->packet(monitor_id,40);assert(read_le(latest,16,4)==14*128);assert(read_le(latest,8,8)==86*128);assert(read_le(latest,20,4)==86);assert(read_le(latest,32,2)==32767&&read_le(latest,34,2)==32768);
    for(unsigned n=0;n<100;++n)monitor->push(signal.data(),128);latest=monitor->packet(monitor_id,100);assert(read_le(latest,16,4)==35*128);
    melodee::web::ScreenFeed screen;
    screen.publish("png",1000,123456,48000);
    packet=screen.packet(); assert(packet.substr(0,4)=="TWVF" && read_le(packet,4,4)==48000);
    assert(read_le(packet,8,8)==123456 && packet.substr(16)=="png");
}
