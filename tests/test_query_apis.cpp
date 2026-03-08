#include <cmath>
#include <cstdlib>
#include <iostream>

#include "data_model/CommonCodes.hpp"
#include "data_model/TrafficDataset.hpp"
#include "query/CongestionQuery.hpp"
#include "query/TimeWindowQuery.hpp"
#include "query/TrafficAggregator.hpp"

namespace {

void Add(urbandrop::TrafficDataset* dataset,
         std::int64_t link_id,
         double speed,
         double travel,
         std::int64_t ts,
         const std::string& borough) {
  urbandrop::TrafficRecord r;
  r.link_id = link_id;
  r.speed_mph = speed;
  r.travel_time_seconds = travel;
  r.timestamp_epoch_seconds = ts;
  r.borough = borough;
  r.borough_code = urbandrop::ToInt(urbandrop::ParseBoroughCode(borough));
  dataset->AddRecord(r);
}

bool NearlyEqual(double lhs, double rhs) {
  return std::fabs(lhs - rhs) < 1e-9;
}

}  // namespace

int main() {
  urbandrop::TrafficDataset dataset;
  Add(&dataset, 100, 10.0, 50.0, 1700000000, "Manhattan");
  Add(&dataset, 100, 20.0, 45.0, 1700000100, "Manhattan");
  Add(&dataset, 200, 35.0, 30.0, 1700000200, "Queens");
  Add(&dataset, 300, 12.0, 40.0, 1700000300, "Queens");
  Add(&dataset, 400, 50.0, 20.0, 1700000400, "Brooklyn");

  if (dataset.At(1) == nullptr || dataset.At(999) != nullptr) {
    std::cerr << "dataset At() behavior mismatch\n";
    return EXIT_FAILURE;
  }

  std::size_t iter_count = 0;
  for (auto it = dataset.begin(); it != dataset.end(); ++it) {
    ++iter_count;
  }
  if (iter_count != dataset.Size()) {
    std::cerr << "dataset iterator behavior mismatch\n";
    return EXIT_FAILURE;
  }

  const auto slow = urbandrop::CongestionQuery::FilterSpeedBelow(dataset, 15.0);
  if (slow.size() != 2) {
    std::cerr << "expected 2 records below threshold\n";
    return EXIT_FAILURE;
  }

  const auto queens_slow =
      urbandrop::CongestionQuery::FilterBoroughAndSpeedBelow(dataset, "Queens", 20.0);
  if (queens_slow.size() != 1 || queens_slow[0]->link_id != 300) {
    std::cerr << "borough + speed filter mismatch\n";
    return EXIT_FAILURE;
  }

  const auto by_link = urbandrop::CongestionQuery::FindByLinkId(dataset, 100);
  if (by_link.size() != 2) {
    std::cerr << "link-id query mismatch\n";
    return EXIT_FAILURE;
  }

  const auto range = urbandrop::CongestionQuery::FindByLinkRange(dataset, 150, 350);
  if (range.size() != 2) {
    std::cerr << "link-range query mismatch\n";
    return EXIT_FAILURE;
  }

  const auto window = urbandrop::TimeWindowQuery::FilterByEpochRange(dataset, 1700000050, 1700000350);
  if (window.size() != 3) {
    std::cerr << "time-window query mismatch\n";
    return EXIT_FAILURE;
  }

  const auto top_slowest = urbandrop::CongestionQuery::TopNSlowestRecurringLinks(dataset, 2, 1);
  if (top_slowest.size() != 2 || top_slowest[0].link_id != 300 || top_slowest[1].link_id != 100) {
    std::cerr << "top-N slowest ranking mismatch\n";
    return EXIT_FAILURE;
  }

  const auto filtered_stats = urbandrop::TrafficAggregator::Summarize(by_link);
  if (filtered_stats.record_count != 2 || !NearlyEqual(filtered_stats.average_speed_mph, 15.0) ||
      !NearlyEqual(filtered_stats.average_travel_time_seconds, 47.5)) {
    std::cerr << "filtered aggregation mismatch\n";
    return EXIT_FAILURE;
  }

  const auto summary = urbandrop::TrafficAggregator::SummarizeDataset(dataset);
  if (summary.record_count != 5 || !NearlyEqual(summary.average_speed_mph, 25.4)) {
    std::cerr << "dataset aggregation mismatch\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
