SET(CommonFiles
        src/apex/aaf/aaf.cpp
        src/apex/adf/adf.cpp
        src/apex/adf/sti.cpp
        src/apex/adf/sti_shared.cpp
        src/apex/avtx.cpp
        src/apex/gtoc.cpp
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
        src/apex/package/tab_v21.cpp
        src/apex/package/tab_v31.cpp
        src/utils/murmur3.cpp
)

add_library(Rage2Module MODULE
        modules/rage2/module.cpp
        modules/rage2/export_operations.cpp
        src/exporter/adf_export.cpp
        src/exporter/amf_export.cpp
        src/exporter/amf_attribute_decode.cpp
        src/exporter/common_export.cpp
        src/exporter/ddsc_export.cpp
        src/exporter/rtpc_export.cpp
        src/exporter/havok_export.cpp
        src/exporter/fmod_export.cpp
        src/havok/animations/spline.cpp
        src/utils/xlsx_helper.cpp
        src/modules/shared.cpp

        ${CommonFiles}
)
target_link_libraries(Rage2Module PRIVATE RedsCore ApexOoz Threads::Threads Rage2AdfLib Rage2HavokLib Ogg::ogg vorbis vorbisenc)
target_compile_definitions(Rage2Module PUBLIC GAME=1)
target_include_directories(Rage2Module PRIVATE include module/rage2/include)
target_compile_definitions(Rage2Module PRIVATE APEX_BUILD_GAME_MODULE)
set_target_properties(Rage2Module PROPERTIES
        PREFIX "" OUTPUT_NAME "rage2"
        LIBRARY_OUTPUT_DIRECTORY "${GAME_MODULE_OUTPUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY "${GAME_MODULE_OUTPUT_DIR}"
        CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN YES
)

SET(CommonLibs
        RedsCore
        SQLiteCpp
        OpenXLSX::OpenXLSX
        ApexOoz
)

add_executable(Rage2HashCollector
        src/tools/hash_collector.cpp
        ${CommonFiles}
)
target_compile_definitions(Rage2HashCollector PUBLIC GAME=1)
target_link_libraries(Rage2HashCollector PUBLIC Rage2HavokLib Rage2HavokLib Rage2AdfLib ${CommonLibs})
target_compile_options(Rage2HashCollector PRIVATE -march=native)

add_library(Rage2AdfLib STATIC
        modules/rage2/src/apex/adf/generated/adf_types.cpp
        modules/rage2/src/apex/adf/generated/adf_types_formatters.cpp
        src/apex/adf/adf_read_instance.cpp
)
target_compile_definitions(Rage2AdfLib PUBLIC GAME=1)
target_include_directories(Rage2AdfLib PUBLIC include modules/rage2/include)
target_link_libraries(Rage2AdfLib PUBLIC ${CommonLibs})
target_link_libraries(Rage2AdfLib PRIVATE ${CommonLibs} nlohmann_json::nlohmann_json)

add_executable(Rage2HavokTypeGenerator
        src/tools/havok_type_gen.cpp
        src/havok/havok_codegen.cpp
        ${CommonFiles}
)
target_compile_definitions(Rage2HavokTypeGenerator PUBLIC GAME=1)
target_include_directories(Rage2HavokTypeGenerator PUBLIC include modules/rage2/include)
target_link_libraries(Rage2HavokTypeGenerator PUBLIC ${CommonLibs} ApexOoz)
target_compile_options(Rage2HavokTypeGenerator PRIVATE -march=native)

add_library(Rage2HavokLib STATIC
        modules/rage2/src/havok/generated/havok_types.cpp
        modules/rage2/src/havok/generated/havok_types_formatters.cpp
        src/havok/havok_codegen.cpp
        src/havok/havok_types.cpp
        src/havok/tag_file/havok_tag_file.cpp
        src/havok/tag_file/havok_tag_types.cpp
        src/havok/tag_file/havok_tag_file_get_item.cpp
        src/havok/animations/spline.cpp
        #        src/havok/havok_helpers.cpp
)
target_compile_definitions(Rage2HavokLib PUBLIC GAME=1)
target_include_directories(Rage2HavokLib PUBLIC include modules/rage2/include)
target_link_libraries(Rage2HavokLib PUBLIC ${CommonLibs} nlohmann_json::nlohmann_json)

add_executable(Rage2AdfTypeGenerator
        src/tools/adf_type_gen.cpp
        src/apex/adf/sti_codegen.cpp
        src/apex/adf/adf_read_instance_stub.cpp
        ${CommonFiles}
        src/apex/package/tab_v21.cpp
        src/apex/package/tab_v31.cpp
)
target_compile_definitions(Rage2AdfTypeGenerator PUBLIC GAME=1)
#target_compile_definitions(Rage2AdfTypeGenerator PRIVATE TRACY_ENABLE TRACY_MEMORY TRACY_ON_DEMAND)
target_include_directories(Rage2AdfTypeGenerator PUBLIC include modules/rage2/include)
target_link_libraries(Rage2AdfTypeGenerator PUBLIC ${CommonLibs} ApexOoz)
target_compile_options(Rage2AdfTypeGenerator PRIVATE -march=native)

if(UNIX AND NOT APPLE)
    target_link_options(Rage2Module PRIVATE "LINKER:--version-script=${CMAKE_CURRENT_SOURCE_DIR}/modules/rage2/exports.map")
    set_property(TARGET Rage2Module APPEND PROPERTY LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/modules/rage2/exports.map")
elseif (MSVC)
    target_compile_options(Rage2AdfLib PRIVATE /bigobj)
    target_compile_options(Rage2HavokLib PRIVATE /bigobj)
    target_compile_options(Rage2Module PRIVATE /bigobj)
    target_compile_options(Rage2HashCollector PRIVATE /bigobj)
    target_compile_options(Rage2AdfTypeGenerator PRIVATE /bigobj)
    target_compile_options(Rage2HavokTypeGenerator PRIVATE /bigobj)
endif ()
