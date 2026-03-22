include(FetchContent)

FetchContent_Declare(
        RedsCore
        GIT_REPOSITORY https://github.com/REDxEYE/RedsCore.git
        GIT_TAG origin/master
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(RedsCore)

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
        GIT_REPOSITORY https://github.com/SRombauts/SQLiteCpp.git
        GIT_TAG 3.3.3
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(SQLiteCpp)
