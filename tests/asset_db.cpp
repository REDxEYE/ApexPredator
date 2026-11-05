#include "apex/asset_db.h"
#include "utils/murmur3.h"
#include <bit>
#include <chrono>
#include <format>
#include <iostream>

void check(bool value) { if (!value) throw std::runtime_error("AssetDB regression failed"); }
int main() {
    const auto root = std::filesystem::temp_directory_path() / ("apex-db-" +
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(root);
    try {
        using Type = AssetDB::HashType;
        const std::string name = "AnimationSet";
        const auto hashes = string_hashes(name);
        check(hashes.lookup3 == 2035976115u);
        check(hashes.murmur == MurmurHash3_x64_128(name.data(), name.size(), 0) && hashes.murmur != 0);
        constexpr uint64 high = 18366995012003127915ull;
        {
            auto db = AssetDB::create_new(root / "new.db");
            db.kv_put(hashes, name);
            check(db.kv_get(hashes.lookup3, Type::Lookup3) == name);
            check(!db.kv_has(0, Type::Murmur));
            db.files_put(hashes, name, 123, high);
            {
                SQLite::Database other((root/"new.db").string(), SQLite::OPEN_READWRITE);
                const auto table = game_files_table() == "files_rage2"
                                       ? "files_generation_zero" : "files_rage2";
                other.exec(std::format(
                    "CREATE TABLE {} (lookup3 INTEGER NOT NULL,murmur INTEGER NOT NULL,name TEXT,"
                    "size INTEGER NOT NULL,parent INTEGER NOT NULL,PRIMARY KEY(lookup3,murmur)) WITHOUT ROWID;",
                    table));
                SQLite::Statement insert(other, std::format("INSERT INTO {} VALUES(?,?,?,999,0)", table));
                insert.bind(1, static_cast<int64_t>(hashes.lookup3));
                insert.bind(2, std::bit_cast<int64_t>(hashes.murmur));
                insert.bind(3, "other-game.mesh");
                insert.exec();
            }
            check(db.get_file_name(hashes.lookup3, Type::Lookup3) == name);
            check(db.get_file_parent(hashes.lookup3, Type::Lookup3) == high);
            check(!db.get_file_size(999999));
            db.kv_put({42,high}, "high"); db.kv_put({42,high-1}, "collision");
            check(db.kv_get(high, Type::Murmur) == "high");
            check(db.kv_get(high-1, Type::Murmur) == "collision");
            bool rejected = false;
            try { db.kv_put({42,high}, "updated"); } catch (const std::exception &) { rejected = true; }
            check(rejected && db.kv_get(high, Type::Murmur) == "high");
            db.kv_del(high, Type::Murmur);
            check(!db.kv_has(high, Type::Murmur) && db.kv_has(high-1, Type::Murmur));
            db.files_put({42,high}, "file", 321, high-1);
            auto f = db.get_file(high, Type::Murmur);
            check(f && f->lookup3==42 && f->murmur==high && f->parent_hash==high-1);
            db.files_del(high, Type::Murmur); check(!db.get_file(high, Type::Murmur));
            const std::string first = "models/a01/environment/vegstone/a01_env_vegstone_s_07_lod3.rbn";
            const std::string second = "locations/a01/activities/scarecrow/sc1190/a01_sc1190_brp.stringlookup";
            const auto a = string_hashes(first), b = string_hashes(second);
            check(a.lookup3 == 0x39C01B02 && b.lookup3 == a.lookup3);
            check(a.murmur != 0 && b.murmur != 0 && a.murmur != b.murmur);
            db.kv_put(a, first);
            db.kv_put(b, second);
            check(db.kv_get(a.murmur, Type::Murmur) == first);
            check(db.kv_get(b.murmur, Type::Murmur) == second);
            check(!db.kv_get(a.lookup3, Type::Lookup3));
            check(db.kv_has(a.lookup3, Type::Lookup3));
            db.files_put({a.lookup3, 0}, "", 17, 0);
            db.files_put(a, first, 23, 0);
            db.files_put(b, second, 29, 0);
            check(db.get_file(a.murmur, Type::Murmur)->name == first);
            check(db.get_file(b.murmur, Type::Murmur)->name == second);
            check(!db.get_file(a.lookup3, Type::Lookup3));
            check(!db.get_file_name(a.lookup3, Type::Lookup3));
            rejected = false;
            try { db.files_put(a, second, 23, 0); } catch (const std::exception &) { rejected = true; }
            check(rejected && db.get_file(a.murmur, Type::Murmur)->name == first);
            SQLite::Database raw((root/"new.db").string(), SQLite::OPEN_READONLY);
            SQLite::Statement rows(raw, std::format(
                "SELECT count(*) FROM {} WHERE lookup3=? AND murmur=0 AND name=''", game_files_table()));
            rows.bind(1, static_cast<int64_t>(a.lookup3));
            check(rows.executeStep() && rows.getColumn(0).getInt() == 1);
        }
        {
            AssetDB db(root / "new.db"); check(db.kv_get(hashes.lookup3, Type::Lookup3)==name);
        }
        {
            SQLite::Database db((root/"dual0.db").string(), SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
            db.exec("CREATE TABLE kv(lookup3 INTEGER NOT NULL,murmur INTEGER NOT NULL,v TEXT NOT NULL,"
                    "PRIMARY KEY(lookup3,murmur)) WITHOUT ROWID;"
                    "CREATE TABLE files(lookup3 INTEGER NOT NULL,murmur INTEGER NOT NULL,name TEXT,"
                    "size INTEGER NOT NULL,parent INTEGER NOT NULL,PRIMARY KEY(lookup3,murmur)) WITHOUT ROWID;"
                    "INSERT INTO files VALUES(305419896,0,'',11,0);");
            SQLite::Statement kv(db, "INSERT INTO kv VALUES(?,0,?)");
            kv.bind(1, static_cast<int64_t>(hashes.lookup3));
            kv.bind(2, name);
            kv.exec();
            SQLite::Statement file(db, "INSERT INTO files VALUES(?,0,?,123,0)");
            file.bind(1, static_cast<int64_t>(hashes.lookup3));
            file.bind(2, name);
            file.exec();
        }
        for (int i=0;i<2;++i) {
            AssetDB db(root/"dual0.db");
            check(db.kv_get(hashes.murmur, Type::Murmur) == name);
            check(db.get_file_name(hashes.murmur, Type::Murmur) == name);
            const auto unknown = db.get_file(0x12345678, Type::Lookup3);
            check(unknown && unknown->murmur == 0 && unknown->name.empty());
        }
        const uint64 game_hash = legacy_hash_domain() == LegacyHashDomain::Murmur ? hashes.murmur : hashes.lookup3;
        {
            SQLite::Database db((root/"old.db").string(), SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
            db.exec("CREATE TABLE kv(k INTEGER PRIMARY KEY,v TEXT NOT NULL) WITHOUT ROWID;"
                    "CREATE TABLE files(hash INTEGER PRIMARY KEY,name TEXT,size INTEGER NOT NULL,parent INTEGER NOT NULL DEFAULT 0) WITHOUT ROWID;"
                    "CREATE INDEX idx_files_parent ON files(parent);");
            SQLite::Statement kv(db,"INSERT INTO kv VALUES(?,?)");
            kv.bind(1,std::bit_cast<int64>(game_hash)); kv.bind(2,name); kv.exec();
            SQLite::Statement file(db,"INSERT INTO files VALUES(?,?,123,?)");
            file.bind(1,std::bit_cast<int64>(game_hash)); file.bind(2,name);
            file.bind(3,std::bit_cast<int64>(high)); file.exec();
        }
        for (int i=0;i<2;++i) {
            AssetDB db(root/"old.db");
            check(db.kv_get(game_hash)==name);
            check(db.kv_get(hashes.lookup3,Type::Lookup3)==name);
            auto f=db.get_file(game_hash);
            check(f && f->lookup3==hashes.lookup3 && f->murmur==hashes.murmur && f->parent_hash==high);
            std::vector<std::string> results; db.files_search("Animation%",results);
            check(results==std::vector<std::string>{name});
        }
        std::filesystem::remove_all(root);
        std::cout << "Dual-hash database tests passed for "
                  << (legacy_hash_domain() == LegacyHashDomain::Murmur ? "Murmur" : "Lookup3") << '\n';
    } catch (const std::exception &e) {
        std::cerr << e.what() << " (fixtures: " << root << ")\n"; return 1;
    }
}
