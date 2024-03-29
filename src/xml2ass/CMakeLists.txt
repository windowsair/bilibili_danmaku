﻿cmake_minimum_required(VERSION 3.10)
project(xml2ass)

set(CMAKE_CXX_STANDARD 20)


# force MSVC to use utf8
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")


set(XML2ASS_SOURCES main.cpp
        danmaku.cpp danmaku.h ass_config.cpp ass_config.h ass_danmaku.cpp ass_danmaku.h
        file_helper.cpp file_helper.h danmaku_filter.cpp danmaku_filter.h)


add_executable(xml2ass ${DANMAKU_THIRDPARTY_SRC} ${XML2ASS_SOURCES})

target_link_libraries(xml2ass re2::re2 git_info -lpthread)

install(TARGETS xml2ass
        CONFIGURATIONS Debug
        DESTINATION ${CMAKE_BINARY_DIR}/bin)

install(TARGETS xml2ass
        CONFIGURATIONS Release
        DESTINATION ${CMAKE_BINARY_DIR}/bin)