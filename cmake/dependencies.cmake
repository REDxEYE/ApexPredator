include(FetchContent)

FetchContent_Declare(
        pugixml
        QUIET
        GIT_REPOSITORY "https://github.com/zeux/pugixml.git"
        GIT_TAG  v1.15
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(pugixml)

if(TARGET pugixml::pugixml OR TARGET pugixml)
    set(pugixml_FOUND TRUE)
    set(PUGIXML_FOUND TRUE)
    # This prevents subsequent find_package(pugixml) calls from reloading the targets file
    macro(find_package)
        if(NOT "${ARGV0}" STREQUAL "pugixml")
            _find_package(${ARGV})
        endif()
    endmacro()
endif()

if(WIN32)
    # Tracy's cache option applies to every configuration in a Visual Studio build.
    set(TRACY_ENABLE OFF CACHE BOOL "Enable profiling" FORCE)
endif()


# RedsCore fetches zstd; project() runs this hook in zstd's own directory scope.
# Keep its Debug objects optimized without changing the application's Debug flags.
set(CMAKE_PROJECT_zstd_INCLUDE "${CMAKE_CURRENT_LIST_DIR}/zstd_release.cmake")

set(REDSCORE_LOCAL_DIR "/home/red_eye/CLionProjects/RedsCore")
if(EXISTS "${REDSCORE_LOCAL_DIR}/CMakeLists.txt")
    add_subdirectory(
            "${REDSCORE_LOCAL_DIR}"
            "${CMAKE_BINARY_DIR}/_deps/RedsCore-build"
    )
else()
    # Pin the RedsCore model API used by both game modules.
    FetchContent_Declare(
            RedsCore
            GIT_REPOSITORY https://github.com/REDxEYE/RedsCore.git
            GIT_TAG 75e55c4d
            GIT_PROGRESS TRUE
    )
    FetchContent_MakeAvailable(RedsCore)
endif()
if(WIN32)
    # Keep profiling in Debug, but do not start Tracy workers in unloadable Release DLLs.
    target_compile_definitions(TracyClient PUBLIC "$<$<CONFIG:Debug>:TRACY_ENABLE>")
endif()


FetchContent_Declare(
        ogg
        GIT_REPOSITORY "https://github.com/xiph/ogg"
        GIT_TAG v1.3.6
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(ogg)

set(OGG_FOUND TRUE CACHE BOOL "" FORCE)
set(OGG_INCLUDE_DIR "${ogg_SOURCE_DIR}/include" CACHE PATH "" FORCE)
set(OGG_LIBRARY ogg CACHE STRING "" FORCE) # note: target name is OK here

FetchContent_Declare(
        vorbis
        GIT_REPOSITORY "https://github.com/xiph/vorbis"
        GIT_TAG v1.3.7
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(vorbis)

#FetchContent_Declare(
#        nlohmann_json
#        QUIET
#        GIT_REPOSITORY "https://github.com/nlohmann/json.git"
#        GIT_TAG v3.12.0
#        GIT_SHALLOW TRUE
#)
#FetchContent_MakeAvailable(nlohmann_json)

FetchContent_Declare(
        SQLiteCpp
        QUIET
        GIT_REPOSITORY "https://github.com/SRombauts/SQLiteCpp.git"
        GIT_TAG 3.3.3
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(SQLiteCpp)


FetchContent_Declare(
        OpenXLSX
        QUIET
        GIT_REPOSITORY "https://codeberg.org/lars_uffmann/OpenXLSX.git"
        GIT_TAG v0.5.1
        GIT_SHALLOW TRUE
)
set(OPENXLSX_CREATE_DOCS           OFF)
set(OPENXLSX_BUILD_SAMPLES         OFF)
set(BUILD_SHARED_LIBS     OFF)
FetchContent_MakeAvailable(OpenXLSX)
