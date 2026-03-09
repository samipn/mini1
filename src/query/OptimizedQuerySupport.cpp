#include "query/OptimizedQuerySupport.hpp"

namespace urbandrop {

OptimizedQuerySupport OptimizedQuerySupport::Build(const TrafficDatasetOptimized& dataset) {
  OptimizedQuerySupport support;
  const auto& links = dataset.Columns().LinkIds();
  const auto& boroughs = dataset.Columns().BoroughCodes();
  const auto& timestamps = dataset.Columns().TimestampsEpochSeconds();

  support.link_counts_.reserve(links.size());
  support.borough_counts_.reserve(16);
  support.hour_bucket_counts_.reserve(links.size() / 8 + 1);

  for (std::size_t i = 0; i < links.size(); ++i) {
    ++support.link_counts_[links[i]];
    ++support.borough_counts_[boroughs[i]];
    ++support.hour_bucket_counts_[timestamps[i] / 3600];
  }

  return support;
}

std::size_t OptimizedQuerySupport::CountByLinkId(std::int64_t link_id) const {
  const auto it = link_counts_.find(link_id);
  return it == link_counts_.end() ? 0 : it->second;
}

std::size_t OptimizedQuerySupport::CountByBorough(std::int16_t borough_code) const {
  const auto it = borough_counts_.find(borough_code);
  return it == borough_counts_.end() ? 0 : it->second;
}

std::size_t OptimizedQuerySupport::CountByHourBucket(std::int64_t hour_bucket) const {
  const auto it = hour_bucket_counts_.find(hour_bucket);
  return it == hour_bucket_counts_.end() ? 0 : it->second;
}

std::size_t OptimizedQuerySupport::LinkKeyCount() const {
  return link_counts_.size();
}

std::size_t OptimizedQuerySupport::BoroughKeyCount() const {
  return borough_counts_.size();
}

std::size_t OptimizedQuerySupport::HourBucketCount() const {
  return hour_bucket_counts_.size();
}

}  // namespace urbandrop
