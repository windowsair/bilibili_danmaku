cmake_minimum_required(VERSION 3.10)
project(live_render)

set(CMAKE_CXX_STANDARD 20)


# force MSVC to use utf8
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

function(copy_lib)
    if (WIN32)
        file(COPY
                ${DANMAKU_THIRDPARTY_PATH}/openssl/x64-windows/bin/libcrypto-1_1-x64.dll
                ${DANMAKU_THIRDPARTY_PATH}/openssl/x64-windows/bin/libssl-1_1-x64.dll
                ${DANMAKU_THIRDPARTY_PATH}/libass/x64-windows/ass.dll
                DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    endif ()
endfunction(copy_lib)

set(LIVE_RENDER_SOURCES live_danmaku.cpp live_danmaku.h ffmpeg_render.cpp ffmpeg_render.h
        ../xml2ass/ass_danmaku.cpp
        ../xml2ass/ass_config.cpp
        ../xml2ass/danmaku.cpp
        )

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
    copy_lib()

    #### queue test
    set(TEST_QUEUE_SOURCES
            test/test_queue.cpp
            )
    add_executable(test_queue ${TEST_QUEUE_SOURCES})
    target_link_libraries(test_queue -lpthread)
    copy_lib()


    #### libass test
    if (WIN32)
        set(TEST_LIBASS_SOURCES
                test/test_libass_internal.cpp

                )
        add_executable(test_libass ${TEST_LIBASS_SOURCES})
        target_link_libraries(test_libass -lpthread ${DANMAKU_THIRDPARTY_PATH}/libass/x64-windows/ass.lib)
        copy_lib()
    endif ()


    if (WIN32)
        set(TEST_LIBASS_RENDER_SOURCES
                test/test_libass_render.cpp
                )
        add_executable(test_libass_render
                ${TEST_LIBASS_RENDER_SOURCES}
                ${DANMAKU_THIRDPARTY_SRC}
                ${LIVE_RENDER_SOURCES}
                ${LIVE_RENDER_THIRDPARTY_SRC})
        target_link_libraries(test_libass_render -lpthread OpenSSL::SSL OpenSSL::Crypto ixwebsocket
                re2::re2
                ${DANMAKU_THIRDPARTY_PATH}/libass/x64-windows/ass.lib
                )
        copy_lib()
    endif ()

endif () # LIVE_RENDER_BUILD_TESTING


include_directories(../xml2ass)

add_executable(live_render ./main.cpp ${DANMAKU_THIRDPARTY_SRC} ${LIVE_RENDER_SOURCES} ${LIVE_RENDER_THIRDPARTY_SRC})

target_link_libraries(live_render -lpthread OpenSSL::SSL OpenSSL::Crypto ixwebsocket
        re2::re2
        ${DANMAKU_THIRDPARTY_PATH}/libass/x64-windows/ass.lib
        )


## install

install(TARGETS live_render
        CONFIGURATIONS Debug
        DESTINATION ${CMAKE_BINARY_DIR}/bin)

install(TARGETS live_render
        CONFIGURATIONS Release
        DESTINATION ${CMAKE_BINARY_DIR}/bin)

if (WIN32)
    install(FILES ${DANMAKU_THIRDPARTY_PATH}/openssl/x64-windows/bin/libcrypto-1_1-x64.dll
            ${DANMAKU_THIRDPARTY_PATH}/openssl/x64-windows/bin/libssl-1_1-x64.dll
            CONFIGURATIONS Debug
            DESTINATION ${CMAKE_BINARY_DIR}/bin)

    install(FILES ${DANMAKU_THIRDPARTY_PATH}/openssl/x64-windows/bin/libcrypto-1_1-x64.dll
            ${DANMAKU_THIRDPARTY_PATH}/openssl/x64-windows/bin/libssl-1_1-x64.dll
            CONFIGURATIONS Release
            DESTINATION ${CMAKE_BINARY_DIR}/bin)
endif ()