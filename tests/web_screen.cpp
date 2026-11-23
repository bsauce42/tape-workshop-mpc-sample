// SPDX-License-Identifier: AGPL-3.0-only
#include "melodee/web/screen_feed.h"
#include <cassert>
#include <thread>
int main() {
    melodee::web::ScreenFeed feed;
    assert(!feed.wanted(1000));
    feed.request(1000);
    assert(feed.wanted(1000));
    assert(feed.snapshot().empty());
    feed.publish(std::string("PNG\0bytes", 9), 1000);
    assert(feed.snapshot() == std::string("PNG\0bytes", 9));
    assert(!feed.wanted(1032) && feed.wanted(1033));
    assert(!feed.wanted(3500)); // the viewer closed: capture stops
    feed.publish(std::string(320 * 240 * 4 + 1, 'x'), 1200);
    assert(feed.snapshot() == std::string("PNG\0bytes", 9));
    std::thread writer([&] { for (int n=0; n<1000; ++n) feed.publish(std::string(1000, char('a' + n%2)), 2000); });
    for (int n=0; n<1000; ++n) {
        auto frame = feed.snapshot();
        assert(frame == std::string("PNG\0bytes", 9) || frame == std::string(1000, 'a') || frame == std::string(1000, 'b'));
    }
    writer.join();
    feed.request_png(); assert(feed.take_png_request()); assert(!feed.take_png_request());
    feed.publish_rgb565(std::string(320*240*2, '\x55'), 3000, 48000, 48000);
    auto raw=feed.packet();
    assert(raw.size()==32 && raw.substr(0,4)=="TWF2");
    assert(uint8_t(raw[4])==0x80 && uint8_t(raw[5])==0xbb);
    assert(uint8_t(raw[8])==0x80 && uint8_t(raw[9])==0xbb);
    assert(uint8_t(raw[16])==1 && uint8_t(raw[20])==1);
    assert(uint8_t(raw[24])==255 && uint8_t(raw[25])==255 && raw[26]=='\x55');
    assert(uint8_t(raw[28])==1 && uint8_t(raw[29])==44); // remaining 11265 pixels
    feed.publish_rgb565(std::string(320*240*2,'\x66'),3001,48000,48000);
    auto changed=feed.packet(); assert(changed!=raw && uint8_t(changed[20])==2);
    std::string noise(320*240*2,'\0');
    for(size_t n=0;n<noise.size();++n)noise[n]=char(n%251);
    feed.publish_rgb565(noise,3002,48000,48000);raw=feed.packet();
    assert(raw.size()==24+noise.size() && raw[16]==0 && raw.substr(24)==noise);
    feed.publish_rgb565("short",4000,0,0); assert(feed.packet()==raw);
}
