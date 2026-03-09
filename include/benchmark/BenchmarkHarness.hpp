#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace urbandrop {

enum class BenchmarkExecutionMode {
  kSerial,
  kParallel,
  kOptimized
};

struct BenchmarkConfig {
  std::string binary_name = "run_serial";
  BenchmarkExecutionMode execution_mode = BenchmarkExecutionMode::kSerial;
  std::string dataset_path;
  std::string dataset_label;

  std::string query_type;
  std::string borough;
  double threshold_mph = 0.0;
  bool has_threshold = false;
  std::int64_t start_epoch = 0;
  std::int64_t end_epoch = 0;
  bool has_start_epoch = false;
  bool has_end_epoch = false;
  std::int64_t link_id = 0;
  bool has_link_id = false;
  std::int64_t link_start = 0;
  std::int64_t link_end = 0;
  bool has_link_start = false;
  bool has_link_end = false;
  std::size_t top_n = 0;
  std::size_t min_link_samples = 1;

  std::size_t runs = 10;
  std::size_t thread_count = 0;
  bool validate_parallel_against_serial = false;
  std::string output_csv_path;
  bool append_output = false;
  bool enable_ingest_progress = false;
  std::size_t progress_every_rows = 500000;

  bool enforce_consistent_accepted_rows = true;
  bool has_expected_accepted_rows = false;
  std::size_t expected_accepted_rows = 0;
};

struct BenchmarkRunResult {
  std::size_t run_number = 0;
  BenchmarkExecutionMode execution_mode = BenchmarkExecutionMode::kSerial;
  std::size_t thread_count = 1;
  double ingest_ms = 0.0;
  double query_ms = 0.0;
  double total_ms = 0.0;
  std::size_t rows_read = 0;
  std::size_t rows_accepted = 0;
  std::size_t rows_rejected = 0;
  std::size_t result_count = 0;
  bool has_aggregate = false;
  double average_speed_mph = 0.0;
  double average_travel_time_seconds = 0.0;
  bool serial_match = true;
  double validation_ms = 0.0;
  double validation_ingest_ms = 0.0;
  double cpu_ms = 0.0;
  long peak_rss_kb = -1;
};

class BenchmarkHarness {
 public:
  static bool RunSerial(const BenchmarkConfig& config,
                        std::vector<BenchmarkRunResult>* out_results,
                        std::string* error);
  static bool RunParallel(const BenchmarkConfig& config,
                          std::vector<BenchmarkRunResult>* out_results,
                          std::string* error);
  static bool RunOptimized(const BenchmarkConfig& config,
                           std::vector<BenchmarkRunResult>* out_results,
                           std::string* error);
};

}  // namespace urbandrop
