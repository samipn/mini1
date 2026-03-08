#include "query/TimeWindowQuery.hpp"

#include <algorithm>

namespace urbandrop {

std::vector<const TrafficRecord*> TimeWindowQuery::FilterByEpochRange(
    const TrafficDataset& dataset,
    std::int64_t start_epoch_seconds,
    std::int64_t end_epoch_seconds) {
  std::vector<const TrafficRecord*> results;
  if (end_epoch_seconds < start_epoch_seconds) {
    std::swap(start_epoch_seconds, end_epoch_seconds);
  }

  for (const auto& record : dataset.Records()) {
    if (record.timestamp_epoch_seconds >= start_epoch_seconds &&
        record.timestamp_epoch_seconds <= end_epoch_seconds) {
      results.push_back(&record);
    }
  }
  return results;
}

}  // namespace urbandrop
