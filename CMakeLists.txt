cmake_minimum_required(VERSION 3.25)

project(ariacpp VERSION 0.1.0)
set(CPP_VERSION "c++23")

option(ARIACPP_TESTS "Build testing executable" ON)

set(ARIACPP_SRC    "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(ARIACPP_INCLUDE    "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_library(${PROJECT_NAME})
IF (ARIACPP_TESTS)
    add_subdirectory("tests")
ENDIF()

# Only Linux flags for now
target_compile_options(${PROJECT_NAME} PRIVATE "${CMAKE_CXX_FLAGS} -std=${CPP_VERSION} -lstdc++ -Ofast")

target_include_directories(${PROJECT_NAME} PUBLIC
    "${ARIACPP_INCLUDE}"
)

target_sources(${PROJECT_NAME} PUBLIC
    "${ARIACPP_SRC}/aria.cpp"
)