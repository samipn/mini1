#include "query/ParallelTrafficAggregator.hpp"

#include <algorithm>
#include <cstdint>

#if defined(_OPENMP)
#include <omp.h>
#endif

#include "data_model/CommonCodes.hpp"

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

AggregateStats SummarizeConditional(const TrafficDataset& dataset,
                                    std::size_t num_threads,
                                    bool (*predicate)(const TrafficRecord&, const void*),
                                    const void* context) {
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
    if (!predicate(row, context)) {
      continue;
    }
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

struct SpeedContext {
  double threshold = 0.0;
};

bool SpeedPredicate(const TrafficRecord& row, const void* context) {
  const auto* cfg = static_cast<const SpeedContext*>(context);
  return row.speed_mph < cfg->threshold;
}

struct TimeWindowContext {
  std::int64_t start_epoch = 0;
  std::int64_t end_epoch = 0;
};

bool TimeWindowPredicate(const TrafficRecord& row, const void* context) {
  const auto* cfg = static_cast<const TimeWindowContext*>(context);
  return row.timestamp_epoch_seconds >= cfg->start_epoch && row.timestamp_epoch_seconds <= cfg->end_epoch;
}

struct BoroughSpeedContext {
  std::int16_t borough_code = -1;
  std::string borough;
  double threshold = 0.0;
};

bool BoroughSpeedPredicate(const TrafficRecord& row, const void* context) {
  const auto* cfg = static_cast<const BoroughSpeedContext*>(context);
  const bool borough_matches =
      (cfg->borough_code >= 0 && row.borough_code == cfg->borough_code) ||
      (cfg->borough_code < 0 && row.borough == cfg->borough);
  return borough_matches && row.speed_mph < cfg->threshold;
}

bool AlwaysTruePredicate(const TrafficRecord&, const void*) { return true; }

}  // namespace

AggregateStats ParallelTrafficAggregator::SummarizeDataset(const TrafficDataset& dataset,
                                                           std::size_t num_threads) {
  return SummarizeConditional(dataset, num_threads, AlwaysTruePredicate, nullptr);
}

AggregateStats ParallelTrafficAggregator::SummarizeSpeedBelow(const TrafficDataset& dataset,
                                                              double threshold_mph,
                                                              std::size_t num_threads) {
  const SpeedContext cfg{threshold_mph};
  return SummarizeConditional(dataset, num_threads, SpeedPredicate, &cfg);
}

AggregateStats ParallelTrafficAggregator::SummarizeTimeWindow(const TrafficDataset& dataset,
                                                              std::int64_t start_epoch_seconds,
                                                              std::int64_t end_epoch_seconds,
                                                              std::size_t num_threads) {
  if (end_epoch_seconds < start_epoch_seconds) {
    std::swap(start_epoch_seconds, end_epoch_seconds);
  }
  const TimeWindowContext cfg{start_epoch_seconds, end_epoch_seconds};
  return SummarizeConditional(dataset, num_threads, TimeWindowPredicate, &cfg);
}

AggregateStats ParallelTrafficAggregator::SummarizeBoroughAndSpeedBelow(const TrafficDataset& dataset,
                                                                        const std::string& borough,
                                                                        double threshold_mph,
                                                                        std::size_t num_threads) {
  BoroughSpeedContext cfg;
  cfg.borough_code = ToInt(ParseBoroughCode(borough));
  cfg.borough = borough;
  cfg.threshold = threshold_mph;
  return SummarizeConditional(dataset, num_threads, BoroughSpeedPredicate, &cfg);
}

}  // namespace urbandrop
