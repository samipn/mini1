#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>

#if defined(_OPENMP)
#include <omp.h>
#endif

#include "data_model/TrafficDataset.hpp"
#include "io/CSVReader.hpp"
#include "query/CongestionQuery.hpp"
#include "query/ParallelCongestionQuery.hpp"
#include "query/ParallelTrafficAggregator.hpp"
#include "query/TrafficAggregator.hpp"

namespace {

using Clock = std::chrono::steady_clock;

void PrintUsage() {
  std::cout
      << "Usage: run_parallel --traffic <path> [--query <type>] [--threads <n>] [--benchmark-runs <n>]\n"
      << "\n"
      << "Supported query types:\n"
      << "  speed_below         --threshold <mph>\n"
      << "  borough_speed_below --borough <name> --threshold <mph>\n"
      << "  summary             (parallel aggregate over full dataset)\n"
      << "\n"
      << "Validation flag:\n"
      << "  --validate-serial   compare parallel result count/aggregate with serial implementation\n";
}

bool ParseInt64(const std::string& value, std::int64_t* out) {
  try {
    *out = std::stoll(value);
    return true;
  } catch (...) {
    return false;
  }
}

bool ParseDouble(const std::string& value, double* out) {
  try {
    *out = std::stod(value);
    return true;
  } catch (...) {
    return false;
  }
}

}  // namespace

int main(int argc, char** argv) {
  std::string traffic_csv;
  std::string query_type = "speed_below";
  std::string borough;
  std::size_t threads = 0;
  std::size_t benchmark_runs = 1;
  bool validate_serial = false;

  double threshold_mph = 15.0;
  bool has_threshold = false;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--traffic" && i + 1 < argc) {
      traffic_csv = argv[++i];
    } else if (arg == "--query" && i + 1 < argc) {
      query_type = argv[++i];
    } else if (arg == "--threshold" && i + 1 < argc) {
      has_threshold = ParseDouble(argv[++i], &threshold_mph);
    } else if (arg == "--borough" && i + 1 < argc) {
      borough = argv[++i];
    } else if (arg == "--threads" && i + 1 < argc) {
      std::int64_t parsed = 0;
      if (!ParseInt64(argv[++i], &parsed) || parsed < 0) {
        std::cerr << "invalid --threads value\n";
        return 2;
      }
      threads = static_cast<std::size_t>(parsed);
    } else if (arg == "--benchmark-runs" && i + 1 < argc) {
      benchmark_runs = static_cast<std::size_t>(std::stoull(argv[++i]));
    } else if (arg == "--validate-serial") {
      validate_serial = true;
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
  if (benchmark_runs == 0) {
    std::cerr << "--benchmark-runs must be > 0\n";
    return 2;
  }

  urbandrop::TrafficDataset dataset;
  urbandrop::TrafficLoadOptions options;
  options.print_progress = false;

  std::string error;
  std::cout << "[parallel] model=openmp";
#if defined(_OPENMP)
  std::cout << " max_threads=" << omp_get_max_threads();
#endif
  std::cout << "\n";

  std::cout << "[ingest] start path=" << traffic_csv << "\n";
  if (!urbandrop::CSVReader::LoadTrafficCSV(traffic_csv, &dataset, options, nullptr, &error)) {
    std::cerr << "[ingest] failed: " << error << "\n";
    return 1;
  }
  std::cout << "[ingest] complete rows_accepted=" << dataset.Counters().rows_accepted << "\n";

  if ((query_type == "speed_below" || query_type == "borough_speed_below") && !has_threshold) {
    std::cerr << query_type << " requires --threshold\n";
    return 2;
  }
  if (query_type == "borough_speed_below" && borough.empty()) {
    std::cerr << "borough_speed_below requires --borough\n";
    return 2;
  }

  for (std::size_t run = 1; run <= benchmark_runs; ++run) {
    const auto query_start = Clock::now();
    std::size_t result_count = 0;

    if (query_type == "speed_below") {
      result_count = urbandrop::ParallelCongestionQuery::CountSpeedBelow(dataset, threshold_mph, threads);
    } else if (query_type == "borough_speed_below") {
      result_count = urbandrop::ParallelCongestionQuery::CountBoroughAndSpeedBelow(
          dataset, borough, threshold_mph, threads);
    } else if (query_type == "summary") {
      result_count = urbandrop::ParallelTrafficAggregator::SummarizeDataset(dataset, threads).record_count;
    } else {
      std::cerr << "unsupported query type for run_parallel: " << query_type << "\n";
      return 2;
    }

    const auto query_end = Clock::now();
    const auto elapsed_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(query_end - query_start).count();

    std::cout << "[query] run=" << run << " type=" << query_type << " threads=" << threads
              << " matched_records=" << result_count << " elapsed_ms=" << elapsed_ms << "\n";

    if (validate_serial) {
      std::size_t serial_count = 0;
      if (query_type == "speed_below") {
        serial_count = urbandrop::CongestionQuery::FilterSpeedBelow(dataset, threshold_mph).size();
      } else if (query_type == "borough_speed_below") {
        serial_count =
            urbandrop::CongestionQuery::FilterBoroughAndSpeedBelow(dataset, borough, threshold_mph).size();
      } else {
        serial_count = urbandrop::TrafficAggregator::SummarizeDataset(dataset).record_count;
      }

      if (serial_count != result_count) {
        std::cerr << "[validate] mismatch run=" << run << " serial=" << serial_count
                  << " parallel=" << result_count << "\n";
        return 1;
      }
      std::cout << "[validate] run=" << run << " serial_match=true\n";
    }
  }

  return 0;
}
