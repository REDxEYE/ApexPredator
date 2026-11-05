foreach(game IN ITEMS 0 1 2 3 4)
    if(game EQUAL 1)
        set(hash_policy src/modules/murmur_hash_policy.cpp)
    elseif(game EQUAL 2)
        set(hash_policy modules/second_extinction/hash_policy.cpp)
    elseif(game EQUAL 3)
        set(hash_policy modules/just_cause_2/hash_policy.cpp)
    elseif(game EQUAL 4)
        set(hash_policy modules/mad_max/hash_policy.cpp)
    else()
        set(hash_policy src/modules/lookup3_hash_policy.cpp)
    endif()
    add_executable(ApexAssetDbTests${game} tests/asset_db.cpp src/apex/asset_db.cpp
            src/utils/hash_helper.cpp src/utils/lookup3.cpp src/utils/murmur3.cpp ${hash_policy})
    target_include_directories(ApexAssetDbTests${game} PRIVATE include)
    target_link_libraries(ApexAssetDbTests${game} PRIVATE RedsCore SQLiteCpp)
    add_test(NAME Apex.AssetDB${game} COMMAND ApexAssetDbTests${game})
endforeach()

add_test(NAME Apex.HashCollectors COMMAND ${Python3_EXECUTABLE}
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_hash_collectors.py"
    $<TARGET_FILE:GenerationZeroHashCollector> $<TARGET_FILE:Rage2HashCollector>)
add_test(NAME Apex.SecondExtinctionLocations COMMAND ${Python3_EXECUTABLE}
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_second_extinction_locations.py"
    $<TARGET_FILE:SecondExtinctionHashCollector>)
add_test(NAME Apex.ExecutableStrings COMMAND ${Python3_EXECUTABLE}
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_exe_strings.py")
