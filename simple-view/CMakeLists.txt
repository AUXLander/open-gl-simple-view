cmake_minimum_required(VERSION 3.23.0 FATAL_ERROR)

set(PROJECT_NAME simple-view)

set(EXTERNALS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../external)
set(EXTERNALS_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/external)

project(${PROJECT_NAME})

add_executable(${PROJECT_NAME} src/main.cpp)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

add_subdirectory(${EXTERNALS_SOURCE_DIR}/glfw ${EXTERNALS_BINARY_DIR}/glfw)
target_link_libraries(${PROJECT_NAME} glfw)

add_subdirectory(${EXTERNALS_SOURCE_DIR}/glad ${EXTERNALS_BINARY_DIR}/glad)
target_link_libraries(${PROJECT_NAME} glad)