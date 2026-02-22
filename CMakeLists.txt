cmake_minimum_required(VERSION 3.20)
project(ApexPredator CXX)
include(FetchContent)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
FetchContent_Declare(sqlite3 GIT_REPOSITORY https://github.com/sjinks/sqlite3-cmake GIT_TAG v3.49.1)
FetchContent_MakeAvailable(sqlite3)

FetchContent_Declare(
        tracy
        GIT_REPOSITORY https://github.com/wolfpld/tracy.git
        GIT_TAG master
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)

set(TRACY_ONLY_LOCALHOST ON CACHE BOOL "" FORCE)
set(TRACY_NO_FRAME_IMAGE ON CACHE BOOL "" FORCE)
set(TRACY_NO_VSYNC_CAPTURE ON CACHE BOOL "" FORCE)
set(TRACY_IGNORE_MEMORY_FAULTS ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(tracy)

FetchContent_Declare(zlib-ng GIT_REPOSITORY https://github.com/zlib-ng/zlib-ng GIT_TAG 2.3.2)
set(WITH_GTEST OFF CACHE BOOL "Disable building tests for zlib-ng")
set(WITH_FUZZERS OFF CACHE BOOL "Disable building examples for zlib-ng")
set(WITH_BENCHMARKS OFF CACHE BOOL "Disable building benchmarks for zlib-ng")
set(WITH_BENCHMARK_APPS OFF CACHE BOOL "Disable building benchmarks-apps for zlib-ng")
set(ZLIB_COMPAT OFF CACHE BOOL "Disable zlib-ng zlib compatibility layer")
set(ZLIBNG_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(ZLIBNG_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(ZLIBNG_ENABLE_PACKAGE_CONFIG OFF CACHE BOOL "" FORCE)
set(ZLIB_ENABLE_OPTIMIZATIONS ON CACHE BOOL "Enable zlib-ng optimizations")
FetchContent_MakeAvailable(zlib-ng)

FetchContent_Declare(
        zstd
        GIT_REPOSITORY "https://github.com/facebook/zstd"
        GIT_TAG v1.5.7
        SOURCE_SUBDIR build/cmake
)

SET(ZSTD_BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
SET(ZSTD_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(zstd)

FetchContent_Declare(
        cglm
        GIT_REPOSITORY "https://github.com/recp/cglm"
        GIT_TAG v0.9.6
)
FetchContent_MakeAvailable(cglm)

FetchContent_Declare(
        ogg
        GIT_REPOSITORY "https://github.com/xiph/ogg"
        GIT_TAG v1.3.6
)
FetchContent_MakeAvailable(ogg)

set(OGG_FOUND TRUE CACHE BOOL "" FORCE)
set(OGG_INCLUDE_DIR "${ogg_SOURCE_DIR}/include" CACHE PATH "" FORCE)
set(OGG_LIBRARY ogg CACHE STRING "" FORCE) # note: target name is OK here

FetchContent_Declare(
        vorbis
        GIT_REPOSITORY "https://github.com/xiph/vorbis"
        GIT_TAG v1.3.7
)
FetchContent_MakeAvailable(vorbis)


SET(ZLIB_ALIASES OFF CACHE BOOL "" FORCE)
SET(ZLIB_BUILD_TESTING OFF CACHE BOOL "" FORCE)
add_subdirectory(external/tinycpng)

add_library(AdfLib STATIC
        src/utils/memory_profiler.cpp
        src/utils/memory_tracker.cpp
        src/apex/adf/adf_support_types.cpp
        src/apex/adf/adf_types.cpp
)

add_library(HavokLib STATIC
        src/havok/animations/animation.cpp
        src/havok/animations/spline.cpp
        src/havok/animations/track.cpp
        src/havok/generated/havok_generated.cpp
        src/havok/havok_helpers.cpp
        src/utils/memory_profiler.cpp
        src/utils/memory_tracker.cpp
)

SET(CommonFiles
        src/apex/aaf/aaf.cpp
        src/apex/adf/adf.cpp
        src/apex/adf/sti.cpp
        src/apex/adf/sti_shared.cpp
        src/apex/avtx.cpp
        src/apex/hashes.cpp
        src/apex/package/tab_archive.cpp
        src/apex/rtpc.cpp
        src/apex/sarc.cpp
        src/havok/havok_types.cpp
        src/havok/tag_file/havok_tag_file.cpp
        src/havok/tag_file/havok_tag_types.cpp
        src/platform/app_state.cpp
        src/platform/archive.cpp
        src/platform/archive_manager.cpp
        src/platform/cli_parser.cpp
        src/platform/logger.cpp
        src/platform/texture.cpp
        src/platform/texture_ops.cpp
        src/utils/buffer/buffer.cpp
        src/utils/buffer/file_buffer.cpp
        src/utils/buffer/memory_buffer.cpp
        src/utils/common.cpp
        src/utils/dynamic_array.cpp
        src/utils/dynamic_map.cpp
        src/utils/gltf/cgltf_helper.cpp
        src/utils/hash_helper.cpp
        src/utils/json.cpp
        src/utils/lookup3.cpp
        src/utils/memory_profiler.cpp
        src/utils/memory_tracker.cpp
        src/utils/path.cpp
        src/utils/simple_fileio.cpp
        src/utils/sqlite_wrapper.cpp
        src/utils/stb.cpp
        src/utils/string.cpp
        src/utils/string_view.cpp
        src/utils/zlib_wrapper.cpp
)

add_executable(ApexPredator
        src/tools/apex_predator/apex_predator.cpp
        src/exporter/adf_export.cpp
        src/exporter/amf_export.cpp
        src/exporter/common_export.cpp
        src/exporter/ddsc_export.cpp
        src/exporter/rtpc_export.cpp
        src/exporter/havok_export.cpp
        src/exporter/fmod_export.cpp

        ${CommonFiles}
        src/tools/apex_predator/commands/extract.cpp
        src/tools/apex_predator/commands/extract_anim.cpp
        src/tools/apex_predator/commands/search.cpp
        src/tools/apex_predator/commands/convert.cpp

)

add_executable(AdfTypeGenerator
        src/tools/adf_type_gen.cpp
        src/apex/adf/sti_codegen.cpp
        src/utils/memory_tracker.cpp
        ${CommonFiles}
)

add_executable(HavokTypeGenerator
        src/tools/havok_type_gen.cpp
        src/havok/havok_codegen.cpp
        ${CommonFiles}
)

add_executable(HashCollector
        src/tools/hash_collector.cpp
        ${CommonFiles}
)

add_executable(StringHasher
        src/tools/string_hasher.cpp
        src/utils/lookup3.cpp
)


if (UNIX)
    target_compile_options(AdfLib PRIVATE -fms-extensions)
    target_compile_options(HavokLib PRIVATE -fms-extensions)
    target_compile_options(ApexPredator PRIVATE -fms-extensions)
    target_compile_options(AdfTypeGenerator PRIVATE -fms-extensions)
    target_compile_options(HavokTypeGenerator PRIVATE -fms-extensions)
    target_compile_options(HashCollector PRIVATE -fms-extensions)

    target_link_libraries(ApexPredator PUBLIC m)
    target_link_libraries(AdfTypeGenerator PUBLIC m)
    target_link_libraries(HavokTypeGenerator PUBLIC m)
    target_link_libraries(HashCollector PUBLIC m)
else ()
    target_compile_options(AdfLib PRIVATE /bigobj)
    target_compile_options(HavokLib PRIVATE /bigobj)
    target_compile_options(ApexPredator PRIVATE /bigobj)
    target_compile_options(HashCollector PRIVATE /bigobj)
endif ()

SET(CommonLibs
        libzstd_static
        zlib-ng-static
        cglm
        tinycpng-static
        SQLite::SQLite3
)

SET(CommonIncludes
        external/cgltf
        external/tinycpng/include
)

target_include_directories(AdfLib PUBLIC include)
target_include_directories(HavokLib PUBLIC include)
target_include_directories(ApexPredator PUBLIC include ${CommonIncludes})
target_include_directories(AdfTypeGenerator PUBLIC include ${CommonIncludes})
target_include_directories(HavokTypeGenerator PUBLIC include ${CommonIncludes})
target_include_directories(HashCollector PUBLIC include ${CommonIncludes})
target_include_directories(StringHasher PUBLIC include)

target_link_libraries(ApexPredator PUBLIC HavokLib AdfLib Ogg::ogg vorbis vorbisenc ${CommonLibs})
target_link_libraries(HashCollector PUBLIC AdfLib ${CommonLibs})
target_link_libraries(AdfTypeGenerator PUBLIC ${CommonLibs})
target_link_libraries(HavokTypeGenerator PUBLIC ${CommonLibs})
target_link_libraries(HavokLib PUBLIC cglm)
target_link_libraries(AdfLib PUBLIC cglm)

target_link_libraries(AdfLib PUBLIC Tracy::TracyClient)
target_link_libraries(HavokLib PUBLIC Tracy::TracyClient)
target_link_libraries(ApexPredator PUBLIC Tracy::TracyClient)
target_link_libraries(HashCollector PUBLIC Tracy::TracyClient)
target_link_libraries(AdfTypeGenerator PUBLIC Tracy::TracyClient)
target_link_libraries(HavokTypeGenerator PUBLIC Tracy::TracyClient)
target_link_libraries(StringHasher PUBLIC Tracy::TracyClient)


target_compile_definitions(ApexPredator PRIVATE TRACY_ENABLE TRACY_MEMORY TRACY_CALLSTACK=62)
# target_compile_definitions(AdfTypeGenerator PRIVATE TRACY_ENABLE TRACY_MEMORY TRACY_CALLSTACK=62)

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