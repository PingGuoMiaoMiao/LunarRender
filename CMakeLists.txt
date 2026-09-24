cmake_minimum_required(VERSION 3.20)

project(LunarRenderNative C)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded" CACHE STRING "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

add_subdirectory(third_party/glfw ${CMAKE_BINARY_DIR}/glfw)

add_library(lunarrender_glad STATIC third_party/glad-generated/src/gl.c)
target_include_directories(lunarrender_glad PUBLIC third_party/glad-generated/include)

foreach(config Debug Release RelWithDebInfo MinSizeRel)
  set_target_properties(lunarrender_glad PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY_${config} "${CMAKE_BINARY_DIR}/lib"
  )
  set_target_properties(glfw PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY_${config} "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY_${config} "${CMAKE_BINARY_DIR}/bin"
  )
endforeach()
