cmake_minimum_required(VERSION 3.10)
project(live_render)

set(CMAKE_CXX_STANDARD 20)


# force MSVC to use utf8
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")


set(LIVE_RENDER_SOURCES main.cpp live_danmaku.cpp live_danmaku.h)


add_executable(live_render ${DANMAKU_THIRDPARTY_SRC} ${LIVE_RENDER_SOURCES} )

target_link_libraries(live_render -lpthread  OpenSSL::SSL OpenSSL::Crypto ixwebsocket)

## install

install(TARGETS live_render
        CONFIGURATIONS Debug
        DESTINATION ${CMAKE_BINARY_DIR}/bin)

install(TARGETS live_render
        CONFIGURATIONS Release
        DESTINATION ${CMAKE_BINARY_DIR}/bin)