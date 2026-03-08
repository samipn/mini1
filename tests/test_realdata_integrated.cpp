#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>

#include "data_model/TrafficDataset.hpp"
#include "io/CSVReader.hpp"
#include "query/CongestionQuery.hpp"
#include "query/TimeWindowQuery.hpp"
#include "query/TrafficAggregator.hpp"
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
    std::cerr << "failed to prepare sample CSV for integrated test\n";
    return EXIT_FAILURE;
  }

  urbandrop::TrafficDataset dataset;
  urbandrop::TrafficLoadOptions options;
  options.print_progress = false;
  std::string error;
  if (!urbandrop::CSVReader::LoadTrafficCSV(sample_csv, &dataset, options, nullptr, &error)) {
    std::cerr << "CSVReader failed on sample: " << error << "\n";
    return EXIT_FAILURE;
  }

  if (dataset.Empty() || dataset.Size() == 0) {
    std::cerr << "expected accepted sample rows\n";
    return EXIT_FAILURE;
  }
  if (dataset.Counters().rows_read == 0 || dataset.Counters().rows_read > copied_rows) {
    std::cerr << "unexpected rows_read counter\n";
    return EXIT_FAILURE;
  }

  const auto summary = urbandrop::TrafficAggregator::SummarizeDataset(dataset);
  if (summary.record_count != dataset.Size()) {
    std::cerr << "summary mismatch\n";
    return EXIT_FAILURE;
  }

  const auto all_by_threshold = urbandrop::CongestionQuery::FilterSpeedBelow(dataset, 1000.0);
  if (all_by_threshold.size() != dataset.Size()) {
    std::cerr << "threshold query mismatch\n";
    return EXIT_FAILURE;
  }

  const auto* first = dataset.At(0);
  if (first == nullptr) {
    std::cerr << "missing first row\n";
    return EXIT_FAILURE;
  }

  std::int64_t min_ts = first->timestamp_epoch_seconds;
  std::int64_t max_ts = first->timestamp_epoch_seconds;
  for (const auto& row : dataset) {
    min_ts = std::min(min_ts, row.timestamp_epoch_seconds);
    max_ts = std::max(max_ts, row.timestamp_epoch_seconds);
  }

  const auto all_by_window = urbandrop::TimeWindowQuery::FilterByEpochRange(dataset, min_ts, max_ts);
  if (all_by_window.size() != dataset.Size()) {
    std::cerr << "time-window query mismatch\n";
    return EXIT_FAILURE;
  }

  const auto by_link = urbandrop::CongestionQuery::FindByLinkId(dataset, first->link_id);
  const auto by_link_range = urbandrop::CongestionQuery::FindByLinkRange(dataset, first->link_id, first->link_id);
  if (by_link.empty() || by_link_range.size() != by_link.size()) {
    std::cerr << "link query mismatch\n";
    return EXIT_FAILURE;
  }

  const auto by_borough =
      urbandrop::CongestionQuery::FilterBoroughAndSpeedBelow(dataset, first->borough, 1000.0);
  if (by_borough.empty()) {
    std::cerr << "borough query mismatch\n";
    return EXIT_FAILURE;
  }

  const auto top_n = urbandrop::CongestionQuery::TopNSlowestRecurringLinks(dataset, 5, 1);
  if (top_n.empty() || top_n.size() > 5) {
    std::cerr << "top_n query mismatch\n";
    return EXIT_FAILURE;
  }

  const std::string quoted_csv = urbandrop::testutil::ShellQuote(sample_csv);
  if (urbandrop::testutil::RunShell("./run_serial --traffic " + quoted_csv + " --query summary > /dev/null") !=
      0) {
    std::cerr << "CLI summary failed\n";
    return EXIT_FAILURE;
  }
  if (urbandrop::testutil::RunShell("./run_serial --traffic " + quoted_csv + " --query speed_below --threshold 1000 > /dev/null") !=
      0) {
    std::cerr << "CLI speed_below failed\n";
    return EXIT_FAILURE;
  }
  if (urbandrop::testutil::RunShell("./run_serial --traffic " + quoted_csv + " --query top_n_slowest --top-n 5 > /dev/null") !=
      0) {
    std::cerr << "CLI top_n_slowest failed\n";
    return EXIT_FAILURE;
  }

  (void)reused_sample;
  return EXIT_SUCCESS;
}
