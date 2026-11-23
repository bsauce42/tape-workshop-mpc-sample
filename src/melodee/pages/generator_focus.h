// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include "melodee/ui/paint.h"
#include <cmath>
#include <algorithm>
namespace melodee {
// Shared generator colours follow Settings > Screen > Theme, including LCD ink.
inline juce::Colour generator_accent() {
    return ui::themed() ? ui::theme_accent() : ui::hex(0xc18cff);
}
inline juce::Colour generator_activity() {
    return ui::lcd_ink_theme() ? ui::colour::text : ui::hex(0x62e0d5);
}
// Large, single-setting card. Long values scroll at their full type size,
// rather than making file/track names too small to read.
inline void generator_focus(ui::Paint& p,const juce::String& name,const juce::String& value,
                            unsigned index,unsigned count,uint32_t now,uint32_t changed){
 using namespace ui;const auto purple=generator_accent(),mint=generator_activity();
 p.fill(7,94,306,100,colour::bg,5);
 p.ring(7,94,306,100,purple,5);
 p.text(name,14,99,288,27,p.fit(name,plain(purple,20,Weight::Bold),288,18));
 auto style=plain(colour::text,30,Weight::Bold);float width=p.width(value,style);float offset=0;
 if(width>290){float travel=(width-290)/.035f;float elapsed=float(now-changed);float phase=std::fmod(elapsed,2400+2*travel);offset=std::min(std::clamp((phase-1200)*.035f,0.f,width-290),std::clamp((2400+2*travel-phase-1200)*.035f,0.f,width-290));}
 p.g.saveState();p.g.reduceClipRegion(juce::Rectangle<int>(14,128,290,44));p.text(value,14-offset,128,std::max(290.f,width+2),44,style);p.g.restoreState();
 p.text(juce::String(index+1)+" / "+juce::String(count) ,14,175,288,16,plain(mint,12,Weight::Bold));
}
}
