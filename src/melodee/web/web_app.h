// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Kerem Kilic (Ellic Studio)
#pragma once
#include "melodee/web/mdns.h"
#include "melodee/web/screen_feed.h"
#include "melodee/web/control_feed.h"
#include "melodee/web/audio_feed.h"
#include "melodee/web/web_server.h"
#include <atomic>
#include <array>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>

// Melodee's web page (docs/MELODEE.md, "Web page"): http://192.168.155.1
// on a computer joined to the MPC by USB-C. Its pages, the quickstart and the
// user guide come from the bundle (web/web_bundle.h). The device page shows
// the version running, the card and the plug-ins, and takes two kinds of zip:
//
// - A signed Tape Workshop release zip. It goes to the card's top folder, as a card
//   update does, and the card is checked again at once: when it is a newer
//   signed release, the MPC asks INSTALL / LATER as for a zip copied there
//   (shell/update.h). Nothing installs without INSTALL pressed on the MPC.
//   One that is not a newer signed release is deleted again.
// - A plug-in zip (a release's -mpc-sample.zip). It goes to Melodee/Plugins
//   and is unzipped there at once (plugin/plugin_files.h, unzip_plugins());
//   the picker lists it the next time it opens. One that will not unzip is
//   deleted again, with the reason. A plug-in listed there can be deleted
//   from the page too.
//
// The page's thread writes the card only while the base has it home; the
// service thread says so in publish().
namespace melodee::web {

// Port 80, which the stock web server gives up while Melodee runs (melodee_app.cpp); 8080 when it is held.
constexpr uint16_t port = 80, spare_port = 8080;

// What the page shows, from the service thread.
struct Facts {
    uint32_t measured_ms=0, xruns=0, capture_xruns=0, guard_blocks=0, guard_steals=0;
    uint32_t midi_late=0, audio_rate=0, audio_periods=0;
    float render_load=0, plugins_load=0;
    uint8_t cores=0, plugin_count=0, bank=0;
    bool playing=false;
    std::array<uint8_t,8> core_load{};
    std::array<float,16> track_load{};
    std::string version;                 // this build's
    std::string card;                    // the card's top folder while it is home; empty: no card or lent
    std::string offer_zip, offer_version; // the update the MPC is asking about, if any
    bool install_queued = false;
    uint8_t phase = 0, error = 0;        // UpdatePhase, UpdateError (shell/update.h)
    uint64_t scans = 0;                  // counts card checks for updates
    uint64_t card_free = 0, card_total = 0; // bytes, as About reads them
};

class WebApp {
public:
    ~WebApp() { stop(); }
    bool start(std::string& error, std::function<bool(const std::string&)> sample_search = {},
               std::function<bool(const std::string&)> install = {},
               std::function<bool(const std::string&)> control = {},
               std::function<std::string()> control_response = {});
    AudioFeed audio; // bounded audio-thread tap, consumed by one browser session
    ControlFeed controls; // physical and remote actions for the tutorial view
    ScreenFeed screen; // screen thread publishes; HTTP reads the completed PNG
    void stop(); // a wait for the service thread's check ends at once
    // Service thread, every pass.
    void publish(const Facts& facts);
    bool take_rescan() { return rescan.exchange(false); } // an update zip came: check the card again now
    bool take_note(std::string& text);                    // "Installed Melodee Poly", for the header
    bool take_files_changed() { return files_changed.exchange(false); } // the Files tab changed the card: look again

private:
    Reply handle(const Request& request, const Body& body);
    Reply status();
    Reply performance();
    Reply plugins();
    Reply upload_update(const Request& request, const Body& body);
    Reply upload_plugin(const Request& request, const Body& body);
    Reply delete_plugin(const Request& request); // ?file=<name in Melodee/Plugins>, bundle or folder and all
    Reply list_files(const Request& request);
    Reply send_file(const Request& request);
    Reply upload_file(const Request& request, const Body& body);
    Reply make_folder(const Request& request);
    Reply delete_entry(const Request& request);
    Facts current();
    Server server;
    Mdns mdns; // melodee.local
    std::mutex lock;
    std::condition_variable published;
    Facts facts;
    std::string note;
    std::atomic<bool> rescan {false}, files_changed {false};
    bool closing = false; // under `lock`
};

} // namespace melodee::web
