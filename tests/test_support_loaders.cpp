#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "data_model/CommonCodes.hpp"
#include "data_model/BuildingRecord.hpp"
#include "data_model/GarageRecord.hpp"
#include "io/BuildingLoader.hpp"
#include "io/GarageLoader.hpp"

namespace {

std::string WriteGarageFixture() {
  const auto path = std::filesystem::temp_directory_path() / "urbandrop_garages_fixture.csv";
  std::ofstream out(path);
  out << "license_nbr,lic_expir_dd,license_creation_date,business_name,address_building,address_street_name,address_street_name_2,address_city,address_state,address_zip,contact_phone,address_borough,detail,nta,community_board,council_district,bin,bbl,census_tract,latitude,longitude\n";
  out << "G1,2026-12-31T00:00:00.000,2016-01-01T00:00:00.000,Garage One,100,Main St,Cross St,New York,NY,10001,2120000000,Manhattan,24hr service,NTA1,CB1,CD1,BIN1,BBL1,CT1,40.1,-73.9\n";
  out << "G2,invalid_timestamp,2016-01-02T00:00:00.000,Garage Two,200,Second St,,New York,NY,10002,2120000001,Brooklyn,limited,NTA2,CB2,CD2,BIN2,BBL2,CT2,40.2,-73.8\n";
  out << "G3,2026-12-31T00:00:00.000,2016-01-03T00:00:00.000,Garage Three,300,Third St,,New York,NY,10003,2120000002,Queens,none,NTA3,CB3,CD3,BIN3,BBL3,CT3,not_a_lat,-73.7\n";
  return path.string();
}

std::string WriteBesFixture() {
  const auto path = std::filesystem::temp_directory_path() / "urbandrop_bes_fixture.csv";
  std::ofstream out(path);
  out << "bbl,subgrade_level,elevation_feet,latitude,longitude\n";
  out << "1000000001,2,15.5,40.5,-73.8\n";
  out << "1000000002,1,not_number,40.4,-73.7\n";
  return path.string();
}

}  // namespace

int main() {
  {
    std::vector<urbandrop::GarageRecord> garages;
    urbandrop::LoaderStats stats;
    std::string error;

    if (!urbandrop::GarageLoader::LoadCSV(WriteGarageFixture(), &garages, &stats, &error)) {
      std::cerr << "garage load failed: " << error << "\n";
      return EXIT_FAILURE;
    }

    if (stats.rows_read != 3 || stats.rows_accepted != 2 || stats.rows_rejected != 1) {
      std::cerr << "garage stats mismatch\n";
      return EXIT_FAILURE;
    }
    if (garages.size() != 2 || garages[0].license_number != "G1" || garages[0].bbl != "BBL1" ||
        !garages[0].has_location || garages[1].has_location ||
        garages[0].borough_code != urbandrop::ToInt(urbandrop::BoroughCode::kManhattan) ||
        garages[1].borough_code != urbandrop::ToInt(urbandrop::BoroughCode::kQueens)) {
      std::cerr << "garage metadata mapping mismatch\n";
      return EXIT_FAILURE;
    }

    // Reusing output vector should replace contents, not append.
    garages.push_back(urbandrop::GarageRecord{});
    if (!urbandrop::GarageLoader::LoadCSV(WriteGarageFixture(), &garages, &stats, &error)) {
      std::cerr << "garage second load failed: " << error << "\n";
      return EXIT_FAILURE;
    }
    if (garages.size() != 2 || stats.rows_read != 3 || stats.rows_accepted != 2 ||
        stats.rows_rejected != 1) {
      std::cerr << "garage loader should replace output vector per load\n";
      return EXIT_FAILURE;
    }
  }

  {
    std::vector<urbandrop::BuildingRecord> buildings;
    urbandrop::LoaderStats stats;
    std::string error;

    if (!urbandrop::BuildingLoader::LoadCSV(WriteBesFixture(), &buildings, &stats, &error)) {
      std::cerr << "building load failed: " << error << "\n";
      return EXIT_FAILURE;
    }

    if (stats.rows_read != 2 || stats.rows_accepted != 1 || stats.rows_rejected != 1) {
      std::cerr << "building stats mismatch\n";
      return EXIT_FAILURE;
    }

    // Reusing stats object should not accumulate across calls.
    stats.rows_read = 999;
    stats.rows_accepted = 999;
    stats.rows_rejected = 999;
    std::vector<urbandrop::BuildingRecord> buildings_second;
    if (!urbandrop::BuildingLoader::LoadCSV(WriteBesFixture(), &buildings_second, &stats, &error)) {
      std::cerr << "building second load failed: " << error << "\n";
      return EXIT_FAILURE;
    }
    if (stats.rows_read != 2 || stats.rows_accepted != 1 || stats.rows_rejected != 1) {
      std::cerr << "building stats should reset per load\n";
      return EXIT_FAILURE;
    }

    // Reusing output vector should replace contents, not append.
    buildings_second.push_back(urbandrop::BuildingRecord{});
    if (!urbandrop::BuildingLoader::LoadCSV(WriteBesFixture(), &buildings_second, &stats, &error)) {
      std::cerr << "building third load failed: " << error << "\n";
      return EXIT_FAILURE;
    }
    if (buildings_second.size() != 1 || stats.rows_read != 2 || stats.rows_accepted != 1 ||
        stats.rows_rejected != 1) {
      std::cerr << "building loader should replace output vector per load\n";
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
