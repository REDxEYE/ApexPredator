//
// Created by red_eye on 9/24/26.
//

#pragma once
#include <mutex>

#include "apex/asset_db.h"



// Legacy database helpers are module-local. Serialize their use and bind the current
// session on every call; never retain a pointer to a previous session's database.
struct ActiveDatabase {
    std::unique_lock<std::mutex> lock;
    explicit ActiveDatabase(std::mutex &mutex, AssetDB &db): lock{mutex} { AssetDB::set_instance(&db); }
    ~ActiveDatabase() { AssetDB::set_instance(nullptr); }
};
