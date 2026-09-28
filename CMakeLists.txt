cmake_minimum_required(VERSION 3.20)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
project(ApexPredator CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

find_program(SCCACHE_PROGRAM sccache)

if(SCCACHE_PROGRAM)
    message(STATUS "Found sccache: ${SCCACHE_PROGRAM}")
    set(CMAKE_C_COMPILER_LAUNCHER "${SCCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${SCCACHE_PROGRAM}")
else()
    message(STATUS "sccache not found; compiler cache disabled")
endif()

if(UNIX AND NOT APPLE)
    find_program(MOLD_LINKER mold)
    find_program(LLD_LINKER ld.lld)

    if(MOLD_LINKER)
        message(STATUS "Found mold linker: ${MOLD_LINKER}")
        add_link_options(-fuse-ld=mold)
    elseif(LLD_LINKER)
        message(STATUS "Found lld linker: ${LLD_LINKER}")
        add_link_options(-fuse-ld=lld)
    else()
        message(STATUS "Using default system linker")
    endif()

    add_compile_options(
            "$<$<CONFIG:Debug>:-fdebug-types-section>"
    )

endif()

# Static dependencies linked into game modules must be position-independent.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
include(cmake/dependencies.cmake)
add_subdirectory(external/ooz)
find_package(Threads REQUIRED)
set(GAME_MODULE_OUTPUT_DIR "${CMAKE_BINARY_DIR}/modules")
if(CMAKE_CONFIGURATION_TYPES)
    set(GAME_MODULE_OUTPUT_DIR "${CMAKE_BINARY_DIR}/$<CONFIG>/modules")
endif()
include(modules/generation_zero/module.cmake)
include(modules/rage2/module.cmake)

# The host contains only CLI and module-loading code. Game code is private to the module.
add_executable(ApexPredator
        src/tools/apex_predator/apex_predator.cpp
        src/modules/module_loader.cpp
        src/tools/apex_predator/commands/extract.cpp
        src/tools/apex_predator/commands/extract_anim.cpp
        src/tools/apex_predator/commands/search.cpp
        src/tools/apex_predator/commands/modules.cpp
        src/tools/apex_predator/commands/export_everything.cpp
)

add_dependencies(ApexPredator GenerationZeroModule Rage2Module)

add_executable(StringHasher
        src/tools/string_hasher.cpp
        src/utils/lookup3.cpp
)

if (UNIX)
    target_link_libraries(ApexPredator PUBLIC m)
elseif (MSVC)
    target_compile_options(ApexPredator PRIVATE /bigobj)

endif ()

SET(CommonLibs
        RedsCore
        SQLiteCpp
        OpenXLSX::OpenXLSX
)


target_include_directories(ApexPredator PUBLIC include)
target_include_directories(StringHasher PUBLIC include)

target_link_libraries(ApexPredator PRIVATE ${CMAKE_DL_LIBS})

add_compile_definitions(GLM_ENABLE_EXPERIMENTAL TRACY_CALLSTACK=32)

find_package(Threads REQUIRED)
target_link_libraries(ApexPredator PRIVATE CLI11::CLI11 Threads::Threads RedsCore)


if (WIN32)

    FetchContent_Declare(
            minhook
            GIT_REPOSITORY "https://github.com/TsudaKageyu/minhook"
            GIT_TAG v1.3.4
    )

    FetchContent_MakeAvailable(minhook)

    add_library(builtinAdfDumper SHARED
            src/tools/builtin_adf_dumper/builtinAdfDumper.cpp
            src/tools/builtin_adf_dumper/exports.cpp
            src/tools/builtin_adf_dumper/patterns.cpp
    )
    target_link_libraries(builtinAdfDumper PRIVATE minhook)
    target_link_directories(builtinAdfDumper PRIVATE external/detours/src)
    #set_target_properties(builtinAdfDumper PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")


    set(GAME_PATH "C:\\Program Files (x86)\\Steam\\steamapps\\common\\GenerationZero")
    add_custom_command(TARGET builtinAdfDumper POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            $<TARGET_FILE:builtinAdfDumper>
            "\"${GAME_PATH}\\winhttp.dll\""
    )
endif ()

find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_custom_target(CompressDatabases
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/package_databases.py"
                "${CMAKE_COMMAND}" "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Snapshot and compress hashes.db and rage2_hashes.db"
        VERBATIM
)

option(APEX_BUILD_MODULE_TESTS "Build fixture modules and module integration tests" OFF)
if(APEX_BUILD_MODULE_TESTS)
    enable_testing()
    include(tests/rtpc.cmake)
    include(tests/gtoc.cmake)
    include(tests/adf_typegen.cmake)
    include(tests/havok_typegen.cmake)
    add_executable(ApexArchiveManagerTests tests/archive_manager.cpp)
    target_link_libraries(ApexArchiveManagerTests PRIVATE RedsCore)
    add_test(NAME Apex.ArchiveManager COMMAND ApexArchiveManagerTests)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_test(NAME Apex.ArchiveEviction COMMAND ${Python3_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_archive_eviction.py" $<TARGET_FILE:GenerationZeroHashCollector>)
    include(tests/asset_db.cmake)
    foreach(fixture IN ITEMS one two bad)
        add_library(MockGame_${fixture} MODULE tests/mock_game_module.cpp)
        target_include_directories(MockGame_${fixture} PRIVATE include)
        target_link_libraries(MockGame_${fixture} PRIVATE RedsCore)
        target_compile_definitions(MockGame_${fixture} PRIVATE APEX_BUILD_GAME_MODULE FIXTURE_ID="fixture-${fixture}")
        set_target_properties(MockGame_${fixture} PROPERTIES PREFIX "" LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test-modules" RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test-modules")
    endforeach()
    target_compile_definitions(MockGame_bad PRIVATE FIXTURE_BAD_ABI)
    add_executable(ApexModuleSessionTests tests/module_sessions.cpp src/modules/module_loader.cpp)
    target_include_directories(ApexModuleSessionTests PRIVATE include)
    target_link_libraries(ApexModuleSessionTests PRIVATE ${CMAKE_DL_LIBS} Threads::Threads RedsCore)
    add_custom_target(ApexModuleFixtures DEPENDS MockGame_one MockGame_two MockGame_bad ApexModuleSessionTests)
    add_test(NAME Apex.GameModules COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_game_modules.py"
            $<TARGET_FILE:ApexPredator> $<TARGET_FILE:MockGame_one> $<TARGET_FILE:MockGame_two> $<TARGET_FILE:MockGame_bad>)
    add_test(NAME Apex.TabV31 COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_tab_v31.py" $<TARGET_FILE:ApexPredator>)
    add_test(NAME Apex.Rage2Module COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_rage2_module.py"
            $<TARGET_FILE:ApexPredator> $<TARGET_FILE:Rage2Module>)
    add_test(NAME Apex.Rage2AMF COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_rage2_amf_cli.py"
            $<TARGET_FILE:ApexPredator>)
    add_test(NAME Apex.VirtualModelCLI COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_virtual_model_cli.py" $<TARGET_FILE:ApexPredator> --module-library $<TARGET_FILE:GenerationZeroModule> --session-runner $<TARGET_FILE:ApexModuleSessionTests>)
endif()
