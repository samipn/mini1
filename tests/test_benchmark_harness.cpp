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

std::size_t CountLines(const std::string& path) {
  std::ifstream in(path);
  std::size_t lines = 0;
  std::string line;
  while (std::getline(in, line)) {
    ++lines;
  }
  return lines;
}

}  // namespace

int main() {
  const std::string csv_path = WriteFixtureCsv();
  const std::string out_path =
      (std::filesystem::temp_directory_path() / "urbandrop_benchmark_output.csv").string();

  urbandrop::BenchmarkConfig config;
  config.dataset_path = csv_path;
  config.dataset_label = "fixture";
  config.query_type = "speed_below";
  config.has_threshold = true;
  config.threshold_mph = 1000.0;
  config.runs = 3;
  config.output_csv_path = out_path;
  config.append_output = false;
  config.has_expected_accepted_rows = true;
  config.expected_accepted_rows = 3;

  std::vector<urbandrop::BenchmarkRunResult> results;
  std::string error;
  if (!urbandrop::BenchmarkHarness::RunSerial(config, &results, &error)) {
    std::cerr << "BenchmarkHarness failed: " << error << "\n";
    return EXIT_FAILURE;
  }

  if (results.size() != 3) {
    std::cerr << "expected 3 benchmark runs\n";
    return EXIT_FAILURE;
  }

  for (const auto& row : results) {
    if (row.rows_read != 3 || row.rows_accepted != 3 || row.rows_rejected != 0 || row.result_count != 3) {
      std::cerr << "unexpected benchmark row counters\n";
      return EXIT_FAILURE;
    }
  }

  if (!std::filesystem::exists(out_path)) {
    std::cerr << "benchmark output CSV not found\n";
    return EXIT_FAILURE;
  }
  if (CountLines(out_path) != 4) {
    std::cerr << "benchmark output CSV should have header + 3 rows\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
