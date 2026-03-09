#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "data_model/BuildingRecord.hpp"
#include "data_model/GarageRecord.hpp"
#include "data_model/TrafficDataset.hpp"

namespace urbandrop {

struct JoinedBblStat {
  std::string bbl;
  std::size_t garage_count = 0;
  std::size_t building_count = 0;
};

struct BoroughRollupStat {
  std::int16_t borough_code = -1;
  std::size_t traffic_record_count = 0;
  std::size_t garage_record_count = 0;
};

struct CrossDatasetBuildOptions {
  bool build_link_id_index = true;
  bool build_bbl_indexes = true;
  bool build_borough_rollups = true;
};

struct BoostIndexEvaluation {
  bool flat_map_available = false;
  bool unordered_flat_map_available = false;
  bool dynamic_bitset_available = false;
  double flat_map_build_ms = -1.0;
  double unordered_flat_map_build_ms = -1.0;
  double dynamic_bitset_build_ms = -1.0;
  std::size_t dynamic_bitset_shared_bbl_count = 0;
};

class CrossDatasetIndex {
 public:
  static CrossDatasetIndex Build(const TrafficDataset& traffic,
                                 const std::vector<GarageRecord>& garages,
                                 const std::vector<BuildingRecord>& buildings,
                                 const CrossDatasetBuildOptions& options);

  std::size_t LookupTrafficCountByLinkId(std::int64_t link_id) const;
  std::size_t LookupGarageCountByBbl(const std::string& bbl) const;
  std::size_t LookupBuildingCountByBbl(const std::string& bbl) const;

  std::size_t SharedBblKeyCount() const;
  std::vector<JoinedBblStat> MaterializeSharedBblStats() const;
  std::vector<BoroughRollupStat> MaterializeBoroughRollups() const;

  std::size_t LinkIdKeyCount() const;
  std::size_t GarageBblKeyCount() const;
  std::size_t BuildingBblKeyCount() const;

  static BoostIndexEvaluation EvaluateBoostIndexing(const std::vector<GarageRecord>& garages,
                                                    const std::vector<BuildingRecord>& buildings,
                                                    std::size_t repeats);

 private:
  std::unordered_map<std::int64_t, std::size_t> traffic_by_link_id_count_;
  std::unordered_map<std::string, std::size_t> garages_by_bbl_count_;
  std::unordered_map<std::string, std::size_t> buildings_by_bbl_count_;
  std::unordered_map<std::int16_t, std::size_t> traffic_by_borough_count_;
  std::unordered_map<std::int16_t, std::size_t> garages_by_borough_count_;
};

}  // namespace urbandrop
