#include <cstdlib>
#include <iostream>
#include <vector>

#include "data_model/TrafficDataset.hpp"
#include "data_model/TrafficDatasetOptimized.hpp"
#include "query/CongestionQuery.hpp"
#include "query/OptimizedCongestionQuery.hpp"
#include "query/OptimizedTrafficAggregator.hpp"
#include "query/TimeWindowQuery.hpp"
#include "query/TrafficAggregator.hpp"

int main() {
  urbandrop::TrafficDataset rows;

  urbandrop::TrafficRecord a;
  a.link_id = 100;
  a.speed_mph = 10.0;
  a.travel_time_seconds = 30.0;
  a.timestamp_epoch_seconds = 1000;
  a.borough = "Manhattan";
  a.borough_code = 1;
  a.link_name = "Link A";
  rows.AddRecord(a);

  urbandrop::TrafficRecord b = a;
  b.link_id = 200;
  b.speed_mph = 20.0;
  b.travel_time_seconds = 20.0;
  b.timestamp_epoch_seconds = 2000;
  b.borough = "Queens";
  b.borough_code = 4;
  b.link_name = "Link B";
  rows.AddRecord(b);

  urbandrop::TrafficRecord c = a;
  c.link_id = 100;
  c.speed_mph = 12.0;
  c.travel_time_seconds = 40.0;
  c.timestamp_epoch_seconds = 3000;
  c.borough = "Manhattan";
  c.borough_code = 1;
  c.link_name = "Link A";
  rows.AddRecord(c);

  const urbandrop::TrafficDatasetOptimized optimized =
      urbandrop::TrafficDatasetOptimized::FromRowDataset(rows);

  if (optimized.Size() != rows.Size()) {
    std::cerr << "optimized size mismatch\n";
    return EXIT_FAILURE;
  }

  const auto row_speed = urbandrop::CongestionQuery::FilterSpeedBelow(rows, 15.0).size();
  const auto opt_speed = urbandrop::OptimizedCongestionQuery::CountSpeedBelow(optimized, 15.0);
  const auto opt_speed_p =
      urbandrop::OptimizedCongestionQuery::CountSpeedBelowParallel(optimized, 15.0, 2);
  if (row_speed != opt_speed || row_speed != opt_speed_p) {
    std::cerr << "speed_below mismatch\n";
    return EXIT_FAILURE;
  }

  const auto row_time = urbandrop::TimeWindowQuery::FilterByEpochRange(rows, 1500, 3500).size();
  const auto opt_time = urbandrop::OptimizedCongestionQuery::CountTimeWindow(optimized, 1500, 3500);
  const auto opt_time_p =
      urbandrop::OptimizedCongestionQuery::CountTimeWindowParallel(optimized, 1500, 3500, 2);
  if (row_time != opt_time || row_time != opt_time_p) {
    std::cerr << "time_window mismatch\n";
    return EXIT_FAILURE;
  }

  const auto row_borough =
      urbandrop::CongestionQuery::FilterBoroughAndSpeedBelow(rows, "Manhattan", 15.0).size();
  const auto opt_borough =
      urbandrop::OptimizedCongestionQuery::CountBoroughAndSpeedBelow(optimized, 1, 15.0);
  const auto opt_borough_p =
      urbandrop::OptimizedCongestionQuery::CountBoroughAndSpeedBelowParallel(optimized, 1, 15.0, 2);
  if (row_borough != opt_borough || row_borough != opt_borough_p) {
    std::cerr << "borough speed mismatch\n";
    return EXIT_FAILURE;
  }

  const auto row_summary = urbandrop::TrafficAggregator::SummarizeDataset(rows);
  const auto opt_summary = urbandrop::OptimizedTrafficAggregator::SummarizeDataset(optimized);
  const auto opt_summary_p = urbandrop::OptimizedTrafficAggregator::SummarizeDatasetParallel(optimized, 2);

  if (row_summary.record_count != opt_summary.record_count ||
      row_summary.record_count != opt_summary_p.record_count) {
    std::cerr << "summary count mismatch\n";
    return EXIT_FAILURE;
  }

  const auto row_topn = urbandrop::CongestionQuery::TopNSlowestRecurringLinks(rows, 2, 1);
  const auto opt_topn = urbandrop::OptimizedCongestionQuery::TopNSlowestRecurringLinks(optimized, 2, 1);
  const auto opt_topn_p =
      urbandrop::OptimizedCongestionQuery::TopNSlowestRecurringLinksParallel(optimized, 2, 1, 2);

  if (row_topn.size() != opt_topn.size() || row_topn.size() != opt_topn_p.size()) {
    std::cerr << "topn size mismatch\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
