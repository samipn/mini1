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

add_library(congestion_core
  src/data_model/TrafficRecord.cpp
  src/data_model/TrafficDataset.cpp
  src/io/CSVReader.cpp
  src/io/GarageLoader.cpp
  src/io/BuildingLoader.cpp
  src/query/TimeWindowQuery.cpp
  src/query/CongestionQuery.cpp
  src/query/TrafficAggregator.cpp
)

target_include_directories(congestion_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(ENABLE_WARNINGS)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(congestion_core PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endif()

add_executable(run_serial apps/run_serial.cpp)
target_link_libraries(run_serial PRIVATE congestion_core)

add_executable(run_parallel apps/run_parallel.cpp)
target_link_libraries(run_parallel PRIVATE congestion_core)

add_executable(run_optimized apps/run_optimized.cpp)
target_link_libraries(run_optimized PRIVATE congestion_core)

include(CTest)
if(BUILD_TESTING)
  add_executable(test_traffic_dataset tests/test_traffic_dataset.cpp)
  target_link_libraries(test_traffic_dataset PRIVATE congestion_core)

  add_executable(test_csv_reader tests/test_csv_reader.cpp)
  target_link_libraries(test_csv_reader PRIVATE congestion_core)

  add_executable(test_support_loaders tests/test_support_loaders.cpp)
  target_link_libraries(test_support_loaders PRIVATE congestion_core)

  add_executable(test_query_apis tests/test_query_apis.cpp)
  target_link_libraries(test_query_apis PRIVATE congestion_core)

  add_executable(test_realdata_ingest tests/test_realdata_ingest.cpp)
  target_link_libraries(test_realdata_ingest PRIVATE congestion_core)

  add_executable(test_realdata_api tests/test_realdata_api.cpp)
  target_link_libraries(test_realdata_api PRIVATE congestion_core)

  add_executable(test_run_serial_queries tests/test_run_serial_queries.cpp)
  target_link_libraries(test_run_serial_queries PRIVATE congestion_core)

  add_executable(test_realdata_integrated tests/test_realdata_integrated.cpp)
  target_link_libraries(test_realdata_integrated PRIVATE congestion_core)

  add_test(NAME test_traffic_dataset COMMAND test_traffic_dataset)
  add_test(NAME test_csv_reader COMMAND test_csv_reader)
  add_test(NAME test_support_loaders COMMAND test_support_loaders)
  add_test(NAME test_query_apis COMMAND test_query_apis)
  add_test(NAME test_realdata_ingest COMMAND test_realdata_ingest)
  add_test(NAME test_realdata_api COMMAND test_realdata_api)
  add_test(NAME test_realdata_cli COMMAND test_run_serial_queries)
  add_test(NAME test_realdata_integrated COMMAND test_realdata_integrated)
endif()
