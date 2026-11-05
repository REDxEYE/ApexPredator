set(SecondExtinctionCommonFiles
        ${ApexCommonFiles}
        src/modules/apex_archive_mount.cpp
        modules/second_extinction/hash_policy.cpp
)
set(SecondExtinctionCommonLibs RedsCore SQLiteCpp OpenXLSX::OpenXLSX ApexOoz)
set(SecondExtinctionIncludeDirs include modules/second_extinction/include)
set(SecondExtinctionSourceDir "${CMAKE_CURRENT_SOURCE_DIR}/modules/second_extinction")

# Havok generation reads actual game archives; it does not need generated bindings.
add_executable(SecondExtinctionHavokTypeGenerator
        src/tools/havok_type_gen.cpp
        modules/second_extinction/havok_type_generator_config.cpp
        src/havok/havok_codegen.cpp
        ${SecondExtinctionCommonFiles}
)
target_include_directories(SecondExtinctionHavokTypeGenerator PRIVATE ${SecondExtinctionIncludeDirs})
target_link_libraries(SecondExtinctionHavokTypeGenerator PRIVATE ${SecondExtinctionCommonLibs})

add_executable(SecondExtinctionAdfTypeGenerator
        src/tools/adf_type_gen.cpp
        modules/second_extinction/adf_type_generator_config.cpp
        src/apex/adf/sti_codegen.cpp
        src/apex/adf/adf_read_instance_stub.cpp
        ${SecondExtinctionCommonFiles}
)
target_include_directories(SecondExtinctionAdfTypeGenerator PRIVATE ${SecondExtinctionIncludeDirs})
target_link_libraries(SecondExtinctionAdfTypeGenerator PRIVATE ${SecondExtinctionCommonLibs})

set(SecondExtinctionGeneratedFiles
        include/apex/adf/generated/adf_types.h
        include/apex/adf/generated/adf_types_fwd.h
        src/apex/adf/generated/adf_types.cpp
        src/apex/adf/generated/adf_types_formatters.cpp
        include/havok/generated/havok_types.h
        include/havok/generated/havok_types_fwd.h
        src/havok/generated/havok_types.cpp
        src/havok/generated/havok_types_formatters.cpp
)

add_executable(SecondExtinctionHashCollector
        src/tools/hash_collector.cpp
        modules/second_extinction/collector.cpp
        ${SecondExtinctionCommonFiles}
)
target_include_directories(SecondExtinctionHashCollector PRIVATE ${SecondExtinctionIncludeDirs})
target_link_libraries(SecondExtinctionHashCollector PRIVATE ${SecondExtinctionCommonLibs})

add_library(SecondExtinctionAdfLib STATIC
        modules/second_extinction/src/apex/adf/generated/adf_types.cpp
        modules/second_extinction/src/apex/adf/generated/adf_types_formatters.cpp
        src/apex/adf/adf_read_instance.cpp
)
target_include_directories(SecondExtinctionAdfLib PUBLIC ${SecondExtinctionIncludeDirs})
target_link_libraries(SecondExtinctionAdfLib PUBLIC ${SecondExtinctionCommonLibs})
target_link_libraries(SecondExtinctionAdfLib PRIVATE nlohmann_json::nlohmann_json)

add_library(SecondExtinctionHavokLib STATIC
        modules/second_extinction/src/havok/generated/havok_types.cpp
        modules/second_extinction/src/havok/generated/havok_types_formatters.cpp
        src/havok/havok_codegen.cpp
        src/havok/havok_types.cpp
        src/havok/tag_file/havok_tag_file.cpp
        src/havok/tag_file/havok_tag_types.cpp
        src/havok/tag_file/havok_tag_file_get_item.cpp
        src/havok/animations/spline.cpp
)
target_include_directories(SecondExtinctionHavokLib PUBLIC ${SecondExtinctionIncludeDirs})
target_link_libraries(SecondExtinctionHavokLib PUBLIC ${SecondExtinctionCommonLibs} nlohmann_json::nlohmann_json)

add_library(SecondExtinctionModule MODULE
        modules/second_extinction/module.cpp
        modules/second_extinction/export_operations.cpp
        src/exporter/raw_export.cpp
        src/exporter/gltf_export.cpp
        src/exporter/adf_export.cpp
        src/exporter/apex/adf_actions.cpp
        modules/second_extinction/export/adf_actions.cpp
        src/exporter/amf_export.cpp
        src/exporter/apex/amf_weight_sets.cpp
        src/exporter/apex/amf_merged_geometry.cpp
        modules/second_extinction/export/amf_material.cpp
        src/exporter/apex/amf_material_merged.cpp
        src/exporter/amf_attribute_decode.cpp
        src/exporter/common_export.cpp
        src/exporter/apex/format_dispatch.cpp
        src/exporter/ddsc_export.cpp
        src/exporter/rtpc_export.cpp
        modules/rage2/rtpc_adapter.cpp
        src/exporter/havok_export.cpp
        src/exporter/fmod_export.cpp
        src/havok/animations/spline.cpp
        src/utils/xlsx_helper.cpp
        src/modules/shared.cpp
        ${SecondExtinctionCommonFiles}
)
target_compile_definitions(SecondExtinctionModule PRIVATE APEX_BUILD_GAME_MODULE)
target_include_directories(SecondExtinctionModule PRIVATE ${SecondExtinctionIncludeDirs})
target_link_libraries(SecondExtinctionModule PRIVATE
        RedsCore ApexOoz Threads::Threads SecondExtinctionAdfLib SecondExtinctionHavokLib
        Ogg::ogg vorbis vorbisenc)
set_target_properties(SecondExtinctionModule PROPERTIES
        PREFIX "" OUTPUT_NAME "second_extinction"
        LIBRARY_OUTPUT_DIRECTORY "${GAME_MODULE_OUTPUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY "${GAME_MODULE_OUTPUT_DIR}"
        CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN YES
)


if(UNIX AND NOT APPLE)
    target_link_options(SecondExtinctionModule PRIVATE
            "LINKER:--version-script=${SecondExtinctionSourceDir}/exports.map")
    set_property(TARGET SecondExtinctionModule APPEND PROPERTY LINK_DEPENDS
            "${SecondExtinctionSourceDir}/exports.map")
endif()

if(MSVC)
    target_compile_options(SecondExtinctionHavokTypeGenerator PRIVATE /bigobj)
    target_compile_options(SecondExtinctionAdfTypeGenerator PRIVATE /bigobj)
    foreach(target IN ITEMS SecondExtinctionAdfLib SecondExtinctionHavokLib
            SecondExtinctionModule SecondExtinctionHashCollector)
        target_compile_options(${target} PRIVATE /bigobj)
    endforeach()
endif()
