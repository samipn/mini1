#include <cstdlib>
#include <iostream>
#include <vector>

#include "data_model/BuildingRecord.hpp"
#include "data_model/GarageRecord.hpp"
#include "data_model/TrafficDataset.hpp"
#include "query/CrossDatasetIndex.hpp"

int main() {
  urbandrop::TrafficDataset traffic;

  urbandrop::TrafficRecord r1;
  r1.link_id = 100;
  r1.borough_code = 1;
  traffic.AddRecord(r1);

  urbandrop::TrafficRecord r2;
  r2.link_id = 100;
  r2.borough_code = 1;
  traffic.AddRecord(r2);

  urbandrop::TrafficRecord r3;
  r3.link_id = 200;
  r3.borough_code = 2;
  traffic.AddRecord(r3);

  std::vector<urbandrop::GarageRecord> garages;
  urbandrop::GarageRecord g1;
  g1.bbl = "1-00001-0001";
  g1.borough_code = 1;
  garages.push_back(g1);

  urbandrop::GarageRecord g2;
  g2.bbl = "1-00001-0001";
  g2.borough_code = 1;
  garages.push_back(g2);

  urbandrop::GarageRecord g3;
  g3.bbl = "2-00002-0002";
  g3.borough_code = 2;
  garages.push_back(g3);

  std::vector<urbandrop::BuildingRecord> buildings;
  urbandrop::BuildingRecord b1;
  b1.bbl = "1-00001-0001";
  buildings.push_back(b1);

  urbandrop::BuildingRecord b2;
  b2.bbl = "3-00003-0003";
  buildings.push_back(b2);

  const urbandrop::CrossDatasetBuildOptions options;
  const urbandrop::CrossDatasetIndex index =
      urbandrop::CrossDatasetIndex::Build(traffic, garages, buildings, options);

  if (index.LookupTrafficCountByLinkId(100) != 2 || index.LookupTrafficCountByLinkId(200) != 1 ||
      index.LookupTrafficCountByLinkId(999) != 0) {
    std::cerr << "link-id index lookup mismatch\n";
    return EXIT_FAILURE;
  }

  if (index.LookupGarageCountByBbl("1-00001-0001") != 2 ||
      index.LookupBuildingCountByBbl("1-00001-0001") != 1 ||
      index.LookupGarageCountByBbl("missing") != 0) {
    std::cerr << "bbl index lookup mismatch\n";
    return EXIT_FAILURE;
  }

  if (index.SharedBblKeyCount() != 1) {
    std::cerr << "shared bbl key count mismatch\n";
    return EXIT_FAILURE;
  }

  const auto joins = index.MaterializeSharedBblStats();
  if (joins.size() != 1 || joins[0].bbl != "1-00001-0001" || joins[0].garage_count != 2 ||
      joins[0].building_count != 1) {
    std::cerr << "materialized bbl join mismatch\n";
    return EXIT_FAILURE;
  }

  const auto borough_rollups = index.MaterializeBoroughRollups();
  if (borough_rollups.size() < 2) {
    std::cerr << "borough rollup size mismatch\n";
    return EXIT_FAILURE;
  }

  bool saw_b1 = false;
  bool saw_b2 = false;
  for (const auto& row : borough_rollups) {
    if (row.borough_code == 1) {
      saw_b1 = true;
      if (row.traffic_record_count != 2 || row.garage_record_count != 2) {
        std::cerr << "borough code 1 rollup mismatch\n";
        return EXIT_FAILURE;
      }
    }
    if (row.borough_code == 2) {
      saw_b2 = true;
      if (row.traffic_record_count != 1 || row.garage_record_count != 1) {
        std::cerr << "borough code 2 rollup mismatch\n";
        return EXIT_FAILURE;
      }
    }
  }
  if (!saw_b1 || !saw_b2) {
    std::cerr << "missing borough rollup rows\n";
    return EXIT_FAILURE;
  }

  const urbandrop::BoostIndexEvaluation boost_eval =
      urbandrop::CrossDatasetIndex::EvaluateBoostIndexing(garages, buildings, 2);
  if (boost_eval.flat_map_available && boost_eval.flat_map_build_ms < 0.0) {
    std::cerr << "boost flat_map timing should be non-negative when available\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
