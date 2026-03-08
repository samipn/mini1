#include "query/TrafficAggregator.hpp"

namespace urbandrop {

AggregateStats TrafficAggregator::Summarize(const std::vector<const TrafficRecord*>& records) {
  AggregateStats stats;
  if (records.empty()) {
    return stats;
  }

  double speed_sum = 0.0;
  double travel_time_sum = 0.0;
  for (const auto* record : records) {
    speed_sum += record->speed_mph;
    travel_time_sum += record->travel_time_seconds;
  }

  stats.record_count = records.size();
  stats.average_speed_mph = speed_sum / static_cast<double>(stats.record_count);
  stats.average_travel_time_seconds = travel_time_sum / static_cast<double>(stats.record_count);
  return stats;
}

AggregateStats TrafficAggregator::SummarizeDataset(const TrafficDataset& dataset) {
  AggregateStats stats;
  if (dataset.Empty()) {
    return stats;
  }

  double speed_sum = 0.0;
  double travel_time_sum = 0.0;
  for (const auto& record : dataset.Records()) {
    speed_sum += record.speed_mph;
    travel_time_sum += record.travel_time_seconds;
  }

  stats.record_count = dataset.Size();
  stats.average_speed_mph = speed_sum / static_cast<double>(stats.record_count);
  stats.average_travel_time_seconds = travel_time_sum / static_cast<double>(stats.record_count);
  return stats;
}

}  // namespace urbandrop
