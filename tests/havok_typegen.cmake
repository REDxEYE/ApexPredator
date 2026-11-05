add_executable(ApexHavokTypegenTests tests/havok_typegen.cpp
    src/havok/havok_codegen.cpp src/havok/havok_types.cpp
    src/havok/tag_file/havok_tag_types.cpp src/havok/tag_file/havok_tag_file.cpp
    src/utils/hash_helper.cpp src/utils/lookup3.cpp src/utils/murmur3.cpp
    src/modules/lookup3_hash_policy.cpp)
target_include_directories(ApexHavokTypegenTests PRIVATE include)
target_link_libraries(ApexHavokTypegenTests PRIVATE RedsCore nlohmann_json::nlohmann_json)
add_test(NAME Apex.HavokTypegen COMMAND ApexHavokTypegenTests "${CMAKE_CURRENT_BINARY_DIR}/havok-typegen-test")
set(havok_fixture_dir "${CMAKE_CURRENT_BINARY_DIR}/havok-typegen-fixture")
add_custom_command(
    OUTPUT "${havok_fixture_dir}/havok_types.cpp" "${havok_fixture_dir}/havok_types_formatters.cpp"
           "${havok_fixture_dir}/havok/generated/havok_types.h"
           "${havok_fixture_dir}/havok/generated/havok_types_fwd.h"
    COMMAND $<TARGET_FILE:ApexHavokTypegenTests> "${havok_fixture_dir}"
    DEPENDS ApexHavokTypegenTests VERBATIM)

add_executable(ApexGeneratorKeywordRuntimeTests tests/generator_keyword_runtime.cpp
    src/apex/adf/adf_read_instance.cpp
    src/havok/tag_file/havok_tag_types.cpp
    src/havok/tag_file/havok_tag_file.cpp
    src/havok/tag_file/havok_tag_file_get_item.cpp
    $<TARGET_OBJECTS:ApexAdfTypegenFixture> $<TARGET_OBJECTS:ApexHavokTypegenFixture>)
target_include_directories(ApexGeneratorKeywordRuntimeTests PRIVATE
    "${adf_fixture_dir}" "${havok_fixture_dir}" include)
target_link_libraries(ApexGeneratorKeywordRuntimeTests PRIVATE RedsCore nlohmann_json::nlohmann_json)
add_test(NAME Apex.GeneratorKeywords COMMAND ApexGeneratorKeywordRuntimeTests)
add_library(ApexHavokTypegenFixture OBJECT
    "${havok_fixture_dir}/havok_types.cpp" "${havok_fixture_dir}/havok_types_formatters.cpp")
target_include_directories(ApexHavokTypegenFixture PRIVATE "${havok_fixture_dir}" include)
target_link_libraries(ApexHavokTypegenFixture PRIVATE RedsCore nlohmann_json::nlohmann_json)
