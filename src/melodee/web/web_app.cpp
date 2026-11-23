// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Kerem Kilic (Ellic Studio)
#include "melodee/web/web_app.h"
#include "melodee/web/card_files.h"
#include "melodee/web/web_bundle.h"
#include "melodee/plugin/plugin_files.h"
#include "melodee/shell/update.h"
#include <fcntl.h>
#include <unistd.h>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <vector>

namespace melodee::web {
namespace fs = std::filesystem;
namespace {
constexpr uint64_t update_limit = uint64_t(1) << 30;       // sd_update::zips_at() looks at nothing bigger
constexpr uint64_t plugin_limit = (uint64_t(4) << 30) - 1; // a FAT32 card's largest file
constexpr uint64_t room_margin = uint64_t(8) << 20;

Reply fail(int status, const std::string& words) { return {status, "application/json; charset=utf-8", "{\"error\":" + json_text(words) + "}"}; }

// The request's body into `path`, flushed to the card. False, with nothing left behind, when it stopped short.
bool receive(const Body& body, uint64_t length, const fs::path& path) {
    int fd = ::open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
    if (fd < 0) return false;
    std::vector<char> buffer(256 * 1024);
    uint64_t written = 0;
    bool ok = true;
    while (ok && written < length) {
        long got = body(buffer.data(), buffer.size());
        if (got <= 0) { ok = false; break; }
        for (long at = 0; at < got;) {
            ssize_t n = ::write(fd, buffer.data() + at, size_t(got - at));
            if (n <= 0) { ok = false; break; }
            at += n;
        }
        written += uint64_t(got);
    }
    ok = ok && ::fsync(fd) == 0;
    ok = ::close(fd) == 0 && ok;
    std::error_code error;
    if (!ok) fs::remove(path, error);
    return ok;
}

// Room on the card for `length` bytes, kept back from the end.
bool room_for(const fs::path& folder, uint64_t length) {
    std::error_code error;
    auto space = fs::space(folder, error);
    return !error && space.available >= length + room_margin;
}

const char* phase_word(uint8_t phase) {
    switch (UpdatePhase(phase)) {
    case UpdatePhase::Saving: return "saving";
    case UpdatePhase::Checking: return "checking";
    case UpdatePhase::Writing: return "writing";
    case UpdatePhase::Starting: return "starting";
    case UpdatePhase::Failed: return "failed";
    default: return "none";
    }
}
} // namespace

bool WebApp::start(std::string& error, std::function<bool(const std::string&)> sample_search,
                   std::function<bool(const std::string&)> install,
                   std::function<bool(const std::string&)> control, std::function<std::string()> control_response) {
    {
        std::lock_guard<std::mutex> held(lock);
        closing = false;
    }
    auto serve = [this, sample_search = std::move(sample_search), install = std::move(install), control = std::move(control), control_response = std::move(control_response)](const Request& request, const Body& body) {
        if (request.path == "/studio" || request.path.rfind("/studio/", 0) == 0 || request.path == "/studio.js" || request.path == "/studio.css" || request.path == "/studio-edit-core.js")
            return fail(404, "Browser DAW is not included in this build");
        if (request.method == "GET" && request.path == "/controls/response")
            return Reply {200, "application/json; charset=utf-8", control_response ? control_response() : "{}"};
        if (request.method == "POST" && request.path == "/controls/command") {
            if (!control || request.length == 0 || request.length > 32768) return fail(400, "Invalid control request size");
            std::string data; data.resize(size_t(request.length)); size_t got = 0;
            while (got < data.size()) { long n = body(data.data()+got, data.size()-got); if (n <= 0) return fail(400, "Incomplete control request"); got += size_t(n); }
            if (!control(data)) return fail(409, "Another control command is pending");
            return Reply {202, "application/json; charset=utf-8", "{\"queued\":true}"};
        }
        if (request.method == "POST" && request.path == "/sample-search") {
            const auto query = request.query.find("q");
            if (query == request.query.end() || query->second.size() > 48
                || query->second.find_first_of("\r\n") != std::string::npos
                || query->second.find('\0') != std::string::npos)
                return fail(400, "Enter a search of up to 48 bytes");
            if (!sample_search || !sample_search(query->second))
                return fail(409, "Open Sample Select on the MPC first");
            return Reply {200, "application/json; charset=utf-8", "{\"ok\":true}"};
        }
        if (request.method == "POST" && request.path == "/update/install") {
            const auto version = request.query.find("version");
            if (version == request.query.end() || version->second.size() > 96 || !install || !install(version->second))
                return fail(409, "No matching verified update is ready on the MPC");
            return Reply {200, "application/json; charset=utf-8", "{\"queued\":true}"};
        }
        return handle(request, body);
    };
    if (!server.start(port, serve, error) && !server.start(spare_port, serve, error)) return false;
    std::string name_error; // without the name the page still opens at the address
    if (!mdns.start(name_error)) std::fprintf(stderr, "melodee: %s not answered: %s\n", mdns_name, name_error.c_str());
    return true;
}

void WebApp::stop() {
    {
        std::lock_guard<std::mutex> held(lock);
        closing = true;
    }
    published.notify_all();
    mdns.stop();
    server.stop();
}

void WebApp::publish(const Facts& now) {
    {
        std::lock_guard<std::mutex> held(lock);
        facts = now;
    }
    published.notify_all();
}

bool WebApp::take_note(std::string& text) {
    std::lock_guard<std::mutex> held(lock);
    if (note.empty()) return false;
    text.swap(note);
    note.clear();
    return true;
}

Facts WebApp::current() {
    std::lock_guard<std::mutex> held(lock);
    return facts;
}

Reply WebApp::handle(const Request& request, const Body& body) {
    const bool get = request.method == "GET" || request.method == "HEAD";
    auto audio_id = [&] {
        const auto found = request.query.find("id");
        if (found == request.query.end() || found->second.empty() || found->second.size() > 10) return uint32_t(0);
        uint64_t value = 0;
        for (char c : found->second) {
            if (c < '0' || c > '9') return uint32_t(0);
            value = value * 10 + unsigned(c - '0');
        }
        return value <= UINT32_MAX ? uint32_t(value) : uint32_t(0);
    };
    if (request.method == "POST" && request.path == "/audio/start")
        return {200, "application/json; charset=utf-8", "{\"id\":" + std::to_string(audio.start()) + "}"};
    if (request.method == "POST" && request.path == "/audio/stop") {
        audio.stop(audio_id());
        return {200, "application/json; charset=utf-8", "{\"ok\":true}"};
    }
    if (get && request.path == "/audio") {
        auto pcm = audio.packet(audio_id(), !request.query.count("live")?0u:request.query.at("live")=="1"?40u:request.query.at("live")=="stable"?100u:0u);
        if (pcm.empty()) return fail(409, "Audio session ended; enable audio again");
        return {200, "application/octet-stream", std::move(pcm)};
    }
    if (get && request.path == "/screen-frame") {
        screen.request();
        auto png = screen.packet();
        if (png.empty()) return fail(503, "Waiting for the MPC screen");
        return {200, "application/octet-stream", std::move(png)};
    }
    if (get && request.path == "/surface") {
        const auto stamp = audio.stamp();
        auto json = controls.json(); json.pop_back();
        json += ",\"bank\":" + std::to_string(current().bank) + ",\"sample\":" + std::to_string(stamp.sample) + ",\"rate\":" + std::to_string(stamp.rate) + "}";
        return {200, "application/json; charset=utf-8", std::move(json)};
    }
    if (get && request.path == "/screen.png") {
        screen.request_png();
        auto png = screen.snapshot();
        if (png.empty()) return fail(503, "Waiting for the MPC screen");
        return {200, "image/png", std::move(png)};
    }
    if (get && request.path == "/status") return status();
    if (get && request.path == "/performance") return performance();
    if (get && request.path == "/plugins") return plugins();
    if (request.method == "POST" && request.path == "/update") return upload_update(request, body);
    if (request.method == "POST" && request.path == "/plugin") return upload_plugin(request, body);
    if (request.method == "POST" && request.path == "/plugin/delete") return delete_plugin(request);
    if (get && request.path == "/list") return list_files(request);
    if (get && request.path == "/file") return send_file(request);
    if (request.method == "POST" && request.path == "/upload") return upload_file(request, body);
    if (request.method == "POST" && request.path == "/mkdir") return make_folder(request);
    if (request.method == "POST" && request.path == "/delete") return delete_entry(request);
    if (get)
        if (auto file = bundled(request.path)) return {200, file.type, std::string(file.data, file.size)};
    if (get && request.path == "/favicon.ico") return {204, "image/x-icon", ""}; // the page names its own icon
    return fail(404, "Not found");
}

Reply WebApp::status() {
    Facts f = current();
    std::string out = "{\"version\":" + json_text(f.version)
        + ",\"install_queued\":" + (f.install_queued ? "true" : "false") + ",\"card\":" + (f.card.empty() ? "false" : "true")
        + ",\"offer\":" + (f.offer_version.empty() ? std::string("null") : json_text(f.offer_version))
        + ",\"phase\":" + json_text(phase_word(f.phase))
        + ",\"error\":" + json_text(update_error_text(UpdateError(f.error)))
        + ",\"card_free\":" + std::to_string(f.card_free) + ",\"card_total\":" + std::to_string(f.card_total) + "}";
    return {200, "application/json; charset=utf-8", out};
}

// Read-only service-thread snapshot; no new audio-thread timing or work.
Reply WebApp::performance() {
    const auto f=current();
    std::string out="{\"version\":"+json_text(f.version)+",\"measured_ms\":"+std::to_string(f.measured_ms)
        +",\"playing\":"+(f.playing?"true":"false")
        +",\"render_percent\":"+std::to_string(f.render_load*100.f)
        +",\"plugin_percent\":"+std::to_string(f.plugins_load*100.f)
        +",\"plugin_count\":"+std::to_string(f.plugin_count)
        +",\"xruns\":"+std::to_string(f.xruns)+",\"capture_xruns\":"+std::to_string(f.capture_xruns)
        +",\"guard_blocks\":"+std::to_string(f.guard_blocks)+",\"guard_steals\":"+std::to_string(f.guard_steals)
        +",\"midi_late\":"+std::to_string(f.midi_late)
        +",\"audio_rate\":"+std::to_string(f.audio_rate)+",\"audio_periods\":"+std::to_string(f.audio_periods)
        +",\"core_percent\":[";
    for(unsigned i=0;i<std::min<unsigned>(f.cores,f.core_load.size());++i){if(i)out+=",";out+=std::to_string(f.core_load[i]);}
    out+="],\"track_percent\":[";
    for(unsigned i=0;i<f.track_load.size();++i){if(i)out+=",";out+=std::to_string(f.track_load[i]*100.f);}
    return {200,"application/json; charset=utf-8",out+"]}"};
}

Reply WebApp::plugins() {
    Facts f = current();
    if (f.card.empty()) return {200, "application/json; charset=utf-8", "{\"card\":false,\"plugins\":[]}"};
    std::string out = "{\"card\":true,\"plugins\":[";
    bool first = true;
    for (const auto& p : list_plugins(plugin_folder(f.card))) {
        out += first ? "" : ",";
        first = false;
        out += "{\"name\":" + json_text(p.name) + ",\"file\":" + json_text(fs::path(p.path).filename().string())
            + ",\"format\":" + json_text(p.fixed_rate ? "OPEN MPC" : format_tag(p.format))
            + ",\"folder\":" + (p.folder ? "true" : "false") + ",\"loadable\":" + (p.loadable() ? "true" : "false")
            + ",\"why\":" + json_text(p.why) + "}";
    }
    return {200, "application/json; charset=utf-8", out + "]}"};
}

Reply WebApp::upload_update(const Request& request, const Body& body) {
    Facts f = current();
    if (f.card.empty()) return fail(409, "No SD card in the MPC, or it is mounted on a computer");
    if (update_running(f.phase)) return fail(409, "An update is installing");
    auto name = request.query.count("name") ? safe_name(request.query.at("name"), ".zip") : std::string();
    if (name.empty()) return fail(400, "Choose a signed Tape Workshop release .zip");
    if (request.length == 0) return fail(400, "The file is empty");
    if (request.length > update_limit) return fail(413, "Too big for a Tape Workshop release");
    const fs::path card = f.card;
    if (!room_for(card, request.length)) return fail(507, "The SD card is full");
    const fs::path temp = card / ".melodee-upload.part";
    if (!receive(body, request.length, temp)) return fail(400, "The upload stopped before the end");
    const fs::path zip = card / name; // the same release sent again takes the place of the copy already there
    std::error_code error;
    fs::rename(temp, zip, error);
    if (error) { fs::remove(temp, error); return fail(500, "Could not write to the SD card"); }
    // The service thread checks the card again: wait for that check, at most 30 s (sd-update.sh's check reads every zip).
    std::unique_lock<std::mutex> held(lock);
    const uint64_t before = facts.scans;
    rescan.store(true);
    bool checked = published.wait_for(held, std::chrono::seconds(30), [&] { return facts.scans > before || closing; }) && !closing;
    Facts after = facts;
    held.unlock();
    if (checked && after.offer_zip == zip.string())
        return {200, "application/json; charset=utf-8", "{\"offered\":" + json_text(after.offer_version) + "}"};
    fs::remove(zip, error);
    if (!checked) return fail(500, "The MPC did not check the card");
    return fail(400, "Not a newer signed Tape Workshop release (this MPC runs " + after.version + ")");
}

Reply WebApp::upload_plugin(const Request& request, const Body& body) {
    Facts f = current();
    if (f.card.empty()) return fail(409, "No SD card in the MPC, or it is mounted on a computer");
    auto name = request.query.count("name") ? safe_name(request.query.at("name"), ".zip") : std::string();
    if (name.empty()) return fail(400, "Choose a plug-in .zip");
    if (request.length == 0) return fail(400, "The file is empty");
    if (request.length > plugin_limit) return fail(413, "Too big for the SD card (4 GB at most)");
    const fs::path folder = plugin_folder(f.card);
    std::error_code error;
    fs::create_directories(folder, error);
    if (!room_for(folder, request.length)) return fail(507, "The SD card is full");
    const fs::path temp = folder / ".melodee-upload.part"; // hidden: the picker and unzip_plugins() pass it by
    if (!receive(body, request.length, temp)) return fail(400, "The upload stopped before the end");
    const fs::path zip = free_path(folder, name);
    fs::rename(temp, zip, error);
    if (error) { fs::remove(temp, error); return fail(500, "Could not write to the SD card"); }
    auto placed = unzip_plugins(folder.string());
    if (fs::exists(zip, error)) { // it stayed: say why, as the picker would, and take it away again
        std::string why = "Won't unzip";
        for (const auto& p : list_plugins(folder.string()))
            if (p.path == zip.string() && !p.why.empty()) why = p.why;
        fs::remove(zip, error);
        return fail(why == "Card full" ? 507 : 400, why == "Card full" ? "The SD card is full" : "This zip will not unzip");
    }
    {
        std::lock_guard<std::mutex> held(lock);
        note = "Plug-in installed";
    }
    std::string out = "{\"installed\":[";
    for (size_t n = 0; n < placed.size(); ++n) out += (n ? "," : "") + json_text(placed[n]);
    return {200, "application/json; charset=utf-8", out + "]}"};
}

Reply WebApp::delete_plugin(const Request& request) {
    Facts f = current();
    if (f.card.empty()) return fail(409, "No SD card in the MPC, or it is mounted on a computer");
    auto file = request.query.count("file") ? safe_name(request.query.at("file"), "") : std::string();
    if (file.empty()) return fail(400, "No such plug-in");
    const fs::path place = fs::path(plugin_folder(f.card)) / file; // one entry straight in Melodee/Plugins, no further
    std::error_code error;
    if (!fs::exists(fs::symlink_status(place, error))) return fail(404, "No such plug-in");
    fs::remove_all(place, error);
    if (error) return fail(500, "Could not delete it");
    ::sync();
    files_changed.store(true);
    {
        std::lock_guard<std::mutex> held(lock);
        note = "Plug-in removed";
    }
    return {200, "application/json; charset=utf-8", "{}"};
}

// ---- Files: the card's folders, read and changed from the page's Files tab ----

namespace {
std::string query_of(const Request& request, const char* key) {
    auto found = request.query.find(key);
    return found == request.query.end() ? std::string() : found->second;
}
} // namespace

Reply WebApp::list_files(const Request& request) {
    Facts f = current();
    const std::string rel = query_of(request, "path");
    fs::path folder;
    std::string why;
    if (!card_place(f.card, rel, folder, why)) return fail(f.card.empty() ? 409 : 404, why);
    std::error_code error;
    if (!fs::is_directory(folder, error)) return fail(404, "Not a folder");
    return {200, "application/json; charset=utf-8", "{\"path\":" + json_text(rel) + ",\"writable\":"
        + (card_writable(rel) ? "true" : "false") + ",\"entries\":" + folder_json(folder, rel.empty()) + "}"};
}

Reply WebApp::send_file(const Request& request) {
    Facts f = current();
    fs::path file;
    std::string why;
    if (!card_place(f.card, query_of(request, "path"), file, why)) return fail(f.card.empty() ? 409 : 404, why);
    Reply reply {200, file_type(file.filename().string()), {}, file.string(), {}};
    if (query_of(request, "save") == "1") reply.download = file.filename().string(); // else shown or played in place
    return reply;
}

Reply WebApp::upload_file(const Request& request, const Body& body) {
    Facts f = current();
    const std::string rel = query_of(request, "path");
    fs::path folder;
    std::string why;
    if (!card_place(f.card, rel, folder, why)) return fail(f.card.empty() ? 409 : 404, why);
    if (!card_writable(rel)) return fail(403, "This folder is read-only");
    auto name = safe_name(query_of(request, "name"), "");
    if (name.empty()) return fail(400, "Not a name the card takes");
    if (request.length > plugin_limit) return fail(413, "Too big for the SD card (4 GB at most)");
    std::error_code error;
    if (!fs::is_directory(folder, error)) return fail(404, "Not a folder");
    if (!room_for(folder, request.length)) return fail(507, "The SD card is full");
    const fs::path temp = folder / ".melodee-upload.part";
    if (!receive(body, request.length, temp)) return fail(400, "The upload stopped before the end");
    const fs::path to = free_path(folder, name);
    fs::rename(temp, to, error);
    if (error) { fs::remove(temp, error); return fail(500, "Could not write to the SD card"); }
    files_changed.store(true);
    return {200, "application/json; charset=utf-8", "{\"name\":" + json_text(to.filename().string()) + "}"};
}

Reply WebApp::make_folder(const Request& request) {
    Facts f = current();
    const std::string rel = query_of(request, "path");
    fs::path folder;
    std::string why;
    if (!card_place(f.card, rel, folder, why)) return fail(f.card.empty() ? 409 : 404, why);
    if (!card_writable(rel)) return fail(403, "This folder is read-only");
    auto name = safe_name(query_of(request, "name"), "");
    if (name.empty()) return fail(400, "Not a name the card takes");
    std::error_code error;
    if (fs::exists(folder / name, error)) return fail(409, "Already there");
    if (!fs::create_directory(folder / name, error) || error) return fail(500, "Could not write to the SD card");
    files_changed.store(true);
    return {200, "application/json; charset=utf-8", "{\"name\":" + json_text(name) + "}"};
}

Reply WebApp::delete_entry(const Request& request) {
    Facts f = current();
    const std::string rel = query_of(request, "path");
    fs::path place;
    std::string why;
    if (!card_place(f.card, rel, place, why)) return fail(f.card.empty() ? 409 : 404, why);
    if (rel.empty() || !card_writable(rel) || rel == "Melodee") return fail(403, "This one stays"); // Melodee's own folder too
    std::error_code error;
    if (!fs::exists(place, error)) return fail(404, "Not found");
    fs::remove_all(place, error);
    if (error) return fail(500, "Could not delete it");
    ::sync();
    files_changed.store(true);
    return {200, "application/json; charset=utf-8", "{}"};
}

} // namespace melodee::web
