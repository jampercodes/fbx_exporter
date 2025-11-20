cmake_minimum_required(VERSION 3.10)


project(fbxExport)

set(CMAKE_CXX_STANDARD 17)

message(STATUS "Target pointer size: ${CMAKE_SIZEOF_VOID_P}")
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(STATUS "Building for x86")
    set(target_architecture "x86")
else()
    message(STATUS "Building for x64")
    set(target_architecture "x64")
endif()

add_executable(fbx_test src/fbx_test.cpp)

target_include_directories(fbx_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(fbx_test PRIVATE ${CMAKE_SOURCE_DIR}/lib/FbxSdk/${target_architecture}/debug/libfbxsdk.a)