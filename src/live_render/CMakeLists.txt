cmake_minimum_required(VERSION 3.10)
project(live_render)

set(CMAKE_CXX_STANDARD 20)


# force MSVC to use utf8
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")


set(LIVE_RENDER_SOURCES main.cpp live_danmaku.cpp live_danmaku.h
        ../xml2ass/ass_danmaku.cpp)

# DANMAKU_THIRDPARTY_PATH

option(LIVE_RENDER_BUILD_TESTING "live render build test" ON)
if (LIVE_RENDER_BUILD_TESTING)
    #### danmaku test
    set(TEST_DANMAKU_SOURCES
            test/danmaku_json_parse_benchmark.cpp

            )
    add_executable(test_danmaku ${TEST_DANMAKU_SOURCES}
            ${DANMAKU_THIRDPARTY_SRC}
            ${LIVE_RENDER_THIRDPARTY_SRC})
    target_link_libraries(test_danmaku re2::re2 -lpthread)
    file(COPY test/danmaku_list.txt DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

    #### queue test
    set(TEST_QUEUE_SOURCES
            test/test_queue.cpp
            )
    add_executable(test_queue ${TEST_QUEUE_SOURCES})
    target_link_libraries(test_queue -lpthread)
endif () # LIVE_RENDER_BUILD_TESTING


include_directories(../xml2ass)

add_executable(live_render ${DANMAKU_THIRDPARTY_SRC} ${LIVE_RENDER_SOURCES} ${LIVE_RENDER_THIRDPARTY_SRC})

target_link_libraries(live_render -lpthread OpenSSL::SSL OpenSSL::Crypto ixwebsocket
        re2::re2)

## install

install(TARGETS live_render
        CONFIGURATIONS Debug
        DESTINATION ${CMAKE_BINARY_DIR}/bin)

install(TARGETS live_render
        CONFIGURATIONS Release
        DESTINATION ${CMAKE_BINARY_DIR}/bin)