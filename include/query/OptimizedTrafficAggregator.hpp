#pragma once

#include <cstddef>
#include <cstdint>

#include "data_model/TrafficDatasetOptimized.hpp"
#include "query/TrafficAggregator.hpp"

namespace urbandrop {

class OptimizedTrafficAggregator {
 public:
  static AggregateStats SummarizeDataset(const TrafficDatasetOptimized& dataset);
  static AggregateStats SummarizeDatasetParallel(const TrafficDatasetOptimized& dataset,
                                                 std::size_t num_threads);

  static AggregateStats SummarizeSpeedBelow(const TrafficDatasetOptimized& dataset,
                                            double threshold_mph);
  static AggregateStats SummarizeSpeedBelowParallel(const TrafficDatasetOptimized& dataset,
                                                    double threshold_mph,
                                                    std::size_t num_threads);

  static AggregateStats SummarizeTimeWindow(const TrafficDatasetOptimized& dataset,
                                            std::int64_t start_epoch_seconds,
                                            std::int64_t end_epoch_seconds);
  static AggregateStats SummarizeTimeWindowParallel(const TrafficDatasetOptimized& dataset,
                                                    std::int64_t start_epoch_seconds,
                                                    std::int64_t end_epoch_seconds,
                                                    std::size_t num_threads);

  static AggregateStats SummarizeBoroughAndSpeedBelow(const TrafficDatasetOptimized& dataset,
                                                      std::int16_t borough_code,
                                                      double threshold_mph);
  static AggregateStats SummarizeBoroughAndSpeedBelowParallel(
      const TrafficDatasetOptimized& dataset,
      std::int16_t borough_code,
      double threshold_mph,
      std::size_t num_threads);
};

}  // namespace urbandrop
