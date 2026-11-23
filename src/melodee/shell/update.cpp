// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Kerem Kilic (Ellic Studio)
#include "melodee/shell/update.h"

#ifndef MELODEE_VERSION
#define MELODEE_VERSION "dev"
#endif

namespace melodee {

const char* melodee_version() { return MELODEE_VERSION; }

UpdateError update_error(const std::string& word) {
    if (word.empty()) return UpdateError::None;
    if (word == "signature") return UpdateError::Signature;
    if (word == "damaged") return UpdateError::Damaged;
    if (word == "self-test") return UpdateError::SelfTest;
    if (word == "no-card") return UpdateError::NoCard;
    if (word == "other-app") return UpdateError::OtherApp;
    if (word == "card-full") return UpdateError::CardFull;
    if (word == "card-write") return UpdateError::CardWrite;
    if (word == "no-start") return UpdateError::NoStart;
    if (word == "busy") return UpdateError::Busy;
    return UpdateError::Other;
}

const char* update_error_text(UpdateError error) {
    switch (error) {
    case UpdateError::Signature: return "Not a signed Tape Workshop update";
    case UpdateError::Damaged: return "Update file is damaged";
    case UpdateError::SelfTest: return "New version did not start";
    case UpdateError::NoCard: return "No SD card";
    case UpdateError::OtherApp: return "Startup folder is not Tape Workshop's";
    case UpdateError::CardFull: return "SD card is full";
    case UpdateError::CardWrite: return "Could not write to SD card";
    case UpdateError::NoStart: return "New version did not start";
    case UpdateError::Busy: return "Update already running";
    case UpdateError::Other: return "Update failed";
    default: return "";
    }
}

void UpdateLink::offer(const std::string& path, const std::string& v) {
    std::lock_guard<std::mutex> guard(lock);
    remote_install = false;
    zip = path; // every call is a new offer, so a skipped zip is offered again after the next check
    version = v;
    if (!zip.empty()) ++offers;
}

bool UpdateLink::take_offer(std::string& v) {
    std::lock_guard<std::mutex> guard(lock);
    if (offers == offers_seen || zip.empty()) return false;
    offers_seen = offers;
    v = version;
    return true;
}

bool UpdateLink::offering() {
    std::lock_guard<std::mutex> guard(lock);
    return !zip.empty();
}

bool UpdateLink::request_remote_install(const std::string& expected) {
    std::lock_guard<std::mutex> guard(lock);
    if (zip.empty() || expected != version || update_running(phase_now.load())) return false;
    remote_install = true;
    return true;
}
bool UpdateLink::take_remote_install(const std::string& expected) {
    std::lock_guard<std::mutex> guard(lock);
    if (!remote_install || zip.empty() || expected != version) return false;
    remote_install = false;
    return true;
}
bool UpdateLink::remote_install_pending() {
    std::lock_guard<std::mutex> guard(lock);
    return remote_install;
}
void UpdateLink::cancel_remote_install() {
    std::lock_guard<std::mutex> guard(lock);
    remote_install = false;
}

void UpdateLink::decline() {
    std::lock_guard<std::mutex> guard(lock);
    remote_install = false;
    asked = 1;
    asked_zip = zip;
}

void UpdateLink::install() {
    std::lock_guard<std::mutex> guard(lock);
    remote_install = false;
    asked = 2;
    asked_zip = zip;
}

uint8_t UpdateLink::take_request(std::string& path) {
    std::lock_guard<std::mutex> guard(lock);
    uint8_t out = asked;
    asked = 0;
    path = asked_zip;
    return out;
}

void UpdateLink::progress(UpdatePhase phase, UpdateError error) {
    phase_now.store(uint8_t(phase), std::memory_order_relaxed);
    error_now.store(uint8_t(error), std::memory_order_relaxed);
}

void UpdateLink::note(const std::string& text) {
    std::lock_guard<std::mutex> guard(lock);
    notice = text;
    noticed = true;
}

bool UpdateLink::take_note(std::string& text) {
    std::lock_guard<std::mutex> guard(lock);
    if (!noticed) return false;
    noticed = false;
    text = notice;
    return true;
}

} // namespace melodee
