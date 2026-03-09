#pragma once

#include <cstddef>

#include "query/TrafficAggregator.hpp"

namespace urbandrop {

class ParallelTrafficAggregator {
 public:
  static AggregateStats SummarizeDataset(const TrafficDataset& dataset,
                                         std::size_t num_threads);
};

}  // namespace urbandrop
