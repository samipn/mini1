#include "query/CrossDatasetIndex.hpp"

#include <algorithm>
#include <chrono>
#include <unordered_set>
#include <utility>
#include <vector>

#if defined(__has_include)
#if __has_include(<boost/container/flat_map.hpp>)
#define URBANDROP_HAS_BOOST_FLAT_MAP 1
#include <boost/container/flat_map.hpp>
#endif
#if __has_include(<boost/unordered/unordered_flat_map.hpp>)
#define URBANDROP_HAS_BOOST_UNORDERED_FLAT_MAP 1
#include <boost/unordered/unordered_flat_map.hpp>
#endif
#if __has_include(<boost/dynamic_bitset.hpp>)
#define URBANDROP_HAS_BOOST_DYNAMIC_BITSET 1
#include <boost/dynamic_bitset.hpp>
#endif
#endif

namespace urbandrop {
namespace {

using Clock = std::chrono::steady_clock;

bool IsNonEmpty(const std::string& value) {
  return !value.empty();
}

}  // namespace

CrossDatasetIndex CrossDatasetIndex::Build(const TrafficDataset& traffic,
                                           const std::vector<GarageRecord>& garages,
                                           const std::vector<BuildingRecord>& buildings,
                                           const CrossDatasetBuildOptions& options) {
  CrossDatasetIndex index;

  if (options.build_link_id_index) {
    for (const auto& record : traffic) {
      ++index.traffic_by_link_id_count_[record.link_id];
    }
  }

  if (options.build_borough_rollups) {
    for (const auto& record : traffic) {
      ++index.traffic_by_borough_count_[record.borough_code];
    }
    for (const auto& garage : garages) {
      ++index.garages_by_borough_count_[garage.borough_code];
    }
  }

  if (options.build_bbl_indexes) {
    for (const auto& garage : garages) {
      if (IsNonEmpty(garage.bbl)) {
        ++index.garages_by_bbl_count_[garage.bbl];
      }
    }
    for (const auto& building : buildings) {
      if (IsNonEmpty(building.bbl)) {
        ++index.buildings_by_bbl_count_[building.bbl];
      }
    }
  }

  return index;
}

std::size_t CrossDatasetIndex::LookupTrafficCountByLinkId(std::int64_t link_id) const {
  const auto it = traffic_by_link_id_count_.find(link_id);
  return it == traffic_by_link_id_count_.end() ? 0 : it->second;
}

std::size_t CrossDatasetIndex::LookupGarageCountByBbl(const std::string& bbl) const {
  const auto it = garages_by_bbl_count_.find(bbl);
  return it == garages_by_bbl_count_.end() ? 0 : it->second;
}

std::size_t CrossDatasetIndex::LookupBuildingCountByBbl(const std::string& bbl) const {
  const auto it = buildings_by_bbl_count_.find(bbl);
  return it == buildings_by_bbl_count_.end() ? 0 : it->second;
}

std::size_t CrossDatasetIndex::SharedBblKeyCount() const {
  std::size_t count = 0;
  for (const auto& kv : garages_by_bbl_count_) {
    if (buildings_by_bbl_count_.find(kv.first) != buildings_by_bbl_count_.end()) {
      ++count;
    }
  }
  return count;
}

std::vector<JoinedBblStat> CrossDatasetIndex::MaterializeSharedBblStats() const {
  std::vector<JoinedBblStat> stats;
  stats.reserve(SharedBblKeyCount());

  for (const auto& kv : garages_by_bbl_count_) {
    const auto it = buildings_by_bbl_count_.find(kv.first);
    if (it == buildings_by_bbl_count_.end()) {
      continue;
    }
    JoinedBblStat row;
    row.bbl = kv.first;
    row.garage_count = kv.second;
    row.building_count = it->second;
    stats.push_back(row);
  }

  std::sort(stats.begin(), stats.end(), [](const JoinedBblStat& lhs, const JoinedBblStat& rhs) {
    if (lhs.bbl == rhs.bbl) {
      return lhs.garage_count < rhs.garage_count;
    }
    return lhs.bbl < rhs.bbl;
  });

  return stats;
}

std::vector<BoroughRollupStat> CrossDatasetIndex::MaterializeBoroughRollups() const {
  std::unordered_set<std::int16_t> keys;
  keys.reserve(traffic_by_borough_count_.size() + garages_by_borough_count_.size());

  for (const auto& kv : traffic_by_borough_count_) {
    keys.insert(kv.first);
  }
  for (const auto& kv : garages_by_borough_count_) {
    keys.insert(kv.first);
  }

  std::vector<BoroughRollupStat> rollups;
  rollups.reserve(keys.size());

  for (const std::int16_t code : keys) {
    BoroughRollupStat row;
    row.borough_code = code;

    const auto traffic_it = traffic_by_borough_count_.find(code);
    if (traffic_it != traffic_by_borough_count_.end()) {
      row.traffic_record_count = traffic_it->second;
    }

    const auto garage_it = garages_by_borough_count_.find(code);
    if (garage_it != garages_by_borough_count_.end()) {
      row.garage_record_count = garage_it->second;
    }

    rollups.push_back(row);
  }

  std::sort(rollups.begin(), rollups.end(), [](const BoroughRollupStat& lhs, const BoroughRollupStat& rhs) {
    return lhs.borough_code < rhs.borough_code;
  });

  return rollups;
}

std::size_t CrossDatasetIndex::LinkIdKeyCount() const {
  return traffic_by_link_id_count_.size();
}

std::size_t CrossDatasetIndex::GarageBblKeyCount() const {
  return garages_by_bbl_count_.size();
}

std::size_t CrossDatasetIndex::BuildingBblKeyCount() const {
  return buildings_by_bbl_count_.size();
}

BoostIndexEvaluation CrossDatasetIndex::EvaluateBoostIndexing(const std::vector<GarageRecord>& garages,
                                                              const std::vector<BuildingRecord>& buildings,
                                                              std::size_t repeats) {
  BoostIndexEvaluation result;
  const std::size_t safe_repeats = repeats == 0 ? 1 : repeats;

#if defined(URBANDROP_HAS_BOOST_FLAT_MAP)
  result.flat_map_available = true;
  {
    const auto started = Clock::now();
    for (std::size_t i = 0; i < safe_repeats; ++i) {
      std::unordered_map<std::string, std::pair<std::size_t, std::size_t>> temp;
      for (const auto& garage : garages) {
        if (garage.bbl.empty()) {
          continue;
        }
        ++temp[garage.bbl].first;
      }
      for (const auto& building : buildings) {
        if (building.bbl.empty()) {
          continue;
        }
        ++temp[building.bbl].second;
      }

      boost::container::flat_map<std::string, std::pair<std::size_t, std::size_t>> flat;
      for (const auto& kv : temp) {
        flat.emplace(kv.first, kv.second);
      }
    }
    const auto finished = Clock::now();
    result.flat_map_build_ms =
        std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(finished - started).count() /
        static_cast<double>(safe_repeats);
  }
#endif

#if defined(URBANDROP_HAS_BOOST_UNORDERED_FLAT_MAP)
  result.unordered_flat_map_available = true;
  {
    const auto started = Clock::now();
    for (std::size_t i = 0; i < safe_repeats; ++i) {
      boost::unordered::unordered_flat_map<std::string, std::pair<std::size_t, std::size_t>> map;
      map.reserve(garages.size() + buildings.size());

      for (const auto& garage : garages) {
        if (garage.bbl.empty()) {
          continue;
        }
        ++map[garage.bbl].first;
      }
      for (const auto& building : buildings) {
        if (building.bbl.empty()) {
          continue;
        }
        ++map[building.bbl].second;
      }
    }
    const auto finished = Clock::now();
    result.unordered_flat_map_build_ms =
        std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(finished - started).count() /
        static_cast<double>(safe_repeats);
  }
#endif

#if defined(URBANDROP_HAS_BOOST_DYNAMIC_BITSET)
  result.dynamic_bitset_available = true;
  {
    std::unordered_map<std::string, std::size_t> key_to_index;
    key_to_index.reserve(garages.size() + buildings.size());

    for (const auto& garage : garages) {
      if (garage.bbl.empty()) {
        continue;
      }
      key_to_index.emplace(garage.bbl, key_to_index.size());
    }
    for (const auto& building : buildings) {
      if (building.bbl.empty()) {
        continue;
      }
      key_to_index.emplace(building.bbl, key_to_index.size());
    }

    const auto started = Clock::now();
    for (std::size_t i = 0; i < safe_repeats; ++i) {
      boost::dynamic_bitset<> garage_mask(key_to_index.size());
      boost::dynamic_bitset<> building_mask(key_to_index.size());

      for (const auto& garage : garages) {
        if (garage.bbl.empty()) {
          continue;
        }
        garage_mask.set(key_to_index[garage.bbl]);
      }
      for (const auto& building : buildings) {
        if (building.bbl.empty()) {
          continue;
        }
        building_mask.set(key_to_index[building.bbl]);
      }

      const boost::dynamic_bitset<> shared = garage_mask & building_mask;
      result.dynamic_bitset_shared_bbl_count = shared.count();
    }
    const auto finished = Clock::now();
    result.dynamic_bitset_build_ms =
        std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(finished - started).count() /
        static_cast<double>(safe_repeats);
  }
#endif

  return result;
}

}  // namespace urbandrop
