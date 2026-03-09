#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "query/TrafficAggregator.hpp"

namespace urbandrop {

class ParallelTrafficAggregator {
 public:
  static AggregateStats SummarizeDataset(const TrafficDataset& dataset,
                                         std::size_t num_threads);

  static AggregateStats SummarizeSpeedBelow(const TrafficDataset& dataset,
                                            double threshold_mph,
                                            std::size_t num_threads);

  static AggregateStats SummarizeTimeWindow(const TrafficDataset& dataset,
                                            std::int64_t start_epoch_seconds,
                                            std::int64_t end_epoch_seconds,
                                            std::size_t num_threads);

  static AggregateStats SummarizeBoroughAndSpeedBelow(const TrafficDataset& dataset,
                                                      const std::string& borough,
                                                      double threshold_mph,
                                                      std::size_t num_threads);
};

}  // namespace urbandrop
