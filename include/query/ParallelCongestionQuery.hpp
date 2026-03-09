#pragma once

#include <cstddef>
#include <string>

#include "data_model/TrafficDataset.hpp"

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
};

}  // namespace urbandrop
