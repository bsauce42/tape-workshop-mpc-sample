// SPDX-License-Identifier: AGPL-3.0-only
#include "melodee/shell/update.h"
#include <cassert>
int main() {
    melodee::UpdateLink link;
    const std::string v="v0.3.0-tapeworkshop-community.80";
    assert(!link.request_remote_install(v));
    link.offer("/signed.zip",v);
    assert(!link.request_remote_install("wrong version"));
    assert(link.request_remote_install(v) && link.remote_install_pending());
    assert(!link.take_remote_install("wrong version"));
    assert(link.take_remote_install(v) && !link.remote_install_pending());
    assert(!link.take_remote_install(v));
    assert(link.request_remote_install(v)); link.cancel_remote_install();
    assert(!link.take_remote_install(v));
    assert(link.request_remote_install(v)); link.decline();
    assert(!link.remote_install_pending());
    assert(link.request_remote_install(v)); link.offer("/different.zip",v);
    assert(!link.remote_install_pending());
    link.progress(melodee::UpdatePhase::Checking,melodee::UpdateError::None);
    assert(!link.request_remote_install(v));
    link.progress(melodee::UpdatePhase::None,melodee::UpdateError::None);
    assert(link.request_remote_install(v)); link.install();
    assert(!link.remote_install_pending());
}
