#include "havok/havok_codegen.h"
#include <fstream>
#include <iostream>

using namespace Havok;
static void check(bool ok) { if (!ok) throw std::runtime_error("Havok codegen regression failed"); }
static std::string read(const std::filesystem::path &p) {
    std::ifstream f(p); return {std::istreambuf_iterator<char>(f), {}};
}
static Tag::SharedType make(std::string name, Tag::DataType kind, uint32 size) {
    auto t = std::make_shared<Tag::Type>(); t->name = std::move(name); t->data_type = kind;
    t->size(size); t->align(size ? std::min(size, 8u) : 1u); return t;
}
static void arg(const Tag::SharedType &t, const Tag::SharedType &v) {
    t->template_args.push_back({"t" + std::to_string(t->template_args.size()), Tag::WeakType(v)});
}
static void field(const Tag::SharedType &t, std::string name, uint64 offset, const Tag::SharedType &v) {
    t->members.emplace_back(name, 0, offset, Tag::WeakType(v));
}
int main(int argc, char **argv) {
    try {
        check(argc == 2);
        const auto out = std::filesystem::path(argv[1]);
        using D = Tag::DataType;
        auto integer = make("int", D::BASIC, 4); integer->format = 0x20;
        auto tiny = make("unsigned char", D::BASIC, 1);
        auto number = make("CustomNumber", D::BASIC, 4); number->format = 0x20;
        auto real = make("CustomReal", D::FLOAT, 4);
        auto boolean = make("CustomBool", D::BOOL, 1);
        auto unknown = make("OpaquePayload", D::OPAQUE, 3);
        auto base = make("ZBase", D::RECORD, 4); field(base,"number",0,number);
        auto leaf = make("ZLeaf", D::RECORD, 4); field(leaf,"value",0,integer);
        auto fixed = make("T[N]", D::ARRAY, 8); arg(fixed,leaf); fixed->template_args.push_back({"N",int64(2)});
        auto kind = make("Kind", D::OPAQUE, 4); kind->format = 0x20;
        auto enumeration = make("hkEnum", D::BASIC, 4); arg(enumeration,kind); arg(enumeration,integer);
        auto small_enum = make("hkEnum", D::PRIMITIVE, 1); arg(small_enum,kind); arg(small_enum,tiny);
        auto derived = make("ADerived", D::RECORD, 24); derived->parent=base;
        field(derived,"leaf",4,leaf); field(derived,"fixed",8,fixed); field(derived,"kind",16,enumeration);
        field(derived,"smallKind",20,small_enum); field(derived,"opaque",21,unknown);
        auto node = make("Node", D::RECORD, 8), ptr = make("T*", D::POINTER, 8);
        arg(ptr,node); field(node,"next",0,ptr);
        auto a = make("CycleA",D::RECORD,8), b = make("CycleB",D::RECORD,8);
        auto pa=make("T*",D::POINTER,8),pb=make("T*",D::POINTER,8);
        arg(pa,a);arg(pb,b);field(a,"other",0,pb);field(b,"other",0,pa);
        auto allocator=make("Allocator",D::OPAQUE,0);
        auto vector=make("hkArray",D::ARRAY,16);arg(vector,node);arg(vector,allocator);
        auto vector_holder=make("VectorHolder",D::RECORD,16);field(vector_holder,"nodes",0,vector);
        auto box=make("Box",D::RECORD,4);arg(box,integer);field(box,"value",0,integer);
        auto alias=make("BoxAlias",D::PRIMITIVE,4);alias->parent=box;
        auto holder=make("BoxHolder",D::RECORD,4);field(holder,"box",0,alias);
        auto hkbase=make("hkBaseObject",D::RECORD,8);
        auto free=make("hkFreeListArrayElement",D::RECORD,4);arg(free,leaf);
        auto support=make("SupportHolder",D::RECORD,12);field(support,"base",0,hkbase);field(support,"free",8,free);
        auto rotation=make("hkRotationImpl",D::ARRAY,48);arg(rotation,real);
        auto rot_alias=make("Rotation",D::PRIMITIVE,48);rot_alias->parent=rotation;
        auto rot_holder=make("RotationHolder",D::RECORD,48);field(rot_holder,"rotation",0,rot_alias);
        auto scalars=make("ScalarHolder",D::RECORD,9);field(scalars,"directKind",5,kind);field(scalars,"real",0,real);field(scalars,"boolean",4,boolean);
        auto parent_cycle=make("ParentCycle",D::RECORD,8),child_cycle=make("ChildCycle",D::RECORD,12);
        child_cycle->parent=parent_cycle;field(child_cycle,"number",8,integer);
        auto child_ptr=make("T*",D::POINTER,8);arg(child_ptr,child_cycle);field(parent_cycle,"child",0,child_ptr);
        auto ref_node=make("RefNode",D::RECORD,8),ref=make("hkRefPtr",D::RECORD,8);
        arg(ref,ref_node);field(ref_node,"next",0,ref);
        auto tree=make("Tree",D::RECORD,16),children=make("hkArray",D::ARRAY,16);
        arg(children,tree);arg(children,allocator);field(tree,"children",0,children);
        std::vector<Tag::SharedType> roots{rot_holder,tree,ref_node,parent_cycle,child_cycle,derived,node,a,b,vector_holder,holder,support,scalars};
        CodeGen::TypeLibrary lib;
        lib.register_type(std::make_shared<Tag::Type>());
        for (auto &t:roots) lib.register_type(t);
        auto generate=[&](const CodeGen::TypeLibrary &l){CodeGen::generate_code(l,out,out/"havok/generated");};
        generate(lib);
        const auto h=read(out/"havok/generated/havok_types.h"), cpp=read(out/"havok_types.cpp");
        check(h.find("struct ZBase :")<h.find("struct ADerived :"));
        check(h.find("struct ZLeaf :")<h.find("struct ADerived :"));
        check(h.contains("using CustomNumber = int32;"));
        check(h.contains("using CustomReal = float32;"));
        check(h.contains("using CustomBool = bool;"));
        check(h.contains("enum class Kind : int32 {}"));
        check(h.find("enum class Kind") == h.rfind("enum class Kind"));
        check(h.contains("using BoxAlias = Box<int32>;"));
        check(cpp.contains("directKind = buffer.read_pod<Kind>()"));
        check(cpp.contains("base.read(buffer, tag_file)"));
        check(cpp.contains("free.read(buffer, tag_file)"));
        check(!cpp.contains("read_pod<hkBaseObject>"));
        check(!cpp.contains("read_pod<hkFreeListArrayElement"));
        generate(lib); check(h==read(out/"havok/generated/havok_types.h"));
        CodeGen::TypeLibrary reverse;
        reverse.register_type(std::make_shared<Tag::Type>());
        for (auto it=roots.rbegin();it!=roots.rend();++it) reverse.register_type(*it);
        generate(reverse);check(h==read(out/"havok/generated/havok_types.h"));check(cpp==read(out/"havok_types.cpp"));
        // Real by-value cycles cannot be represented by C++; fail with an actionable path.
        auto bad_a=make("BadA",D::RECORD,4),bad_b=make("BadB",D::RECORD,4);
        field(bad_a,"other",0,bad_b);field(bad_b,"other",0,bad_a);
        CodeGen::TypeLibrary bad;bad.register_type(bad_a);
        bool rejected=false;
        try {CodeGen::generate_code(bad,out/"bad",out/"bad/havok/generated");}
        catch (const std::runtime_error &e) {rejected=std::string(e.what()).contains("dependency cycle");}
        check(rejected);
        std::cout << "Havok codegen regressions passed\n";
    } catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}
