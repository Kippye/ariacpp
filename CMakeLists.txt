cmake_minimum_required(VERSION 3.25)

project(ariacpp VERSION 0.1.0)
set(CPP_VERSION "c++23")

option(IS_LIBRARY "Build as a library" OFF)

set(ARIA_CPP_SRC    "${CMAKE_SOURCE_DIR}")

IF (IS_LIBRARY)
    add_library(${PROJECT_NAME})
ELSE()
    add_executable(${PROJECT_NAME})
ENDIF()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Only Linux flags for now
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=${CPP_VERSION} -lstdc++")

target_sources(${PROJECT_NAME} PUBLIC
    "${ARIA_CPP_SRC}/aria.cpp"
    "${ARIA_CPP_SRC}/main.cpp"
)