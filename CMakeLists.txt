cmake_minimum_required(VERSION 3.28.3)
project(ApexPredator C)

set(CMAKE_C_STANDARD 17)

add_subdirectory(external/zstd/build/cmake)

add_executable(ApexPredator
        src/main.c
        src/utils/string.c
        src/utils/buffer/file_buffer.c
        src/utils/buffer/buffer.c
        src/utils/lookup3.c
        src/utils/dynamic_array.c
        src/apex/package/archive.c
        src/utils/buffer/memory_buffer.c
        src/apex/adf/adf.c
        src/apex/adf/sti.c
        src/utils/dynamic_insert_only_map.c
        src/apex/adf/sti_shared.c
        src/utils/path.c
        src/utils/stb.c
        src/utils/ply_writer.c
        src/utils/common.c
        src/apex/adf/adf_types.c
)

add_executable(TypeGenerator
        src/type_gen.c
        src/utils/string.c
        src/utils/buffer/file_buffer.c
        src/utils/buffer/buffer.c
        src/utils/lookup3.c
        src/utils/dynamic_array.c
        src/apex/package/archive.c
        src/utils/buffer/memory_buffer.c
        src/apex/adf/adf.c
        src/apex/adf/sti.c
        src/utils/dynamic_insert_only_map.c
        src/utils/path.c
        src/utils/common.c
)
add_executable(HashCollector
        src/hash_collector.c
        src/utils/string.c
        src/utils/buffer/file_buffer.c
        src/utils/buffer/buffer.c
        src/utils/lookup3.c
        src/utils/dynamic_array.c
        src/apex/package/archive.c
        src/utils/buffer/memory_buffer.c
        src/apex/adf/adf.c
        src/apex/adf/sti.c
        src/utils/dynamic_insert_only_map.c
        src/utils/path.c
        src/utils/common.c
)

if(UNIX)
    target_compile_options(ApexPredator PRIVATE -fms-extensions)
    target_compile_options(TypeGenerator PRIVATE -fms-extensions)
    target_compile_options(HashCollector PRIVATE -fms-extensions)
endif()
target_include_directories(ApexPredator PUBLIC include)
target_include_directories(TypeGenerator PUBLIC include)
target_include_directories(HashCollector PUBLIC include)

target_link_libraries(ApexPredator PUBLIC libzstd)
