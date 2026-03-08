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
    std::cerr << "failed to prepare sample CSV for API test\n";
    return EXIT_FAILURE;
  }

  urbandrop::TrafficDataset dataset;
  urbandrop::TrafficLoadOptions options;
  options.print_progress = false;

  std::string error;
  if (!urbandrop::CSVReader::LoadTrafficCSV(sample_csv, &dataset, options, nullptr, &error)) {
    std::cerr << "CSVReader load failed: " << error << "\n";
    return EXIT_FAILURE;
  }
  if (dataset.Empty()) {
    std::cerr << "dataset should not be empty\n";
    return EXIT_FAILURE;
  }

  const auto summary = urbandrop::TrafficAggregator::SummarizeDataset(dataset);
  if (summary.record_count != dataset.Size()) {
    std::cerr << "summary record count mismatch\n";
    return EXIT_FAILURE;
  }

  const auto all_by_threshold = urbandrop::CongestionQuery::FilterSpeedBelow(dataset, 1000.0);
  if (all_by_threshold.size() != dataset.Size()) {
    std::cerr << "speed_below(1000) should include all sample records\n";
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
    std::cerr << "time-window(min,max) should include all sample records\n";
    return EXIT_FAILURE;
  }

  const auto by_link = urbandrop::CongestionQuery::FindByLinkId(dataset, first->link_id);
  const auto by_range = urbandrop::CongestionQuery::FindByLinkRange(dataset, first->link_id, first->link_id);
  if (by_link.empty() || by_link.size() != by_range.size()) {
    std::cerr << "link_id and single-value link_range mismatch\n";
    return EXIT_FAILURE;
  }

  const auto by_borough =
      urbandrop::CongestionQuery::FilterBoroughAndSpeedBelow(dataset, first->borough, 1000.0);
  if (by_borough.empty()) {
    std::cerr << "borough filter returned no records for first-row borough\n";
    return EXIT_FAILURE;
  }

  const auto top_n = urbandrop::CongestionQuery::TopNSlowestRecurringLinks(dataset, 5, 1);
  if (top_n.empty() || top_n.size() > 5) {
    std::cerr << "top-N query result size out of bounds\n";
    return EXIT_FAILURE;
  }

  const auto link_stats = urbandrop::TrafficAggregator::Summarize(by_link);
  if (link_stats.record_count != by_link.size()) {
    std::cerr << "summarize(by_link) count mismatch\n";
    return EXIT_FAILURE;
  }

  (void)copied_rows;
  (void)reused_sample;
  return EXIT_SUCCESS;
}
