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
    std::cerr << "failed to prepare sample CSV for ingest test\n";
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

  const auto& counters = dataset.Counters();
  if (counters.rows_read == 0 || counters.rows_read > copied_rows) {
    std::cerr << "unexpected rows_read=" << counters.rows_read << " copied_rows=" << copied_rows << "\n";
    return EXIT_FAILURE;
  }
  if (dataset.Size() != counters.rows_accepted) {
    std::cerr << "dataset size does not match accepted rows\n";
    return EXIT_FAILURE;
  }
  if (counters.rows_accepted + counters.rows_rejected != counters.rows_read) {
    std::cerr << "accepted + rejected should equal rows_read\n";
    return EXIT_FAILURE;
  }

  const auto* first = dataset.At(0);
  if (first == nullptr) {
    std::cerr << "expected at least one accepted row\n";
    return EXIT_FAILURE;
  }

  (void)reused_sample;
  return EXIT_SUCCESS;
}
