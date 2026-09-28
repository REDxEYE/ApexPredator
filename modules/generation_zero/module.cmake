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


SET(CommonLibs
        RedsCore
        SQLiteCpp
        OpenXLSX::OpenXLSX
        ApexOoz
)


add_library(GenerationZeroAdfLib STATIC
        modules/generation_zero/src/apex/adf/generated/adf_types.cpp
        modules/generation_zero/src/apex/adf/generated/adf_types_formatters.cpp
        src/apex/adf/adf_read_instance.cpp
)
target_compile_definitions(GenerationZeroAdfLib PRIVATE GAME=0)
target_include_directories(GenerationZeroAdfLib PUBLIC include modules/generation_zero/include)
target_link_libraries(GenerationZeroAdfLib PUBLIC ${CommonLibs})
target_link_libraries(GenerationZeroAdfLib PRIVATE ${CommonLibs} nlohmann_json::nlohmann_json)


add_library(GenerationZeroHavokLib STATIC
        modules/generation_zero/src/havok/generated/havok_types.cpp
        modules/generation_zero/src/havok/generated/havok_types_formatters.cpp
        src/havok/havok_codegen.cpp
        src/havok/havok_types.cpp
        src/havok/tag_file/havok_tag_file.cpp
        src/havok/tag_file/havok_tag_types.cpp
        src/havok/tag_file/havok_tag_file_get_item.cpp
        src/havok/animations/spline.cpp
        #        src/havok/havok_helpers.cpp
)
target_compile_definitions(GenerationZeroHavokLib PRIVATE GAME=0)
target_include_directories(GenerationZeroHavokLib PUBLIC include modules/generation_zero/include)
target_link_libraries(GenerationZeroHavokLib PUBLIC ${CommonLibs})


add_library(GenerationZeroModule MODULE
        modules/generation_zero/module.cpp
        modules/generation_zero/export_operations.cpp
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
target_compile_definitions(GenerationZeroModule PRIVATE GAME=0)
set_target_properties(GenerationZeroModule PROPERTIES
        PREFIX "" OUTPUT_NAME "generation_zero"
        LIBRARY_OUTPUT_DIRECTORY "${GAME_MODULE_OUTPUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY "${GAME_MODULE_OUTPUT_DIR}"
        CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN YES
)
target_include_directories(GenerationZeroModule PRIVATE include modules/generation_zero/include)
target_compile_definitions(GenerationZeroModule PRIVATE APEX_BUILD_GAME_MODULE)
target_link_libraries(GenerationZeroModule PRIVATE GenerationZeroHavokLib GenerationZeroAdfLib Ogg::ogg vorbis vorbisenc ${CommonLibs})


add_executable(GenerationZeroReadTester
        src/tools/read_tester.cpp
        ${CommonFiles}
)
target_compile_definitions(GenerationZeroReadTester PRIVATE GAME=0)
target_compile_definitions(GenerationZeroReadTester PRIVATE TRACY_MEMORY TRACY_ON_DEMAND)
target_link_libraries(GenerationZeroReadTester PUBLIC GenerationZeroHavokLib GenerationZeroAdfLib ${CommonLibs})


add_executable(GenerationZeroHashCollector
        src/tools/hash_collector.cpp
        ${CommonFiles}
)
target_compile_definitions(GenerationZeroHashCollector PRIVATE GAME=0)
target_include_directories(GenerationZeroHashCollector PUBLIC include)
target_link_libraries(GenerationZeroHashCollector PUBLIC GenerationZeroHavokLib GenerationZeroAdfLib ${CommonLibs})


add_executable(GenerationZeroAdfTypeGenerator
        src/tools/adf_type_gen.cpp
        src/apex/adf/sti_codegen.cpp
        src/apex/adf/adf_read_instance_stub.cpp
        ${CommonFiles}
        src/apex/package/tab_v21.cpp
        src/apex/package/tab_v31.cpp
)
target_compile_definitions(GenerationZeroAdfTypeGenerator PRIVATE GAME=0)
#target_compile_definitions(GenerationZeroAdfTypeGenerator PRIVATE TRACY_ENABLE TRACY_MEMORY TRACY_ON_DEMAND)
target_include_directories(GenerationZeroAdfTypeGenerator PUBLIC include modules/generation_zero/include)
target_link_libraries(GenerationZeroAdfTypeGenerator PUBLIC ${CommonLibs} ApexOoz)


add_executable(GenerationZeroHavokTypeGenerator
        src/tools/havok_type_gen.cpp
        src/havok/havok_codegen.cpp
        ${CommonFiles}
)
target_compile_definitions(GenerationZeroHavokTypeGenerator PRIVATE GAME=0)
target_include_directories(GenerationZeroHavokTypeGenerator PUBLIC include modules/generation_zero/include)
target_link_libraries(GenerationZeroHavokTypeGenerator PUBLIC ${CommonLibs} ApexOoz)

if (UNIX AND NOT APPLE)
    target_link_options(GenerationZeroModule PRIVATE "LINKER:--version-script=${CMAKE_CURRENT_SOURCE_DIR}/modules/generation_zero/exports.map")
    set_property(TARGET GenerationZeroModule APPEND PROPERTY LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/modules/generation_zero/exports.map")
    target_link_libraries(GenerationZeroReadTester PUBLIC m)
    target_link_libraries(GenerationZeroHashCollector PUBLIC m)
    target_link_libraries(GenerationZeroAdfTypeGenerator PUBLIC m)
    target_link_libraries(GenerationZeroHavokTypeGenerator PUBLIC m)
elseif (MSVC)
    target_compile_options(GenerationZeroAdfLib PRIVATE /bigobj)
    target_compile_options(GenerationZeroHavokLib PRIVATE /bigobj)
    target_compile_options(GenerationZeroModule PRIVATE /bigobj)
    target_compile_options(GenerationZeroReadTester PRIVATE /bigobj)
    target_compile_options(GenerationZeroHashCollector PRIVATE /bigobj)
    target_compile_options(GenerationZeroAdfTypeGenerator PRIVATE /bigobj)
    target_compile_options(GenerationZeroHavokTypeGenerator PRIVATE /bigobj)
endif ()



