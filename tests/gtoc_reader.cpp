#include "apex/gtoc.h"
#include "apex/hashes.h"
#include "redscore/platform/file/memory_file.h"
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <utility>

void check(bool ok) { if (!ok) throw std::runtime_error("GTOC regression failed"); }
void put(std::vector<uint8> &b, size_t p, uint32 v) {
    for (size_t i = 0; i < 4; ++i) b.at(p+i) = (v >> (8*i)) & 255;
}
GTOCFile parse(std::vector<uint8> b) {
    IO::MemoryFile file(std::move(b));
    return GTOCFile(file);
}
std::vector<uint8> fixture() {
    std::vector<uint8> b(100);
    std::memcpy(b.data(), "GT0C", 4); put(b,4,2);
    put(b,8,0x12345678); put(b,12,0xfedcba98); put(b,16,2);
    put(b,20,64-20); put(b,24,0x10203040);
    put(b,28,84-28); put(b,32,0xffffffff);
    put(b,36,0x87654321); put(b,40,0xabcdef01); put(b,44,2);
    put(b,48,64-48); put(b,52,512);
    put(b,56,84-56); put(b,60,0); // external resource, still a member
    put(b,64,0xdeadbeef); put(b,68,0x99887766); put(b,72,0xffffffff);
    std::memcpy(b.data()+76,"a.bin",6);
    put(b,84,0x11223344); put(b,88,0x55667788); put(b,92,0);
    std::memcpy(b.data()+96,"end",4); // terminator at the final byte
    return b;
}
void rejects(std::vector<uint8> b) {
    try { (void)parse(std::move(b)); } catch (const std::runtime_error &) { return; }
    throw std::runtime_error("Malformed GTOC accepted");
}
void verify(const GTOCFile &toc) {
    check(toc.archives().size()==2 && toc.files().size()==2);
    const auto &a=toc.archives()[0], &b=toc.archives()[1];
    check(a.hash==0x12345678 && a.tag==0xfedcba98 && a.members.size()==2);
    check(b.hash==0x87654321 && b.tag==0xabcdef01 && b.members.size()==2);
    check(a.members[0].file_index==b.members[0].file_index);
    check(a.members[1].file_index==b.members[1].file_index);
    check(a.members[0].file_index!=a.members[1].file_index);
    check(a.members[0].offset==0x10203040 && a.members[1].offset==0xffffffff);
    check(b.members[0].offset==512 && b.members[1].offset==0);
    const auto &first=toc.files().at(a.members[0].file_index);
    const auto &last=toc.files().at(a.members[1].file_index);
    check(first.hash==0xdeadbeef && first.ext_hash==0x99887766);
    check(first.size==0xffffffff && first.name=="a.bin");
    check(last.hash==0x11223344 && last.ext_hash==0x55667788);
    check(last.size==0 && last.name=="end");
}
void verify_enumerated_keys() {
    class FixtureManager final : public ArchiveManager<uint64> {
        std::pair<bool, uint64> load_child_archive(const uint64 &) override { return {false, 0}; }
    } manager;
    GTOCArchive archive(manager, std::make_unique<IO::MemoryFile>(fixture()), 1);
    std::vector<uint64> keys;
    archive.foreach_file([&](const Archive<uint64>::ArchiveEntry &entry) {
        check(archive.has(entry.key));
        keys.push_back(entry.key);
        return true;
    });
    check(keys == std::vector<uint64>{asset_path_hash("a.bin"), asset_path_hash("end")});
}
int main() {
    try {
        verify(parse(fixture())); // input IO::File has already been destroyed
        verify_enumerated_keys();
        auto moved=[] { auto source=parse(fixture()); return GTOCFile(std::move(source)); }();
        verify(moved);
        auto assigned=parse(fixture());
        { auto source=parse(fixture()); assigned=std::move(source); }
        verify(assigned);
        auto b=fixture(); b.resize(8); put(b,4,0);
        check(parse(b).archives().empty() && parse(b).files().empty());
        b=fixture(); b.resize(20); put(b,4,1); put(b,16,0);
        check(parse(b).archives().size()==1 && parse(b).files().empty());
        for (size_t size : {0,3,7,19,27,47,63,75,95,99}) {
            b=fixture(); b.resize(size); rejects(b);
        }
        b=fixture(); b[0]='S'; rejects(b); // STOC uses GT0C, not ST0C
        b=fixture(); put(b,4,0xffffffff); rejects(b);
        b=fixture(); put(b,16,0xffffffff); rejects(b);
        b=fixture(); put(b,44,0xffffffff); rejects(b);
        b=fixture(); put(b,20,100-20); rejects(b); // target at EOF
        b=fixture(); put(b,20,96-20); rejects(b); // incomplete metadata header
        b=fixture(); put(b,20,0xffffffff); rejects(b); // unsigned, never a backward pointer
        b=fixture(); put(b,56,0x80000000); rejects(b);
        b=fixture(); b[99]='x'; rejects(b); // no NUL before EOF
        std::cout << "GTOC regression tests passed\n";
    } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
