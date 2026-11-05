set(CommonFiles
        ${ApexCommonFiles}
        modules/just_cause_2/archive_mount.cpp
        modules/just_cause_2/hash_policy.cpp
)

add_library(JustCause2Module MODULE
        modules/just_cause_2/module.cpp
        src/exporter/raw_export.cpp
        src/exporter/gltf_export.cpp
        src/apex/rbmdl/rbmdl_file.cpp
        src/apex/rbmdl/halo.cpp
        src/apex/rbmdl/general.cpp
        src/apex/rbmdl/facade.cpp
        src/apex/rbmdl/lambert.cpp
        src/apex/rbmdl/common.cpp
        src/apex/pcbb/pcbb.cpp
        src/exporter/pcbb_export.cpp
        src/exporter/common_export.cpp
        modules/just_cause_2/export/format_dispatch.cpp
        src/modules/shared.cpp
        ${CommonFiles}
)
target_include_directories(JustCause2Module PRIVATE include)
target_compile_definitions(JustCause2Module PRIVATE APEX_BUILD_GAME_MODULE)
target_link_libraries(JustCause2Module PUBLIC RedsCore SQLiteCpp zlib-ng ApexOoz)
set_target_properties(JustCause2Module PROPERTIES
        PREFIX "" OUTPUT_NAME "just_cause_2"
        LIBRARY_OUTPUT_DIRECTORY "${GAME_MODULE_OUTPUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY "${GAME_MODULE_OUTPUT_DIR}"
        CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN YES
)

add_executable(JustCause2HashCollector
        src/tools/hash_collector.cpp
        modules/just_cause_2/collector.cpp
        src/apex/asset_db.cpp
        ${CommonFiles}
)
target_include_directories(JustCause2HashCollector PRIVATE include)
target_link_libraries(JustCause2HashCollector PRIVATE RedsCore SQLiteCpp zlib-ng ApexOoz)
if(UNIX AND NOT APPLE)
    target_link_options(JustCause2Module PRIVATE "LINKER:--version-script=${CMAKE_CURRENT_SOURCE_DIR}/modules/just_cause_2/exports.map")
    set_property(TARGET JustCause2Module APPEND PROPERTY LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/modules/just_cause_2/exports.map")
endif()
