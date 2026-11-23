// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
namespace melodee {
class Context;struct Snapshot;namespace ui {class Paint;}
void bar_open(Context& c,bool octave);
void bar_wheel(Context& c,int delta);
void bar_push(Context& c);
void bar_soft(Context& c,int key);
void bar_pot(Context& c,unsigned knob,int value);
void bar_tick(Context& c);
void bar_draw(ui::Paint& p,const Snapshot& s);
}
