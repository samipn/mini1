#include "query/ParallelCongestionQuery.hpp"

#include <algorithm>
#include <cstdint>
#include <unordered_map>

#if defined(_OPENMP)
#include <omp.h>
#endif

#include "data_model/CommonCodes.hpp"

namespace urbandrop {
namespace {

struct PartialLinkStat {
  std::size_t count = 0;
  double speed_sum = 0.0;
};

std::size_t NormalizeThreadCount(std::size_t requested, std::size_t total_records) {
  if (total_records == 0) {
    return 1;
  }

  std::size_t hw = 1;
#if defined(_OPENMP)
  hw = static_cast<std::size_t>(omp_get_max_threads());
#endif

  const std::size_t baseline = requested == 0 ? (hw == 0 ? 1 : hw) : requested;
  return std::max<std::size_t>(1, std::min(baseline, total_records));
}

}  // namespace

std::size_t ParallelCongestionQuery::CountSpeedBelow(const TrafficDataset& dataset,
                                                     double threshold_mph,
                                                     std::size_t num_threads) {
  const auto& rows = dataset.Records();
  const std::size_t n = rows.size();
  if (n == 0) {
    return 0;
  }

  const std::size_t threads = NormalizeThreadCount(num_threads, n);
#if !defined(_OPENMP)
  (void)threads;
#endif

  std::size_t total = 0;
#pragma omp parallel for num_threads(threads) reduction(+ : total)
  for (std::int64_t i = 0; i < static_cast<std::int64_t>(n); ++i) {
    if (rows[static_cast<std::size_t>(i)].speed_mph < threshold_mph) {
      ++total;
    }
  }

  return total;
}

std::size_t ParallelCongestionQuery::CountBoroughAndSpeedBelow(const TrafficDataset& dataset,
                                                               const std::string& borough,
                                                               double threshold_mph,
                                                               std::size_t num_threads) {
  const auto& rows = dataset.Records();
  const std::size_t n = rows.size();
  if (n == 0) {
    return 0;
  }

  const std::size_t threads = NormalizeThreadCount(num_threads, n);
  const std::int16_t borough_code = ToInt(ParseBoroughCode(borough));
#if !defined(_OPENMP)
  (void)threads;
#endif

  std::size_t total = 0;
#pragma omp parallel for num_threads(threads) reduction(+ : total)
  for (std::int64_t i = 0; i < static_cast<std::int64_t>(n); ++i) {
    const auto& row = rows[static_cast<std::size_t>(i)];
    const bool borough_matches =
        (borough_code >= 0 && row.borough_code == borough_code) ||
        (borough_code < 0 && row.borough == borough);
    if (borough_matches && row.speed_mph < threshold_mph) {
      ++total;
    }
  }

  return total;
}

std::size_t ParallelCongestionQuery::CountTimeWindow(const TrafficDataset& dataset,
                                                     std::int64_t start_epoch_seconds,
                                                     std::int64_t end_epoch_seconds,
                                                     std::size_t num_threads) {
  const auto& rows = dataset.Records();
  const std::size_t n = rows.size();
  if (n == 0) {
    return 0;
  }

  if (end_epoch_seconds < start_epoch_seconds) {
    std::swap(start_epoch_seconds, end_epoch_seconds);
  }

  const std::size_t threads = NormalizeThreadCount(num_threads, n);
#if !defined(_OPENMP)
  (void)threads;
#endif

  std::size_t total = 0;
#pragma omp parallel for num_threads(threads) reduction(+ : total)
  for (std::int64_t i = 0; i < static_cast<std::int64_t>(n); ++i) {
    const auto ts = rows[static_cast<std::size_t>(i)].timestamp_epoch_seconds;
    if (ts >= start_epoch_seconds && ts <= end_epoch_seconds) {
      ++total;
    }
  }

  return total;
}

std::vector<LinkSpeedStat> ParallelCongestionQuery::TopNSlowestRecurringLinks(
    const TrafficDataset& dataset,
    std::size_t n,
    std::size_t min_samples_per_link,
    std::size_t num_threads) {
  if (n == 0 || dataset.Empty()) {
    return {};
  }

  const auto& rows = dataset.Records();
  const std::size_t count = rows.size();
  const std::size_t threads = NormalizeThreadCount(num_threads, count);

  std::vector<std::unordered_map<std::int64_t, PartialLinkStat>> local_maps(threads);

#pragma omp parallel num_threads(threads)
  {
#if defined(_OPENMP)
    const int tid_raw = omp_get_thread_num();
#else
    const int tid_raw = 0;
#endif
    const std::size_t tid = static_cast<std::size_t>(tid_raw);
    auto& local = local_maps[tid];

#pragma omp for
    for (std::int64_t i = 0; i < static_cast<std::int64_t>(count); ++i) {
      const auto& row = rows[static_cast<std::size_t>(i)];
      auto& slot = local[row.link_id];
      ++slot.count;
      slot.speed_sum += row.speed_mph;
    }
  }

  std::unordered_map<std::int64_t, PartialLinkStat> merged;
  for (const auto& local : local_maps) {
    for (const auto& entry : local) {
      auto& dst = merged[entry.first];
      dst.count += entry.second.count;
      dst.speed_sum += entry.second.speed_sum;
    }
  }

  std::vector<LinkSpeedStat> stats;
  stats.reserve(merged.size());
  for (const auto& entry : merged) {
    if (entry.second.count < min_samples_per_link) {
      continue;
    }
    LinkSpeedStat row;
    row.link_id = entry.first;
    row.record_count = entry.second.count;
    row.average_speed_mph = entry.second.speed_sum / static_cast<double>(entry.second.count);
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
