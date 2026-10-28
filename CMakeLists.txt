cmake_minimum_required(VERSION 3.24)
project(historical_risk VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<1:bin>")
include(CTest)
include(FetchContent)
find_package(Python3 3.9 REQUIRED COMPONENTS Interpreter)
option(RISK_BUILD_BENCHMARKS "Build sort versus partition risk benchmarks" ON)
option(RISK_BUILD_DOCS "Generate checked Doxygen documentation" ON)

add_library(risk_core src/analytics.cpp)
target_include_directories(risk_core PUBLIC "${PROJECT_SOURCE_DIR}/include")
if(MSVC)
    target_compile_options(risk_core PRIVATE /W4)
else()
    target_compile_options(risk_core PRIVATE -Wall -Wextra -Wpedantic)
endif()
add_executable(risk_analytics src/main.cpp)
target_link_libraries(risk_analytics PRIVATE risk_core)

if(BUILD_TESTING)
    set(DOCTEST_WITH_TESTS OFF CACHE BOOL "" FORCE)
    set(DOCTEST_NO_INSTALL ON CACHE BOOL "" FORCE)
    FetchContent_Declare(doctest
        URL "${PROJECT_SOURCE_DIR}/third_party/archives/doctest-2.5.3.tar.gz"
        URL_HASH SHA256=174ebc4e769928959614789c5b4e9c3d0a0f81a62bb608756b127bfebfb21331
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(doctest)
    add_executable(risk_tests tests/test_risk.cpp)
    target_link_libraries(risk_tests PRIVATE risk_core doctest::doctest)
    include("${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake")
    doctest_discover_tests(risk_tests)
    add_test(NAME cli_decimal_oracle
        COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tests/check_cli.py"
                --app "$<TARGET_FILE:risk_analytics>"
                --data-dir "${PROJECT_SOURCE_DIR}/data")
    set_tests_properties(cli_decimal_oracle PROPERTIES TIMEOUT 60)
    add_test(NAME workflow_evidence_checks
        COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1
                "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tests/test_workflow_tools.py")
    set_tests_properties(workflow_evidence_checks PROPERTIES TIMEOUT 30)
endif()

if(RISK_BUILD_BENCHMARKS)
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_DOWNLOAD_DEPENDENCIES OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_WERROR OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(benchmark
        URL "${PROJECT_SOURCE_DIR}/third_party/archives/benchmark-1.9.5.tar.gz"
        URL_HASH SHA256=9631341c82bac4a288bef951f8b26b41f69021794184ece969f8473977eaa340
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(benchmark)
    add_executable(risk_benchmark benchmarks/benchmark_risk.cpp)
    target_link_libraries(risk_benchmark PRIVATE risk_core benchmark::benchmark)
    add_custom_target(run_benchmarks
        COMMAND "$<TARGET_FILE:risk_benchmark>" --benchmark_min_time=0.02s
                --benchmark_repetitions=3
                "--benchmark_out=${CMAKE_BINARY_DIR}/benchmark.json"
                --benchmark_out_format=json
        COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/scripts/check_benchmark.py"
                "${CMAKE_BINARY_DIR}/benchmark.json"
        DEPENDS risk_benchmark
        USES_TERMINAL VERBATIM
        COMMENT "Compare full sorting and partition selection; validate JSON")
endif()

if(RISK_BUILD_DOCS)
    find_package(Doxygen REQUIRED)
    configure_file(Doxyfile.in "${CMAKE_BINARY_DIR}/Doxyfile" @ONLY)
    add_custom_target(docs
        COMMAND "${DOXYGEN_EXECUTABLE}" "${CMAKE_BINARY_DIR}/Doxyfile"
        COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/scripts/check_docs.py"
                "${CMAKE_BINARY_DIR}/docs/html"
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        USES_TERMINAL VERBATIM
        COMMENT "Generate Doxygen HTML and validate local links")
endif()
