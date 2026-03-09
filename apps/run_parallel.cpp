#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#if defined(_OPENMP)
#include <omp.h>
#endif

#include "data_model/TrafficDataset.hpp"
#include "io/CSVReader.hpp"
#include "query/CongestionQuery.hpp"
#include "query/ParallelCongestionQuery.hpp"
#include "query/ParallelTrafficAggregator.hpp"
#include "query/TimeWindowQuery.hpp"
#include "query/TrafficAggregator.hpp"

namespace {

using Clock = std::chrono::steady_clock;

void PrintUsage() {
  std::cout
      << "Usage: run_parallel --traffic <path> [--query <type>] [--threads <n>] [--thread-list <csv>] [--benchmark-runs <n>]\n"
      << "\n"
      << "Supported query types:\n"
      << "  speed_below         --threshold <mph>\n"
      << "  borough_speed_below --borough <name> --threshold <mph>\n"
      << "  time_window         --start-epoch <sec> --end-epoch <sec>\n"
      << "  top_n_slowest       --top-n <n> [--min-link-samples <n>]\n"
      << "  summary             (parallel aggregate over full dataset)\n"
      << "\n"
      << "Validation flag:\n"
      << "  --validate-serial   compare parallel result(s) with serial implementation\n";
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

bool ParseThreadList(const std::string& value, std::vector<std::size_t>* out) {
  if (out == nullptr) {
    return false;
  }

  std::stringstream ss(value);
  std::string token;
  std::vector<std::size_t> parsed;
  while (std::getline(ss, token, ',')) {
    if (token.empty()) {
      continue;
    }
    std::int64_t thread_count = 0;
    if (!ParseInt64(token, &thread_count) || thread_count < 0) {
      return false;
    }
    parsed.push_back(static_cast<std::size_t>(thread_count));
  }

  if (parsed.empty()) {
    return false;
  }

  *out = parsed;
  return true;
}

bool NearlyEqual(double lhs, double rhs) {
  return std::fabs(lhs - rhs) < 1e-9;
}

}  // namespace

int main(int argc, char** argv) {
  std::string traffic_csv;
  std::string query_type = "speed_below";
  std::string borough;
  std::size_t threads = 0;
  std::vector<std::size_t> thread_list;
  std::size_t benchmark_runs = 1;
  bool validate_serial = false;

  double threshold_mph = 15.0;
  bool has_threshold = false;

  std::int64_t start_epoch = 0;
  std::int64_t end_epoch = 0;
  bool has_start_epoch = false;
  bool has_end_epoch = false;

  std::size_t top_n = 0;
  std::size_t min_link_samples = 1;

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
    } else if (arg == "--thread-list" && i + 1 < argc) {
      if (!ParseThreadList(argv[++i], &thread_list)) {
        std::cerr << "invalid --thread-list value\n";
        return 2;
      }
    } else if (arg == "--benchmark-runs" && i + 1 < argc) {
      benchmark_runs = static_cast<std::size_t>(std::stoull(argv[++i]));
    } else if (arg == "--start-epoch" && i + 1 < argc) {
      has_start_epoch = ParseInt64(argv[++i], &start_epoch);
    } else if (arg == "--end-epoch" && i + 1 < argc) {
      has_end_epoch = ParseInt64(argv[++i], &end_epoch);
    } else if (arg == "--top-n" && i + 1 < argc) {
      top_n = static_cast<std::size_t>(std::stoull(argv[++i]));
    } else if (arg == "--min-link-samples" && i + 1 < argc) {
      min_link_samples = static_cast<std::size_t>(std::stoull(argv[++i]));
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

  if ((query_type == "speed_below" || query_type == "borough_speed_below") && !has_threshold) {
    std::cerr << query_type << " requires --threshold\n";
    return 2;
  }
  if (query_type == "borough_speed_below" && borough.empty()) {
    std::cerr << "borough_speed_below requires --borough\n";
    return 2;
  }
  if (query_type == "time_window" && (!has_start_epoch || !has_end_epoch)) {
    std::cerr << "time_window requires --start-epoch and --end-epoch\n";
    return 2;
  }
  if (query_type == "top_n_slowest" && top_n == 0) {
    std::cerr << "top_n_slowest requires --top-n > 0\n";
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

  if (thread_list.empty()) {
    thread_list.push_back(threads);
  }

  for (std::size_t configured_threads : thread_list) {
    for (std::size_t run = 1; run <= benchmark_runs; ++run) {
      std::cout << "[query] start run=" << run << " type=" << query_type
                << " threads=" << configured_threads << "\n";

      const auto query_start = Clock::now();
      std::size_t result_count = 0;
      std::vector<urbandrop::LinkSpeedStat> topn_result;
      urbandrop::AggregateStats aggregate_result;

      if (query_type == "speed_below") {
        result_count =
            urbandrop::ParallelCongestionQuery::CountSpeedBelow(dataset, threshold_mph, configured_threads);
        aggregate_result =
            urbandrop::ParallelTrafficAggregator::SummarizeSpeedBelow(dataset, threshold_mph, configured_threads);
      } else if (query_type == "borough_speed_below") {
        result_count = urbandrop::ParallelCongestionQuery::CountBoroughAndSpeedBelow(
            dataset, borough, threshold_mph, configured_threads);
        aggregate_result = urbandrop::ParallelTrafficAggregator::SummarizeBoroughAndSpeedBelow(
            dataset, borough, threshold_mph, configured_threads);
      } else if (query_type == "time_window") {
        result_count = urbandrop::ParallelCongestionQuery::CountTimeWindow(
            dataset, start_epoch, end_epoch, configured_threads);
        aggregate_result = urbandrop::ParallelTrafficAggregator::SummarizeTimeWindow(
            dataset, start_epoch, end_epoch, configured_threads);
      } else if (query_type == "top_n_slowest") {
        topn_result = urbandrop::ParallelCongestionQuery::TopNSlowestRecurringLinks(
            dataset, top_n, min_link_samples, configured_threads);
        result_count = topn_result.size();
      } else if (query_type == "summary") {
        aggregate_result = urbandrop::ParallelTrafficAggregator::SummarizeDataset(dataset, configured_threads);
        result_count = aggregate_result.record_count;
      } else {
        std::cerr << "unsupported query type for run_parallel: " << query_type << "\n";
        return 2;
      }

      const auto query_end = Clock::now();
      const auto elapsed_ms =
          std::chrono::duration_cast<std::chrono::milliseconds>(query_end - query_start).count();

      std::cout << "[query] end run=" << run << " type=" << query_type << " threads="
                << configured_threads << " matched_records=" << result_count
                << " elapsed_ms=" << elapsed_ms << "\n";

      if (validate_serial) {
        std::size_t serial_count = 0;
        std::vector<urbandrop::LinkSpeedStat> serial_topn;
        urbandrop::AggregateStats serial_aggregate;

        if (query_type == "speed_below") {
          const auto serial_rows = urbandrop::CongestionQuery::FilterSpeedBelow(dataset, threshold_mph);
          serial_count = serial_rows.size();
          serial_aggregate = urbandrop::TrafficAggregator::Summarize(serial_rows);
        } else if (query_type == "borough_speed_below") {
          const auto serial_rows =
              urbandrop::CongestionQuery::FilterBoroughAndSpeedBelow(dataset, borough, threshold_mph);
          serial_count = serial_rows.size();
          serial_aggregate = urbandrop::TrafficAggregator::Summarize(serial_rows);
        } else if (query_type == "time_window") {
          const auto serial_rows =
              urbandrop::TimeWindowQuery::FilterByEpochRange(dataset, start_epoch, end_epoch);
          serial_count = serial_rows.size();
          serial_aggregate = urbandrop::TrafficAggregator::Summarize(serial_rows);
        } else if (query_type == "top_n_slowest") {
          serial_topn =
              urbandrop::CongestionQuery::TopNSlowestRecurringLinks(dataset, top_n, min_link_samples);
          serial_count = serial_topn.size();
        } else {
          serial_aggregate = urbandrop::TrafficAggregator::SummarizeDataset(dataset);
          serial_count = serial_aggregate.record_count;
        }

        bool match = serial_count == result_count;
        if (match && query_type != "top_n_slowest") {
          match = NearlyEqual(serial_aggregate.average_speed_mph, aggregate_result.average_speed_mph) &&
                  NearlyEqual(serial_aggregate.average_travel_time_seconds,
                              aggregate_result.average_travel_time_seconds);
        }
        if (match && query_type == "top_n_slowest") {
          if (serial_topn.size() != topn_result.size()) {
            match = false;
          } else {
            for (std::size_t i = 0; i < serial_topn.size(); ++i) {
              if (serial_topn[i].link_id != topn_result[i].link_id ||
                  serial_topn[i].record_count != topn_result[i].record_count ||
                  !NearlyEqual(serial_topn[i].average_speed_mph, topn_result[i].average_speed_mph)) {
                match = false;
                break;
              }
            }
          }
        }

        if (!match) {
          std::cerr << "[validate] mismatch run=" << run << " serial=" << serial_count
                    << " parallel=" << result_count << "\n";
          return 1;
        }

        std::cout << "[validate] run=" << run << " serial_match=true\n";
      }
    }
  }

  return 0;
}
