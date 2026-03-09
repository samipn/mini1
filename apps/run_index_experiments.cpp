#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <numeric>
#include <string>
#include <unordered_set>
#include <vector>

#include "data_model/BuildingRecord.hpp"
#include "data_model/GarageRecord.hpp"
#include "data_model/TrafficDataset.hpp"
#include "io/BuildingLoader.hpp"
#include "io/CSVReader.hpp"
#include "io/GarageLoader.hpp"
#include "query/CrossDatasetIndex.hpp"

namespace {

using Clock = std::chrono::steady_clock;

void PrintUsage() {
  std::cout
      << "Usage: run_index_experiments --traffic <path> [--garages <path>] [--bes <path>] [--repeats <n>] [--output <csv>]\n"
      << "\n"
      << "Runs baseline scan vs index lookup experiments and optional Boost index evaluations.\n";
}

std::string EscapeCsv(const std::string& value) {
  if (value.find(',') == std::string::npos && value.find('"') == std::string::npos) {
    return value;
  }
  std::string escaped = "\"";
  for (char c : value) {
    if (c == '"') {
      escaped += "\"\"";
    } else {
      escaped.push_back(c);
    }
  }
  escaped += "\"";
  return escaped;
}

void AppendCsvRow(const std::string& out_path,
                  const std::string& traffic_path,
                  const std::string& garages_path,
                  const std::string& bes_path,
                  std::size_t repeats,
                  const urbandrop::TrafficDataset& traffic,
                  const std::vector<urbandrop::GarageRecord>& garages,
                  const std::vector<urbandrop::BuildingRecord>& buildings,
                  std::size_t unique_link_keys,
                  std::size_t shared_bbl_keys,
                  double baseline_link_scan_ms,
                  double indexed_link_lookup_ms,
                  double baseline_bbl_scan_ms,
                  double indexed_bbl_lookup_ms,
                  double index_build_ms,
                  const urbandrop::BoostIndexEvaluation& boost_eval) {
  const bool write_header = !std::ifstream(out_path).good();
  std::ofstream out(out_path, std::ios::app);
  if (!out.is_open()) {
    std::cerr << "failed to write output CSV: " << out_path << "\n";
    return;
  }

  if (write_header) {
    out << "traffic_path,garages_path,bes_path,repeats,traffic_rows,garage_rows,building_rows,"
           "unique_link_keys,shared_bbl_keys,index_build_ms,baseline_link_scan_ms,indexed_link_lookup_ms,"
           "baseline_bbl_scan_ms,indexed_bbl_lookup_ms,boost_flat_map_available,boost_flat_map_build_ms,"
           "boost_unordered_flat_map_available,boost_unordered_flat_map_build_ms,"
           "boost_dynamic_bitset_available,boost_dynamic_bitset_build_ms,"
           "boost_dynamic_bitset_shared_bbl_count\n";
  }

  out << EscapeCsv(traffic_path) << ',' << EscapeCsv(garages_path) << ',' << EscapeCsv(bes_path) << ','
      << repeats << ',' << traffic.Counters().rows_accepted << ',' << garages.size() << ','
      << buildings.size() << ',' << unique_link_keys << ',' << shared_bbl_keys << ',' << index_build_ms << ','
      << baseline_link_scan_ms << ',' << indexed_link_lookup_ms << ',' << baseline_bbl_scan_ms << ','
      << indexed_bbl_lookup_ms << ',' << (boost_eval.flat_map_available ? "true" : "false") << ','
      << boost_eval.flat_map_build_ms << ','
      << (boost_eval.unordered_flat_map_available ? "true" : "false") << ','
      << boost_eval.unordered_flat_map_build_ms << ','
      << (boost_eval.dynamic_bitset_available ? "true" : "false") << ','
      << boost_eval.dynamic_bitset_build_ms << ',' << boost_eval.dynamic_bitset_shared_bbl_count << '\n';
}

}  // namespace

