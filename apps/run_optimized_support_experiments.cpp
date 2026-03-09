#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_set>

#include "data_model/TrafficDatasetOptimized.hpp"
#include "io/CSVReader.hpp"
#include "query/OptimizedQuerySupport.hpp"

namespace {
using Clock = std::chrono::steady_clock;

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

}  // namespace

int main(int argc, char** argv) {
  std::string traffic_csv;
  std::size_t repeats = 5;
  std::string out_csv = "results/raw/phase3_dev/optimized_support_benchmark.csv";

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--traffic" && i + 1 < argc) {
      traffic_csv = argv[++i];
    } else if (arg == "--repeats" && i + 1 < argc) {
      repeats = static_cast<std::size_t>(std::stoull(argv[++i]));
    } else if (arg == "--output" && i + 1 < argc) {
      out_csv = argv[++i];
    } else if (arg == "--help") {
      std::cout << "Usage: run_optimized_support_experiments --traffic <path> [--repeats <n>] [--output <csv>]\n";
      return 0;
    }
  }

  if (traffic_csv.empty()) {
    std::cerr << "--traffic is required\n";
    return 2;
  }

  urbandrop::TrafficDatasetOptimized dataset;
  urbandrop::TrafficLoadOptions options;
  options.print_progress = false;
  std::string error;
  if (!urbandrop::CSVReader::LoadTrafficCSVOptimized(traffic_csv, &dataset, options, nullptr, &error)) {
    std::cerr << "load failed: " << error << "\n";
    return 1;
  }

  std::unordered_set<std::int64_t> link_keys;
  std::unordered_set<std::int16_t> borough_keys;
  std::unordered_set<std::int64_t> hour_keys;

  const auto& links = dataset.Columns().LinkIds();
  const auto& boroughs = dataset.Columns().BoroughCodes();
  const auto& timestamps = dataset.Columns().TimestampsEpochSeconds();
  for (std::size_t i = 0; i < dataset.Size(); ++i) {
    link_keys.insert(links[i]);
    borough_keys.insert(boroughs[i]);
    hour_keys.insert(timestamps[i] / 3600);
  }

  const auto build_start = Clock::now();
  const urbandrop::OptimizedQuerySupport support = urbandrop::OptimizedQuerySupport::Build(dataset);
  const auto build_end = Clock::now();

  std::size_t scan_link_hits = 0;
  const auto scan_link_start = Clock::now();
  for (std::size_t r = 0; r < repeats; ++r) {
    for (const auto key : link_keys) {
      for (const auto id : links) {
        if (id == key) {
          ++scan_link_hits;
        }
      }
    }
  }
  const auto scan_link_end = Clock::now();

  std::size_t indexed_link_hits = 0;
  const auto indexed_link_start = Clock::now();
  for (std::size_t r = 0; r < repeats; ++r) {
    for (const auto key : link_keys) {
      indexed_link_hits += support.CountByLinkId(key);
    }
  }
  const auto indexed_link_end = Clock::now();

  std::size_t scan_borough_hits = 0;
  const auto scan_borough_start = Clock::now();
  for (std::size_t r = 0; r < repeats; ++r) {
    for (const auto key : borough_keys) {
      for (const auto code : boroughs) {
        if (code == key) {
          ++scan_borough_hits;
        }
      }
    }
  }
  const auto scan_borough_end = Clock::now();

  std::size_t indexed_borough_hits = 0;
  const auto indexed_borough_start = Clock::now();
  for (std::size_t r = 0; r < repeats; ++r) {
    for (const auto key : borough_keys) {
      indexed_borough_hits += support.CountByBorough(key);
    }
  }
  const auto indexed_borough_end = Clock::now();

  std::size_t indexed_hour_hits = 0;
  for (const auto bucket : hour_keys) {
    indexed_hour_hits += support.CountByHourBucket(bucket);
  }

  const double build_ms =
      std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(build_end - build_start).count();
  const double scan_link_ms =
      std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(scan_link_end - scan_link_start)
          .count();
  const double indexed_link_ms = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
                                     indexed_link_end - indexed_link_start)
                                     .count();
  const double scan_borough_ms =
      std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(scan_borough_end - scan_borough_start)
          .count();
  const double indexed_borough_ms = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
                                        indexed_borough_end - indexed_borough_start)
                                        .count();

  std::cout << "[optimized_support] rows=" << dataset.Size() << " link_keys=" << link_keys.size()
            << " borough_keys=" << borough_keys.size() << " hour_keys=" << hour_keys.size() << "\n";
  std::cout << "[optimized_support] timings_ms build=" << build_ms << " scan_link=" << scan_link_ms
            << " indexed_link=" << indexed_link_ms << " scan_borough=" << scan_borough_ms
            << " indexed_borough=" << indexed_borough_ms << "\n";

  const bool write_header = !std::ifstream(out_csv).good();
  std::ofstream out(out_csv, std::ios::app);
  if (!out.is_open()) {
    std::cerr << "failed to write output csv\n";
    return 1;
  }

  if (write_header) {
    out << "traffic_path,repeats,rows,link_keys,borough_keys,hour_keys,build_ms,scan_link_ms,indexed_link_ms,scan_borough_ms,indexed_borough_ms,scan_link_hits,indexed_link_hits,scan_borough_hits,indexed_borough_hits,indexed_hour_hits\n";
  }

  out << EscapeCsv(traffic_csv) << ',' << repeats << ',' << dataset.Size() << ',' << link_keys.size() << ','
      << borough_keys.size() << ',' << hour_keys.size() << ',' << build_ms << ',' << scan_link_ms << ','
      << indexed_link_ms << ',' << scan_borough_ms << ',' << indexed_borough_ms << ',' << scan_link_hits
      << ',' << indexed_link_hits << ',' << scan_borough_hits << ',' << indexed_borough_hits << ','
      << indexed_hour_hits << '\n';

  return 0;
}
