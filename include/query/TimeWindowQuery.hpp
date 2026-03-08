#pragma once

#include <cstdint>
#include <vector>

#include "data_model/TrafficDataset.hpp"

namespace urbandrop {

class TimeWindowQuery {
 public:
  static std::vector<const TrafficRecord*> FilterByEpochRange(const TrafficDataset& dataset,
                                                              std::int64_t start_epoch_seconds,
                                                              std::int64_t end_epoch_seconds);
};

}  // namespace urbandrop
