#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "data_model/CommonCodes.hpp"
#include "data_model/TrafficDataset.hpp"
#include "query/CongestionQuery.hpp"
#include "query/ParallelCongestionQuery.hpp"
#include "query/ParallelTrafficAggregator.hpp"
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
  Add(&dataset, 500, 14.0, 25.0, 1700000500, "Brooklyn");

  const std::size_t serial_slow = urbandrop::CongestionQuery::FilterSpeedBelow(dataset, 15.0).size();
  const std::size_t serial_queens =
      urbandrop::CongestionQuery::FilterBoroughAndSpeedBelow(dataset, "Queens", 40.0).size();
  const std::size_t serial_window =
      urbandrop::TimeWindowQuery::FilterByEpochRange(dataset, 1700000050, 1700000350).size();

  for (std::size_t threads : std::vector<std::size_t>{0, 1, 2, 4, 8}) {
    const std::size_t parallel_slow =
        urbandrop::ParallelCongestionQuery::CountSpeedBelow(dataset, 15.0, threads);
    const std::size_t parallel_queens =
        urbandrop::ParallelCongestionQuery::CountBoroughAndSpeedBelow(dataset, "Queens", 40.0, threads);
    const std::size_t parallel_window =
        urbandrop::ParallelCongestionQuery::CountTimeWindow(dataset, 1700000050, 1700000350, threads);

    if (parallel_slow != serial_slow || parallel_queens != serial_queens ||
        parallel_window != serial_window) {
      std::cerr << "parallel/serial mismatch for threads=" << threads << "\n";
      return EXIT_FAILURE;
    }
  }

  const auto serial_summary = urbandrop::TrafficAggregator::SummarizeDataset(dataset);
  const auto parallel_summary = urbandrop::ParallelTrafficAggregator::SummarizeDataset(dataset, 4);
  if (serial_summary.record_count != parallel_summary.record_count ||
      !NearlyEqual(serial_summary.average_speed_mph, parallel_summary.average_speed_mph) ||
      !NearlyEqual(serial_summary.average_travel_time_seconds,
                   parallel_summary.average_travel_time_seconds)) {
    std::cerr << "parallel summary mismatch\n";
    return EXIT_FAILURE;
  }

  const auto serial_topn = urbandrop::CongestionQuery::TopNSlowestRecurringLinks(dataset, 3, 1);
  const auto parallel_topn =
      urbandrop::ParallelCongestionQuery::TopNSlowestRecurringLinks(dataset, 3, 1, 4);
  if (serial_topn.size() != parallel_topn.size()) {
    std::cerr << "parallel top-n size mismatch\n";
    return EXIT_FAILURE;
  }
  for (std::size_t i = 0; i < serial_topn.size(); ++i) {
    if (serial_topn[i].link_id != parallel_topn[i].link_id ||
        serial_topn[i].record_count != parallel_topn[i].record_count ||
        !NearlyEqual(serial_topn[i].average_speed_mph, parallel_topn[i].average_speed_mph)) {
      std::cerr << "parallel top-n value mismatch\n";
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
