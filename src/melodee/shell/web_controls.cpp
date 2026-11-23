// SPDX-License-Identifier: AGPL-3.0-only
// Bounded tutorial-control mailbox. No web project/arrangement editing.
#include "melodee/shell/shell.h"
#include "melodee/engine/engine_host.h"
#include "melodee/shell/fader.h"
#include <juce_core/juce_core.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace melodee {
namespace {
using juce::var; using juce::DynamicObject;
var object() { return var(new DynamicObject); }
void put(var& v, const char* key, const var& x) { v.getDynamicObject()->setProperty(key, x); }
void put(var& v, const char* key, const char* x) { put(v,key,var(x)); }
int number(const var& v, const char* key, int low, int high) {
    const var x = v.getProperty(key, var());
    if (!(x.isInt() || x.isInt64() || x.isDouble())) throw std::runtime_error(std::string("Missing number: ")+key);
    double n = double(x);
    if (!std::isfinite(n) || std::floor(n) != n || n < low || n > high) throw std::runtime_error(std::string("Out of range: ")+key);
    return int(n);
}

}
bool Shell::remote_controls(const std::string& request) {
    if (request.empty() || request.size()>32768) return false;
    std::lock_guard<std::mutex> guard(controls_lock_);
    if (!controls_pending_.empty()) return false;
    controls_pending_=request; return true;
}
std::string Shell::remote_controls_response() { std::lock_guard<std::mutex> guard(controls_lock_);return controls_response_; }
void Shell::remote_controls_tick(uint32_t now) {
    // Called with Shell::lock held, on the MIDI thread. A lost browser cannot leave a pad or modifier held.
    for(unsigned i=0;i<controls_button_until_.size();++i) if(controls_button_until_[i] && int32_t(now-controls_button_until_[i])>=0){controls_button_until_[i]=0;button(SurfaceButton(i),false);if(controls_feed_){SurfaceInput input;input.kind=SurfaceInput::Kind::Button;input.index=uint8_t(i);controls_feed_->record(input,-1,false,false);}}
    for(unsigned i=0;i<controls_pad_until_.size();++i) if(controls_pad_until_[i] && int32_t(now-controls_pad_until_[i])>=0){controls_pad_until_[i]=0;pad(i,0,false);if(controls_feed_)controls_feed_->record({},int(i),false,false);}
    std::string pending;
    {std::lock_guard<std::mutex> guard(controls_lock_);pending.swap(controls_pending_);}
    if(pending.empty())return;
    context_.now=now; last_input_=now;
    var result=object(),ask=juce::JSON::parse(juce::String(pending));
    put(result,"id",ask.getProperty("id",var()).toString().substring(0,80));
    try {
        if(!ask.isObject())throw std::runtime_error("Invalid tutorial control");
        const std::string action=ask.getProperty("action",var()).toString().toStdString();
        if (ui_.loading || ui_.update.phase || (action!="studio_state" && action!="studio_stop" && (ui_.ask!=UiState::Ask::None || ui_.exit_open || ui_.page==PageId::CardAccess)))
            throw std::runtime_error("Close the MPC dialog or card-sharing screen first");
        if(action.rfind("studio_",0)==0) {
            throw std::runtime_error("Browser DAW is not included in this build");
        }
        if(action=="button") {unsigned index=unsigned(number(ask,"index",0,23));button(SurfaceButton(index),true);controls_button_until_[index]=now+120;if(controls_feed_){SurfaceInput input;input.kind=SurfaceInput::Kind::Button;input.index=uint8_t(index);input.pressed=true;controls_feed_->record(input,-1,false,false);}}
        else if(action=="pad") {unsigned index=unsigned(number(ask,"index",0,15));pad(index,100,true);controls_pad_until_[index]=now+180;if(controls_feed_)controls_feed_->record({},int(index),true,false);}
        else if(action=="control") {unsigned index=unsigned(number(ask,"index",0,5));if(index==5)wheel(number(ask,"delta",-8,8));else { auto value=uint8_t(number(ask,"value",0,127));if(index==0)main_volume.store(main_volume_gain(value),std::memory_order_relaxed);else if(index==4)fader_move(context_,value,false);else pot(index-1,value,false); } if(controls_feed_){SurfaceInput input;input.kind=SurfaceInput::Kind::Control;input.index=uint8_t(index);if(index==5)input.delta=number(ask,"delta",-8,8);else input.value=uint8_t(number(ask,"value",0,127));controls_feed_->record(input,-1,false,false);}}
        else throw std::runtime_error("Unknown tutorial control");
        var state=object();put(state,"playing",host.playing());put(result,"state",state);put(result,"ok",true);
    } catch(const std::exception& error){put(result,"error",error.what());}
    {std::lock_guard<std::mutex> guard(controls_lock_);controls_response_=juce::JSON::toString(result,true).toStdString();}
}
} // namespace melodee
