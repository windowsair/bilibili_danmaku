cmake_minimum_required(VERSION 3.10)
project(live_render)

set(CMAKE_CXX_STANDARD 20)


# force MSVC to use utf8
if (WIN32)
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
endif ()

function(copy_lib)
    if (WIN32)
        file(COPY
                ${DANMAKU_THIRDPARTY_PATH}/openssl/x64-windows/bin/libcrypto-3-x64.dll
                ${DANMAKU_THIRDPARTY_PATH}/openssl/x64-windows/bin/libssl-3-x64.dll
                ${DANMAKU_THIRDPARTY_PATH}/libass/x64-windows/ass.dll
                DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    endif ()
endfunction(copy_lib)

set(LIVE_RENDER_SOURCES
        live_danmaku.cpp live_danmaku.h ffmpeg_render.cpp ffmpeg_render.h
        live_info.cpp
        ../xml2ass/ass_sc.cpp
        ../xml2ass/ass_danmaku.cpp
        ../xml2ass/ass_config.cpp
        ../xml2ass/danmaku.cpp
        ../xml2ass/danmaku_filter.cpp
        live_monitor.cpp live_monitor.h live_render_config.cpp ffmpeg_utils.cpp ffmpeg_utils.h ass_render_utils.h sc_control.cpp)

## TODO: windows only
if (WIN32)
    set(LIVE_RENDER_THIRDPARTY_SRC
            ${DANMAKU_THIRDPARTY_PATH}/windows-kill-library/windows-kill-library/ctrl-routine.cpp
            ${DANMAKU_THIRDPARTY_PATH}/windows-kill-library/windows-kill-library/ctrl-routine.h
            ${DANMAKU_THIRDPARTY_PATH}/windows-kill-library/windows-kill-library/remote-process.cpp
            ${DANMAKU_THIRDPARTY_PATH}/windows-kill-library/windows-kill-library/remote-process.h
            ${DANMAKU_THIRDPARTY_PATH}/windows-kill-library/windows-kill-library/sender.cpp
            ${DANMAKU_THIRDPARTY_PATH}/windows-kill-library/windows-kill-library/sender.h
            ${DANMAKU_THIRDPARTY_PATH}/windows-kill-library/windows-kill-library/signal.cpp
            ${DANMAKU_THIRDPARTY_PATH}/windows-kill-library/windows-kill-library/signal.h
            ${DANMAKU_THIRDPARTY_PATH}/windows-kill-library/windows-kill-library/stdafx.cpp
            ${DANMAKU_THIRDPARTY_PATH}/windows-kill-library/windows-kill-library/stdafx.h
            ${DANMAKU_THIRDPARTY_PATH}/windows-kill-library/windows-kill-library/targetver.h
            ${DANMAKU_THIRDPARTY_PATH}/windows-kill-library/windows-kill-library/windows-kill-library.cpp
            ${DANMAKU_THIRDPARTY_PATH}/windows-kill-library/windows-kill-library/windows-kill-library.h
            )
else ()
    set(LIVE_RENDER_THIRDPARTY_SRC)
endif ()
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

    set(TEST_LIBASS_SOURCES
            test/test_libass_internal.cpp

            )
    add_executable(test_libass ${TEST_LIBASS_SOURCES})
    target_link_libraries(test_libass -lpthread ${DANMAKU_THIRDPARTY_PATH}/libass/x64-windows/ass.lib)
    copy_lib()


    #    if (WIN32)
    #        set(TEST_LIBASS_RENDER_SOURCES
    #                test/test_libass_render.cpp
    #                )
    #        add_executable(test_libass_render
    #                ${TEST_LIBASS_RENDER_SOURCES}
    #                ${DANMAKU_THIRDPARTY_SRC}
    #                ${LIVE_RENDER_SOURCES}
    #                ${LIVE_RENDER_THIRDPARTY_SRC})
    #        target_link_libraries(test_libass_render -lpthread OpenSSL::SSL OpenSSL::Crypto ixwebsocket
    #                re2::re2
    #                ${DANMAKU_THIRDPARTY_PATH}/libass/x64-windows/ass.lib
    #                )
    #        copy_lib()
    #    endif ()

    if (WIN32)
        set(TEST_FFMPEG_GET_INFO_SOURCES
                test/test_ffmpeg_get_info.cpp
                )
        add_executable(test_ffmpeg_get_info ${TEST_FFMPEG_GET_INFO_SOURCES})
    endif ()

    set(TEST_FFMPEG_QUICK_START_SOURCES
            test/test_ffmpeg_quick_start.cpp
            )

    add_executable(test_ffmpeg_quick_start ${TEST_FFMPEG_QUICK_START_SOURCES})

    if (WIN32)
        set(TEST_WINDOWS_ENCODE_SOURCES
                test/test_windows_encode.cpp
                ${DANMAKU_THIRDPARTY_PATH}/simdutf/simdutf.cpp
                ${DANMAKU_THIRDPARTY_PATH}/fmt/src/format.cc
                ${DANMAKU_THIRDPARTY_PATH}/fmt/src/os.cc
                )

        add_executable(test_windows_encode ${TEST_WINDOWS_ENCODE_SOURCES})
        file(COPY test/encode_test.txt DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

    endif ()

    if (WIN32)
        set(TEST_FILENAME_CHECK_SOURCE
                test/test_filename_check.cpp
                ${DANMAKU_THIRDPARTY_PATH}/simdutf/simdutf.cpp
                ${DANMAKU_THIRDPARTY_PATH}/fmt/src/format.cc
                ${DANMAKU_THIRDPARTY_PATH}/fmt/src/os.cc
                )
        add_executable(test_filename_check ${TEST_FILENAME_CHECK_SOURCE}
                ${DANMAKU_THIRDPARTY_SRC}
                ${LIVE_RENDER_THIRDPARTY_SRC})
        target_link_libraries(test_filename_check re2::re2)

    endif ()


    #### cr replace test
    set(TEST_CR_REPLACE_SOURCES
            test/test_cr_char_replace.cpp

            )
    add_executable(test_cr_char_replace ${TEST_CR_REPLACE_SOURCES}
            ${DANMAKU_THIRDPARTY_SRC}
            ${LIVE_RENDER_THIRDPARTY_SRC})
    target_link_libraries(test_cr_char_replace re2::re2)
    copy_lib()


    # superchat ass template test
    set(TEST_SC_TEMPLATE
            test/test_sc_common.cpp
            test/test_sc_template.cpp
            ../xml2ass/ass_sc.cpp
            ../xml2ass/ass_danmaku.cpp
            ../xml2ass/ass_config.cpp
            ../xml2ass/ass_util.hpp
            ../xml2ass/sc_item.h
            ${DANMAKU_THIRDPARTY_PATH}/simdutf/simdutf.cpp
            ${DANMAKU_THIRDPARTY_PATH}/fmt/src/format.cc
            ${DANMAKU_THIRDPARTY_PATH}/fmt/src/os.cc
    )
    add_executable(test_sc_template ${TEST_SC_TEMPLATE}
            ${DANMAKU_THIRDPARTY_SRC}
            ${LIVE_RENDER_THIRDPARTY_SRC})
    # test_sc_control link lib
    if (WIN32)
        target_link_libraries(test_sc_template
                ${DANMAKU_THIRDPARTY_PATH}/libass/x64-windows/ass.lib
                )
    else ()
        # Linux version
        find_package(PkgConfig REQUIRED)
        set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:${DANMAKU_THIRDPARTY_PATH}/libass/linux/")
        pkg_check_modules(LIBASS_DEP_PKG REQUIRED fontconfig harfbuzz fribidi freetype2)

        target_link_libraries(test_sc_template
                -L${DANMAKU_THIRDPARTY_PATH}/libass/linux/lib -lass
                ${LIBASS_DEP_PKG_LDFLAGS}
                )
    endif ()
    file(COPY test/sc_content_list.txt test/sc_username_list.txt DESTINATION ${CMAKE_CURRENT_BINARY_DIR})


    # superchat ass control & output control
    set(TEST_SC_COMMON
            test/test_sc_common.cpp
            sc_control.cpp
            live_render_config.cpp
            ../xml2ass/ass_sc.cpp
            ../xml2ass/ass_danmaku.cpp
            ../xml2ass/ass_config.cpp
            ../xml2ass/ass_util.hpp
            ../xml2ass/sc_item.h
            ../xml2ass/file_helper.cpp
            ${DANMAKU_THIRDPARTY_PATH}/simdutf/simdutf.cpp
            ${DANMAKU_THIRDPARTY_PATH}/fmt/src/format.cc
            ${DANMAKU_THIRDPARTY_PATH}/fmt/src/os.cc
            )
    add_executable(test_sc_control test/test_sc_control.cpp ${TEST_SC_COMMON}
            ${DANMAKU_THIRDPARTY_SRC}
            ${LIVE_RENDER_THIRDPARTY_SRC})
    add_executable(test_sc_control_render test/test_sc_control_render.cpp ${TEST_SC_COMMON}
            ${DANMAKU_THIRDPARTY_SRC}
            ${LIVE_RENDER_THIRDPARTY_SRC})
    file(GLOB sc_file_list "test/sc_list/*.xml")
    foreach(file ${sc_file_list})
        file(COPY "${file}" DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/sc_list/)
    endforeach()

    # test_sc_control link lib
    if (WIN32)
        target_link_libraries(test_sc_control
                ${DANMAKU_THIRDPARTY_PATH}/libass/x64-windows/ass.lib
                )
    else ()
        # Linux version
        find_package(PkgConfig REQUIRED)
        set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:${DANMAKU_THIRDPARTY_PATH}/libass/linux/")
        pkg_check_modules(LIBASS_DEP_PKG REQUIRED fontconfig harfbuzz fribidi freetype2)

        target_link_libraries(test_sc_control
                -L${DANMAKU_THIRDPARTY_PATH}/libass/linux/lib -lass
                ${LIBASS_DEP_PKG_LDFLAGS}
                )
    endif ()

    # test_sc_control_render link lib
    if (WIN32)
        target_link_libraries(test_sc_control_render -lpthread
                ${DANMAKU_THIRDPARTY_PATH}/libass/x64-windows/ass.lib
                )
    else ()
        # Linux version
        find_package(PkgConfig REQUIRED)
        set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:${DANMAKU_THIRDPARTY_PATH}/libass/linux/")
        pkg_check_modules(LIBASS_DEP_PKG REQUIRED fontconfig harfbuzz fribidi freetype2)


        target_link_libraries(test_sc_control_render -lpthread
                -L${DANMAKU_THIRDPARTY_PATH}/libass/linux/lib -lass
                ${LIBASS_DEP_PKG_LDFLAGS}
                )
    endif ()

endif () # LIVE_RENDER_BUILD_TESTING


include_directories(../xml2ass ../live_render)

add_executable(live_render ./main.cpp ${DANMAKU_THIRDPARTY_SRC} ${LIVE_RENDER_SOURCES} ${LIVE_RENDER_THIRDPARTY_SRC})

if (WIN32)
    target_link_libraries(live_render -lpthread OpenSSL::SSL OpenSSL::Crypto ixwebsocket
            re2::re2 git_info
            brotlicommon
            brotlidec
            brotlienc
            ${DANMAKU_THIRDPARTY_PATH}/libass/x64-windows/ass.lib
            )
else ()
    # Linux version
    find_package(PkgConfig REQUIRED)
    set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:${DANMAKU_THIRDPARTY_PATH}/libass/linux/")
    pkg_check_modules(LIBASS_DEP_PKG REQUIRED fontconfig harfbuzz fribidi freetype2)


    target_link_libraries(live_render -lpthread OpenSSL::SSL OpenSSL::Crypto ixwebsocket
            re2::re2 git_info
            brotlicommon
            brotlidec
            brotlienc
            -L${DANMAKU_THIRDPARTY_PATH}/libass/linux/lib -lass
            ${LIBASS_DEP_PKG_LDFLAGS}
            )
endif ()

## install

install(TARGETS live_render
        CONFIGURATIONS Debug
        DESTINATION ${CMAKE_BINARY_DIR}/bin)

install(TARGETS live_render
        CONFIGURATIONS Release
        DESTINATION ${CMAKE_BINARY_DIR}/bin)

if (WIN32)
    install(FILES ${DANMAKU_THIRDPARTY_PATH}/openssl/x64-windows/bin/libcrypto-3-x64.dll
            ${DANMAKU_THIRDPARTY_PATH}/openssl/x64-windows/bin/libssl-3-x64.dll
            ${DANMAKU_THIRDPARTY_PATH}/libass/x64-windows/ass.dll
            CONFIGURATIONS Debug
            DESTINATION ${CMAKE_BINARY_DIR}/bin)

    install(FILES ${DANMAKU_THIRDPARTY_PATH}/openssl/x64-windows/bin/libcrypto-3-x64.dll
            ${DANMAKU_THIRDPARTY_PATH}/openssl/x64-windows/bin/libssl-3-x64.dll
            ${DANMAKU_THIRDPARTY_PATH}/libass/x64-windows/ass.dll
            CONFIGURATIONS Release
            DESTINATION ${CMAKE_BINARY_DIR}/bin)


endif ()