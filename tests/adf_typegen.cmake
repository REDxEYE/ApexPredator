add_executable(ApexAdfTypegenTests tests/adf_typegen.cpp
    src/apex/adf/adf.cpp src/apex/adf/sti.cpp src/apex/adf/sti_codegen.cpp)
target_include_directories(ApexAdfTypegenTests PRIVATE include)
target_link_libraries(ApexAdfTypegenTests PRIVATE RedsCore nlohmann_json::nlohmann_json)
add_test(NAME Apex.AdfTypegen COMMAND ApexAdfTypegenTests "${CMAKE_CURRENT_BINARY_DIR}/adf-typegen-test")

# Compile the generated definitions, readers, factories and enum formatters too.
set(adf_fixture_dir "${CMAKE_CURRENT_BINARY_DIR}/adf-typegen-fixture")
add_custom_command(
    OUTPUT "${adf_fixture_dir}/adf_types.cpp" "${adf_fixture_dir}/adf_types_formatters.cpp"
           "${adf_fixture_dir}/apex/adf/generated/adf_types.h"
           "${adf_fixture_dir}/apex/adf/generated/adf_types_fwd.h"
    COMMAND $<TARGET_FILE:ApexAdfTypegenTests> "${adf_fixture_dir}"
    DEPENDS ApexAdfTypegenTests
    VERBATIM)
add_library(ApexAdfTypegenFixture OBJECT
    "${adf_fixture_dir}/adf_types.cpp" "${adf_fixture_dir}/adf_types_formatters.cpp")
target_include_directories(ApexAdfTypegenFixture PRIVATE "${adf_fixture_dir}" include)
target_link_libraries(ApexAdfTypegenFixture PRIVATE RedsCore nlohmann_json::nlohmann_json)
