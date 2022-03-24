cmake_minimum_required(VERSION 3.16)
set(project_name xml2ass)
project(${project_name})

set(CMAKE_CXX_STANDARD 20)


set(SOURCES src/main.cpp
        thirdparty/pugixml/pugixml.cpp
        thirdparty/fmt/src/format.cc thirdparty/fmt/src/os.cc
        src/danmuku.cpp src/danmuku.h src/config.cpp src/config.h src/ass.cpp src/ass.h)

add_subdirectory(thirdparty/scnlib)
include_directories(./ thirdparty/fmt/include thirdparty/utf8)


add_executable(${project_name} ${SOURCES})

target_link_libraries(${project_name} scn::scn -lpthread)
