// SPDX-License-Identifier: AGPL-3.0-only
// Tape Workshop; based on Melodee by Kerem Kiliç (Ellic Studio).
#include "melodee/model/harmonizer.h"
#include "melodee/model/music_genres.h"
#include "melodee/model/audio_harmony.h"
#include "melodee/model/bass_line.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <filesystem>
#include <fstream>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
namespace melodee::harmony {
namespace {
struct Quality { const char* name; unsigned category,count; std::array<int,5> intervals; };
const Quality qualities[]={{"",1,3,{0,4,7}},{"m",1,3,{0,3,7}},{"dim",1,3,{0,3,6}},{"aug",1,3,{0,4,8}},
 {"maj7",2,4,{0,4,7,11}},{"m7",2,4,{0,3,7,10}},{"7",2,4,{0,4,7,10}},{"m7b5",2,4,{0,3,6,10}},
 {"maj9",4,5,{0,4,7,11,14}},{"m9",4,5,{0,3,7,10,14}},{"9",4,5,{0,4,7,10,14}}};
struct Candidate { Chord chord; double local=0; };
bool allowed(const Settings& s,unsigned root,unsigned quality){if(!s.strict_scale)return true;if(s.root<0||s.root>11||!s.scale_mask)return false;for(unsigned n=0;n<qualities[quality].count;++n)if(!(s.scale_mask&(1u<<((root+qualities[quality].intervals[n]+12-unsigned(s.root))%12))))return false;return true;}
bool tone(const Chord& c,unsigned pitch){const auto& q=qualities[c.quality];for(unsigned n=0;n<q.count;++n)if((c.root+q.intervals[n])%12==pitch%12)return true;return false;}
unsigned hash(unsigned x){x^=x>>16;x*=0x7feb352d;x^=x>>15;x*=0x846ca68b;return x^(x>>16);}
void voice(Chord& c,unsigned inv){const auto& q=qualities[c.quality];c.count=uint8_t(q.count);c.inversion=uint8_t(inv%q.count);for(unsigned n=0;n<q.count;++n)c.notes[n]=uint8_t(48+c.root+q.intervals[n]+(n<c.inversion?12:0));std::sort(c.notes.begin(),c.notes.begin()+c.count);}
double movement(const Chord& a,const Chord& b){double sum=0;for(unsigned n=0;n<b.count;++n){int best=127;for(unsigned k=0;k<a.count;++k)best=std::min(best,std::abs(int(a.notes[k])-b.notes[n]));sum+=best;}return sum/std::max(1u,unsigned(b.count));}
void be(std::vector<uint8_t>& b,uint32_t v,unsigned count){for(int i=int(count)-1;i>=0;--i)b.push_back(uint8_t(v>>(i*8)));}
void vlq(std::vector<uint8_t>& b,uint32_t v){uint8_t out[4];unsigned n=0;do{out[n++]=v&127;v>>=7;}while(v&&n<4);while(n){--n;b.push_back(out[n]|(n?128:0));}}
}
const char* follow_name(Follow f){const char* names[]={"Every Note","Every Beat","Every Phrase","Every Bar","Auto"};return names[std::min(4u,unsigned(f))];}
std::string chord_name(const Chord& c){const char* names[]={"C","C#","D","Eb","E","F","F#","G","Ab","A","Bb","B"};return std::string(names[c.root%12])+qualities[c.quality%11].name;}
void invert(Chord& c,int delta){int count=qualities[c.quality%11].count;voice(c,unsigned((int(c.inversion)+delta%count+count)%count));}
Result generate(const std::vector<Note>& input,uint32_t length,unsigned beat,unsigned bar,const Settings& s){
    Result result;if(input.empty()||!length||!s.complexity)return result;
    beat=std::max(1u,beat);bar=std::max(beat,bar);length=std::min(length,128u*bar);
    std::vector<Note> notes;notes.reserve(std::min(input.size(),size_t(max_melody)));
    for(const auto& n:input)if(n.velocity&&n.key<128&&n.length&&n.tick<length){notes.push_back(n);notes.back().length=std::min(n.length,length-n.tick);if(notes.size()==max_melody)break;}
    if(notes.empty())return result;
    std::stable_sort(notes.begin(),notes.end(),[](const Note&a,const Note&b){return a.tick<b.tick;});
    std::array<double,12> histogram{};
    for(const auto& n:notes)histogram[n.key%12]+=std::min(n.length,length-n.tick)*(0.4+n.velocity/127.0)*(n.tick%beat==0?1.5:1);
    const double profiles[2][12]={{6.35,2.23,3.48,2.33,4.38,4.09,2.52,5.19,2.39,3.66,2.29,2.88},{6.33,2.68,3.52,5.38,2.6,3.53,2.54,4.75,3.98,2.69,3.34,3.17}};
    double best=-1e30;for(unsigned mode=0;mode<2;++mode)for(unsigned root=0;root<12;++root){double score=0;for(unsigned n=0;n<12;++n)score+=histogram[(root+n)%12]*profiles[mode][n];if(score>best){best=score;result.tonic=uint8_t(root);result.minor=mode;}}
    if(s.root>=0&&s.root<12){result.tonic=uint8_t(s.root);result.minor=(s.scale_mask&(1u<<3))&&!(s.scale_mask&(1u<<4));}
    const unsigned major[]={0,2,4,5,7,9,11},minor[]={0,2,3,5,7,8,10};std::array<bool,12> scale{};for(unsigned n=0;n<7;++n)scale[(result.tonic+(result.minor?minor[n]:major[n]))%12]=true;
    if(s.root>=0&&s.root<12&&s.scale_mask){for(unsigned n=0;n<12;++n)scale[(result.tonic+n)%12]=s.scale_mask&(1u<<n);}
    std::vector<uint32_t> boundaries{notes.front().tick};
    const auto mode=unsigned(s.follow)<=4?s.follow:Follow::Phrase;
    if(mode==Follow::Beat||mode==Follow::Bar){unsigned step=mode==Follow::Beat?beat:bar;boundaries[0]=0;for(uint32_t t=step;t<length&&boundaries.size()<max_chords;t+=step)boundaries.push_back(t);}
    else if(mode==Follow::Note){for(const auto& n:notes){boundaries.push_back(n.tick);boundaries.push_back(std::min(length,n.tick+n.length));}}
    else{uint32_t last=notes.front().tick,previousEnd=last;for(const auto& n:notes){bool rest=n.tick>previousEnd&&n.tick-previousEnd>=beat/2;bool cadence=n.tick%beat==0&&n.length>=beat&&(n.key%12==result.tonic||n.tick-last>=bar);bool timeout=n.tick-last>=2*bar;bool automatic=mode==Follow::Auto&&n.tick%beat==0&&n.tick-last>=beat&&n.velocity>=100;
        if(n.tick>last&&(rest||cadence||timeout||automatic)){boundaries.push_back(n.tick);last=n.tick;}previousEnd=std::max(previousEnd,std::min(length,n.tick+n.length));}}
    boundaries.push_back(length);std::sort(boundaries.begin(),boundaries.end());boundaries.erase(std::unique(boundaries.begin(),boundaries.end()),boundaries.end());
    if(boundaries.size()>max_chords+1){boundaries.resize(max_chords+1);boundaries.back()=length;}
    std::vector<std::vector<Candidate>> rows;rows.reserve(boundaries.size());
    for(size_t at=0;at+1<boundaries.size();++at){uint32_t begin=boundaries[at],end=boundaries[at+1];double weight=0;unsigned velocity=0,count=0;
        for(const auto& n:notes)if(n.tick<end&&uint64_t(n.tick)+n.length>begin){weight+=std::min<uint64_t>(end,uint64_t(n.tick)+n.length)-std::max(begin,n.tick);velocity+=n.velocity;++count;}
        if(!weight)continue;
        std::vector<Candidate> row;
        for(unsigned root=0;root<12;++root)for(unsigned q=0;q<11;++q){if(!(s.complexity&qualities[q].category)||!allowed(s,root,q))continue;
            Chord c;c.tick=begin;c.length=end-begin;c.root=uint8_t(root);c.quality=uint8_t(q);c.velocity=uint8_t(std::clamp(velocity/std::max(1u,count),35u,115u));
            double compatibility=0;for(const auto& n:notes){uint64_t stop=std::min<uint64_t>(end,uint64_t(n.tick)+n.length);uint32_t start=std::max(begin,n.tick);if(stop<=start)continue;double emphasis=(0.4+n.velocity/127.0)*(n.tick%beat==0?1.7:1.0);double span=(stop-start)/weight;bool matches=tone(c,n.key);compatibility+=span*emphasis*(matches?2.4:-1.3);}
            double keyFit=0;for(unsigned n=0;n<qualities[q].count;++n)keyFit+=scale[(root+qualities[q].intervals[n])%12]?0.3:-0.6;
            double genre=music::quality_bonus(s.genre,q,qualities[q].category);
            double variation=(int(hash(unsigned(at)*121+root*11+q+s.variation*7919)%1000)-500)/500.0;
            double score=compatibility*(0.4+std::min(100u,unsigned(s.strength))*.055)+keyFit+genre+variation*(s.variation?1.4:0.01)*(1.1-std::min(100u,unsigned(s.strength))*.008);
            if(end==length&&root==result.tonic)score+=.5;
            for(unsigned inv=0;inv<(s.leading?qualities[q].count:1);++inv){voice(c,inv);row.push_back({c,score-.008*inv});}
        }
        std::stable_sort(row.begin(),row.end(),[](const Candidate&a,const Candidate&b){return a.local>b.local;});if(row.size()>32)row.resize(32);if(row.empty())return Result{};rows.push_back(std::move(row));
    }
    if(rows.empty())return result;
    std::vector<std::array<double,32>> scores(rows.size());std::vector<std::array<uint8_t,32>> parents(rows.size());
    for(size_t i=0;i<rows.size();++i)for(size_t j=0;j<rows[i].size();++j){double previous=-1e30;unsigned parent=0;
        if(!i)previous=0;else for(size_t k=0;k<rows[i-1].size();++k){const auto&a=rows[i-1][k].chord;const auto&b=rows[i][j].chord;int interval=(b.root+12-a.root)%12;double progression=(interval==5||interval==7?.16:interval==0?.08:0);double candidate=scores[i-1][k]+progression-(s.leading?movement(a,b)*.1:0);if(candidate>previous){previous=candidate;parent=unsigned(k);}}
        scores[i][j]=previous+rows[i][j].local;parents[i][j]=uint8_t(parent);
    }
    unsigned choice=0;for(unsigned j=1;j<rows.back().size();++j)if(scores.back()[j]>scores.back()[choice])choice=j;
    result.scale_mask=uint16_t(s.scale_mask?s.scale_mask:(result.minor?0x5ad:0xab5));
    result.count=uint16_t(rows.size());for(size_t i=rows.size();i-->0;){result.chords[i]=rows[i][choice].chord;choice=parents[i][choice];}return result;
}
Result recognize(const std::vector<Chroma>& bins,const Settings& s){
    Result out;if(!(s.complexity&7)||bins.size()>max_chords)return out;
    std::vector<std::vector<Candidate>> rows;std::array<float,12> histogram{};
    for(const auto& b:bins){std::vector<Candidate> row;if(!b.frames){rows.push_back({});continue;}
        for(unsigned n=0;n<12;++n)histogram[n]+=b.tones[n]*b.length;
        for(unsigned root=0;root<12;++root)for(unsigned q=0;q<11;++q){const auto& quality=qualities[q];if(!(s.complexity&quality.category)||!allowed(s,root,q))continue;
            float coverage=0,missing=0;for(unsigned n=0;n<quality.count;++n){float level=b.tones[(root+quality.intervals[n])%12];coverage+=level;if(level<.045f)missing+=.045f-level;}
            double score=coverage*4.5/std::sqrt(double(quality.count))+.18*b.bass[root]-.8*missing-.035*(quality.count-3);
            if(s.genre)score+=music::quality_bonus(s.genre,q,quality.category)*.15;
            Chord c;c.tick=b.tick;c.length=b.length;c.root=uint8_t(root);c.quality=uint8_t(q);c.velocity=90;voice(c,0);row.push_back({c,score});
        }
        std::stable_sort(row.begin(),row.end(),[](const Candidate&a,const Candidate&b){return a.local>b.local;});if(row.size()>24)row.resize(24);rows.push_back(std::move(row));
    }
    // Continuity rewards consistent harmonic regions without forcing every bar
    // into the same key; borrowed chords/modulations remain possible.
    std::vector<std::array<double,24>> scores(rows.size());std::vector<std::array<uint8_t,24>> parents(rows.size());
    for(size_t i=0;i<rows.size();++i)for(size_t j=0;j<rows[i].size();++j){double best=0;unsigned parent=0;
        if(i&&!rows[i-1].empty()){best=-1e30;for(unsigned k=0;k<rows[i-1].size();++k){auto&a=rows[i-1][k].chord;auto&b=rows[i][j].chord;double v=scores[i-1][k]+(a.root==b.root&&a.quality==b.quality?(.18-.0014*std::min(100u,unsigned(s.strength))):0);if(v>best){best=v;parent=k;}}}
        scores[i][j]=best+rows[i][j].local;parents[i][j]=uint8_t(parent);
    }
    std::vector<Chord> chosen(rows.size());for(size_t end=rows.size();end;){if(rows[end-1].empty()){--end;continue;}size_t start=end-1;while(start&&!rows[start-1].empty())--start;unsigned pick=0;for(unsigned j=1;j<rows[end-1].size();++j)if(scores[end-1][j]>scores[end-1][pick])pick=j;for(size_t i=end;i-->start;){chosen[i]=rows[i][pick].chord;pick=parents[i][pick];}end=start;}
    for(const auto& c:chosen){if(!c.count)continue;if(out.count){auto& previous=out.chords[out.count-1];if(previous.root==c.root&&previous.quality==c.quality&&previous.tick+previous.length==c.tick){previous.length+=c.length;continue;}}
        Chord voiced=c;if(s.leading&&out.count){double best=1e30;for(unsigned inv=0;inv<c.count;++inv){auto v=c;voice(v,inv);double cost=movement(out.chords[out.count-1],v);if(cost<best){best=cost;voiced=v;}}}out.chords[out.count++]=voiced;
    }
    // Whole-WAV generation keeps the file's duration, including a quiet tail.
    // Carry only the last known harmony forward; leave internal rests alone
    // and never invent harmony for a wholly silent/unrecognized recording.
    if(out.count&&!bins.empty()){
        auto& last=out.chords[out.count-1];
        const uint64_t end=uint64_t(bins.back().tick)+bins.back().length;
        if(end>uint64_t(last.tick)+last.length&&end<=std::numeric_limits<uint32_t>::max())
            last.length=uint32_t(end-last.tick);
    }
    const double profiles[2][12]={{6.35,2.23,3.48,2.33,4.38,4.09,2.52,5.19,2.39,3.66,2.29,2.88},{6.33,2.68,3.52,5.38,2.6,3.53,2.54,4.75,3.98,2.69,3.34,3.17}};double best=-1e30;for(unsigned mode=0;mode<2;++mode)for(unsigned root=0;root<12;++root){double fit=0;for(unsigned n=0;n<12;++n)fit+=histogram[(root+n)%12]*profiles[mode][n];if(fit>best){best=fit;out.tonic=root;out.minor=mode;}}
    if(s.root>=0&&s.root<12){out.tonic=uint8_t(s.root);if(s.scale_mask)out.minor=(s.scale_mask&(1u<<3))&&!(s.scale_mask&(1u<<4));}
    out.scale_mask=uint16_t(s.scale_mask?s.scale_mask:(out.minor?0x5ad:0xab5));
    return out;
}
Result progression(unsigned bars,unsigned bar,const Settings& s){
    Result out;if(!(s.complexity&7)||!bar||bar>49152)return out;
    bars=bars>=8?8:4;out.tonic=uint8_t(std::clamp(int(s.root),0,11));
    const unsigned mask=s.scale_mask?s.scale_mask:0xab5;out.scale_mask=uint16_t(mask);
    out.minor=(mask&(1u<<3))&&!(mask&(1u<<4));
    std::array<unsigned,12> degrees{};unsigned size=0;
    for(unsigned n=0;n<12;++n)if(mask&(1u<<n))degrees[size++]=n;
    if(!size)return out;
    constexpr unsigned paths[][4]={{0,5,3,4},{0,3,4,0},{0,4,5,3},{0,2,3,4},{0,5,1,4},{0,3,1,4},{0,1,4,0},{0,2,5,4}};
    const unsigned path=s.genre?(music::profile(s.genre).path+hash(s.variation))%8:hash(s.variation)%8;
    for(unsigned i=0;i<bars;++i){
        Chord bestChord;double best=-1e30;
        const unsigned degree=paths[(path+(i>=4?hash(s.variation+7)%3:0))%8][i%4]%size;
        const unsigned desired=(out.tonic+degrees[degree])%12;
        for(unsigned offset=0;offset<(s.strict_scale?12u:1u);++offset){const unsigned root=(desired+offset)%12;
        for(unsigned q=0;q<11;++q){if(!(s.complexity&qualities[q].category)||!allowed(s,root,q))continue;
            Chord c;c.root=uint8_t(root);c.quality=uint8_t(q);c.tick=i*bar;c.length=bar;c.velocity=90;
            double fit=0;for(unsigned n=0;n<qualities[q].count;++n)fit+=(mask&(1u<<((root+qualities[q].intervals[n]+12-out.tonic)%12)))?1.0:-3.0;
            fit/=qualities[q].count;
            if(offset)fit-=.8+.08*std::min(offset,12-offset);
            fit+=(hash(s.variation*7919+i*127+q*73)%1000)/1500.;
            if(s.genre)fit+=music::quality_bonus(s.genre,q,qualities[q].category);
            for(unsigned inv=0;inv<(s.leading?qualities[q].count:1);++inv){voice(c,inv);double score=fit-(i&&s.leading?movement(out.chords[i-1],c)*.06:inv*.005);if(score>best){best=score;bestChord=c;}}
        }
        }
        if(!bestChord.count)return Result{};out.chords[out.count++]=bestChord;
    }
    return out;
}
std::vector<uint8_t> midi(const Result& r,unsigned tempo,unsigned beats,unsigned unit){
    if(!r.count||r.count>max_chords)return {};
    std::vector<Note> notes;notes.reserve(r.count*5);
    for(unsigned i=0;i<r.count;++i){const auto& c=r.chords[i];if(c.count<3||c.count>5)return {};for(unsigned n=0;n<c.count;++n)notes.push_back({c.tick,c.length,c.notes[n],c.velocity});}
    return midi_notes(notes,tempo,beats,unit);
}
std::vector<uint8_t> midi_notes(const std::vector<Note>& notes,unsigned tempo,unsigned beats,unsigned unit){
    if(notes.empty()||notes.size()>max_bass_notes)return {};
    struct Message {uint32_t tick;uint8_t key,velocity;bool on;};std::vector<Message> events;events.reserve(notes.size()*2);
    for(const auto& n:notes){if(!n.length||n.length>0x0fffffff||n.key>127||n.tick>0x0fffffff-n.length)return {};events.push_back({n.tick,n.key,uint8_t(std::clamp<unsigned>(n.velocity,1,127)),true});events.push_back({n.tick+n.length,n.key,0,false});}
    std::stable_sort(events.begin(),events.end(),[](const Message&a,const Message&b){return a.tick!=b.tick?a.tick<b.tick:!a.on&&b.on;});
    std::vector<uint8_t> body{0,255,81,3};uint32_t us=uint32_t(600000000ull/std::clamp(tempo,100u,9990u));be(body,us,3);unsigned power=0;while((1u<<power)<std::max(1u,unit)&&power<6)++power;body.insert(body.end(),{0,255,88,4,uint8_t(std::clamp(beats,1u,64u)),uint8_t(power),24,8});
    uint32_t last=0;for(const auto&e:events){vlq(body,e.tick-last);last=e.tick;body.push_back(e.on?0x90:0x80);body.push_back(e.key);body.push_back(e.velocity);}body.insert(body.end(),{0,255,47,0});
    std::vector<uint8_t> out{'M','T','h','d',0,0,0,6,0,0,0,1,0,96,'M','T','r','k'};be(out,uint32_t(body.size()),4);out.insert(out.end(),body.begin(),body.end());return out;
}
bool save(const std::string& card,const std::string& name,const std::vector<uint8_t>& bytes,std::string& path,std::string& error){
    path.clear();error.clear();namespace fs=std::filesystem;
    std::error_code cardError;
    if(card.empty()||!fs::is_directory(card,cardError)){error="No SD card";return false;}
    if(bytes.empty()||bytes.size()>65536){error="No generated chords or MIDI too large";return false;}
    std::string clean;for(unsigned char c:name)if((c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='_'||c=='-')if(clean.size()<64)clean+=char(c);
    if(clean.empty()){error="Invalid filename";return false;}
    std::error_code ec;auto folder=fs::path(card)/"Melodee/MIDI";auto root=fs::weakly_canonical(card,ec);if(ec){error="SD card unavailable";return false;}auto resolved=fs::weakly_canonical(folder,ec);auto relative=resolved.lexically_relative(root);if(ec||relative.empty()||*relative.begin()==".."){error="MIDI folder is outside SD card";return false;}fs::create_directories(folder,ec);if(ec){error="SD folder: "+ec.message();return false;}
    auto space=fs::space(folder,ec);if(ec||space.available<bytes.size()+4096){error="SD card full or unavailable";return false;}
    for(unsigned n=0;n<1000;++n){auto dest=folder/(clean+(n?"_"+std::to_string(n):"")+".mid");// O_EXCL works on FAT/exFAT SD cards too; hard links do not.
        int fd=::open(dest.c_str(),O_CREAT|O_EXCL|O_WRONLY,0600);if(fd<0){if(errno==EEXIST)continue;error=std::strerror(errno);return false;}
        size_t written=0;while(written<bytes.size()){auto count=::write(fd,bytes.data()+written,bytes.size()-written);if(count<0&&errno==EINTR)continue;if(count<=0)break;written+=size_t(count);}bool ok=written==bytes.size()&&::fsync(fd)==0;int closed=::close(fd);ok=ok&&closed==0;
        std::ifstream verify(dest,std::ios::binary);std::vector<uint8_t> check((std::istreambuf_iterator<char>(verify)),{});ok=ok&&check==bytes;verify.close();
        if(ok){path=dest.string();return true;}::unlink(dest.c_str());error="SD write/verification failed";return false;
    }error="No free MIDI filename";return false;
}
}
