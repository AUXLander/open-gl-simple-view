cmake_minimum_required(VERSION 3.23.0 FATAL_ERROR)

set(PROJECT_NAME simple-view)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(EXTERNALS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../external)
set(EXTERNALS_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/external)

project(${PROJECT_NAME})

# simple open gl view
add_executable(${PROJECT_NAME} src/main.cpp 
					 src/ws_server.h
					 src/gl_draw.h
					 src/mcml_types.h)

# boost
include_directories(${EXTERNALS_SOURCE_DIR}/boost)

# GLFW options
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

add_subdirectory(${EXTERNALS_SOURCE_DIR}/glfw ${EXTERNALS_BINARY_DIR}/glfw)
target_link_libraries(${PROJECT_NAME} glfw)

add_subdirectory(${EXTERNALS_SOURCE_DIR}/glad ${EXTERNALS_BINARY_DIR}/glad)
target_link_libraries(${PROJECT_NAME} glad)