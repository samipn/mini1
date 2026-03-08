#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "data_model/BuildingRecord.hpp"
#include "data_model/GarageRecord.hpp"
#include "data_model/TrafficDataset.hpp"
#include "io/BuildingLoader.hpp"
#include "io/CSVReader.hpp"
#include "io/GarageLoader.hpp"

namespace {

void PrintUsage() {
  std::cout << "Usage: run_serial --traffic <path> [--garages <path>] [--bes <path>] "
               "[--progress-every <rows>] [--sample <count>]\\n";
}

}  // namespace

int main(int argc, char** argv) {
  std::string traffic_csv;
  std::string garages_csv;
  std::string bes_csv;
  std::size_t progress_every = 500000;
  std::size_t sample_count = 0;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--traffic" && i + 1 < argc) {
      traffic_csv = argv[++i];
    } else if (arg == "--garages" && i + 1 < argc) {
      garages_csv = argv[++i];
    } else if (arg == "--bes" && i + 1 < argc) {
      bes_csv = argv[++i];
    } else if (arg == "--progress-every" && i + 1 < argc) {
      progress_every = static_cast<std::size_t>(std::stoull(argv[++i]));
    } else if (arg == "--sample" && i + 1 < argc) {
      sample_count = static_cast<std::size_t>(std::stoull(argv[++i]));
    } else if (arg == "--help") {
      PrintUsage();
      return 0;
    } else {
      std::cerr << "Unknown or incomplete argument: " << arg << "\\n";
      PrintUsage();
      return 2;
    }
  }

  if (traffic_csv.empty()) {
    PrintUsage();
    return 2;
  }

  urbandrop::TrafficDataset dataset;
  urbandrop::TrafficLoadOptions options;
  options.progress_every_rows = progress_every;
  options.print_progress = true;

  std::string error;
  std::cout << "[ingest] start path=" << traffic_csv << "\\n";
  if (!urbandrop::CSVReader::LoadTrafficCSV(traffic_csv, &dataset, options, &std::cout, &error)) {
    std::cerr << "[ingest] failed: " << error << "\\n";
    return 1;
  }

  const urbandrop::IngestionCounters& counters = dataset.Counters();
  std::cout << "[ingest] complete\\n";
  std::cout << "  rows_read=" << counters.rows_read << "\\n";
  std::cout << "  rows_accepted=" << counters.rows_accepted << "\\n";
  std::cout << "  rows_rejected=" << counters.rows_rejected << "\\n";
  std::cout << "  malformed_rows=" << counters.malformed_rows << "\\n";
  std::cout << "  missing_timestamp=" << counters.missing_timestamp << "\\n";
  std::cout << "  suspicious_speed=" << counters.suspicious_speed << "\\n";
  std::cout << "  suspicious_travel_time=" << counters.suspicious_travel_time << "\\n";

  if (sample_count > 0) {
    std::cout << "[sample] showing up to " << sample_count << " accepted rows\\n";
    const auto& rows = dataset.Records();
    for (std::size_t i = 0; i < rows.size() && i < sample_count; ++i) {
      const auto& r = rows[i];
      std::cout << "  #" << i << " link_id=" << r.link_id << " speed_mph=" << r.speed_mph
                << " travel_time_seconds=" << r.travel_time_seconds
                << " timestamp_epoch_seconds=" << r.timestamp_epoch_seconds
                << " borough=\"" << r.borough << "\""
                << " link_name=\"" << r.link_name << "\"\\n";
    }
  }

  if (!garages_csv.empty()) {
    std::vector<urbandrop::GarageRecord> garages;
    urbandrop::LoaderStats stats;
    if (!urbandrop::GarageLoader::LoadCSV(garages_csv, &garages, &stats, &error)) {
      std::cerr << "[garages] failed: " << error << "\n";
      return 1;
    }
    std::cout << "[garages] complete rows_read=" << stats.rows_read
              << " accepted=" << stats.rows_accepted
              << " rejected=" << stats.rows_rejected << "\n";
  }

  if (!bes_csv.empty()) {
    std::vector<urbandrop::BuildingRecord> buildings;
    urbandrop::LoaderStats stats;
    if (!urbandrop::BuildingLoader::LoadCSV(bes_csv, &buildings, &stats, &error)) {
      std::cerr << "[bes] failed: " << error << "\n";
      return 1;
    }
    std::cout << "[bes] complete rows_read=" << stats.rows_read
              << " accepted=" << stats.rows_accepted
              << " rejected=" << stats.rows_rejected << "\n";
  }

  return 0;
}
