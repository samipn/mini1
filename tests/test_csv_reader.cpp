#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "data_model/CommonCodes.hpp"
#include "data_model/TrafficDataset.hpp"
#include "io/CSVReader.hpp"

namespace {

std::string WriteFixtureCsv() {
  const auto path = std::filesystem::temp_directory_path() / "urbandrop_phase1_fixture.csv";
  std::ofstream out(path);
  out << "id,speed,travel_time,status,data_as_of,link_id,borough,link_name\n";
  out << "1,10.5,35.2,ok,2024-01-01T10:00:00,1001,Manhattan,\"FDR Drive\"\n";
  out << "2,15.0,40.0,ok,2024-01-01 10:05:00,1002,Brooklyn,\"Atlantic, Ave\"\n";
  out << "3,foo,39.0,ok,2024-01-01T10:10:00,1003,Queens,Main St\n";
  out << "4,20.0,42.0,ok,,1004,Bronx,Grand Concourse\n";
  return path.string();
}

}  // namespace

int main() {
  const std::string csv_path = WriteFixtureCsv();

  urbandrop::TrafficDataset dataset;
  urbandrop::TrafficLoadOptions options;
  options.print_progress = false;

  std::string error;
  if (!urbandrop::CSVReader::LoadTrafficCSV(csv_path, &dataset, options, nullptr, &error)) {
    std::cerr << "load failed: " << error << "\n";
    return EXIT_FAILURE;
  }

  const auto& counters = dataset.Counters();
  if (counters.rows_read != 4) {
    std::cerr << "expected rows_read=4 got " << counters.rows_read << "\n";
    return EXIT_FAILURE;
  }
  if (counters.rows_accepted != 2) {
    std::cerr << "expected rows_accepted=2 got " << counters.rows_accepted << "\n";
    return EXIT_FAILURE;
  }
  if (counters.rows_rejected != 2) {
    std::cerr << "expected rows_rejected=2 got " << counters.rows_rejected << "\n";
    return EXIT_FAILURE;
  }
  if (counters.malformed_rows != 1 || counters.missing_timestamp != 1) {
    std::cerr << "unexpected reject reason counters\n";
    return EXIT_FAILURE;
  }

  if (dataset.Records().at(1).link_name != "Atlantic, Ave") {
    std::cerr << "expected quoted comma field parsing to preserve link_name\n";
    return EXIT_FAILURE;
  }
  if (dataset.Records().at(0).borough_code !=
          urbandrop::ToInt(urbandrop::BoroughCode::kManhattan) ||
      dataset.Records().at(1).borough_code !=
          urbandrop::ToInt(urbandrop::BoroughCode::kBrooklyn)) {
    std::cerr << "expected borough string to borough_code normalization\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
