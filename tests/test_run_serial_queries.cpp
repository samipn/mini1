#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>

#include "data_model/TrafficDataset.hpp"
#include "io/CSVReader.hpp"
#include "test_data_fixture.hpp"

int main() {
  constexpr std::size_t kSampleRows = 1000;
  std::string sample_csv;
  std::size_t copied_rows = 0;
  bool reused_sample = false;

  if (!urbandrop::testutil::EnsureFirstNLinesSample(urbandrop::testutil::GetTrafficSourceCsvPath(),
                                                    kSampleRows,
                                                    &sample_csv,
                                                    &copied_rows,
                                                    &reused_sample)) {
    std::cerr << "failed to prepare sample CSV for CLI test\n";
    return EXIT_FAILURE;
  }

  urbandrop::TrafficDataset dataset;
  urbandrop::TrafficLoadOptions options;
  options.print_progress = false;
  std::string error;
  if (!urbandrop::CSVReader::LoadTrafficCSV(sample_csv, &dataset, options, nullptr, &error)) {
    std::cerr << "failed to load sample before CLI test: " << error << "\n";
    return EXIT_FAILURE;
  }
  const auto* first = dataset.At(0);
  if (first == nullptr) {
    std::cerr << "expected at least one accepted sample row\n";
    return EXIT_FAILURE;
  }

  std::int64_t min_ts = first->timestamp_epoch_seconds;
  std::int64_t max_ts = first->timestamp_epoch_seconds;
  for (const auto& row : dataset) {
    min_ts = std::min(min_ts, row.timestamp_epoch_seconds);
    max_ts = std::max(max_ts, row.timestamp_epoch_seconds);
  }

  const std::string quoted_csv = urbandrop::testutil::ShellQuote(sample_csv);

  if (urbandrop::testutil::RunShell("./run_serial --traffic " + quoted_csv + " --query summary > /dev/null") !=
      0) {
    std::cerr << "run_serial summary failed\n";
    return EXIT_FAILURE;
  }

  if (urbandrop::testutil::RunShell("./run_serial --traffic " + quoted_csv +
                                    " --query speed_below --threshold 1000 > /dev/null") != 0) {
    std::cerr << "run_serial speed_below failed\n";
    return EXIT_FAILURE;
  }

  if (urbandrop::testutil::RunShell("./run_serial --traffic " + quoted_csv + " --query borough_speed_below --borough " +
                                    urbandrop::testutil::ShellQuote(first->borough) +
                                    " --threshold 1000 > /dev/null") != 0) {
    std::cerr << "run_serial borough_speed_below failed\n";
    return EXIT_FAILURE;
  }

  if (urbandrop::testutil::RunShell("./run_serial --traffic " + quoted_csv + " --query link_id --link-id " +
                                    std::to_string(first->link_id) + " > /dev/null") != 0) {
    std::cerr << "run_serial link_id failed\n";
    return EXIT_FAILURE;
  }

  if (urbandrop::testutil::RunShell("./run_serial --traffic " + quoted_csv +
                                    " --query link_range --link-start " + std::to_string(first->link_id) +
                                    " --link-end " + std::to_string(first->link_id) + " > /dev/null") != 0) {
    std::cerr << "run_serial link_range failed\n";
    return EXIT_FAILURE;
  }

  if (urbandrop::testutil::RunShell("./run_serial --traffic " + quoted_csv +
                                    " --query time_window --start-epoch " + std::to_string(min_ts) +
                                    " --end-epoch " + std::to_string(max_ts) + " > /dev/null") != 0) {
    std::cerr << "run_serial time_window failed\n";
    return EXIT_FAILURE;
  }

  if (urbandrop::testutil::RunShell("./run_serial --traffic " + quoted_csv +
                                    " --query top_n_slowest --top-n 5 > /dev/null") != 0) {
    std::cerr << "run_serial top_n_slowest failed\n";
    return EXIT_FAILURE;
  }

  (void)copied_rows;
  (void)reused_sample;
  return EXIT_SUCCESS;
}
