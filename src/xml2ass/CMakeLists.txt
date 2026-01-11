cmake_minimum_required(VERSION 3.10)
project(xml2ass)

set(CMAKE_CXX_STANDARD 20)

# force MSVC to use utf8
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

function(copy_lib)
    if (WIN32)
        file(COPY
             ${DANMAKU_THIRDPARTY_PATH}/libass/x64-windows/ass.dll
             DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    endif ()
endfunction(copy_lib)

set(XML2ASS_SOURCES
    main.cpp
    danmaku.cpp
    ass_config.cpp
    ass_danmaku.cpp
    file_helper.cpp
    file_helper.h
    danmaku_filter.cpp
    sc_control.cpp
    ../live_render/ass_render_utils.cpp)

include_directories(../xml2ass ../live_render)

add_executable(xml2ass ${DANMAKU_THIRDPARTY_SRC} ${XML2ASS_SOURCES})

target_link_libraries(xml2ass re2::re2 git_info -lpthread)
copy_lib()

if (WIN32)
    target_link_libraries(xml2ass
                          ${DANMAKU_THIRDPARTY_PATH}/libass/x64-windows/ass.lib)
else ()
    # Linux version
    find_package(PkgConfig REQUIRED)
    set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:${DANMAKU_THIRDPARTY_PATH}/libass/linux/")
    pkg_check_modules(LIBASS_DEP_PKG REQUIRED
                      fontconfig
                      harfbuzz
                      fribidi
                      freetype2)

    target_link_libraries(xml2ass
                          -L${DANMAKU_THIRDPARTY_PATH}/libass/linux/lib -lass
                          ${LIBASS_DEP_PKG_LDFLAGS})
endif ()

install(TARGETS xml2ass
        CONFIGURATIONS Debug
        DESTINATION ${CMAKE_BINARY_DIR}/bin)

install(TARGETS xml2ass
        CONFIGURATIONS Release
        DESTINATION ${CMAKE_BINARY_DIR}/bin)

if (WIN32)
    install(FILES
            ${DANMAKU_THIRDPARTY_PATH}/libass/x64-windows/ass.dll
            CONFIGURATIONS Debug
            DESTINATION ${CMAKE_BINARY_DIR}/bin)

    install(FILES
            ${DANMAKU_THIRDPARTY_PATH}/libass/x64-windows/ass.dll
            CONFIGURATIONS Release
            DESTINATION ${CMAKE_BINARY_DIR}/bin)
endif ()