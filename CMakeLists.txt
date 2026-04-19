cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

project(Wandelt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_SUPPRESS_REGENERATION ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

option(WDT_COMPILE_COMMANDS "Generate compile_commands.json for clangd" OFF)

if(WDT_COMPILE_COMMANDS)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# Dist configuration: optimized, no debug symbols
set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Dist" CACHE STRING "" FORCE)

set(CMAKE_CXX_FLAGS_DIST "${CMAKE_CXX_FLAGS_RELEASE}")
set(CMAKE_C_FLAGS_DIST "${CMAKE_C_FLAGS_RELEASE}")
set(CMAKE_EXE_LINKER_FLAGS_DIST "${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
set(CMAKE_SHARED_LINKER_FLAGS_DIST "${CMAKE_SHARED_LINKER_FLAGS_RELEASE}")
set(CMAKE_STATIC_LINKER_FLAGS_DIST "${CMAKE_STATIC_LINKER_FLAGS_RELEASE}")

if(MSVC)
    # Strip MSVC's default /EHsc before adding /EHs-c- to avoid D9025 override warnings
    string(REGEX REPLACE "/EH[a-z]+" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    add_compile_options(/W4 /GR- /EHs-c- /Zc:preprocessor /utf-8 /wd4251)
    add_compile_options(/w44062) #C4062: switch statement contains unhandled enum cases
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-D_HAS_EXCEPTIONS=0)

    # Debug: no optimization, full symbols
    set(CMAKE_CXX_FLAGS_DEBUG "/Od /Zi" CACHE STRING "" FORCE)
    
    # Release: full optimization, full symbols
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /Zi" CACHE STRING "" FORCE)
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/DEBUG /INCREMENTAL:NO /OPT:REF /OPT:ICF" CACHE STRING "" FORCE)

    # Dist: max optimization, whole program optimization, no symbols
    set(CMAKE_CXX_FLAGS_DIST "/O2 /GL" CACHE STRING "" FORCE)
    set(CMAKE_EXE_LINKER_FLAGS_DIST "/LTCG /OPT:REF /OPT:ICF" CACHE STRING "" FORCE)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -fno-exceptions -fno-rtti)

    # Debug: no optimization, full symbols
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g" CACHE STRING "" FORCE)

    # Release: full optimization, full symbols
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -g" CACHE STRING "" FORCE)

    # Dist: max optimization, LTO, no symbols
    set(CMAKE_CXX_FLAGS_DIST "-O3 -flto" CACHE STRING "" FORCE)
endif()

add_compile_definitions(
    $<$<CONFIG:Debug>:WDT_CONFIG_DEBUG>
    $<$<CONFIG:Release>:WDT_CONFIG_RELEASE>
    $<$<CONFIG:Dist>:WDT_CONFIG_DIST>
)

set(DEMO_PATH ${CMAKE_CURRENT_SOURCE_DIR}/demo/)
add_compile_definitions(DEMO_PATH="${DEMO_PATH}")
message(STATUS "Demos path: ${DEMO_PATH}")

# Manual PCH setup — avoids CMake's generated cmake_pch files polluting the VS solution tree
# wdt_target_pch: creates and uses PCH (for targets with pch.cpp)
function(wdt_target_pch target pch_header pch_source)
    if(MSVC)
        target_compile_options(${target} PRIVATE /Yu${pch_header} /FI${pch_header})
        set_source_files_properties(${pch_source} TARGET_DIRECTORY ${target} PROPERTIES COMPILE_FLAGS /Yc${pch_header})
    else()
        get_filename_component(_pch_dir "${pch_source}" DIRECTORY)
        target_compile_options(${target} PRIVATE -include ${_pch_dir}/${pch_header})
    endif()
endfunction()

add_subdirectory(src)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Wandelt)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "Dependencies")
