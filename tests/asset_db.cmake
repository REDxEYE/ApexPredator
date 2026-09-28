foreach(game IN ITEMS 0 1)
    add_executable(ApexAssetDbTests${game} tests/asset_db.cpp src/apex/asset_db.cpp
            src/utils/hash_helper.cpp src/utils/lookup3.cpp src/utils/murmur3.cpp)
    target_include_directories(ApexAssetDbTests${game} PRIVATE include)
    target_compile_definitions(ApexAssetDbTests${game} PRIVATE GAME=${game})
    target_link_libraries(ApexAssetDbTests${game} PRIVATE RedsCore SQLiteCpp)
    add_test(NAME Apex.AssetDB${game} COMMAND ApexAssetDbTests${game})
endforeach()

add_test(NAME Apex.HashCollectors COMMAND ${Python3_EXECUTABLE}
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_hash_collectors.py"
    $<TARGET_FILE:GenerationZeroHashCollector> $<TARGET_FILE:Rage2HashCollector>)
