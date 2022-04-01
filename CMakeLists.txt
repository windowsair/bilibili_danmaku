cmake_minimum_required(VERSION 3.10)
project(bilibili_danmaku)

set(THIRDPARTY_SRC
        ${CMAKE_SOURCE_DIR}/thirdparty/pugixml/pugixml.cpp
        ${CMAKE_SOURCE_DIR}/thirdparty/fmt/src/format.cc ${CMAKE_SOURCE_DIR}/thirdparty/fmt/src/os.cc
        ${CMAKE_SOURCE_DIR}/thirdparty/simdutf/simdutf.cpp
        )

include_directories(
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/thirdparty/fmt/include
        )


add_subdirectory(src/xml2ass)


install(TARGETS xml2ass
        CONFIGURATIONS Debug
        DESTINATION ${CMAKE_BINARY_DIR}/bin)

install(TARGETS xml2ass
        CONFIGURATIONS Release
        DESTINATION ${CMAKE_BINARY_DIR}/bin)