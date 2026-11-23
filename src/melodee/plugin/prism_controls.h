// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <algorithm>
#include <cstdint>
namespace melodee::prism_ui {
constexpr int32_t effect_id=0x54575046,opcode=50,command=0x50524953;
struct Info {uint32_t version=1;float harmonics[128]{},curve[8]{};};
constexpr unsigned rows=64;
constexpr unsigned parameter(unsigned row,unsigned knob){
    if(row<8)return row*3+knob;
    if(row<32){constexpr unsigned bank[4][3]={{0,1,2},{3,4,9},{5,6,7},{8,1000,1001}};unsigned p=bank[(row-8)%4][knob];return p==1000?21:p==1001?22:24+(row-8)/4*10+p;}
    if(row<38)return 84+(row-32)*3+knob;
    if(row<50)return 102+(row-38)*3+knob;
    if(row<62)return 138+(row-50)*3+knob;
    return 174+(std::min(row,63u)-62)*3+knob;
}
inline unsigned jump(unsigned row,int direction){constexpr unsigned starts[]={0,8,12,16,20,24,28,32,34,36,38,50,62,63};if(direction>0){for(unsigned start:starts)if(start>row)return start;return 63;}for(int n=13;n>=0;--n)if(starts[n]<row)return starts[n];return 0;}
inline const char* section(unsigned row){return row<8?"MAIN":row<32?"OPERATOR":row<38?"FILTER":row<50?"FM MATRIX":row<62?"RM MATRIX":row==62?"HARMONICS":"MOTION";}
}
