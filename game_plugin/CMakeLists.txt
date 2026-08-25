cmake_minimum_required(VERSION 3.25)

project(SR2ArchipelagoGame LANGUAGES C CXX)

option(SR2AP_BUILD_TESTS "Build portable unit tests" ON)
option(SR2AP_BUILD_RUST "Build the Rust support library" ON)
if(SR2AP_BUILD_TESTS)
        enable_testing()
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(WIN32)
        if(NOT CMAKE_C_COMPILER_ID STREQUAL "MSVC" OR NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
                message(FATAL_ERROR "The SR2Archipelago game integration requires Microsoft Visual C++")
        endif()
        if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
                message(FATAL_ERROR "The SR2Archipelago game integration requires 32-bit x86 MSVC")
        endif()

        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>")
        add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/utf-8> $<$<COMPILE_LANGUAGE:CXX>:/EHsc>)
endif()

if(WIN32 AND SR2AP_BUILD_RUST)
        find_program(CARGO_EXECUTABLE NAMES cargo REQUIRED)

        set(SR2AP_RUST_TARGET "i686-pc-windows-msvc" CACHE STRING "Rust target triple for the game plugin")
        set(SR2AP_RUST_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/rust")
        set(SR2AP_RUST_TARGET_DIR "${CMAKE_CURRENT_BINARY_DIR}/cargo-target")
        set(SR2AP_RUST_CXX_INCLUDE_DIR "${SR2AP_RUST_TARGET_DIR}/cxxbridge")
        set(SR2AP_RUST_CXX_HEADER "${SR2AP_RUST_CXX_INCLUDE_DIR}/sr2ap/src/ffi.rs.h")
        set(SR2AP_RUST_CXX_RUNTIME_HEADER "${SR2AP_RUST_CXX_INCLUDE_DIR}/rust/cxx.h")

        file(MAKE_DIRECTORY "${SR2AP_RUST_CXX_INCLUDE_DIR}")

        set(SR2AP_RUST_DEBUG_LIBRARY
                "${SR2AP_RUST_TARGET_DIR}/${SR2AP_RUST_TARGET}/debug/sr2ap_rust.lib")
        set(SR2AP_RUST_RELEASE_LIBRARY
                "${SR2AP_RUST_TARGET_DIR}/${SR2AP_RUST_TARGET}/release/sr2ap_rust.lib")
        set(SR2AP_RUST_LIBRARY
                "$<IF:$<CONFIG:Debug>,${SR2AP_RUST_DEBUG_LIBRARY},${SR2AP_RUST_RELEASE_LIBRARY}>")
        file(GLOB_RECURSE SR2AP_RUST_SOURCES CONFIGURE_DEPENDS
                "${SR2AP_RUST_SOURCE_DIR}/src/*.rs"
        )

        add_custom_command(
                OUTPUT
                "${SR2AP_RUST_LIBRARY}"
                "${SR2AP_RUST_CXX_HEADER}"
                "${SR2AP_RUST_CXX_RUNTIME_HEADER}"
                COMMAND ${CMAKE_COMMAND} -E env
                "CARGO_TARGET_DIR=${SR2AP_RUST_TARGET_DIR}"
                "CXXFLAGS=$<$<CONFIG:Debug>:/MTd /D_ITERATOR_DEBUG_LEVEL=2>"
                "${CARGO_EXECUTABLE}" build
                --locked
                --manifest-path "${SR2AP_RUST_SOURCE_DIR}/Cargo.toml"
                --target "${SR2AP_RUST_TARGET}"
                $<$<NOT:$<CONFIG:Debug>>:--release>
                DEPENDS
                ${SR2AP_RUST_SOURCES}
                "${SR2AP_RUST_SOURCE_DIR}/Cargo.toml"
                "${SR2AP_RUST_SOURCE_DIR}/Cargo.lock"
                "${SR2AP_RUST_SOURCE_DIR}/.cargo/config.toml"
                "${SR2AP_RUST_SOURCE_DIR}/build.rs"
                WORKING_DIRECTORY "${SR2AP_RUST_SOURCE_DIR}"
                COMMENT "Building Rust support library"
                COMMAND_EXPAND_LISTS
                VERBATIM
        )

        add_custom_target(sr2ap_rust_build
                DEPENDS
                "${SR2AP_RUST_LIBRARY}"
                "${SR2AP_RUST_CXX_HEADER}"
                "${SR2AP_RUST_CXX_RUNTIME_HEADER}"
        )

        add_library(sr2ap_rust STATIC IMPORTED GLOBAL)
        set_target_properties(sr2ap_rust PROPERTIES
                IMPORTED_CONFIGURATIONS "DEBUG;RELEASE;RELWITHDEBINFO;MINSIZEREL"
                IMPORTED_LOCATION_DEBUG "${SR2AP_RUST_DEBUG_LIBRARY}"
                IMPORTED_LOCATION_RELEASE "${SR2AP_RUST_RELEASE_LIBRARY}"
                IMPORTED_LOCATION_RELWITHDEBINFO "${SR2AP_RUST_RELEASE_LIBRARY}"
                IMPORTED_LOCATION_MINSIZEREL "${SR2AP_RUST_RELEASE_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${SR2AP_RUST_CXX_INCLUDE_DIR}"
        )
        add_dependencies(sr2ap_rust sr2ap_rust_build)
endif()

file(GLOB_RECURSE SR2AP_FORMAT_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp"
)

find_program(CLANG_FORMAT_EXECUTABLE NAMES clang-format)
if(CLANG_FORMAT_EXECUTABLE)
        add_custom_target(format-game
                COMMAND ${CLANG_FORMAT_EXECUTABLE} -i --style=file ${SR2AP_FORMAT_SOURCES}
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                COMMENT "Formatting game C++ sources"
                VERBATIM
        )
        add_custom_target(check-format-game
                COMMAND ${CLANG_FORMAT_EXECUTABLE} --dry-run --Werror --style=file ${SR2AP_FORMAT_SOURCES}
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                COMMENT "Checking game C++ source formatting"
                VERBATIM
        )
else()
        add_custom_target(format-game
                COMMAND ${CMAKE_COMMAND} -E echo "clang-format was not found on PATH; install it and configure CMake again"
                COMMAND ${CMAKE_COMMAND} -E false
                VERBATIM
        )
        add_custom_target(check-format-game
                COMMAND ${CMAKE_COMMAND} -E echo "clang-format was not found on PATH; install it and configure CMake again"
                COMMAND ${CMAKE_COMMAND} -E false
                VERBATIM
        )
endif()

add_library(sr2ap_inih STATIC vendor/inih/ini.c)
target_include_directories(sr2ap_inih PUBLIC vendor/inih)

add_library(sr2ap_nlohmann_json INTERFACE)
target_include_directories(sr2ap_nlohmann_json INTERFACE vendor/nlohmann-json)

add_library(sr2ap_cxx INTERFACE)
target_include_directories(sr2ap_cxx INTERFACE vendor/cxx)

add_library(sr2ap_core STATIC
        src/Config.cpp
        src/ArchipelagoProtocol.cpp
        src/ReaderResult.cpp
        src/CollectibleDefinitions.cpp
        src/StatusSerialization.cpp
        src/ProgressionEventTracker.cpp
        src/RevisionJournal.cpp
)
target_include_directories(sr2ap_core PUBLIC include)
target_link_libraries(sr2ap_core PRIVATE sr2ap_inih sr2ap_nlohmann_json)
target_compile_features(sr2ap_core PUBLIC cxx_std_17)

if(WIN32)
        add_library(sr2ap_safetyhook STATIC
                vendor/safetyhook/safetyhook.cpp
                vendor/safetyhook/Zydis.c
        )
        target_include_directories(sr2ap_safetyhook PUBLIC vendor/safetyhook)
        target_compile_features(sr2ap_safetyhook PUBLIC cxx_std_23)

        add_library(SR2Archipelago SHARED
                src/dllmain.cpp
                src/Plugin.cpp
                src/Logger.cpp
                src/AtomicFile.cpp
                src/ModuleInfo.cpp
                src/Memory.cpp
                src/GameState.cpp
                src/Hitman.cpp
                src/ChopShop.cpp
                src/Missions.cpp
                src/Status.cpp
                src/Progression.cpp
                src/ProgressionMonitor.cpp
                src/Activities.cpp
                src/Racing.cpp
                src/Collectibles.cpp
                src/ArchipelagoTcpClient.cpp
                src/Cheats.cpp
                src/Notoriety.cpp
                src/Respect.cpp
                src/SaveRevisionMonitor.cpp
                src/Unlockables.cpp
        )
        target_include_directories(SR2Archipelago PRIVATE include)
        target_link_libraries(SR2Archipelago PRIVATE
                sr2ap_core
                sr2ap_nlohmann_json
                sr2ap_safetyhook
                ntdll
                userenv
                ws2_32
        )
        if(SR2AP_BUILD_RUST)
                target_link_libraries(SR2Archipelago PRIVATE sr2ap_rust)
                add_dependencies(SR2Archipelago sr2ap_rust_build)
        endif()
        target_compile_features(SR2Archipelago PRIVATE cxx_std_23)
        target_compile_definitions(SR2Archipelago PRIVATE
                WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE
                SR2AP_VERSION="${SR2AP_PROJECT_VERSION}"
        )
        target_compile_options(SR2Archipelago PRIVATE /W4 /permissive-)
        target_link_options(SR2Archipelago PRIVATE /INCREMENTAL:NO)
        set_target_properties(SR2Archipelago PROPERTIES PREFIX "" SUFFIX ".asi")
endif()

if(SR2AP_BUILD_TESTS)
        include(FetchContent)
        FetchContent_Declare(
                googletest
                URL https://github.com/google/googletest/archive/refs/tags/v1.18.0.tar.gz
                DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)

        add_executable(sr2ap_tests
                tests/ArchipelagoProtocolTests.cpp
                tests/BaselineTrackerTests.cpp
                tests/CollectibleDefinitionsTests.cpp
                tests/ConfigTests.cpp
                tests/HandlerActivityTests.cpp
                tests/ProgressionEventTrackerTests.cpp
                tests/ReaderResultTests.cpp
                tests/RevisionJournalTests.cpp
                tests/StatusSerializationTests.cpp
        )
        target_link_libraries(sr2ap_tests PRIVATE sr2ap_core GTest::gtest_main)

        include(GoogleTest)
        gtest_discover_tests(sr2ap_tests)

        if(WIN32 AND CMAKE_HOST_WIN32 AND SR2AP_BUILD_RUST)
                add_test(
                        NAME sr2ap_rust_tests
                        COMMAND ${CMAKE_COMMAND} -E env
                        "CARGO_TARGET_DIR=${SR2AP_RUST_TARGET_DIR}"
                        "${CARGO_EXECUTABLE}" test
                        --locked
                        --manifest-path "${SR2AP_RUST_SOURCE_DIR}/Cargo.toml"
                        --target "${SR2AP_RUST_TARGET}"
                        $<$<NOT:$<CONFIG:Debug>>:--release>
                        COMMAND_EXPAND_LISTS
                )
        endif()
endif()
