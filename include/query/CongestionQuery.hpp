#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "data_model/TrafficDataset.hpp"

namespace urbandrop {

struct LinkSpeedStat {
  std::int64_t link_id = -1;
  std::size_t record_count = 0;
  double average_speed_mph = 0.0;
};

class CongestionQuery {
 public:
  static std::vector<const TrafficRecord*> FilterSpeedBelow(const TrafficDataset& dataset,
                                                            double threshold_mph);
  static std::vector<const TrafficRecord*> FilterBoroughAndSpeedBelow(const TrafficDataset& dataset,
                                                                      const std::string& borough,
                                                                      double threshold_mph);
  static std::vector<const TrafficRecord*> FindByLinkId(const TrafficDataset& dataset,
                                                        std::int64_t link_id);
  static std::vector<const TrafficRecord*> FindByLinkRange(const TrafficDataset& dataset,
                                                           std::int64_t start_link_id,
                                                           std::int64_t end_link_id);

  static std::vector<LinkSpeedStat> TopNSlowestRecurringLinks(const TrafficDataset& dataset,
                                                              std::size_t n,
                                                              std::size_t min_samples_per_link);
};

}  // namespace urbandrop
