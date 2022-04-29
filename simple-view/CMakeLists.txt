cmake_minimum_required(VERSION 3.23.0 FATAL_ERROR)

set(PROJECT_NAME simple-view)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(EXTERNALS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../external)
set(EXTERNALS_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/external)

set(UTILS_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/utils/directory.hpp)

project(${PROJECT_NAME})

# simple open gl view
add_executable(${PROJECT_NAME} src/main.cpp src/gl_draw.h)

# boost
include_directories(${EXTERNALS_SOURCE_DIR}/boost)

# GLFW options
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

add_subdirectory(${EXTERNALS_SOURCE_DIR}/glfw ${EXTERNALS_BINARY_DIR}/glfw)
target_link_libraries(${PROJECT_NAME} glfw)
set_target_properties(glfw PROPERTIES FOLDER "external")

add_subdirectory(${EXTERNALS_SOURCE_DIR}/glad ${EXTERNALS_BINARY_DIR}/glad)
target_link_libraries(${PROJECT_NAME} glad)
set_target_properties(glad PROPERTIES FOLDER "external")


# cmake lib
set(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    ${EXTERNALS_SOURCE_DIR}/cef-cmake/cmake
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CEF_VERSION "92.0.27+g274abcf+chromium-92.0.4515.159")

set(CEF_USE_SANDBOX OFF CACHE BOOL "Force turning off of sandbox")

include(cef_cmake)

#
add_subdirectory(${EXTERNALS_SOURCE_DIR}/cef-cmake  ${EXTERNALS_BINARY_DIR}/cef-cmake)
set_target_properties(cefdll_wrapper PROPERTIES FOLDER "external")

#
add_subdirectory(${EXTERNALS_SOURCE_DIR}/jsbind-cef ${EXTERNALS_BINARY_DIR}/jsbind-cef)
set_target_properties(jsbind-cef PROPERTIES FOLDER "external")

# simple open gl view
add_executable(cef-async
    src/cef/client.hpp
    src/cef/render.hpp
    src/cef/types.hpp
    src/cef-async.cpp
    ${UTILS_SOURCES}
    ${CEF_CMAKE_EXECUTABLE_RESOURCES}
)

target_link_libraries(cef-async
    cefdll_wrapper
    jsbind-cef
)

# this should be added by cef-cmake
# but to support earlier cmake versions which don't support
# target_link_options we manually add them here
if (CEF_CMAKE_OS_POSIX)
    set_target_properties(cef-async PROPERTIES
        # passing local rpath linker flags
        LINK_FLAGS "-Wl,-rpath,.:$ORIGIN"
    )
endif()