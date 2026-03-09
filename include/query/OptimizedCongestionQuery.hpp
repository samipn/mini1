#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "data_model/TrafficDatasetOptimized.hpp"
#include "query/CongestionQuery.hpp"

namespace urbandrop {

class OptimizedCongestionQuery {
 public:
  static std::size_t CountSpeedBelow(const TrafficDatasetOptimized& dataset,
                                     double threshold_mph);
  static std::size_t CountSpeedBelowParallel(const TrafficDatasetOptimized& dataset,
                                             double threshold_mph,
                                             std::size_t num_threads);

  static std::size_t CountTimeWindow(const TrafficDatasetOptimized& dataset,
                                     std::int64_t start_epoch_seconds,
                                     std::int64_t end_epoch_seconds);
  static std::size_t CountTimeWindowParallel(const TrafficDatasetOptimized& dataset,
                                             std::int64_t start_epoch_seconds,
                                             std::int64_t end_epoch_seconds,
                                             std::size_t num_threads);

  static std::size_t CountBoroughAndSpeedBelow(const TrafficDatasetOptimized& dataset,
                                               std::int16_t borough_code,
                                               double threshold_mph);
  static std::size_t CountBoroughAndSpeedBelowParallel(const TrafficDatasetOptimized& dataset,
                                                       std::int16_t borough_code,
                                                       double threshold_mph,
                                                       std::size_t num_threads);

  static std::vector<LinkSpeedStat> TopNSlowestRecurringLinks(const TrafficDatasetOptimized& dataset,
                                                              std::size_t n,
                                                              std::size_t min_samples_per_link);
  static std::vector<LinkSpeedStat> TopNSlowestRecurringLinksParallel(
      const TrafficDatasetOptimized& dataset,
      std::size_t n,
      std::size_t min_samples_per_link,
      std::size_t num_threads);
};

}  // namespace urbandrop
