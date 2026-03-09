#include "query/OptimizedTrafficAggregator.hpp"

#include <algorithm>
#include <functional>

#if defined(_OPENMP)
#include <omp.h>
#endif

namespace urbandrop {
namespace {

AggregateStats SummarizeConditional(const TrafficDatasetOptimized& dataset,
                                    const std::function<bool(std::size_t)>& predicate,
                                    std::size_t num_threads,
                                    bool use_parallel) {
  AggregateStats stats;
  const auto& speeds = dataset.Columns().SpeedsMph();
  const auto& travel_times = dataset.Columns().TravelTimesSeconds();

  const std::size_t size = dataset.Size();
  if (size == 0) {
    return stats;
  }

  double speed_sum = 0.0;
  double travel_sum = 0.0;
  std::size_t count = 0;

#if !defined(_OPENMP)
  (void)num_threads;
  (void)use_parallel;
#endif

#if defined(_OPENMP)
  if (use_parallel) {
#pragma omp parallel for reduction(+ : speed_sum, travel_sum, count) num_threads(static_cast<int>(num_threads))
    for (std::size_t i = 0; i < size; ++i) {
      if (!predicate(i)) {
        continue;
      }
      speed_sum += speeds[i];
      travel_sum += travel_times[i];
      ++count;
    }
  } else
#endif
  {
    for (std::size_t i = 0; i < size; ++i) {
      if (!predicate(i)) {
        continue;
      }
      speed_sum += speeds[i];
      travel_sum += travel_times[i];
      ++count;
    }
  }

  if (count == 0) {
    return stats;
  }

  stats.record_count = count;
  stats.average_speed_mph = speed_sum / static_cast<double>(count);
  stats.average_travel_time_seconds = travel_sum / static_cast<double>(count);
  return stats;
}

}  // namespace

AggregateStats OptimizedTrafficAggregator::SummarizeDataset(const TrafficDatasetOptimized& dataset) {
  return SummarizeConditional(dataset, [](std::size_t) { return true; }, 1, false);
}

AggregateStats OptimizedTrafficAggregator::SummarizeDatasetParallel(const TrafficDatasetOptimized& dataset,
                                                                    std::size_t num_threads) {
  return SummarizeConditional(dataset, [](std::size_t) { return true; }, num_threads, true);
}

AggregateStats OptimizedTrafficAggregator::SummarizeSpeedBelow(const TrafficDatasetOptimized& dataset,
                                                               double threshold_mph) {
  const auto& speeds = dataset.Columns().SpeedsMph();
  return SummarizeConditional(dataset, [&speeds, threshold_mph](std::size_t i) {
    return speeds[i] < threshold_mph;
  }, 1, false);
}

AggregateStats OptimizedTrafficAggregator::SummarizeSpeedBelowParallel(const TrafficDatasetOptimized& dataset,
                                                                       double threshold_mph,
                                                                       std::size_t num_threads) {
  const auto& speeds = dataset.Columns().SpeedsMph();
  return SummarizeConditional(dataset, [&speeds, threshold_mph](std::size_t i) {
    return speeds[i] < threshold_mph;
  }, num_threads, true);
}

AggregateStats OptimizedTrafficAggregator::SummarizeTimeWindow(const TrafficDatasetOptimized& dataset,
                                                               std::int64_t start_epoch_seconds,
                                                               std::int64_t end_epoch_seconds) {
  if (end_epoch_seconds < start_epoch_seconds) {
    std::swap(start_epoch_seconds, end_epoch_seconds);
  }
  const auto& timestamps = dataset.Columns().TimestampsEpochSeconds();
  return SummarizeConditional(dataset, [&timestamps, start_epoch_seconds, end_epoch_seconds](std::size_t i) {
    return timestamps[i] >= start_epoch_seconds && timestamps[i] <= end_epoch_seconds;
  }, 1, false);
}

AggregateStats OptimizedTrafficAggregator::SummarizeTimeWindowParallel(const TrafficDatasetOptimized& dataset,
                                                                       std::int64_t start_epoch_seconds,
                                                                       std::int64_t end_epoch_seconds,
                                                                       std::size_t num_threads) {
  if (end_epoch_seconds < start_epoch_seconds) {
    std::swap(start_epoch_seconds, end_epoch_seconds);
  }
  const auto& timestamps = dataset.Columns().TimestampsEpochSeconds();
  return SummarizeConditional(dataset, [&timestamps, start_epoch_seconds, end_epoch_seconds](std::size_t i) {
    return timestamps[i] >= start_epoch_seconds && timestamps[i] <= end_epoch_seconds;
  }, num_threads, true);
}

AggregateStats OptimizedTrafficAggregator::SummarizeBoroughAndSpeedBelow(
    const TrafficDatasetOptimized& dataset,
    std::int16_t borough_code,
    double threshold_mph) {
  const auto& boroughs = dataset.Columns().BoroughCodes();
  const auto& speeds = dataset.Columns().SpeedsMph();
  return SummarizeConditional(dataset, [&boroughs, &speeds, borough_code, threshold_mph](std::size_t i) {
    return boroughs[i] == borough_code && speeds[i] < threshold_mph;
  }, 1, false);
}

AggregateStats OptimizedTrafficAggregator::SummarizeBoroughAndSpeedBelowParallel(
    const TrafficDatasetOptimized& dataset,
    std::int16_t borough_code,
    double threshold_mph,
    std::size_t num_threads) {
  const auto& boroughs = dataset.Columns().BoroughCodes();
  const auto& speeds = dataset.Columns().SpeedsMph();
  return SummarizeConditional(dataset, [&boroughs, &speeds, borough_code, threshold_mph](std::size_t i) {
    return boroughs[i] == borough_code && speeds[i] < threshold_mph;
  }, num_threads, true);
}

}  // namespace urbandrop
