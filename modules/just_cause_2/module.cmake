set(CommonFiles
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
        src/apex/package/tab_jc2.cpp
        src/utils/murmur3.cpp
        src/utils/zlib_wrapper.cpp
)

add_library(JustCause2Module MODULE
        modules/just_cause_2/module.cpp
        modules/just_cause_2/export_operations.cpp
        src/apex/rbmdl/rbmdl_file.cpp
        src/apex/rbmdl/halo.cpp
        src/apex/rbmdl/general.cpp
        src/apex/rbmdl/facade.cpp
        src/apex/rbmdl/lambert.cpp
        src/apex/rbmdl/common.cpp
        src/apex/pcbb/pcbb.cpp
        src/exporter/pcbb_export.cpp
        src/exporter/common_export.cpp
#        src/exporter/adf_export.cpp
#        src/exporter/amf_export.cpp
#        src/exporter/amf_attribute_decode.cpp
#        src/exporter/ddsc_export.cpp
#        src/exporter/rtpc_export.cpp
#        src/exporter/havok_export.cpp
#        src/exporter/fmod_export.cpp
#        src/havok/animations/spline.cpp
        src/modules/shared.cpp
        ${CommonFiles}
)
target_include_directories(JustCause2Module PRIVATE include)
target_compile_definitions(JustCause2Module PUBLIC APEX_BUILD_GAME_MODULE GAME=3)
target_link_libraries(JustCause2Module PUBLIC RedsCore SQLiteCpp zlib-ng ApexOoz)
set_target_properties(JustCause2Module PROPERTIES
        PREFIX "" OUTPUT_NAME "just_cause_2"
        LIBRARY_OUTPUT_DIRECTORY "${GAME_MODULE_OUTPUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY "${GAME_MODULE_OUTPUT_DIR}"
        CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN YES
)

add_executable(JustCause2HashCollector
        src/tools/hash_collector.cpp
        src/apex/asset_db.cpp
        ${CommonFiles}
)
target_include_directories(JustCause2HashCollector PRIVATE include)
target_compile_definitions(JustCause2HashCollector PRIVATE GAME=3)
target_link_libraries(JustCause2HashCollector PRIVATE RedsCore SQLiteCpp zlib-ng ApexOoz)
if(UNIX AND NOT APPLE)
    target_link_options(JustCause2Module PRIVATE "LINKER:--version-script=${CMAKE_CURRENT_SOURCE_DIR}/modules/just_cause_2/exports.map")
    set_property(TARGET JustCause2Module APPEND PROPERTY LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/modules/just_cause_2/exports.map")
endif()
