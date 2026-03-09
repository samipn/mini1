#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "benchmark/BenchmarkHarness.hpp"

namespace {

std::string WriteFixtureCsv() {
  const auto path = std::filesystem::temp_directory_path() / "urbandrop_benchmark_fixture.csv";
  std::ofstream out(path);
  out << "id,speed,travel_time,status,data_as_of,link_id,borough,link_name\n";
  out << "1,10.0,30.0,ok,2024-01-01T00:00:00,100,Manhattan,Link A\n";
  out << "2,20.0,20.0,ok,2024-01-01T00:01:00,200,Queens,Link B\n";
  out << "3,30.0,10.0,ok,2024-01-01T00:02:00,300,Brooklyn,Link C\n";
  return path.string();
}

std::vector<std::string> ReadAllLines(const std::string& path) {
  std::ifstream in(path);
  std::vector<std::string> lines;
  std::string line;
  while (std::getline(in, line)) {
    lines.push_back(line);
  }
  return lines;
}

bool Contains(const std::string& haystack, const std::string& needle) {
  return haystack.find(needle) != std::string::npos;
}

}  // namespace

int main() {
  const std::string csv_path = WriteFixtureCsv();
  const std::string serial_out_path =
      (std::filesystem::temp_directory_path() / "urbandrop_benchmark_serial_output.csv").string();
  const std::string parallel_out_path =
      (std::filesystem::temp_directory_path() / "urbandrop_benchmark_parallel_output.csv").string();

  urbandrop::BenchmarkConfig serial_config;
  serial_config.dataset_path = csv_path;
  serial_config.dataset_label = "fixture";
  serial_config.query_type = "speed_below";
  serial_config.has_threshold = true;
  serial_config.threshold_mph = 1000.0;
  serial_config.runs = 3;
  serial_config.output_csv_path = serial_out_path;
  serial_config.append_output = false;
  serial_config.has_expected_accepted_rows = true;
  serial_config.expected_accepted_rows = 3;

  std::vector<urbandrop::BenchmarkRunResult> serial_results;
  std::string error;
  if (!urbandrop::BenchmarkHarness::RunSerial(serial_config, &serial_results, &error)) {
    std::cerr << "BenchmarkHarness serial failed: " << error << "\n";
    return EXIT_FAILURE;
  }

  if (serial_results.size() != 3) {
    std::cerr << "expected 3 serial benchmark runs\n";
    return EXIT_FAILURE;
  }

  for (const auto& row : serial_results) {
    if (row.execution_mode != urbandrop::BenchmarkExecutionMode::kSerial || row.thread_count != 1 ||
        row.rows_read != 3 || row.rows_accepted != 3 || row.rows_rejected != 0 || row.result_count != 3 ||
        !row.serial_match) {
      std::cerr << "unexpected serial benchmark row counters\n";
      return EXIT_FAILURE;
    }
  }

  const auto serial_lines = ReadAllLines(serial_out_path);
  if (serial_lines.size() != 4) {
    std::cerr << "serial benchmark output CSV should have header + 3 rows\n";
    return EXIT_FAILURE;
  }
  if (!Contains(serial_lines[0], "execution_mode") || !Contains(serial_lines[0], "thread_count") ||
      !Contains(serial_lines[0], "serial_match")) {
    std::cerr << "serial CSV header missing new benchmark columns\n";
    return EXIT_FAILURE;
  }

  urbandrop::BenchmarkConfig parallel_config;
  parallel_config.binary_name = "run_parallel";
  parallel_config.execution_mode = urbandrop::BenchmarkExecutionMode::kParallel;
  parallel_config.dataset_path = csv_path;
  parallel_config.dataset_label = "fixture";
  parallel_config.query_type = "summary";
  parallel_config.runs = 2;
  parallel_config.thread_count = 2;
  parallel_config.validate_parallel_against_serial = true;
  parallel_config.output_csv_path = parallel_out_path;
  parallel_config.append_output = false;
  parallel_config.has_expected_accepted_rows = true;
  parallel_config.expected_accepted_rows = 3;

  std::vector<urbandrop::BenchmarkRunResult> parallel_results;
  if (!urbandrop::BenchmarkHarness::RunParallel(parallel_config, &parallel_results, &error)) {
    std::cerr << "BenchmarkHarness parallel failed: " << error << "\n";
    return EXIT_FAILURE;
  }

  if (parallel_results.size() != 2) {
    std::cerr << "expected 2 parallel benchmark runs\n";
    return EXIT_FAILURE;
  }

  for (const auto& row : parallel_results) {
    if (row.execution_mode != urbandrop::BenchmarkExecutionMode::kParallel || row.thread_count != 2 ||
        row.rows_read != 3 || row.rows_accepted != 3 || row.rows_rejected != 0 || row.result_count != 3 ||
        !row.serial_match) {
      std::cerr << "unexpected parallel benchmark row counters\n";
      return EXIT_FAILURE;
    }
  }

  const auto parallel_lines = ReadAllLines(parallel_out_path);
  if (parallel_lines.size() != 3) {
    std::cerr << "parallel benchmark output CSV should have header + 2 rows\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
