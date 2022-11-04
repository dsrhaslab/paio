# ---------------------------------------------------------------------------- #

cmake_minimum_required(VERSION 3.16)
include(FetchContent)

project(paio VERSION 0.1.0 DESCRIPTION "PAIO: General, Portable I/O Optimizations With Minor Application Modifications")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Setup the basic C++ Compiler flags
if (APPLE)
    message(STATUS "Detecting Local Environment (apple-clang)")
    option(IS_LOCAL "Install PAIO in local setup" ON)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
endif (APPLE)

if (UNIX AND NOT APPLE)
    message(STATUS "Detecting UNIX")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -Wl,--no-as-needed -ldl")
endif ()

set(warn_opts -Wall -Wextra -Wno-unused-parameter -Wno-implicit-fallthrough)

# Install PAIO to be used in production. This option will modify the 
# compilation options, regarding compiler-based optimizations.
option(PRODUCTION "Compiling with release mode" ON)

if (PRODUCTION) 
    message(STATUS "Compiling with compiler-based optimizations (-O3)")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
else() 
    message(STATUS "Compiling with default compiling options")
endif (PRODUCTION)

# Setup the options that CMake can take in
option(PAIO_INSTALL "Install PAIO header and library" ON)

# build static or shared lib option
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

# functional tests options
option(PAIO_BUILD_TESTS "Build PAIO unit tests" ON)

# benchmarking options
option(PAIO_BUILD_BENCHMARKS "Build PAIO benchmarks" ON)

# Test whether -Wthread-safety is available. See
# https://clang.llvm.org/docs/ThreadSafetyAnalysis.html
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag(-Wthread-safety HAVE_CLANG_THREAD_SAFETY)

# ---------------------------------------------------------------------------- #
# paio

if (BUILD_SHARED_LIBS)
    message(STATUS "Compiling shared libraries ...")
    add_library(paio SHARED "")
else()
    message(STATUS "Compiling static libraries ...")
    add_library(paio STATIC "")
endif(BUILD_SHARED_LIBS)

target_compile_options(paio PRIVATE ${warn_opts})

