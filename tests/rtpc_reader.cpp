#include "apex/rtpc.h"
#include "redscore/platform/file/memory_file.h"
#include <cstring>
#include <fstream>
#include <iostream>

// JSON name resolution is unrelated to parsing; keep this test independent of the asset DB.
std::optional<std::string> find_lookup3_name(uint32) { return std::nullopt; }

void check(bool ok) { if (!ok) throw std::runtime_error("RTPC regression failed"); }
void put(std::vector<uint8> &b, size_t p, uint32 v, size_t n = 4) {
    for (size_t i = 0; i < n; ++i) b.at(p+i) = (v >> (8*i)) & 255;
}
RuntimeNode parse(std::vector<uint8> b) {
    std::unique_ptr<IO::File> f = std::make_unique<IO::MemoryFile>(std::move(b));
    return RuntimeNode::RootNode(f);
}
std::vector<uint8> fixture(uint32 version) {
    std::vector<uint8> b(87);
    std::memcpy(b.data(), "RTPC", 4); put(b,4,version);
    put(b,12,20); put(b,16,3,2); put(b,18,1,2);
    put(b,20,2035976115); put(b,24,68); b[28]=9;
    put(b,29,2); put(b,33,76); b[37]=10;
    put(b,38,3); put(b,42,84); b[46]=11;
    // One child header at 48; separate node bodies and metadata.
    put(b,52,64); if (version==3) { put(b,60,42); put(b,64,7); }
    put(b,68,1); put(b,72,123);
    put(b,76,1); float value=1.25f; std::memcpy(b.data()+80,&value,4);
    b.resize(91); put(b,84,3); b[88]='A'; b[89]='D'; b[90]='F';
    return b;
}
void rejects(std::vector<uint8> b) {
    try { (void)parse(std::move(b)); } catch (const std::runtime_error &) { return; }
    throw std::runtime_error("Malformed RTPC accepted");
}
int main(int argc, char **argv) {
    try {
        for (uint32 version : {1,2,3}) {
            const auto n=parse(fixture(version));
            check(n.version()==version && n.children().size()==1);
            check(n.has("AnimationSet") && n.is<std::vector<uint32>>("AnimationSet"));
            check(n.get<std::vector<uint32>>("AnimationSet")==std::vector<uint32>{123});
            check(n.get<std::vector<float32>>(2)==std::vector<float32>{1.25f});
            check(n.get<std::vector<uint8>>(3)==std::vector<uint8>({'A','D','F'}));
            check(n.children()[0].version()==version);
            if(version==3) {
                check(n.v3_metadata()==42 && n.children()[0].v3_metadata()==7);
                check(n.to_json()["v3_metadata"]==42);
            } else check(!n.v3_metadata() && !n.children()[0].v3_metadata());
        }
        auto b=fixture(3); put(b,68,0); check(parse(b).get<std::vector<uint32>>("AnimationSet").empty());
        b=fixture(3); put(b,68,0xffffffff); rejects(b);
        b=fixture(3); put(b,24,1000); rejects(b);
        b=fixture(3); b[28]=15; rejects(b);
        b=fixture(3); b[28]=3; put(b,24,90); rejects(b); // unterminated string
        b=fixture(3); b.resize(62); rejects(b); // truncated metadata
        b=fixture(3); put(b,52,20); put(b,56,3,2); put(b,58,1,2); rejects(b); // cycle
        b=fixture(3); put(b,4,4); rejects(b);
        b=fixture(3); b[0]='X'; rejects(b);
        if (argc>1) {
            std::ifstream f(argv[1],std::ios::binary);
            check(f.is_open());
            std::vector<uint8> data((std::istreambuf_iterator<char>(f)),{});
            const auto n=parse(std::move(data));
            check(n.version()==3 && n.children().size()==1);
            const auto &child=n.children()[0];
            check(child.v3_metadata()==2 && child.children().size()==4);
            const auto &adf=child.get<std::vector<uint8>>(3119224088u);
            check(adf.size()==2538366 && std::memcmp(adf.data()," FDA",4)==0);
            check(std::memcmp(adf.data()+adf.size()-13,"AnimationSet",12)==0);
            std::cout << "Rage 2 sample: complete 2538366-byte embedded ADF read\n";
        }
        std::cout << "RTPC regression tests passed\n";
    } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
