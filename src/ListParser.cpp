#include "ListParser.hpp"
#include <algorithm>
#include <cctype>
#include <regex>
#include <unordered_set>

namespace tsl {
namespace {
std::string normalize(std::string_view s) {
    std::string out;
    out.reserve(s.size());
    bool space = false;
    for (unsigned char c : s) {
        if (std::isspace(c)) { space = true; continue; }
        if (space && !out.empty()) out.push_back(' ');
        space = false;
        out.push_back(static_cast<char>(c));
    }
    return out;
}
std::string decode(std::string_view s) {
    std::string out;
    out.reserve(s.size());
    for (size_t i=0; i<s.size(); ++i) {
        if (s[i]!='&') { out.push_back(s[i]); continue; }
        auto semi=s.find(';',i+1);
        if (semi==std::string_view::npos || semi-i>12) { out.push_back(s[i]); continue; }
        auto e=s.substr(i+1,semi-i-1);
        if (e=="amp") out+='&';
        else if (e=="lt") out+='<';
        else if (e=="gt") out+='>';
        else if (e=="quot") out+='"';
        else if (e=="apos" || e=="#39") out+='\'';
        else if (e=="nbsp") out+=' ';
        else if (e.size()>1 && e[0]=='#') {
            try {
                bool hex=(e.size()>2 && (e[1]=='x'||e[1]=='X'));
                unsigned long cp=std::stoul(std::string(e.substr(hex?2:1)),nullptr,hex?16:10);
                if(cp<=0x7f) out.push_back(static_cast<char>(cp));
                else if(cp<=0x7ff) {out.push_back(char(0xc0|(cp>>6)));out.push_back(char(0x80|(cp&63)));}
                else if(cp<=0xffff){out.push_back(char(0xe0|(cp>>12)));out.push_back(char(0x80|((cp>>6)&63)));out.push_back(char(0x80|(cp&63)));}
                else if(cp<=0x10ffff){out.push_back(char(0xf0|(cp>>18)));out.push_back(char(0x80|((cp>>12)&63)));out.push_back(char(0x80|((cp>>6)&63)));out.push_back(char(0x80|(cp&63)));}
                else out+=std::string(s.substr(i,semi-i+1));
            } catch(...) {out+=std::string(s.substr(i,semi-i+1));}
        }
        else {out+=std::string(s.substr(i,semi-i+1));}
        i=semi;
    }
    return out;
}
std::string stripHTML(std::string_view html) {
    // Skip script/style contents: this prevents hydration JSON and CSS from
    // being interpreted as visible level cards.
    std::string out;
    out.reserve(html.size()/3);
    bool skipping=false;
    std::string tag;
    std::string skipName;
    for(size_t i=0;i<html.size();) {
        if(html[i]=='<') {
            auto end=html.find('>',i+1);
            if(end==std::string_view::npos) break;
            tag=lowercase(std::string(html.substr(i+1,std::min<size_t>(end-i-1,90))));
            auto p=tag.find_first_of(" \t\r\n/>");
            auto raw=tag.substr(tag[0]=='/'?1:0,p==std::string::npos?std::string::npos:p-(tag[0]=='/'?1:0));
            if(!skipping && (raw=="script"||raw=="style"||raw=="noscript")) {skipping=true;skipName=raw;}
            else if(skipping && tag.size()>1 && tag[0]=='/' && raw==skipName) {skipping=false;skipName.clear();}
            if(!skipping) out.push_back(' ');
            i=end+1;
        } else {
            if(!skipping) out.push_back(html[i]);
            ++i;
        }
    }
    return normalize(decode(out));
}
}

std::string lowercase(std::string s) {
    std::transform(s.begin(),s.end(),s.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
    return s;
}

std::vector<Level> parseListText(std::string_view input) {
    const auto text=normalize(input);
    // All list cards follow '#N Shitty NAME by CREATOR'. Avoid consuming the
    // next card by first identifying each rank/name boundary.
    static const std::regex cardStart(R"(#\s*([0-9]{1,4})\s+(Shitty\s+))",std::regex::icase);
    std::vector<std::pair<size_t,std::smatch>> positions;
    for(auto it=std::sregex_iterator(text.begin(),text.end(),cardStart);it!=std::sregex_iterator();++it)
        positions.emplace_back(static_cast<size_t>(it->position()),*it);
    std::vector<Level> out;
    std::unordered_set<int> seen;
    for(size_t k=0;k<positions.size();++k) {
        auto const& match=positions[k].second;
        int rank=0;
        try {rank=std::stoi(match[1].str());} catch(...) {continue;}
        if(rank<1 || rank>5000 || seen.contains(rank)) continue;
        size_t start=positions[k].first + static_cast<size_t>(match.length());
        size_t end=k+1<positions.size()?positions[k+1].first:text.size();
        if(end<=start || end-start>1500) continue;
        auto chunk=text.substr(start,end-start);
        // Level names are capped for sanity; 'by' appears after the name in
        // the rendered cards, not in arbitrary freeform site content.
        auto by=chunk.find(" by ");
        if(by==std::string::npos || by<1 || by>100) continue;
        auto name=normalize(std::string("Shitty ")+chunk.substr(0,by));
        auto creator=normalize(chunk.substr(by+4));
        if(name.size()>110 || creator.empty() || creator.size()>130) continue;
        // Eliminate trailing website furniture on the final entry.
        for(auto const& footer : {" Levels Packs", " List Rankings", " Back to Top", " Footer", " The Shitty List v3"}) {
            auto at=creator.find(footer);
            if(at!=std::string::npos) creator.resize(at);
        }
        if(creator.size()>64 || creator.empty()) continue;
        seen.insert(rank);
        out.push_back({rank,std::move(name),std::move(creator)});
    }
    std::sort(out.begin(),out.end(),[](Level const& a,Level const& b){return a.rank<b.rank;});
    return out;
}

std::vector<Level> parseListHTML(std::string_view html) {
    return parseListText(stripHTML(html));
}
}
