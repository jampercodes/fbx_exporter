cmake_minimum_required(VERSION 3.10)


project(fbxExport)

set(CMAKE_CXX_STANDARD 23)

add_library(fbxExport STATIC src/fbx.cpp)

target_include_directories(fbxExport PRIVATE ${CMAKE_SOURCE_DIR}/include)

# zlib is used to optionally compress FBX array payloads
find_package(ZLIB REQUIRED)
if(ZLIB_FOUND)
	target_link_libraries(fbxExport PRIVATE ZLIB::ZLIB)
endif()


# test fbx exporter
project(fbxTest)

add_executable(fbxTest fbx_test.cpp)

target_include_directories(fbxTest PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(fbxTest PRIVATE fbxExport)