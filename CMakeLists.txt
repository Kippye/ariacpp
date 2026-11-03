cmake_minimum_required(VERSION 3.25)

project(ariacpp VERSION 0.1.0)
set(CPP_VERSION "c++23")
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_CXX_EXTENSIONS OFF)
option(ARIACPP_TESTS "Build testing executable" ON)

set(ARIACPP_SRC    "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(ARIACPP_INCLUDE    "${CMAKE_CURRENT_SOURCE_DIR}/include")

# Only Linux flags for now
add_compile_options(-Ofast)

add_library(${PROJECT_NAME})
IF (ARIACPP_TESTS)
    add_subdirectory("tests")
ENDIF()

target_include_directories(${PROJECT_NAME} PUBLIC
    "${ARIACPP_INCLUDE}"
)

target_sources(${PROJECT_NAME} PUBLIC
    "${ARIACPP_SRC}/aria.cpp"
)