cmake_minimum_required(VERSION 3.20)
project(ApexPredator CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

include(cmake/dependencies.cmake)

add_library(AdfLib STATIC
        src/apex/adf/generated/adf_types.cpp
        src/apex/adf/generated/adf_types_formatters.cpp
        src/apex/adf/adf_read_instance.cpp
)

add_library(HavokLib STATIC
        src/havok/generated/havok_types.cpp
        src/havok/generated/havok_types_formatters.cpp
        src/havok/havok_codegen.cpp
        src/havok/havok_types.cpp
        src/havok/tag_file/havok_tag_file.cpp
        src/havok/tag_file/havok_tag_types.cpp
        src/havok/tag_file/havok_tag_file_get_item.cpp
        src/havok/animations/spline.cpp
        #        src/havok/havok_helpers.cpp
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
        src/platform/archive_manager.cpp
        src/utils/hash_helper.cpp
        src/utils/json.cpp
        src/utils/lookup3.cpp
        src/apex/asset_db.cpp
        src/utils/stb.cpp
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

        src/havok/animations/spline.cpp

        ${CommonFiles}
        src/tools/apex_predator/commands/extract.cpp
        src/tools/apex_predator/commands/extract_anim.cpp
        src/tools/apex_predator/commands/search.cpp
        src/tools/apex_predator/commands/export_everything.cpp
        #        src/tools/apex_predator/commands/convert.cpp

)

add_executable(AdfTypeGenerator
        src/tools/adf_type_gen.cpp
        src/apex/adf/sti_codegen.cpp
        src/apex/adf/adf_read_instance_stub.cpp
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
        RedsCore
        SQLiteCpp
)

target_include_directories(AdfLib PUBLIC include)
target_include_directories(HavokLib PUBLIC include)
target_include_directories(ApexPredator PUBLIC include)
target_include_directories(AdfTypeGenerator PUBLIC include)
target_include_directories(HavokTypeGenerator PUBLIC include)
target_include_directories(HashCollector PUBLIC include)
target_include_directories(StringHasher PUBLIC include)

target_link_libraries(ApexPredator PUBLIC HavokLib AdfLib Ogg::ogg vorbis vorbisenc ${CommonLibs})
target_link_libraries(HashCollector PUBLIC HavokLib AdfLib ${CommonLibs})
target_link_libraries(AdfTypeGenerator PUBLIC ${CommonLibs})
target_link_libraries(HavokTypeGenerator PUBLIC ${CommonLibs})
target_link_libraries(AdfLib PUBLIC ${CommonLibs})
target_link_libraries(HavokLib PUBLIC ${CommonLibs})

link_libraries(Tracy::TracyClient)

add_compile_definitions(GLM_ENABLE_EXPERIMENTAL TRACY_CALLSTACK=32)

target_link_libraries(ApexPredator PRIVATE CLI11::CLI11)
target_compile_definitions(ApexPredator PRIVATE TRACY_ENABLE TRACY_MEMORY)
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
