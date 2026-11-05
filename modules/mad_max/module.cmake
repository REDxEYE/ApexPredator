set(MadMaxCommonFiles
        ${ApexCommonFiles}
        modules/mad_max/archive_mount.cpp
        modules/mad_max/hash_policy.cpp
)
set(MadMaxIncludeDirs include modules/mad_max/include )
set(MadMaxSourceDir "${CMAKE_CURRENT_SOURCE_DIR}/modules/mad_max")
set(MadMaxCommonLibs RedsCore SQLiteCpp ApexOoz zlib-ng)

add_library(MadMaxAdfLib STATIC
        modules/mad_max/src/apex/adf/generated/adf_types.cpp
        modules/mad_max/src/apex/adf/generated/adf_types_formatters.cpp
        src/apex/adf/adf_read_instance.cpp
)
target_include_directories(MadMaxAdfLib PUBLIC ${MadMaxIncludeDirs})
target_link_libraries(MadMaxAdfLib PUBLIC ${MadMaxCommonLibs})
target_link_libraries(MadMaxAdfLib PRIVATE nlohmann_json::nlohmann_json)

add_library(MadMaxHavokLib STATIC
        modules/mad_max/src/havok/generated/havok_types.cpp
)
target_include_directories(MadMaxHavokLib PUBLIC ${MadMaxIncludeDirs})
target_link_libraries(MadMaxHavokLib PUBLIC ${MadMaxCommonLibs} nlohmann_json::nlohmann_json)


add_library(MadMaxModule MODULE
        modules/mad_max/archive.cpp
        modules/mad_max/export_operations.cpp
        modules/mad_max/module.cpp
        src/exporter/raw_export.cpp
        src/exporter/common_export.cpp
        modules/mad_max/export/format_dispatch.cpp
        src/exporter/mad_max/havok_export.cpp
        src/havok/legacy/sdk2014/packfile.cpp
        src/modules/shared.cpp
        modules/mad_max/src/havok/generated/havok_types.cpp
        ${MadMaxCommonFiles}
)
target_include_directories(MadMaxModule PUBLIC ${MadMaxIncludeDirs})
target_compile_definitions(MadMaxModule PRIVATE APEX_BUILD_GAME_MODULE)
target_link_libraries(MadMaxModule PRIVATE ${MadMaxCommonLibs} MadMaxAdfLib MadMaxHavokLib)
set_target_properties(MadMaxModule PROPERTIES
        PREFIX "" OUTPUT_NAME "mad_max"
        LIBRARY_OUTPUT_DIRECTORY "${GAME_MODULE_OUTPUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY "${GAME_MODULE_OUTPUT_DIR}"
        CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN YES
)
if(UNIX AND NOT APPLE)
    target_link_options(MadMaxModule PRIVATE "LINKER:--version-script=${CMAKE_CURRENT_SOURCE_DIR}/modules/mad_max/exports.map")
    set_property(TARGET MadMaxModule APPEND PROPERTY LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/modules/mad_max/exports.map")
endif()

add_executable(MadMaxHashCollector
        src/tools/hash_collector.cpp
        modules/mad_max/collector.cpp
        ${ApexCommonFiles}
        modules/mad_max/archive_mount.cpp
        modules/mad_max/hash_policy.cpp
)
target_include_directories(MadMaxHashCollector PRIVATE include src/apex/package)
target_link_libraries(MadMaxHashCollector PRIVATE RedsCore SQLiteCpp zlib-ng ApexOoz)


add_executable(MadMaxHavokTypeGenerator
        src/tools/havok_type_gen.cpp
        modules/mad_max/havok_type_generator_config.cpp
        src/havok/havok_codegen.cpp
        ${MadMaxCommonFiles}
)
target_include_directories(MadMaxHavokTypeGenerator PRIVATE ${MadMaxIncludeDirs})
target_link_libraries(MadMaxHavokTypeGenerator PRIVATE ${MadMaxCommonLibs})

add_executable(MadMaxAdfTypeGenerator
        src/tools/adf_type_gen.cpp
        modules/mad_max/adf_type_generator_config.cpp
        src/apex/adf/sti_codegen.cpp
        src/apex/adf/adf_read_instance_stub.cpp
        ${MadMaxCommonFiles}
)
target_include_directories(MadMaxAdfTypeGenerator PRIVATE ${MadMaxIncludeDirs})
target_link_libraries(MadMaxAdfTypeGenerator PRIVATE ${MadMaxCommonLibs})


if(MSVC)
    target_compile_options(MadMaxHavokTypeGenerator PRIVATE /bigobj)
    target_compile_options(MadMaxAdfTypeGenerator PRIVATE /bigobj)
    target_compile_options(MadMaxAdfLib PRIVATE /bigobj)
    target_compile_options(MadMaxHavokLib PRIVATE /bigobj)
    target_compile_options(MadMaxModule PRIVATE /bigobj)
endif()