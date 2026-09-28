#include "apex/asset_db.h"
#include "games.hpp"
#include <bit>
#include <chrono>
#include <iostream>

void check(bool value) { if (!value) throw std::runtime_error("AssetDB regression failed"); }
int main() {
    const auto root = std::filesystem::temp_directory_path() / ("apex-db-" + std::to_string(GAME) + "-" +
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(root);
    try {
        using Type = AssetDB::HashType;
        const std::string name = "AnimationSet";
        const auto hashes = string_hashes(name);
        check(hashes.lookup3 == 2035976115u);
#if GAME==GAME_GENERATION_ZERO
        check(hashes.murmur == 0);
#elif GAME==GAME_RAGE2
        check(hashes.murmur == hash_string(name) && hashes.murmur != 0);
#else
#error "Unsupported game"
#endif
        constexpr uint64 high = 18366995012003127915ull;
        {
            auto db = AssetDB::create_new(root / "new.db");
            db.kv_put(hashes, name);
            check(db.kv_get(hash_string(name)) == name);
            check(db.kv_get(hashes.lookup3, Type::Lookup3) == name);
            check(!db.kv_has(0, Type::Murmur));
            db.files_put(hashes, name, 123, high);
            check(db.get_file_name(hash_string(name)) == name);
            check(db.get_file_parent(hash_string(name)) == high);
            check(!db.get_file_size(999999));
            db.kv_put({42,high}, "high"); db.kv_put({42,high-1}, "collision");
            check(db.kv_get(high, Type::Murmur) == "high");
            check(db.kv_get(high-1, Type::Murmur) == "collision");
            db.kv_put({42,high}, "updated");
            check(db.kv_get(high, Type::Murmur) == "updated");
            db.kv_del(high, Type::Murmur);
            check(!db.kv_has(high, Type::Murmur) && db.kv_has(high-1, Type::Murmur));
            db.files_put({42,high}, "file", 321, high-1);
            auto f = db.get_file(high, Type::Murmur);
            check(f && f->lookup3==42 && f->murmur==high && f->parent_hash==high-1);
            db.files_del(high, Type::Murmur); check(!db.get_file(high, Type::Murmur));
        }
        {
            AssetDB db(root / "new.db"); check(db.kv_get(hash_string(name))==name);
        }
        {
            SQLite::Database db((root/"old.db").string(), SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
            db.exec("CREATE TABLE kv(k INTEGER PRIMARY KEY,v TEXT NOT NULL) WITHOUT ROWID;"
                    "CREATE TABLE files(hash INTEGER PRIMARY KEY,name TEXT,size INTEGER NOT NULL,parent INTEGER NOT NULL DEFAULT 0) WITHOUT ROWID;"
                    "CREATE INDEX idx_files_parent ON files(parent);");
            SQLite::Statement kv(db,"INSERT INTO kv VALUES(?,?)");
            kv.bind(1,std::bit_cast<int64>(hash_string(name))); kv.bind(2,name); kv.exec();
            SQLite::Statement file(db,"INSERT INTO files VALUES(?,?,123,?)");
            file.bind(1,std::bit_cast<int64>(hash_string(name))); file.bind(2,name);
            file.bind(3,std::bit_cast<int64>(high)); file.exec();
        }
        for (int i=0;i<2;++i) {
            AssetDB db(root/"old.db");
            check(db.kv_get(hash_string(name))==name);
            check(db.kv_get(hashes.lookup3,Type::Lookup3)==name);
            auto f=db.get_file(hash_string(name));
            check(f && f->lookup3==hashes.lookup3 && f->murmur==hashes.murmur && f->parent_hash==high);
            std::vector<std::string> results; db.files_search("Animation%",results);
            check(results==std::vector<std::string>{name});
        }
        std::filesystem::remove_all(root);
        std::cout << "Dual-hash database tests passed for game " << GAME << '\n';
    } catch (const std::exception &e) {
        std::cerr << e.what() << " (fixtures: " << root << ")\n"; return 1;
    }
}
