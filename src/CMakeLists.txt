cmake_minimum_required(VERSION 3.25 FATAL_ERROR)
project(Wandelt LANGUAGES CXX)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/src/Wandelt/*.cpp
    ${CMAKE_SOURCE_DIR}/src/Wandelt/*.hpp
)

file(GLOB DEMO_FILES CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/demo/*
)

file(GLOB_RECURSE TEST_FILES CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/tests/Wandelt/*.cpp
    ${CMAKE_SOURCE_DIR}/tests/Wandelt/*.hpp
)

source_group("src" FILES ${SOURCES})
source_group("demo" FILES ${DEMO_FILES})
source_group("tests" FILES ${TEST_FILES})

add_executable(${PROJECT_NAME} ${SOURCES} ${DEMO_FILES} ${TEST_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../tests)

set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

wdt_target_pch(${PROJECT_NAME} pch.hpp ${CMAKE_CURRENT_SOURCE_DIR}/Wandelt/pch.cpp)
