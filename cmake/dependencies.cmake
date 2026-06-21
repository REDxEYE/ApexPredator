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


set(REDSCORE_LOCAL_DIR "/home/red_eye/CLionProjects/RedsCore")
if(EXISTS "${REDSCORE_LOCAL_DIR}/CMakeLists.txt")
    add_subdirectory(
            "${REDSCORE_LOCAL_DIR}"
            "${CMAKE_BINARY_DIR}/_deps/RedsCore-build"
    )
else()
    FetchContent_Declare(
            RedsCore
            GIT_REPOSITORY https://github.com/REDxEYE/RedsCore.git
            GIT_TAG origin/master
            GIT_SHALLOW TRUE
            GIT_REMOTE_UPDATE_STRATEGY CHECKOUT
            GIT_PROGRESS TRUE
    )
    FetchContent_MakeAvailable(RedsCore)
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
