cmake_minimum_required(VERSION 3.16)
project(urbandrop_mini1a LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(ENABLE_WARNINGS "Enable strict compiler warnings" ON)
option(BUILD_TESTING "Build tests" ON)
option(ENABLE_OPENMP "Enable OpenMP acceleration when available" ON)
option(REQUIRE_OPENMP "Fail configure if OpenMP is unavailable" OFF)

function(urbandrop_enable_warnings target_name)
  if(ENABLE_WARNINGS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
      target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
  endif()
endfunction()

set(URBANDROP_OPENMP_ENABLED OFF)
if(ENABLE_OPENMP)
  find_package(OpenMP)
  if(OpenMP_CXX_FOUND)
    set(URBANDROP_OPENMP_ENABLED ON)
  elseif(REQUIRE_OPENMP)
    message(FATAL_ERROR
      "OpenMP requested but not found.\n"
      "Compiler: ${CMAKE_CXX_COMPILER_ID} (${CMAKE_CXX_COMPILER})\n"
      "Hints:\n"
      "  - Use a compiler toolchain with OpenMP support (for example GCC/g++).\n"
      "  - If switching compilers, configure in a fresh build directory.\n"
      "  - Or re-run with -DREQUIRE_OPENMP=OFF to build without OpenMP acceleration.")
  else()
    message(WARNING
      "OpenMP not found. Building without OpenMP acceleration.\n"
      "Parallel code paths will still build, but run without OpenMP speedups.")
  endif()
endif()

add_library(congestion_core
  src/data_model/TrafficRecord.cpp
  src/data_model/TrafficDataset.cpp
  src/data_model/TrafficColumns.cpp
  src/data_model/TrafficDatasetOptimized.cpp
  src/benchmark/BenchmarkHarness.cpp
  src/io/CSVReader.cpp
  src/io/CsvParseUtils.cpp
  src/io/GarageLoader.cpp
  src/io/BuildingLoader.cpp
  src/query/TimeWindowQuery.cpp
  src/query/CongestionQuery.cpp
  src/query/TrafficAggregator.cpp
  src/query/CrossDatasetIndex.cpp
  src/query/OptimizedCongestionQuery.cpp
  src/query/OptimizedQuerySupport.cpp
  src/query/OptimizedTrafficAggregator.cpp
  src/query/ParallelCongestionQuery.cpp
  src/query/ParallelTrafficAggregator.cpp
)

target_include_directories(congestion_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(URBANDROP_OPENMP_ENABLED)
  target_link_libraries(congestion_core PUBLIC OpenMP::OpenMP_CXX)
endif()

urbandrop_enable_warnings(congestion_core)

add_executable(run_serial apps/run_serial.cpp)
target_link_libraries(run_serial PRIVATE congestion_core)
urbandrop_enable_warnings(run_serial)

add_executable(run_parallel apps/run_parallel.cpp)
target_link_libraries(run_parallel PRIVATE congestion_core)
urbandrop_enable_warnings(run_parallel)

add_executable(run_optimized apps/run_optimized.cpp)
target_link_libraries(run_optimized PRIVATE congestion_core)
urbandrop_enable_warnings(run_optimized)

add_executable(run_index_experiments apps/run_index_experiments.cpp)
target_link_libraries(run_index_experiments PRIVATE congestion_core)
urbandrop_enable_warnings(run_index_experiments)

add_executable(run_optimized_support_experiments apps/run_optimized_support_experiments.cpp)
target_link_libraries(run_optimized_support_experiments PRIVATE congestion_core)
urbandrop_enable_warnings(run_optimized_support_experiments)

include(CTest)
if(BUILD_TESTING)
  add_executable(test_traffic_dataset tests/test_traffic_dataset.cpp)
  target_link_libraries(test_traffic_dataset PRIVATE congestion_core)
  urbandrop_enable_warnings(test_traffic_dataset)

  add_executable(test_csv_reader tests/test_csv_reader.cpp)
  target_link_libraries(test_csv_reader PRIVATE congestion_core)
  urbandrop_enable_warnings(test_csv_reader)

  add_executable(test_support_loaders tests/test_support_loaders.cpp)
  target_link_libraries(test_support_loaders PRIVATE congestion_core)
  urbandrop_enable_warnings(test_support_loaders)

  add_executable(test_query_apis tests/test_query_apis.cpp)
  target_link_libraries(test_query_apis PRIVATE congestion_core)
  urbandrop_enable_warnings(test_query_apis)

  add_executable(test_benchmark_harness tests/test_benchmark_harness.cpp)
  target_link_libraries(test_benchmark_harness PRIVATE congestion_core)
  urbandrop_enable_warnings(test_benchmark_harness)

  add_executable(test_run_serial_benchmark_cli tests/test_run_serial_benchmark_cli.cpp)
  urbandrop_enable_warnings(test_run_serial_benchmark_cli)
  add_executable(test_parallel_query_correctness tests/test_parallel_query_correctness.cpp)
  target_link_libraries(test_parallel_query_correctness PRIVATE congestion_core)
  urbandrop_enable_warnings(test_parallel_query_correctness)

  add_executable(test_run_parallel_cli tests/test_run_parallel_cli.cpp)
  urbandrop_enable_warnings(test_run_parallel_cli)
  add_executable(test_optimized_query_correctness tests/test_optimized_query_correctness.cpp)
  target_link_libraries(test_optimized_query_correctness PRIVATE congestion_core)
  urbandrop_enable_warnings(test_optimized_query_correctness)
  add_executable(test_optimized_query_support tests/test_optimized_query_support.cpp)
  target_link_libraries(test_optimized_query_support PRIVATE congestion_core)
  urbandrop_enable_warnings(test_optimized_query_support)
  add_executable(test_run_optimized_cli tests/test_run_optimized_cli.cpp)
  urbandrop_enable_warnings(test_run_optimized_cli)
  add_executable(test_cross_dataset_index tests/test_cross_dataset_index.cpp)
  target_link_libraries(test_cross_dataset_index PRIVATE congestion_core)
  urbandrop_enable_warnings(test_cross_dataset_index)

  add_executable(test_realdata_ingest tests/test_realdata_ingest.cpp)
  target_link_libraries(test_realdata_ingest PRIVATE congestion_core)
  urbandrop_enable_warnings(test_realdata_ingest)

  add_executable(test_realdata_api tests/test_realdata_api.cpp)
  target_link_libraries(test_realdata_api PRIVATE congestion_core)
  urbandrop_enable_warnings(test_realdata_api)

  add_executable(test_run_serial_queries tests/test_run_serial_queries.cpp)
  target_link_libraries(test_run_serial_queries PRIVATE congestion_core)
  urbandrop_enable_warnings(test_run_serial_queries)

  add_executable(test_realdata_integrated tests/test_realdata_integrated.cpp)
  target_link_libraries(test_realdata_integrated PRIVATE congestion_core)
  urbandrop_enable_warnings(test_realdata_integrated)

  add_test(NAME test_traffic_dataset COMMAND test_traffic_dataset)
  add_test(NAME test_csv_reader COMMAND test_csv_reader)
  add_test(NAME test_support_loaders COMMAND test_support_loaders)
  add_test(NAME test_query_apis COMMAND test_query_apis)
  add_test(NAME test_benchmark_harness COMMAND test_benchmark_harness)
  add_test(NAME test_run_serial_benchmark_cli COMMAND test_run_serial_benchmark_cli)
  add_test(NAME test_parallel_query_correctness COMMAND test_parallel_query_correctness)
  add_test(NAME test_run_parallel_cli COMMAND test_run_parallel_cli)
  add_test(NAME test_optimized_query_correctness COMMAND test_optimized_query_correctness)
  add_test(NAME test_optimized_query_support COMMAND test_optimized_query_support)
  add_test(NAME test_run_optimized_cli COMMAND test_run_optimized_cli)
  add_test(NAME test_cross_dataset_index COMMAND test_cross_dataset_index)
  add_test(NAME test_realdata_ingest COMMAND test_realdata_ingest)
  add_test(NAME test_realdata_api COMMAND test_realdata_api)
  add_test(NAME test_realdata_cli COMMAND test_run_serial_queries)
  add_test(NAME test_realdata_integrated COMMAND test_realdata_integrated)
endif()
