cmake_minimum_required(VERSION 3.10)
project(bilibili_danmaku)

set(DANMAKU_THIRDPARTY_PATH ${CMAKE_SOURCE_DIR}/thirdparty)

set(DANMAKU_THIRDPARTY_SRC
        ${CMAKE_SOURCE_DIR}/thirdparty/pugixml/pugixml.cpp
        ${CMAKE_SOURCE_DIR}/thirdparty/fmt/src/format.cc ${CMAKE_SOURCE_DIR}/thirdparty/fmt/src/os.cc
        ${CMAKE_SOURCE_DIR}/thirdparty/simdutf/simdutf.cpp
        ${CMAKE_SOURCE_DIR}/thirdparty/libdeflate/lib/adler32.c
        ${CMAKE_SOURCE_DIR}/thirdparty/libdeflate/lib/deflate_decompress.c
        ${CMAKE_SOURCE_DIR}/thirdparty/libdeflate/lib/gzip_compress.c
        ${CMAKE_SOURCE_DIR}/thirdparty/libdeflate/lib/zlib_decompress.c
        ${CMAKE_SOURCE_DIR}/thirdparty/libdeflate/lib/deflate_compress.c
        ${CMAKE_SOURCE_DIR}/thirdparty/libdeflate/lib/gzip_decompress.c
        ${CMAKE_SOURCE_DIR}/thirdparty/libdeflate/lib/utils.c
        ${CMAKE_SOURCE_DIR}/thirdparty/libdeflate/lib/crc32.c
        ${CMAKE_SOURCE_DIR}/thirdparty/libdeflate/lib/zlib_compress.c
        )

# check platform
include(detect_platform.cmake)

if ((CLR_CMAKE_PLATFORM_ARCH_AMD64) OR (CLR_CMAKE_PLATFORM_ARCH_I386))
    list(APPEND DANMAKU_THIRDPARTY_SRC ${CMAKE_SOURCE_DIR}/thirdparty/libdeflate/lib/x86/cpu_features.c)
elseif ((CLR_CMAKE_TARGET_ARCH_ARM64) OR (CLR_CMAKE_TARGET_ARCH_ARM))
    list(APPEND DANMAKU_THIRDPARTY_SRC ${CMAKE_SOURCE_DIR}/thirdparty/libdeflate/lib/arm/cpu_features.c)
else ()
    message(FATAL_ERROR "Architecture not support.")
endif ()


include_directories(
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/thirdparty/fmt/include
        ${CMAKE_SOURCE_DIR}/thirdparty/libdeflate/
        ${CMAKE_SOURCE_DIR}/thirdparty/re2
)


# get git info
set(PRE_CONFIGURE_FILE "src/git.h.in")
set(POST_CONFIGURE_FILE "${CMAKE_CURRENT_BINARY_DIR}/git.cc")
include(git_watcher.cmake)

# Create a library out of the compiled post-configure file.
add_library(git_info STATIC ${POST_CONFIGURE_FILE})
target_include_directories(git_info PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
add_dependencies(git_info check_git)



# use prebuild openssl libary on windows
if (WIN32)
    set(OPENSSL_ROOT_DIR ${DANMAKU_THIRDPARTY_PATH}/openssl/x64-windows)
elseif (CLR_CMAKE_PLATFORM_DARWIN) # for macos
    set(OPENSSL_ROOT_DIR /usr/local/opt/openssl)
endif ()
find_package(OpenSSL REQUIRED)

set(USE_ZLIB FALSE)
set(IXWEBSOCKET_INSTALL FALSE)
set(USE_TLS ON)
set(USE_OPEN_SSL ON)

add_subdirectory(${DANMAKU_THIRDPARTY_PATH}/IXWebSocket)

set(RE2_BUILD_TESTING OFF)
add_subdirectory(${DANMAKU_THIRDPARTY_PATH}/re2)

add_subdirectory(src/xml2ass)

# TODO:
if (NOT APPLE)
    add_subdirectory(src/live_render)
endif ()