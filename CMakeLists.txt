cmake_minimum_required(VERSION 3.28.3)
project(ApexPredator C)

set(CMAKE_C_STANDARD 17)

add_subdirectory(external/zstd/build/cmake)
add_subdirectory(external/zlib)

add_executable(ApexPredator
        src/main.c
        src/utils/string.c
        src/utils/buffer/file_buffer.c
        src/utils/buffer/buffer.c
        src/utils/lookup3.c
        src/utils/dynamic_array.c
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
        src/platform/archive.c
        src/platform/archive_manager.c
        src/apex/rtpc.c
        include/apex/rtpc.h
        src/apex/hashes.c
        include/apex/hashes.h
        src/havok/tag_file/havok_tag_file.c
        src/havok/havok_generated.c
        src/havok/tag_file/havok_tag_types.c
)

add_executable(AdfTypeGenerator
        src/adf_type_gen.c
        src/utils/string.c
        src/utils/buffer/file_buffer.c
        src/utils/buffer/buffer.c
        src/utils/lookup3.c
        src/utils/dynamic_array.c
        src/apex/package/tab_archive.c
        src/utils/buffer/memory_buffer.c
        src/apex/adf/adf.c
        src/apex/adf/sti.c
        src/apex/adf/sti_codegen.c
        src/utils/dynamic_insert_only_map.c
        src/utils/path.c
        src/utils/common.c
        src/platform/archive.c
        src/platform/archive_manager.c
        src/apex/aaf/aaf.c
        src/utils/zlib_wrapper.c
        src/apex/sarc.c
        src/apex/hashes.c
        include/apex/hashes.h
)

add_executable(HavokTypeGenerator
        src/havok_type_gen.c
        src/apex/aaf/aaf.c
        src/platform/archive.c
        src/platform/archive_manager.c
        src/apex/package/tab_archive.c
        src/apex/sarc.c
        src/utils/zlib_wrapper.c
        src/utils/lookup3.c
        src/utils/string.c
        src/utils/buffer/file_buffer.c
        src/utils/buffer/buffer.c
        src/utils/dynamic_array.c
        src/utils/buffer/memory_buffer.c
        src/utils/dynamic_insert_only_map.c
        src/utils/path.c
        src/utils/common.c
        src/havok/havok_codegen.c
        src/havok/tag_file/havok_tag_file.c
        src/havok/tag_file/havok_tag_types.c
)

add_executable(HashCollector
        src/hash_collector.c
        src/utils/string.c
        src/utils/buffer/file_buffer.c
        src/utils/buffer/buffer.c
        src/utils/lookup3.c
        src/utils/dynamic_array.c
        src/apex/package/tab_archive.c
        src/utils/buffer/memory_buffer.c
        src/apex/adf/adf.c
        src/apex/adf/sti.c
        src/apex/adf/sti_codegen.c
        src/utils/dynamic_insert_only_map.c
        src/utils/path.c
        src/utils/common.c
        src/apex/hashes.c
        include/apex/hashes.h
)

add_executable(StringHasher
        src/string_hasher.c
        src/utils/lookup3.c
)

add_executable(HashBruteforce
        src/hash_bruteforce.c
        src/utils/lookup3.c
)

if(UNIX)
    target_compile_options(ApexPredator PRIVATE -fms-extensions)
    target_compile_options(AdfTypeGenerator PRIVATE -fms-extensions)
    target_compile_options(HavokTypeGenerator PRIVATE -fms-extensions)
    target_compile_options(HashCollector PRIVATE -fms-extensions)
    target_link_libraries(ApexPredator PUBLIC m)
else ()
    target_compile_options(ApexPredator PRIVATE /bigobj)
endif()
target_include_directories(ApexPredator PUBLIC include external/cgltf external/zlib)
target_include_directories(AdfTypeGenerator PUBLIC include external/zlib)
target_include_directories(HavokTypeGenerator PUBLIC include external/zlib)
target_include_directories(HashCollector PUBLIC include external/zlib)
target_include_directories(StringHasher PUBLIC include )
target_include_directories(HashBruteforce PUBLIC include )

target_link_libraries(ApexPredator PUBLIC libzstd zlibstatic)
target_link_libraries(AdfTypeGenerator PUBLIC zlibstatic)
target_link_libraries(HavokTypeGenerator PUBLIC zlibstatic)
