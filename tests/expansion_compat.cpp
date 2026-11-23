// SPDX-License-Identifier: AGPL-3.0-only
// Native integration probe: run with the directory made by generate_expansion_fixture.js.
#include "melodee/shell/expansion_install.h"
#include <juce_graphics/juce_graphics.h>
#include "melodee/shell/expansion_cover.h"
#include "melodee/shell/mpc_program.h"
#include "melodee/shell/project_kits.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#define CHECK(x) do{if(!(x)){std::cerr<<"FAIL line "<<__LINE__<<": "<<#x<<"\n";return 1;}}while(0)
std::vector<uint8_t> read(const std::string& p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
int main(int argc,char**argv){
 using namespace melodee;namespace fs=std::filesystem;
 if(argc!=2)return 2;juce::ScopedJuceInitialiser_GUI init;
 fs::path root=argv[1],packs=root/"card/Samples/Expansions";
 fs::create_directories(packs);
 ExpansionInstall installer;CHECK(installer.open((root/"Web-Builder.xpn").string(),packs.string()).empty());
 std::string error;while(installer.step(error)){}CHECK(error.empty());
 fs::path pack=installer.folder();auto cover=read_expansion_cover((pack/"Kit.xpm").string(),(root/"card").string());
 CHECK((cover.image && cover.image->isValid())&&cover.image->getWidth()==128&&cover.title=="Bridge Kit"&&cover.author=="Tape Workshop");
 auto from=std::make_unique<Project>();MpcImport report;
 CHECK(import_mpc_program(read((pack/"Kit.xpm").string()),(pack/"Kit.xpm").string(),*from,report).empty());
 CHECK(report.pads==3&&report.missing==0);
 auto bank=bank_of(*from,0,0,"Test");CHECK(bank.used()==3&&bank.pads[0].used()&&bank.pads[7].used()&&bank.pads[15].used());
 for(auto& sample:bank.samples)if(sample.used()){auto wav=read(sample.path);CHECK(wav.size()==8864&&wav[0]=='R'&&wav[8]=='W');bool nonzero=false;for(size_t n=44;n<wav.size();++n)nonzero|=wav[n]!=0;CHECK(nonzero);}
 Track into;into.engine=EngineKind::Sampler;into.pads[0].sample=90;into.pads[33].sample=91;
 auto result=import_bank(bank,1,into,[](const SampleRef&){return 40;});CHECK(result.pads==3&&!result.missing&&into.pads[16].sample==40&&into.pads[23].sample==40&&into.pads[31].sample==40&&into.pads[0].sample==90&&into.pads[33].sample==91);
 auto pad=import_expansion_pad(bank,15,45,into,[](const SampleRef&){return 41;});CHECK(pad.pads==1&&into.pads[45].sample==41&&into.pads[33].sample==91);
 auto mpc=root/"MPC3/Test.xty";auto desktop=std::make_unique<Project>();MpcImport desktopReport;
 CHECK(import_mpc_program(read(mpc.string()),mpc.string(),*desktop,desktopReport).empty());CHECK(desktopReport.pads==2&&desktopReport.missing==0);
 auto desktopBank=bank_of(*desktop,0,1,"Desktop B");CHECK(desktopBank.used()==1&&desktopBank.pads[1].used());
 auto desktopPad=import_expansion_pad(desktopBank,1,49,into,[](const SampleRef&){return 42;});CHECK(desktopPad.pads==1&&into.pads[49].sample==42&&into.pads[45].sample==41);
 auto desktopCover=read_expansion_cover(mpc.string(),root.string());CHECK((desktopCover.image && desktopCover.image->isValid())&&desktopCover.title=="Desktop Test");
 fs::create_directories(root/"Unsafe");std::ofstream(root/"Unsafe/Expansion.xml")<<"<expansion><title>Unsafe</title><image>../Cover.jpg</image></expansion>";
 CHECK(!read_expansion_cover((root/"Unsafe").string(),root.string()).image);
 fs::create_directories(root/"Missing");std::ofstream(root/"Missing/Expansion.xml")<<"<expansion><title>No Cover</title></expansion>";
 auto missing=read_expansion_cover((root/"Missing").string(),root.string());CHECK(!missing.image&&missing.title=="No Cover");
 CHECK(!read_expansion_cover(root.string(),(root/"card").string()).image);
 std::cout<<"PASS web-builder XPN: install, artwork+metadata, 3 assigned pads, non-silent WAV files, bank B load, individual-pad load, other banks preserved\nPASS MPC3-format fixture: gzip XTY, relative sample files, source bank B, single-pad import, cover metadata\nPASS artwork containment and missing-cover fallback\n";
}
