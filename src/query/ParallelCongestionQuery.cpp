#include "query/ParallelCongestionQuery.hpp"

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
#if defined(_OPENMP)
  omp_set_num_threads(static_cast<int>(threads));
#endif

  std::size_t total = 0;
#pragma omp parallel for reduction(+ : total)
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
#if defined(_OPENMP)
  omp_set_num_threads(static_cast<int>(threads));
#endif

  std::size_t total = 0;
#pragma omp parallel for reduction(+ : total)
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

}  // namespace urbandrop
