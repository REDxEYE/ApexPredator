add_executable(ApexHavokTypegenTests tests/havok_typegen.cpp
    src/havok/havok_codegen.cpp src/havok/havok_types.cpp
    src/havok/tag_file/havok_tag_types.cpp src/havok/tag_file/havok_tag_file.cpp
    src/utils/hash_helper.cpp src/utils/lookup3.cpp)
target_include_directories(ApexHavokTypegenTests PRIVATE include)
target_compile_definitions(ApexHavokTypegenTests PUBLIC GAME=0)
target_link_libraries(ApexHavokTypegenTests PRIVATE RedsCore nlohmann_json::nlohmann_json)
add_test(NAME Apex.HavokTypegen COMMAND ApexHavokTypegenTests "${CMAKE_CURRENT_BINARY_DIR}/havok-typegen-test")
set(havok_fixture_dir "${CMAKE_CURRENT_BINARY_DIR}/havok-typegen-fixture")
add_custom_command(
    OUTPUT "${havok_fixture_dir}/havok_types.cpp" "${havok_fixture_dir}/havok_types_formatters.cpp"
           "${havok_fixture_dir}/havok/generated/havok_types.h"
           "${havok_fixture_dir}/havok/generated/havok_types_fwd.h"
    COMMAND $<TARGET_FILE:ApexHavokTypegenTests> "${havok_fixture_dir}"
    DEPENDS ApexHavokTypegenTests VERBATIM)
add_library(ApexHavokTypegenFixture OBJECT
    "${havok_fixture_dir}/havok_types.cpp" "${havok_fixture_dir}/havok_types_formatters.cpp")
target_include_directories(ApexHavokTypegenFixture PRIVATE "${havok_fixture_dir}" include)
target_link_libraries(ApexHavokTypegenFixture PRIVATE RedsCore nlohmann_json::nlohmann_json)
