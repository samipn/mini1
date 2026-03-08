#include <cstdlib>
#include <iostream>

#include "data_model/TrafficDataset.hpp"

int main() {
  urbandrop::TrafficDataset dataset;
  if (!dataset.Empty()) {
    std::cerr << "expected dataset to be empty initially\n";
    return EXIT_FAILURE;
  }

  urbandrop::TrafficRecord record;
  record.link_id = 101;
  record.speed_mph = 24.5;
  record.travel_time_seconds = 120.0;
  record.timestamp_epoch_seconds = 1704067200;
  record.borough = "Queens";
  record.link_name = "Queens Blvd";

  dataset.AddRecord(record);

  if (dataset.Empty() || dataset.Size() != 1) {
    std::cerr << "expected one accepted row\n";
    return EXIT_FAILURE;
  }

  if (dataset.Counters().rows_accepted != 1 || dataset.Counters().rows_read != 0) {
    std::cerr << "unexpected counter values\n";
    return EXIT_FAILURE;
  }

  const auto& row = dataset.Records().front();
  if (row.link_id != 101 || row.borough != "Queens") {
    std::cerr << "stored row mismatch\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