int main(int argc, char** argv) {
  std::string traffic_csv;
  std::string garages_csv;
  std::string bes_csv;
  std::size_t repeats = 3;
  std::string output_csv = "results/raw/phase2/index_lookup_benchmark.csv";

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--traffic" && i + 1 < argc) {
      traffic_csv = argv[++i];
    } else if (arg == "--garages" && i + 1 < argc) {
      garages_csv = argv[++i];
    } else if (arg == "--bes" && i + 1 < argc) {
      bes_csv = argv[++i];
    } else if (arg == "--repeats" && i + 1 < argc) {
      repeats = static_cast<std::size_t>(std::stoull(argv[++i]));
    } else if (arg == "--output" && i + 1 < argc) {
      output_csv = argv[++i];
    } else if (arg == "--help") {
      PrintUsage();
      return 0;
    } else {
      std::cerr << "unknown or incomplete argument: " << arg << "\n";
      PrintUsage();
      return 2;
    }
  }

  if (traffic_csv.empty()) {
    PrintUsage();
    return 2;
  }
  if (repeats == 0) {
    std::cerr << "--repeats must be > 0\n";
    return 2;
  }

  urbandrop::TrafficDataset traffic;
  urbandrop::TrafficLoadOptions options;
  options.print_progress = false;

  std::string error;
  if (!urbandrop::CSVReader::LoadTrafficCSV(traffic_csv, &traffic, options, nullptr, &error)) {
    std::cerr << "traffic ingest failed: " << error << "\n";
    return 1;
  }

  std::vector<urbandrop::GarageRecord> garages;
  if (!garages_csv.empty()) {
    urbandrop::LoaderStats stats;
    if (!urbandrop::GarageLoader::LoadCSV(garages_csv, &garages, &stats, &error)) {
      std::cerr << "garage ingest failed: " << error << "\n";
      return 1;
    }
  }

  std::vector<urbandrop::BuildingRecord> buildings;
  if (!bes_csv.empty()) {
    urbandrop::LoaderStats stats;
    if (!urbandrop::BuildingLoader::LoadCSV(bes_csv, &buildings, &stats, &error)) {
      std::cerr << "building ingest failed: " << error << "\n";
      return 1;
    }
  }

  std::unordered_set<std::int64_t> link_keys;
  link_keys.reserve(traffic.Size());
  for (const auto& row : traffic) {
    link_keys.insert(row.link_id);
  }

  std::unordered_set<std::string> bbl_keys;
  bbl_keys.reserve(garages.size() + buildings.size());
  for (const auto& g : garages) {
    if (!g.bbl.empty()) {
      bbl_keys.insert(g.bbl);
    }
  }
  for (const auto& b : buildings) {
    if (!b.bbl.empty()) {
      bbl_keys.insert(b.bbl);
    }
  }

  const auto index_started = Clock::now();
  const urbandrop::CrossDatasetBuildOptions build_options;
  const urbandrop::CrossDatasetIndex index =
      urbandrop::CrossDatasetIndex::Build(traffic, garages, buildings, build_options);
  const auto index_finished = Clock::now();
  const double index_build_ms =
      std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(index_finished - index_started)
          .count();

  const auto baseline_link_started = Clock::now();
  std::size_t baseline_link_hits = 0;
  for (std::size_t r = 0; r < repeats; ++r) {
    for (const std::int64_t link_id : link_keys) {
      for (const auto& row : traffic) {
        if (row.link_id == link_id) {
          ++baseline_link_hits;
        }
      }
    }
  }
  const auto baseline_link_finished = Clock::now();
  const double baseline_link_scan_ms =
      std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(baseline_link_finished - baseline_link_started)
          .count();

  const auto indexed_link_started = Clock::now();
  std::size_t indexed_link_hits = 0;
  for (std::size_t r = 0; r < repeats; ++r) {
    for (const std::int64_t link_id : link_keys) {
      indexed_link_hits += index.LookupTrafficCountByLinkId(link_id);
    }
  }
  const auto indexed_link_finished = Clock::now();
  const double indexed_link_lookup_ms =
      std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(indexed_link_finished - indexed_link_started)
          .count();

  double baseline_bbl_scan_ms = 0.0;
  double indexed_bbl_lookup_ms = 0.0;
  std::size_t baseline_bbl_hits = 0;
  std::size_t indexed_bbl_hits = 0;

  if (!bbl_keys.empty() && (!garages.empty() || !buildings.empty())) {
    const auto baseline_bbl_started = Clock::now();
    for (std::size_t r = 0; r < repeats; ++r) {
      for (const auto& bbl : bbl_keys) {
        for (const auto& g : garages) {
          if (g.bbl == bbl) {
            ++baseline_bbl_hits;
          }
        }
        for (const auto& b : buildings) {
          if (b.bbl == bbl) {
            ++baseline_bbl_hits;
          }
        }
      }
    }
    const auto baseline_bbl_finished = Clock::now();
    baseline_bbl_scan_ms =
        std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(baseline_bbl_finished - baseline_bbl_started)
            .count();

    const auto indexed_bbl_started = Clock::now();
    for (std::size_t r = 0; r < repeats; ++r) {
      for (const auto& bbl : bbl_keys) {
        indexed_bbl_hits += index.LookupGarageCountByBbl(bbl);
        indexed_bbl_hits += index.LookupBuildingCountByBbl(bbl);
      }
    }
    const auto indexed_bbl_finished = Clock::now();
    indexed_bbl_lookup_ms =
        std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(indexed_bbl_finished - indexed_bbl_started)
            .count();
  }

  const urbandrop::BoostIndexEvaluation boost_eval =
      urbandrop::CrossDatasetIndex::EvaluateBoostIndexing(garages, buildings, repeats);

  std::cout << "[index] traffic_rows=" << traffic.Counters().rows_accepted << " unique_link_keys="
            << index.LinkIdKeyCount() << "\n";
  std::cout << "[index] garages=" << garages.size() << " buildings=" << buildings.size()
            << " shared_bbl_keys=" << index.SharedBblKeyCount() << "\n";
  std::cout << "[index] timings_ms build=" << index_build_ms
            << " baseline_link_scan=" << baseline_link_scan_ms
            << " indexed_link_lookup=" << indexed_link_lookup_ms
            << " baseline_bbl_scan=" << baseline_bbl_scan_ms
            << " indexed_bbl_lookup=" << indexed_bbl_lookup_ms << "\n";
  std::cout << "[index] sanity_hits baseline_link=" << baseline_link_hits
            << " indexed_link=" << indexed_link_hits << " baseline_bbl=" << baseline_bbl_hits
            << " indexed_bbl=" << indexed_bbl_hits << "\n";
  std::cout << "[index] boost flat_map_available=" << (boost_eval.flat_map_available ? "true" : "false")
            << " unordered_flat_map_available="
            << (boost_eval.unordered_flat_map_available ? "true" : "false")
            << " dynamic_bitset_available=" << (boost_eval.dynamic_bitset_available ? "true" : "false")
            << "\n";

  AppendCsvRow(output_csv,
               traffic_csv,
               garages_csv,
               bes_csv,
               repeats,
               traffic,
               garages,
               buildings,
               index.LinkIdKeyCount(),
               index.SharedBblKeyCount(),
               baseline_link_scan_ms,
               indexed_link_lookup_ms,
               baseline_bbl_scan_ms,
               indexed_bbl_lookup_ms,
               index_build_ms,
               boost_eval);

  return 0;
}
