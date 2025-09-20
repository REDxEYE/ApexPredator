cmake_minimum_required(VERSION 3.28.3)
project(ApexPredator C)

set(CMAKE_C_STANDARD 17)

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
)

if(UNIX)
    target_compile_options(ApexPredator PRIVATE -fms-extensions)
endif()
target_include_directories(ApexPredator PUBLIC include)
