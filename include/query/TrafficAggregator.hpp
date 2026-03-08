#pragma once

#include <cstddef>
#include <vector>

#include "data_model/TrafficDataset.hpp"

namespace urbandrop {

struct AggregateStats {
  std::size_t record_count = 0;
  double average_speed_mph = 0.0;
  double average_travel_time_seconds = 0.0;
};

class TrafficAggregator {
 public:
  static AggregateStats Summarize(const std::vector<const TrafficRecord*>& records);
  static AggregateStats SummarizeDataset(const TrafficDataset& dataset);
};

}  // namespace urbandrop
