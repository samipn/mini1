#include "query/ParallelTrafficAggregator.hpp"

#include <algorithm>
#include <cstdint>

#if defined(_OPENMP)
#include <omp.h>
#endif

namespace urbandrop {
namespace {

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

AggregateStats ParallelTrafficAggregator::SummarizeDataset(const TrafficDataset& dataset,
                                                           std::size_t num_threads) {
  const auto& rows = dataset.Records();
  const std::size_t n = rows.size();
  if (n == 0) {
    return AggregateStats{};
  }

  const std::size_t threads = NormalizeThreadCount(num_threads, n);
#if defined(_OPENMP)
  omp_set_num_threads(static_cast<int>(threads));
#endif

  std::size_t total_count = 0;
  double speed_sum = 0.0;
  double travel_sum = 0.0;

#pragma omp parallel for reduction(+ : total_count, speed_sum, travel_sum)
  for (std::int64_t i = 0; i < static_cast<std::int64_t>(n); ++i) {
    const auto& row = rows[static_cast<std::size_t>(i)];
    ++total_count;
    speed_sum += row.speed_mph;
    travel_sum += row.travel_time_seconds;
  }

  AggregateStats stats;
  stats.record_count = total_count;
  if (total_count > 0) {
    stats.average_speed_mph = speed_sum / static_cast<double>(total_count);
    stats.average_travel_time_seconds = travel_sum / static_cast<double>(total_count);
  }
  return stats;
}

}  // namespace urbandrop
