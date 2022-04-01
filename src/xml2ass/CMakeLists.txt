cmake_minimum_required(VERSION 3.10)
project(xml2ass)

set(CMAKE_CXX_STANDARD 20)


# force MSVC to use utf8
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")


set(XML2ASS_SOURCES main.cpp
        danmaku.cpp danmaku.h config.cpp config.h ass.cpp ass.h
        file_helper.cpp file_helper.h)

#add_subdirectory(thirdparty/scnlib)

add_executable(xml2ass ${THIRDPARTY_SRC} ${XML2ASS_SOURCES})

target_link_libraries(xml2ass -lpthread)

#install(TARGETS xml2ass DESTINATION ${CMAKE_BINARY_DIR}/bin)
#install(TARGETS xml2ass DESTINATION ${CMAKE_BINARY_DIR}/bin)
