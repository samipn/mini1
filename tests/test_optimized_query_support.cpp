#include <cstdlib>
#include <iostream>

#include "data_model/CommonCodes.hpp"
#include "data_model/TrafficDatasetOptimized.hpp"
#include "query/OptimizedQuerySupport.hpp"

int main() {
  urbandrop::TrafficDatasetOptimized dataset;
  dataset.AddRecord(100, 10.0, 30.0, 1704067200, urbandrop::ToInt(urbandrop::BoroughCode::kManhattan), "A");
  dataset.AddRecord(100, 12.0, 31.0, 1704067260, urbandrop::ToInt(urbandrop::BoroughCode::kManhattan), "A");
  dataset.AddRecord(200, 20.0, 25.0, 1704070800, urbandrop::ToInt(urbandrop::BoroughCode::kQueens), "B");

  const urbandrop::OptimizedQuerySupport support = urbandrop::OptimizedQuerySupport::Build(dataset);

  if (support.CountByLinkId(100) != 2 || support.CountByLinkId(200) != 1 ||
      support.CountByLinkId(9999) != 0) {
    std::cerr << "link-id counts are incorrect\n";
    return EXIT_FAILURE;
  }

  if (support.CountByBorough(urbandrop::ToInt(urbandrop::BoroughCode::kManhattan)) != 2 ||
      support.CountByBorough(urbandrop::ToInt(urbandrop::BoroughCode::kQueens)) != 1 ||
      support.CountByBorough(urbandrop::ToInt(urbandrop::BoroughCode::kBronx)) != 0) {
    std::cerr << "borough counts are incorrect\n";
    return EXIT_FAILURE;
  }

  if (support.CountByHourBucket(1704067200 / 3600) != 2 ||
      support.CountByHourBucket(1704070800 / 3600) != 1 ||
      support.CountByHourBucket(0) != 0) {
    std::cerr << "hour-bucket counts are incorrect\n";
    return EXIT_FAILURE;
  }

  if (support.LinkKeyCount() != 2 || support.BoroughKeyCount() != 2 || support.HourBucketCount() != 2) {
    std::cerr << "support key counts are incorrect\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
