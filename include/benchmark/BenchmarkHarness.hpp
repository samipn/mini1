#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace urbandrop {

struct BenchmarkConfig {
  std::string binary_name = "run_serial";
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
  double ingest_ms = 0.0;
  double query_ms = 0.0;
  double total_ms = 0.0;
  std::size_t rows_read = 0;
  std::size_t rows_accepted = 0;
  std::size_t rows_rejected = 0;
  std::size_t result_count = 0;
};

class BenchmarkHarness {
 public:
  static bool RunSerial(const BenchmarkConfig& config,
                        std::vector<BenchmarkRunResult>* out_results,
                        std::string* error);
};

}  // namespace urbandrop
