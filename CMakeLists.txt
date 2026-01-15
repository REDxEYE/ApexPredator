cmake_minimum_required(VERSION 3.28.3)
project(ApexPredator C)
include(FetchContent)

FetchContent_Declare(sqlite3 GIT_REPOSITORY https://github.com/sjinks/sqlite3-cmake GIT_TAG v3.49.1)
FetchContent_MakeAvailable(sqlite3)

set(CMAKE_C_STANDARD 17)
SET(ZSTD_BUILD_PROGRAMS OFF)
SET(ZSTD_BUILD_CONTRIB OFF)
add_subdirectory(external/zstd/build/cmake)
SET(ZLIB_BUILD_TESTING OFF)
SET(ZLIB_BUILD_SHARED OFF)
SET(ZLIB_INSTALL OFF)
add_subdirectory(external/zlib)
add_subdirectory(external/cglm)
SET(ZLIB_ALIASES OFF)
SET(ZLIB_BUILD_TESTING OFF)
add_subdirectory(external/tinycpng)

SET(CommonFiles
        src/platform/archive.c
        src/platform/archive_manager.c
        src/utils/string.c
        src/utils/buffer/file_buffer.c
        src/utils/buffer/buffer.c
        src/utils/lookup3.c
        src/utils/dynamic_array.c
        src/utils/json.c
        src/apex/package/tab_archive.c
        src/utils/buffer/memory_buffer.c
        src/apex/adf/adf.c
        src/apex/adf/sti.c
        src/apex/adf/sti_codegen.c
        src/utils/dynamic_insert_only_map.c
        src/apex/adf/sti_shared.c
        src/utils/path.c
        src/utils/stb.c
        src/utils/ply_writer.c
        src/utils/common.c
        src/apex/adf/adf_types.c
        src/utils/gltf/cgltf_helper.c
        src/apex/avtx.c
        src/platform/texture.c
        src/apex/aaf/aaf.c
        src/utils/zlib_wrapper.c
        src/apex/sarc.c
        src/apex/rtpc.c
        src/apex/hashes.c
        src/havok/tag_file/havok_tag_file.c
        src/havok/havok_generated.c
        src/havok/tag_file/havok_tag_types.c
        src/havok/havok_codegen.c
        src/havok/havok_support_types.c
        src/utils/hash_helper.c
        src/exporter/amf_export.c
        src/exporter/common_export.c
        src/exporter/adf_export.c
        src/exporter/ddsc_export.c
        src/exporter/epe_export.c
        src/exporter/havok_export.c
        src/platform/texture_ops.c
        src/utils/sqlite_wrapper.c
)

add_executable(ApexPredator
        src/main.c
        ${CommonFiles}
)

add_executable(AdfTypeGenerator
        src/adf_type_gen.c
        ${CommonFiles}
)

add_executable(HavokTypeGenerator
        src/havok_type_gen.c
        ${CommonFiles}
)

add_executable(HashCollector
        src/hash_collector.c
        ${CommonFiles}
)

add_executable(StringHasher
        src/string_hasher.c
        src/utils/lookup3.c
)

add_executable(HashBruteforce
        src/hash_bruteforce.c
        src/utils/lookup3.c
)

if (UNIX)
    target_compile_options(ApexPredator PRIVATE -fms-extensions)
    target_compile_options(AdfTypeGenerator PRIVATE -fms-extensions)
    target_compile_options(HavokTypeGenerator PRIVATE -fms-extensions)
    target_compile_options(HashCollector PRIVATE -fms-extensions)
    target_link_libraries(ApexPredator PUBLIC m)
else ()
    target_compile_options(ApexPredator PRIVATE /bigobj)
    target_compile_options(HashCollector PRIVATE /bigobj)
endif ()
target_include_directories(ApexPredator PUBLIC include external/cgltf external/zlib external/tinycpng/include SQLite::SQLite3)
target_include_directories(AdfTypeGenerator PUBLIC include external/zlib external/cgltf)
target_include_directories(HavokTypeGenerator PUBLIC include external/zlib external/cgltf)
target_include_directories(HashCollector PUBLIC include external/zlib external/cgltf external/tinycpng/include)
target_include_directories(StringHasher PUBLIC include)
target_include_directories(HashBruteforce PUBLIC include)

target_link_libraries(ApexPredator PUBLIC libzstd zlibstatic cglm tinycpng-static SQLite::SQLite3)
target_link_libraries(HashCollector PUBLIC libzstd zlibstatic cglm tinycpng-static SQLite::SQLite3)
target_link_libraries(AdfTypeGenerator PUBLIC libzstd zlibstatic cglm tinycpng-static SQLite::SQLite3)
target_link_libraries(HavokTypeGenerator PUBLIC libzstd zlibstatic cglm tinycpng-static SQLite::SQLite3)
