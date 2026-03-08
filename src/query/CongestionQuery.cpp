#include "query/CongestionQuery.hpp"

#include <algorithm>
#include <unordered_map>

#include "data_model/CommonCodes.hpp"

namespace urbandrop {

std::vector<const TrafficRecord*> CongestionQuery::FilterSpeedBelow(const TrafficDataset& dataset,
                                                                    double threshold_mph) {
  std::vector<const TrafficRecord*> results;
  for (const auto& record : dataset.Records()) {
    if (record.speed_mph < threshold_mph) {
      results.push_back(&record);
    }
  }
  return results;
}

std::vector<const TrafficRecord*> CongestionQuery::FilterBoroughAndSpeedBelow(
    const TrafficDataset& dataset,
    const std::string& borough,
    double threshold_mph) {
  std::vector<const TrafficRecord*> results;
  const std::int16_t borough_code = ToInt(ParseBoroughCode(borough));
  for (const auto& record : dataset.Records()) {
    const bool borough_matches =
        (borough_code >= 0 && record.borough_code == borough_code) ||
        (borough_code < 0 && record.borough == borough);
    if (borough_matches && record.speed_mph < threshold_mph) {
      results.push_back(&record);
    }
  }
  return results;
}

std::vector<const TrafficRecord*> CongestionQuery::FindByLinkId(const TrafficDataset& dataset,
                                                                std::int64_t link_id) {
  std::vector<const TrafficRecord*> results;
  for (const auto& record : dataset.Records()) {
    if (record.link_id == link_id) {
      results.push_back(&record);
    }
  }
  return results;
}

std::vector<const TrafficRecord*> CongestionQuery::FindByLinkRange(const TrafficDataset& dataset,
                                                                   std::int64_t start_link_id,
                                                                   std::int64_t end_link_id) {
  std::vector<const TrafficRecord*> results;
  if (end_link_id < start_link_id) {
    std::swap(start_link_id, end_link_id);
  }

  for (const auto& record : dataset.Records()) {
    if (record.link_id >= start_link_id && record.link_id <= end_link_id) {
      results.push_back(&record);
    }
  }
  return results;
}

std::vector<LinkSpeedStat> CongestionQuery::TopNSlowestRecurringLinks(const TrafficDataset& dataset,
                                                                      std::size_t n,
                                                                      std::size_t min_samples_per_link) {
  struct Partial {
    std::size_t count = 0;
    double speed_sum = 0.0;
  };

  std::unordered_map<std::int64_t, Partial> grouped;
  grouped.reserve(dataset.Size());
  for (const auto& record : dataset.Records()) {
    auto& partial = grouped[record.link_id];
    ++partial.count;
    partial.speed_sum += record.speed_mph;
  }

  std::vector<LinkSpeedStat> stats;
  stats.reserve(grouped.size());
  for (const auto& entry : grouped) {
    if (entry.second.count < min_samples_per_link) {
      continue;
    }

    LinkSpeedStat stat;
    stat.link_id = entry.first;
    stat.record_count = entry.second.count;
    stat.average_speed_mph = entry.second.speed_sum / static_cast<double>(entry.second.count);
    stats.push_back(stat);
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