set_target_properties(
    paio
    PROPERTIES
    # CXX_VISIBILITY_PRESET hidden
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

target_include_directories(paio PUBLIC include)

if (HAVE_CLANG_THREAD_SAFETY)
    target_compile_options(paio PUBLIC -Wthread-safety)
endif (HAVE_CLANG_THREAD_SAFETY)

find_package(Threads REQUIRED)
target_link_libraries(paio Threads::Threads)

target_sources(
    paio
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include/paio/core/agent.hpp
    ${PROJECT_SOURCE_DIR}/include/paio/core/context.hpp
    ${PROJECT_SOURCE_DIR}/include/paio/core/context_propagation_definitions.hpp
    ${PROJECT_SOURCE_DIR}/include/paio/core/interface_definitions.hpp
    ${PROJECT_SOURCE_DIR}/include/paio/core/stage_info.hpp
    ${PROJECT_SOURCE_DIR}/include/paio/differentiation/channel_differentiation_tuple.hpp
    ${PROJECT_SOURCE_DIR}/include/paio/differentiation/channel_differentiation_builder.hpp
    ${PROJECT_SOURCE_DIR}/include/paio/differentiation/channel_hashing_differentiation.hpp
    ${PROJECT_SOURCE_DIR}/include/paio/differentiation/differentiation_token_builder.hpp
    ${PROJECT_SOURCE_DIR}/include/paio/differentiation/murmurhash_token_builder.hpp
    ${PROJECT_SOURCE_DIR}/include/paio/differentiation/enforcement_object_differentiation_pair.hpp
    ${PROJECT_SOURCE_DIR}/include/paio/differentiation/enforcement_object_differentiation_builder.hpp
    ${PROJECT_SOURCE_DIR}/include/paio/differentiation/enforcement_object_hashing_differentiation.hpp
    ${PROJECT_SOURCE_DIR}/include/paio/enforcement/channel.hpp
    ${PROJECT_SOURCE_DIR}/include/paio/enforcement/channel_default.hpp
    ${PROJECT_SOURCE_DIR}/include/paio/core/core.hpp
    ${PROJECT_SOURCE_DIR}/include/paio/enforcement/completion_queue.hpp
    ${PROJECT_SOURCE_DIR}/include/paio/enforcement/result.hpp
    ${PROJECT_SOURCE_DIR}/include/paio/enforcement/submission_queue.hpp
    ${PROJECT_SOURCE_DIR}/include/paio/enforcement/ticket.hpp
    ${PROJECT_SOURCE_DIR}/include/paio/enforcement/objects/enforcement_object.hpp
    ${PROJECT_SOURCE_DIR}/include/paio/enforcement/objects/noop/enforcement_object_noop.hpp
    ${PROJECT_SOURCE_DIR}/include/paio/enforcement/objects/drl/enforcement_object_drl.hpp
    ${PROJECT_SOURCE_DIR}/include/paio/enforcement/objects/drl/enforcement_object_drl_options.hpp
    ${PROJECT_SOURCE_DIR}/include/paio/enforcement/objects/drl/token_bucket.hpp
    ${PROJECT_SOURCE_DIR}/include/paio/enforcement/objects/drl/token_bucket_threaded.hpp
    ${PROJECT_SOURCE_DIR}/include/paio/enforcement/objects/drl/token_bucket_statistics.hpp
    ${PROJECT_SOURCE_DIR}/include/paio/enforcement/objects/drl/token_bucket_statistics_entry.hpp
    ${PROJECT_SOURCE_DIR}/include/paio/interface/instance_interface.hpp
    ${PROJECT_SOURCE_DIR}/include/paio/interface/lsm_kvs_layer.hpp
    ${PROJECT_SOURCE_DIR}/include/paio/interface/paio_instance.hpp
    ${PROJECT_SOURCE_DIR}/include/paio/interface/posix_layer.hpp
    ${PROJECT_SOURCE_DIR}/include/paio/options/libc_headers.hpp
    ${PROJECT_SOURCE_DIR}/include/paio/options/options.hpp
    ${PROJECT_SOURCE_DIR}/include/paio/rules/differentiation_rule.hpp
    ${PROJECT_SOURCE_DIR}/include/paio/rules/differentiation_table.hpp
    ${PROJECT_SOURCE_DIR}/include/paio/rules/enforcement_rule.hpp
    ${PROJECT_SOURCE_DIR}/include/paio/rules/housekeeping_rule.hpp
    ${PROJECT_SOURCE_DIR}/include/paio/rules/housekeeping_table.hpp
    ${PROJECT_SOURCE_DIR}/include/paio/networking/connection_handler.hpp
    ${PROJECT_SOURCE_DIR}/include/paio/networking/connection_manager.hpp
    ${PROJECT_SOURCE_DIR}/include/paio/networking/connection_options.hpp
    ${PROJECT_SOURCE_DIR}/include/paio/networking/handshake_connection_handler.hpp
    ${PROJECT_SOURCE_DIR}/include/paio/networking/southbound_connection_handler.hpp
    ${PROJECT_SOURCE_DIR}/include/paio/stage/paio_stage.hpp
    ${PROJECT_SOURCE_DIR}/include/paio/statistics/enforcement_object_statistics.hpp
    ${PROJECT_SOURCE_DIR}/include/paio/statistics/channel_statistics.hpp
    ${PROJECT_SOURCE_DIR}/include/paio/utils/logging.hpp
    ${PROJECT_SOURCE_DIR}/include/paio/utils/murmurhash.hpp
    ${PROJECT_SOURCE_DIR}/include/paio/utils/rules_parser.hpp
    ${PROJECT_SOURCE_DIR}/include/paio/utils/status.hpp
)

target_sources(
    paio
    PRIVATE
    src/core/agent.cpp
    src/core/core.cpp
    src/core/stage_info.cpp
    src/differentiation/murmurhash_token_builder.cpp
    src/enforcement/channel_default.cpp
    src/enforcement/completion_queue.cpp
    src/enforcement/result.cpp
    src/enforcement/submission_queue.cpp
    src/enforcement/ticket.cpp
    src/enforcement/objects/noop/enforcement_object_noop.cpp
    src/enforcement/objects/drl/enforcement_object_drl.cpp
    src/enforcement/objects/drl/token_bucket_statistics.cpp
    src/enforcement/objects/drl/token_bucket.cpp
    src/enforcement/objects/drl/token_bucket_threaded.cpp
    src/interface/lsm_kvs_layer.cpp
    src/interface/paio_instance.cpp
    src/interface/posix_layer.cpp
    src/networking/connection_manager.cpp
    src/networking/handshake_connection_handler.cpp
    src/networking/southbound_connection_handler.cpp
    src/rules/differentiation_rule.cpp
    src/rules/differentiation_table.cpp
    src/rules/enforcement_rule.cpp
    src/rules/housekeeping_rule.cpp
    src/rules/housekeeping_table.cpp
    src/stage/paio_stage.cpp
    src/statistics/enforcement_object_statistics.cpp
    src/statistics/channel_statistics.cpp
    src/utils/logging.cpp
    src/utils/murmurhash.cpp
    src/utils/rules_parser.cpp
    src/utils/status.cpp
)

# ---------------------------------------------------------------------------- #
# > spdlog --- logging library

if(${CMAKE_VERSION} VERSION_LESS "3.24.0") 
    FetchContent_Declare(spdlog
        URL             https://github.com/gabime/spdlog/archive/v1.8.1.tar.gz
        UPDATE_COMMAND  ""
        INSTALL_COMMAND ""
    )
else ()
    FetchContent_Declare(spdlog
        URL             https://github.com/gabime/spdlog/archive/v1.8.1.tar.gz
        UPDATE_COMMAND  ""
        INSTALL_COMMAND ""
        DOWNLOAD_EXTRACT_TIMESTAMP NEW
    )
endif ()

FetchContent_MakeAvailable(spdlog)
set_target_properties(spdlog PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(paio spdlog)

# ---------------------------------------------------------------------------- #
# > gflags --- commandline flag processing library

if (PAIO_BUILD_BENCHMARKS)
    message(STATUS "Building gflags lib ...")

    if(${CMAKE_VERSION} VERSION_LESS "3.24.0")
        FetchContent_Declare(gflags
            URL             https://github.com/gflags/gflags/archive/refs/tags/v2.2.2.tar.gz
            UPDATE_COMMAND  ""
            INSTALL_COMMAND ""
        )
    else ()
        FetchContent_Declare(gflags
            URL             https://github.com/gflags/gflags/archive/refs/tags/v2.2.2.tar.gz
            UPDATE_COMMAND  ""
            INSTALL_COMMAND ""
            DOWNLOAD_EXTRACT_TIMESTAMP NEW
        )
    endif ()

    FetchContent_MakeAvailable(gflags)
    target_link_libraries(paio gflags)
endif ()

# ---------------------------------------------------------------------------- #
# tests

if (PAIO_BUILD_TESTS)
    message(STATUS "Compiling PAIO tests ...")
    enable_testing()

    function(paio_test test_file exec_file)
        get_filename_component(test_target_name "${exec_file}" NAME_WE)

        add_executable("${test_target_name}" "")
        target_sources("${test_target_name}"
            PRIVATE
            "${test_file}"
            )

        target_compile_options("${test_target_name}" PRIVATE ${warn_opts})
        target_link_libraries("${test_target_name}" paio)

        add_test(NAME "${test_target_name}" COMMAND "${test_target_name}")
    endfunction(paio_test)

    paio_test("tests/paio_agent_test.cpp" "agent_test")
    paio_test("tests/paio_channel_default_test.cpp" "channel_test")
    paio_test("tests/paio_channel_statistics_test.cpp" "channel_statistics_test")
    paio_test("tests/paio_differentiation_table_test.cpp" "differentiation_table_test")
    paio_test("tests/paio_housekeeping_table_test.cpp" "housekeeping_table_test")
    paio_test("tests/paio_posix_layer_test.cpp" "posix_layer_test")
    paio_test("tests/paio_token_bucket_test.cpp" "token_bucket_test")
    paio_test("tests/paio_rules_file_parser_test.cpp" "rule_parser_test")
    paio_test("tests/paio_stage_info_test.cpp" "stage_info_test")
    paio_test("tests/paio_status_test.cpp" "status_test")
    paio_test("tests/paio_southbound_interface_test.cpp" "southbound_test")
    paio_test("tests/paio_token_bucket_threaded_test.cpp" "token_bucket_threaded_test")
    paio_test("tests/rocksdb_simulation_test.cpp" "rocksdb_test")

endif (PAIO_BUILD_TESTS)


# ---------------------------------------------------------------------------- #
# benchmarks

if (PAIO_BUILD_BENCHMARKS)
    message(STATUS "Compiling PAIO benchmarks ...")
    enable_testing()

    function(paio_bench bench_file exec_file)
        get_filename_component(test_target_name "${exec_file}" NAME_WE)

        add_executable("${test_target_name}" "")
        target_sources("${test_target_name}"
                PRIVATE
                "${bench_file}"
                )

        target_compile_options("${test_target_name}" PRIVATE ${warn_opts})
        target_link_libraries("${test_target_name}" paio)

        add_test(NAME "${test_target_name}" COMMAND "${test_target_name}")
    endfunction(paio_bench)

    paio_bench("benchmarking/murmurhash_performance_test.cpp" "murmur_bench")
    paio_bench("benchmarking/paio_microbenchmarking.cpp" "paio_bench")
    paio_bench("benchmarking/paio_object_drl_test.cpp" "drl_bench")
    paio_bench("benchmarking/paio_object_noop_test.cpp" "noop_bench")

endif (PAIO_BUILD_BENCHMARKS)

# ---------------------------------------------------------------------------- #
# install

if (PAIO_INSTALL)
    message(STATUS "Installing libpaio ...")
    include(GNUInstallDirs)
    install(
        TARGETS paio
        EXPORT paioTargets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        )
    install(DIRECTORY include/paio TYPE INCLUDE)
endif (PAIO_INSTALL)

# ---------------------------------------------------------------------------- #
