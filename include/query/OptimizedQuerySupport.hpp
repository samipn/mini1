#pragma once

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>

#include "data_model/TrafficDatasetOptimized.hpp"

namespace urbandrop {

class OptimizedQuerySupport {
 public:
  static OptimizedQuerySupport Build(const TrafficDatasetOptimized& dataset);

  std::size_t CountByLinkId(std::int64_t link_id) const;
  std::size_t CountByBorough(std::int16_t borough_code) const;
  std::size_t CountByHourBucket(std::int64_t hour_bucket) const;

  std::size_t LinkKeyCount() const;
  std::size_t BoroughKeyCount() const;
  std::size_t HourBucketCount() const;

 private:
  std::unordered_map<std::int64_t, std::size_t> link_counts_;
  std::unordered_map<std::int16_t, std::size_t> borough_counts_;
  std::unordered_map<std::int64_t, std::size_t> hour_bucket_counts_;
};

}  // namespace urbandrop
