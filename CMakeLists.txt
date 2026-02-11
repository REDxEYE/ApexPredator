cmake_minimum_required(VERSION 3.20)
project(ApexPredator C CXX)
include(FetchContent)
set(CMAKE_C_STANDARD 17)
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
#
#if(TARGET ogg AND NOT TARGET Ogg::ogg)
#    add_library(Ogg::ogg ALIAS ogg)
#endif()

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
        src/apex/adf/adf_types.c
        src/utils/memory_profiler.c
        src/utils/memory_tracker.c
        src/apex/adf/adf_support_types.c
)

add_library(HavokLib STATIC
        src/havok/animations/animation.c
        src/havok/animations/spline.c
        src/havok/animations/track.c
        src/havok/generated/havok_generated.c
        src/havok/havok_helpers.c
        include/havok/havok_support_types_functions.h
        src/utils/memory_profiler.c
        src/utils/memory_tracker.c
)

SET(CommonFiles
        src/apex/aaf/aaf.c
        src/apex/adf/adf.c
        src/apex/adf/sti.c
        src/apex/adf/sti_shared.c
        src/apex/avtx.c
        src/apex/hashes.c
        src/apex/package/tab_archive.c
        src/apex/rtpc.c
        src/apex/sarc.c
        src/havok/tag_file/havok_tag_file.c
        src/havok/tag_file/havok_tag_types.c
        src/havok/havok_types.c
        src/platform/app_state.c
        src/platform/archive.c
        src/platform/archive_manager.c
        src/platform/cli_parser.c
        src/platform/logger.c
        src/platform/texture.c
        src/platform/texture_ops.c
        src/utils/buffer/buffer.c
        src/utils/buffer/file_buffer.c
        src/utils/buffer/memory_buffer.c
        src/utils/common.c
        src/utils/dynamic_array.c
        src/utils/dynamic_map.c
        src/utils/gltf/cgltf_helper.c
        src/utils/hash_helper.c
        src/utils/json.c
        src/utils/lookup3.c
        src/utils/memory_profiler.c
        src/utils/memory_tracker.c
        src/utils/path.c
        src/utils/ply_writer.c
        src/utils/sqlite_wrapper.c
        src/utils/stb.c
        src/utils/string.c
        src/utils/zlib_wrapper.c
        src/utils/string_view.c
        include/utils/string_view.h
)

add_executable(ApexPredator
        src/tools/apex_predator.c
        src/exporter/adf_export.c
        src/exporter/amf_export.c
        src/exporter/common_export.c
        src/exporter/ddsc_export.c
        src/exporter/epe_export.c
        src/exporter/havok_export.c
        ${CommonFiles}
        src/exporter/export_fmod.c
        include/exporter/export_fmod.h
)

add_executable(AdfTypeGenerator
        src/tools/adf_type_gen.c
        src/apex/adf/sti_codegen.c
        src/utils/memory_tracker.c
        ${CommonFiles}
)

add_executable(HavokTypeGenerator
        src/tools/havok_type_gen.c
        src/havok/havok_codegen.c
        ${CommonFiles}
)

add_executable(HashCollector
        src/tools/hash_collector.c
        ${CommonFiles}
)

add_executable(StringHasher
        src/tools/string_hasher.c
        src/utils/lookup3.c
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
