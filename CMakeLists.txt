cmake_minimum_required(VERSION 3.28)
project(ipa
    VERSION 0.0.0
    DESCRIPTION "Inline partial application for monadic operations"
    LANGUAGES C CXX)

add_library(ipa INTERFACE)
add_library(ipa::ipa ALIAS ipa)

target_compile_features(ipa INTERFACE cxx_std_23)

target_include_directories(ipa INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

option(IPA_DEV_MODE "Enable strict checks" OFF)
if (IPA_DEV_MODE)
    target_compile_options(ipa INTERFACE
        # GCC and Clang
        $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:
        -Wall
        -Wextra
        -Wpedantic
        -Werror
        >

        # MSVC
        $<$<CXX_COMPILER_ID:MSVC>:
        /W4
        /WX
        >
    )

    set_target_properties(ipa PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )
endif ()

option(IPA_DEPENDENCIES_FROM_CPM "Use CPM for obtaining 3rd-party dependencies" OFF)
if (IPA_DEPENDENCIES_FROM_CPM)
    if (NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake")
        message(STATUS "Downloading CPM.cmake...")
        file(DOWNLOAD
            https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.42.1/CPM.cmake
            "${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake"
            EXPECTED_HASH SHA256=f3a6dcc6a04ce9e7f51a127307fa4f699fb2bade357a8eb4c5b45df76e1dc6a5
        )
    endif ()
    include("${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake")

    CPMAddPackage("gh:catchorg/Catch2@3.12.0")
    # For catch_discover_tests
    list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)
endif ()

option(IPA_ENABLE_TESTING "Enable testing" OFF)
if (IPA_ENABLE_TESTING)
    enable_testing()
    add_subdirectory(test)
endif ()
