add_executable(ApexGtocTests tests/gtoc_reader.cpp src/apex/gtoc.cpp src/apex/asset_db.cpp
        src/utils/hash_helper.cpp src/utils/lookup3.cpp src/utils/murmur3.cpp)
target_include_directories(ApexGtocTests PRIVATE include)
target_compile_definitions(ApexGtocTests PUBLIC GAME=1)
target_link_libraries(ApexGtocTests PRIVATE RedsCore SQLiteCpp)
add_test(NAME Apex.Gtoc COMMAND ApexGtocTests)
