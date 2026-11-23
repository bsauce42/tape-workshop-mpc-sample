// SPDX-License-Identifier: AGPL-3.0-only
// Original bounded partitioned convolution. No vendor DSP or IRs included.
#pragma once
#include <array>
#include <complex>
#include <vector>
#include <fstream>
#include <cstring>
#include <cmath>
#include <algorithm>
namespace bloom {
constexpr unsigned Hop=512,FFTSize=1024,Bins=513,MaxParts=188,MaxIR=Hop*MaxParts;
using Spectrum=std::array<std::complex<float>,Bins>;
struct FFT {
    std::array<unsigned,FFTSize> reverse{};
    std::array<std::complex<float>,FFTSize/2> roots{};
    FFT(){for(unsigned n=0;n<FFTSize;++n){unsigned v=n,r=0;for(unsigned k=0;k<10;++k){r=(r<<1)|(v&1);v>>=1;}reverse[n]=r;}for(unsigned n=0;n<FFTSize/2;++n)roots[n]=std::polar(1.f,-6.28318530718f*n/FFTSize);}
    void run(std::array<std::complex<float>,FFTSize>& x,bool inverse=false) const {
        for(unsigned n=0;n<FFTSize;++n)if(n<reverse[n])std::swap(x[n],x[reverse[n]]);
        for(unsigned len=2;len<=FFTSize;len*=2)for(unsigned start=0;start<FFTSize;start+=len)for(unsigned k=0;k<len/2;++k){auto w=roots[k*FFTSize/len];if(inverse)w=std::conj(w);auto a=x[start+k],b=x[start+k+len/2]*w;x[start+k]=a+b;x[start+k+len/2]=a-b;}
        if(inverse)for(auto& v:x)v*=1.f/FFTSize;
    }
};
struct IRBank {std::array<std::array<Spectrum,MaxParts>,2> spectra{};unsigned parts=0,frames=0;double rate=48000;};
inline unsigned u16(const unsigned char* p){return p[0]|unsigned(p[1])<<8;}
inline uint32_t u32(const unsigned char* p){return uint32_t(p[0])|uint32_t(p[1])<<8|uint32_t(p[2])<<16|uint32_t(p[3])<<24;}
// Worker only: bounded WAV read, mono/stereo PCM16/24/32 or float32.
inline bool loadIR(const std::string& path,double rate,IRBank& bank,const FFT& fft){
    bank.parts=0;std::ifstream in(path,std::ios::binary|std::ios::ate);if(!in)return false;auto bytes=in.tellg();if(bytes<44||bytes>32*1024*1024)return false;
    std::vector<unsigned char> data(static_cast<size_t>(bytes));in.seekg(0);if(!in.read(reinterpret_cast<char*>(data.data()),bytes))return false;
    if(std::memcmp(data.data(),"RIFF",4)||std::memcmp(data.data()+8,"WAVE",4))return false;
    unsigned format=0,channels=0,sr=0,bits=0,align=0;size_t at=12,audio=0,count=0;
    while(at+8<=data.size()){size_t size=u32(data.data()+at+4),start=at+8;if(size>data.size()-start)return false;
        if(!std::memcmp(data.data()+at,"fmt ",4)&&size>=16){format=u16(data.data()+start);channels=u16(data.data()+start+2);sr=u32(data.data()+start+4);align=u16(data.data()+start+12);bits=u16(data.data()+start+14);
            if(format==65534&&size>=40){constexpr unsigned char guidTail[]={0,0,0,0,16,0,128,0,0,170,0,56,155,113};if(std::memcmp(data.data()+start+26,guidTail,sizeof(guidTail)))return false;format=u16(data.data()+start+24);}}

        if(!std::memcmp(data.data()+at,"data",4)){audio=start;count=size;}at=start+size+(size&1);}
    if(!audio||!count||!align||count%align||channels<1||channels>2||sr<8000||sr>192000||!((format==1&&(bits==16||bits==24||bits==32))||(format==3&&bits==32))||align!=channels*(bits/8))return false;
    unsigned frames=unsigned(count/align);if(!frames||double(frames)*rate/sr>MaxIR)return false;unsigned made=std::max(1u,unsigned(std::ceil(double(frames)*rate/sr)));
    std::array<std::vector<float>,2> samples;for(auto& v:samples)v.resize(made);
    auto read=[&](unsigned frame,unsigned channel){const auto* p=data.data()+audio+size_t(std::min(frame,frames-1))*align+(channels==1?0:channel)*(bits/8);float v=0;
        if(format==3)std::memcpy(&v,p,4);else if(bits==16)v=int16_t(u16(p))/32768.f;else if(bits==24){int32_t n=int32_t(uint32_t(p[0])<<8|uint32_t(p[1])<<16|uint32_t(p[2])<<24);v=n/2147483648.f;}else v=int32_t(u32(p))/2147483648.f;return std::isfinite(v)?v:0.f;};
    double energy=0;float peak=0;for(unsigned n=0;n<made;++n){double pos=n*double(sr)/rate;unsigned whole=unsigned(pos);float f=float(pos-whole);for(unsigned ch=0;ch<2;++ch){float v=read(whole,ch)*(1-f)+read(whole+1,ch)*f;samples[ch][n]=v;energy+=double(v)*v;peak=std::max(peak,std::abs(v));}}
    if(energy<1e-15)return false;float gain=std::min(1.f/std::max(.0001f,peak),float(std::sqrt(2./energy)));unsigned parts=(made+Hop-1)/Hop;
    std::array<std::complex<float>,FFTSize> scratch;
    for(unsigned ch=0;ch<2;++ch)for(unsigned part=0;part<parts;++part){scratch.fill({});for(unsigned n=0;n<Hop&&part*Hop+n<made;++n)scratch[n]=samples[ch][part*Hop+n]*gain;fft.run(scratch);std::copy_n(scratch.begin(),Bins,bank.spectra[ch][part].begin());}
    bank.frames=made;bank.rate=rate;bank.parts=parts;return true;
}
struct Convolution {
    std::array<std::array<Spectrum,MaxParts>,2> history{};
    std::array<std::array<float,Hop>,2> input{},queuedInput{},output{},overlap{};
    std::array<std::complex<float>,FFTSize> scratch{};
    std::array<std::array<std::complex<float>,FFTSize>,2> sums{};
    unsigned cursor=0,head=0,pendingHead=0,valid=0;bool queued=false;
    void reset(){cursor=head=pendingHead=valid=0;queued=false;for(auto& v:input)v.fill(0);for(auto& v:output)v.fill(0);for(auto& v:overlap)v.fill(0);}
    std::array<float,2> process(float l,float r,const IRBank& bank,const FFT& fft){
        std::array<float,2> wet={output[0][cursor],output[1][cursor]};input[0][cursor]=l;input[1][cursor]=r;++cursor;
        // Four bounded slices spread the IR multiply-accumulate over the host's
        // typical 128-frame callbacks, rather than a full 2-second IR burst.
        if(queued&&cursor%128==0){unsigned stage=cursor/128-1;
            if(stage==0){pendingHead=head;head=(head+1)%MaxParts;valid=std::min(bank.parts,valid+1);
                for(unsigned ch=0;ch<2;++ch){scratch.fill({});for(unsigned n=0;n<Hop;++n)scratch[n]=queuedInput[ch][n];fft.run(scratch);std::copy_n(scratch.begin(),Bins,history[ch][pendingHead].begin());sums[ch].fill({});}}
            for(unsigned ch=0;ch<2;++ch){auto& sum=sums[ch];
                for(unsigned part=stage*valid/4;part<(stage+1)*valid/4;++part){const auto& h=history[ch][(pendingHead+MaxParts-part)%MaxParts];const auto& ir=bank.spectra[ch][part];for(unsigned bin=0;bin<Bins;++bin)sum[bin]+=h[bin]*ir[bin];}
                if(stage==3){for(unsigned bin=1;bin<Hop;++bin)sum[FFTSize-bin]=std::conj(sum[bin]);fft.run(sum,true);for(unsigned n=0;n<Hop;++n){output[ch][n]=sum[n].real()+overlap[ch][n];overlap[ch][n]=sum[n+Hop].real();}}}
        }
        if(cursor==Hop){cursor=0;queuedInput=input;queued=true;}return wet;
    }
};
}
