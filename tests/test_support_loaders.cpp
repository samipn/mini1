#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "data_model/BuildingRecord.hpp"
#include "data_model/GarageRecord.hpp"
#include "io/BuildingLoader.hpp"
#include "io/GarageLoader.hpp"

namespace {

std::string WriteGarageFixture() {
  const auto path = std::filesystem::temp_directory_path() / "urbandrop_garages_fixture.csv";
  std::ofstream out(path);
  out << "license_number,business_name,borough,address,latitude,longitude\n";
  out << "G1,Garage One,Manhattan,100 Main St,40.1,-73.9\n";
  out << "G2,Garage Two,Brooklyn,200 Main St,not_a_lat,-73.8\n";
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

    if (stats.rows_read != 2 || stats.rows_accepted != 1 || stats.rows_rejected != 1) {
      std::cerr << "garage stats mismatch\n";
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
  }

  return EXIT_SUCCESS;
}
