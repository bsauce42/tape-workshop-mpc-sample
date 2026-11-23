// SPDX-License-Identifier: AGPL-3.0-only
#include "melodee/shell/expansion_cover.h"
#include <juce_graphics/juce_graphics.h>
#include <filesystem>
#include <fstream>
#include <vector>
namespace melodee {
namespace {
namespace fs = std::filesystem;
bool within(const fs::path& p, const fs::path& root) {
    auto a=p.begin();
    for(auto b=root.begin();b!=root.end();++b,++a) if(a==p.end() || *a!=*b) return false;
    return true;
}
std::vector<unsigned char> bytes(const fs::path& p, size_t limit) {
    std::error_code e; auto n=fs::file_size(p,e);
    if(e || !n || n>limit) return {};
    std::vector<unsigned char> out(size_t(n),0);
    std::ifstream f(p,std::ios::binary);
    if(!f.read(reinterpret_cast<char*>(out.data()),std::streamsize(n))) return {};
    return out;
}
bool dimensions(const std::vector<unsigned char>& b) {
    unsigned w=0,h=0;
    if(b.size()>=24 && b[0]==137 && b[1]=='P' && b[2]=='N' && b[3]=='G') {
        auto u32=[&](size_t n){return unsigned(b[n])<<24|unsigned(b[n+1])<<16|unsigned(b[n+2])<<8|b[n+3];};
        w=u32(16);h=u32(20);
    } else if(b.size()>4 && b[0]==255 && b[1]==216) {
        size_t n=2;
        while(n+4<=b.size()) {
            if(b[n++]!=255) return false;
            while(n<b.size() && b[n]==255)++n;
            if(n+3>b.size())return false;
            unsigned marker=b[n++]; if(marker==217 || marker==218)break;
            if(marker==1 || (marker>=208 && marker<=215))continue;
            unsigned len=unsigned(b[n])<<8|b[n+1];
            if(len<2 || n+len>b.size())return false;
            if((marker>=192 && marker<=195) || (marker>=197 && marker<=199) || (marker>=201 && marker<=203) || (marker>=205 && marker<=207)) {
                if(len<7)return false;
                h=unsigned(b[n+3])<<8|b[n+4];w=unsigned(b[n+5])<<8|b[n+6];break;
            }
            n+=len;
        }
    }
    return w && h && w<=2048 && h<=2048 && uint64_t(w)*h<=4194304;
}
}
ExpansionCover read_expansion_cover(const std::string& selected,const std::string& root) {
    ExpansionCover out;
    if(selected.empty() || root.empty())return out;
    std::error_code e;
    auto boundary=fs::weakly_canonical(root,e);if(e)return out;
    auto folder=fs::weakly_canonical(selected,e);if(e || !within(folder,boundary))return out;
    if(!fs::is_directory(folder,e))folder=folder.parent_path();
    for(unsigned depth=0;depth<5 && within(folder,boundary);++depth) {
        auto raw=bytes(folder/"Expansion.xml",256*1024);
        auto xml=raw.empty()?nullptr:juce::XmlDocument::parse(juce::String::fromUTF8(reinterpret_cast<const char*>(raw.data()),int(raw.size())));
        if(xml) {
            out.title=xml->getChildElementAllSubText("title",{}).trim().substring(0,80).toStdString();
            out.author=xml->getChildElementAllSubText("manufacturer",{}).trim().substring(0,80).toStdString();
            out.folder=folder.string();
            std::vector<std::string> candidates;
            for(auto key:{"image","Image","imageName","imageFileName","cover"}) {
                auto name=xml->getChildElementAllSubText(key,{}).trim().toStdString();
                if(!name.empty())candidates.push_back(name);
            }
            for(auto name:{"Artwork.jpg","Artwork.png","artwork.jpg","cover.jpg","cover.png","Expansion.png"}) candidates.push_back(name);
            for(const auto& name:candidates) {
                fs::path relative(name);
                if(relative.is_absolute())continue;
                auto file=fs::weakly_canonical(folder/relative,e);
                if(e || !within(file,folder))continue;
                auto data=bytes(file,4*1024*1024);
                if(!dimensions(data))continue;
                auto image=juce::ImageFileFormat::loadFrom(data.data(),data.size());
                if(image.isValid()) { out.image=std::make_shared<juce::Image>(image.rescaled(128,128,juce::Graphics::highResamplingQuality));return out; }
            }
            return out; // This is the pack, even if its cover is absent or invalid.
        }
        if(folder==boundary)break;
        folder=folder.parent_path();
    }
    return out;
}
}
