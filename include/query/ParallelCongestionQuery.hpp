#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "query/CongestionQuery.hpp"

namespace urbandrop {

class ParallelCongestionQuery {
 public:
  static std::size_t CountSpeedBelow(const TrafficDataset& dataset,
                                     double threshold_mph,
                                     std::size_t num_threads);

  static std::size_t CountBoroughAndSpeedBelow(const TrafficDataset& dataset,
                                               const std::string& borough,
                                               double threshold_mph,
                                               std::size_t num_threads);

  static std::size_t CountTimeWindow(const TrafficDataset& dataset,
                                     std::int64_t start_epoch_seconds,
                                     std::int64_t end_epoch_seconds,
                                     std::size_t num_threads);

  static std::vector<LinkSpeedStat> TopNSlowestRecurringLinks(const TrafficDataset& dataset,
                                                              std::size_t n,
                                                              std::size_t min_samples_per_link,
                                                              std::size_t num_threads);
};

}  // namespace urbandrop
