#include "query/OptimizedCongestionQuery.hpp"

#include <algorithm>
#include <unordered_map>

#if defined(_OPENMP)
#include <omp.h>
#endif

namespace urbandrop {

std::size_t OptimizedCongestionQuery::CountSpeedBelow(const TrafficDatasetOptimized& dataset,
                                                      double threshold_mph) {
  const auto& speeds = dataset.Columns().SpeedsMph();
  std::size_t count = 0;
  for (const double speed : speeds) {
    if (speed < threshold_mph) {
      ++count;
    }
  }
  return count;
}

std::size_t OptimizedCongestionQuery::CountSpeedBelowParallel(const TrafficDatasetOptimized& dataset,
                                                              double threshold_mph,
                                                              std::size_t num_threads) {
  const auto& speeds = dataset.Columns().SpeedsMph();
  std::size_t count = 0;
#if defined(_OPENMP)
#pragma omp parallel for reduction(+ : count) num_threads(static_cast<int>(num_threads))
#endif
  for (std::size_t i = 0; i < speeds.size(); ++i) {
    if (speeds[i] < threshold_mph) {
      ++count;
    }
  }
  return count;
}

std::size_t OptimizedCongestionQuery::CountTimeWindow(const TrafficDatasetOptimized& dataset,
                                                      std::int64_t start_epoch_seconds,
                                                      std::int64_t end_epoch_seconds) {
  if (end_epoch_seconds < start_epoch_seconds) {
    std::swap(start_epoch_seconds, end_epoch_seconds);
  }

  const auto& timestamps = dataset.Columns().TimestampsEpochSeconds();
  std::size_t count = 0;
  for (const std::int64_t ts : timestamps) {
    if (ts >= start_epoch_seconds && ts <= end_epoch_seconds) {
      ++count;
    }
  }
  return count;
}

std::size_t OptimizedCongestionQuery::CountTimeWindowParallel(const TrafficDatasetOptimized& dataset,
                                                              std::int64_t start_epoch_seconds,
                                                              std::int64_t end_epoch_seconds,
                                                              std::size_t num_threads) {
  if (end_epoch_seconds < start_epoch_seconds) {
    std::swap(start_epoch_seconds, end_epoch_seconds);
  }

  const auto& timestamps = dataset.Columns().TimestampsEpochSeconds();
  std::size_t count = 0;
#if defined(_OPENMP)
#pragma omp parallel for reduction(+ : count) num_threads(static_cast<int>(num_threads))
#endif
  for (std::size_t i = 0; i < timestamps.size(); ++i) {
    if (timestamps[i] >= start_epoch_seconds && timestamps[i] <= end_epoch_seconds) {
      ++count;
    }
  }
  return count;
}

std::size_t OptimizedCongestionQuery::CountBoroughAndSpeedBelow(const TrafficDatasetOptimized& dataset,
                                                                std::int16_t borough_code,
                                                                double threshold_mph) {
  const auto& boroughs = dataset.Columns().BoroughCodes();
  const auto& speeds = dataset.Columns().SpeedsMph();

  std::size_t count = 0;
  for (std::size_t i = 0; i < boroughs.size(); ++i) {
    if (boroughs[i] == borough_code && speeds[i] < threshold_mph) {
      ++count;
    }
  }
  return count;
}

std::size_t OptimizedCongestionQuery::CountBoroughAndSpeedBelowParallel(
    const TrafficDatasetOptimized& dataset,
    std::int16_t borough_code,
    double threshold_mph,
    std::size_t num_threads) {
  const auto& boroughs = dataset.Columns().BoroughCodes();
  const auto& speeds = dataset.Columns().SpeedsMph();

  std::size_t count = 0;
#if defined(_OPENMP)
#pragma omp parallel for reduction(+ : count) num_threads(static_cast<int>(num_threads))
#endif
  for (std::size_t i = 0; i < boroughs.size(); ++i) {
    if (boroughs[i] == borough_code && speeds[i] < threshold_mph) {
      ++count;
    }
  }
  return count;
}

std::vector<LinkSpeedStat> OptimizedCongestionQuery::TopNSlowestRecurringLinks(
    const TrafficDatasetOptimized& dataset,
    std::size_t n,
    std::size_t min_samples_per_link) {
  const auto& links = dataset.Columns().LinkIds();
  const auto& speeds = dataset.Columns().SpeedsMph();

  struct Partial {
    std::size_t count = 0;
    double speed_sum = 0.0;
  };

  std::unordered_map<std::int64_t, Partial> grouped;
  grouped.reserve(links.size());

  for (std::size_t i = 0; i < links.size(); ++i) {
    auto& p = grouped[links[i]];
    ++p.count;
    p.speed_sum += speeds[i];
  }

  std::vector<LinkSpeedStat> stats;
  stats.reserve(grouped.size());
  for (const auto& kv : grouped) {
    if (kv.second.count < min_samples_per_link) {
      continue;
    }
    LinkSpeedStat row;
    row.link_id = kv.first;
    row.record_count = kv.second.count;
    row.average_speed_mph = kv.second.speed_sum / static_cast<double>(kv.second.count);
    stats.push_back(row);
  }

  std::sort(stats.begin(), stats.end(), [](const LinkSpeedStat& lhs, const LinkSpeedStat& rhs) {
    if (lhs.average_speed_mph == rhs.average_speed_mph) {
      return lhs.link_id < rhs.link_id;
    }
    return lhs.average_speed_mph < rhs.average_speed_mph;
  });
  if (stats.size() > n) {
    stats.resize(n);
  }
  return stats;
}

std::vector<LinkSpeedStat> OptimizedCongestionQuery::TopNSlowestRecurringLinksParallel(
    const TrafficDatasetOptimized& dataset,
    std::size_t n,
    std::size_t min_samples_per_link,
    std::size_t num_threads) {
  const auto& links = dataset.Columns().LinkIds();
  const auto& speeds = dataset.Columns().SpeedsMph();

  struct Partial {
    std::size_t count = 0;
    double speed_sum = 0.0;
  };

  std::unordered_map<std::int64_t, Partial> grouped;

#if defined(_OPENMP)
#pragma omp parallel num_threads(static_cast<int>(num_threads))
  {
    std::unordered_map<std::int64_t, Partial> local;

#pragma omp for nowait
    for (std::size_t i = 0; i < links.size(); ++i) {
      auto& p = local[links[i]];
      ++p.count;
      p.speed_sum += speeds[i];
    }

#pragma omp critical
    {
      for (const auto& kv : local) {
        auto& merged = grouped[kv.first];
        merged.count += kv.second.count;
        merged.speed_sum += kv.second.speed_sum;
      }
    }
  }
#else
  for (std::size_t i = 0; i < links.size(); ++i) {
    auto& p = grouped[links[i]];
    ++p.count;
    p.speed_sum += speeds[i];
  }
#endif

  std::vector<LinkSpeedStat> stats;
  stats.reserve(grouped.size());
  for (const auto& kv : grouped) {
    if (kv.second.count < min_samples_per_link) {
      continue;
    }
    LinkSpeedStat row;
    row.link_id = kv.first;
    row.record_count = kv.second.count;
    row.average_speed_mph = kv.second.speed_sum / static_cast<double>(kv.second.count);
    stats.push_back(row);
  }

  std::sort(stats.begin(), stats.end(), [](const LinkSpeedStat& lhs, const LinkSpeedStat& rhs) {
    if (lhs.average_speed_mph == rhs.average_speed_mph) {
      return lhs.link_id < rhs.link_id;
    }
    return lhs.average_speed_mph < rhs.average_speed_mph;
  });
  if (stats.size() > n) {
    stats.resize(n);
  }
  return stats;
}

}  // namespace urbandrop
