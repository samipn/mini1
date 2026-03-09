#include <cstdint>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

#if defined(_OPENMP)
#include <omp.h>
#endif

#include "benchmark/BenchmarkHarness.hpp"

namespace {

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
      << "Benchmark mode options:\n"
      << "  --benchmark-runs <n>      run ingest+query benchmark n times\n"
      << "  --benchmark-out <path>    output CSV path\n"
      << "  --benchmark-append        append to existing CSV instead of overwrite\n"
      << "  --dataset-label <label>   metadata label for output rows\n"
      << "  --expect-accepted <n>     fail if accepted row count differs from n\n"
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

}  // namespace

int main(int argc, char** argv) {
  std::string traffic_csv;
  std::string query_type = "speed_below";
  std::string borough;
  std::size_t threads = 0;
  std::vector<std::size_t> thread_list;
  std::size_t benchmark_runs = 1;
  bool validate_serial = false;

  std::string benchmark_out_csv;
  bool benchmark_append = false;
  std::string dataset_label;
  std::size_t expected_accepted = 0;
  bool has_expected_accepted = false;

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
    } else if (arg == "--benchmark-out" && i + 1 < argc) {
      benchmark_out_csv = argv[++i];
    } else if (arg == "--benchmark-append") {
      benchmark_append = true;
    } else if (arg == "--dataset-label" && i + 1 < argc) {
      dataset_label = argv[++i];
    } else if (arg == "--expect-accepted" && i + 1 < argc) {
      expected_accepted = static_cast<std::size_t>(std::stoull(argv[++i]));
      has_expected_accepted = true;
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

  std::cout << "[parallel] model=openmp";
#if defined(_OPENMP)
  std::cout << " max_threads=" << omp_get_max_threads();
#endif
  std::cout << "\n";

  if (thread_list.empty()) {
    thread_list.push_back(threads);
  }

  for (std::size_t configured_threads : thread_list) {
    urbandrop::BenchmarkConfig config;
    config.binary_name = "run_parallel";
    config.execution_mode = urbandrop::BenchmarkExecutionMode::kParallel;
    config.dataset_path = traffic_csv;
    config.dataset_label = dataset_label;
    config.query_type = query_type;
    config.borough = borough;
    config.threshold_mph = threshold_mph;
    config.has_threshold = has_threshold;
    config.start_epoch = start_epoch;
    config.end_epoch = end_epoch;
    config.has_start_epoch = has_start_epoch;
    config.has_end_epoch = has_end_epoch;
    config.top_n = top_n;
    config.min_link_samples = min_link_samples;
    config.runs = benchmark_runs;
    config.thread_count = configured_threads;
    config.validate_parallel_against_serial = validate_serial;
    config.output_csv_path = benchmark_out_csv;
    config.append_output = benchmark_append || configured_threads != thread_list.front();
    config.enable_ingest_progress = false;
    config.has_expected_accepted_rows = has_expected_accepted;
    config.expected_accepted_rows = expected_accepted;

    std::vector<urbandrop::BenchmarkRunResult> results;
    std::string benchmark_error;
    if (!urbandrop::BenchmarkHarness::RunParallel(config, &results, &benchmark_error)) {
      std::cerr << "[benchmark] failed: " << benchmark_error << "\n";
      return 1;
    }

    const double total_ingest = std::accumulate(
        results.begin(), results.end(), 0.0, [](double sum, const urbandrop::BenchmarkRunResult& row) {
          return sum + row.ingest_ms;
        });
    const double total_query = std::accumulate(
        results.begin(), results.end(), 0.0, [](double sum, const urbandrop::BenchmarkRunResult& row) {
          return sum + row.query_ms;
        });
    const double total_total = std::accumulate(
        results.begin(), results.end(), 0.0, [](double sum, const urbandrop::BenchmarkRunResult& row) {
          return sum + row.total_ms;
        });
    const double runs = static_cast<double>(results.size());

    std::cout << "[benchmark] complete runs=" << results.size()
              << " query_type=" << (query_type.empty() ? "ingest_only" : query_type)
              << " threads=" << configured_threads << "\n";
    std::cout << "  avg_ingest_ms=" << (total_ingest / runs) << "\n";
    std::cout << "  avg_query_ms=" << (total_query / runs) << "\n";
    std::cout << "  avg_total_ms=" << (total_total / runs) << "\n";
    if (!benchmark_out_csv.empty()) {
      std::cout << "  output_csv=" << benchmark_out_csv << "\n";
    }
  }

  return 0;
}
