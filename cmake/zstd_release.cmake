# Included by zstd's project() so only its subtree gets Release settings in Debug.
# Keep the surrounding build's Debug CRT selection on MSVC for ABI compatibility.
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # zstd enables DEBUGLEVEL=1 based on CMAKE_BUILD_TYPE, independently of flags.
    set(CMAKE_BUILD_TYPE Release)
endif()
foreach(lang IN ITEMS C CXX ASM)
    set(CMAKE_${lang}_FLAGS_DEBUG "${CMAKE_${lang}_FLAGS_RELEASE}")
endforeach()
