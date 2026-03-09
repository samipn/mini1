#include <cstdint>
#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <numeric>
#include <string>
#include <vector>

#if defined(_OPENMP)
#include <omp.h>
#endif

#include "benchmark/BenchmarkHarness.hpp"
#include "data_model/CommonCodes.hpp"
#include "data_model/TrafficDataset.hpp"
#include "data_model/TrafficDatasetOptimized.hpp"
#include "io/CSVReader.hpp"
#include "query/OptimizedCongestionQuery.hpp"
#include "query/OptimizedTrafficAggregator.hpp"

namespace {

void PrintUsage() {
  std::cout
      << "Usage: run_optimized --traffic <path> [--query <type>] [--execution <serial|parallel>] [--threads <n>]\n"
      << "\n"
      << "Supported query types:\n"
      << "  speed_below         --threshold <mph>\n"
      << "  borough_speed_below --borough <name> --threshold <mph>\n"
      << "  time_window         --start-epoch <sec> --end-epoch <sec>\n"
      << "  top_n_slowest       --top-n <n> [--min-link-samples <n>]\n"
      << "  summary             (full dataset aggregate)\n"
      << "\n"
      << "Options:\n"
      << "  --load-mode <direct|row_convert>      (default: direct)\n"
      << "  --benchmark-runs <n>                  benchmark repeats\n"
      << "  --benchmark-out <path>                output CSV\n"
      << "  --benchmark-append                    append mode\n"
      << "  --dataset-label <label>               output metadata\n"
      << "  --expect-accepted <n>                 validation count\n"
      << "  --validate-serial                     compare optimized against serial baseline\n";
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

bool ParseSizeT(const std::string& value, std::size_t* out) {
  if (out == nullptr || value.empty()) {
    return false;
  }
  errno = 0;
  char* end = nullptr;
  const unsigned long long parsed = std::strtoull(value.c_str(), &end, 10);
  if (errno != 0 || end == value.c_str() || *end != '\0') {
    return false;
  }
  if (parsed > static_cast<unsigned long long>(std::numeric_limits<std::size_t>::max())) {
    return false;
  }
  *out = static_cast<std::size_t>(parsed);
  return true;
}

}  // namespace

int main(int argc, char** argv) {
  std::string traffic_csv;
  std::string query_type = "speed_below";
  std::string borough;
  std::string execution = "serial";
  std::string load_mode = "direct";

  std::size_t threads = 0;
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
    } else if (arg == "--execution" && i + 1 < argc) {
      execution = argv[++i];
    } else if (arg == "--load-mode" && i + 1 < argc) {
      load_mode = argv[++i];
    } else if (arg == "--threads" && i + 1 < argc) {
      std::int64_t parsed = 0;
      if (!ParseInt64(argv[++i], &parsed) || parsed < 0) {
        std::cerr << "invalid --threads value\n";
        return 2;
      }
      threads = static_cast<std::size_t>(parsed);
    } else if (arg == "--benchmark-runs" && i + 1 < argc) {
      if (!ParseSizeT(argv[++i], &benchmark_runs)) {
        std::cerr << "invalid --benchmark-runs value\n";
        return 2;
      }
    } else if (arg == "--benchmark-out" && i + 1 < argc) {
      benchmark_out_csv = argv[++i];
    } else if (arg == "--benchmark-append") {
      benchmark_append = true;
    } else if (arg == "--dataset-label" && i + 1 < argc) {
      dataset_label = argv[++i];
    } else if (arg == "--expect-accepted" && i + 1 < argc) {
      if (!ParseSizeT(argv[++i], &expected_accepted)) {
        std::cerr << "invalid --expect-accepted value\n";
        return 2;
      }
      has_expected_accepted = true;
    } else if (arg == "--start-epoch" && i + 1 < argc) {
      has_start_epoch = ParseInt64(argv[++i], &start_epoch);
    } else if (arg == "--end-epoch" && i + 1 < argc) {
      has_end_epoch = ParseInt64(argv[++i], &end_epoch);
    } else if (arg == "--top-n" && i + 1 < argc) {
      if (!ParseSizeT(argv[++i], &top_n)) {
        std::cerr << "invalid --top-n value\n";
        return 2;
      }
    } else if (arg == "--min-link-samples" && i + 1 < argc) {
      if (!ParseSizeT(argv[++i], &min_link_samples)) {
        std::cerr << "invalid --min-link-samples value\n";
        return 2;
      }
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
  if (execution != "serial" && execution != "parallel") {
    std::cerr << "--execution must be serial or parallel\n";
    return 2;
  }
  if (load_mode != "direct" && load_mode != "row_convert") {
    std::cerr << "--load-mode must be direct or row_convert\n";
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

  std::size_t effective_threads = threads;
  if (execution == "parallel" && effective_threads == 0) {
#if defined(_OPENMP)
    effective_threads = static_cast<std::size_t>(omp_get_max_threads());
#else
    effective_threads = 1;
#endif
  }
  if (execution == "serial") {
    effective_threads = 1;
  }

  std::cout << "[optimized] layout=soa execution=" << execution << " load_mode=" << load_mode
            << " threads=" << effective_threads << "\n";

  urbandrop::BenchmarkConfig config;
  config.binary_name = "run_optimized";
  config.execution_mode = urbandrop::BenchmarkExecutionMode::kOptimized;
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
  config.thread_count = effective_threads;
  config.validate_parallel_against_serial = validate_serial;
  config.output_csv_path = benchmark_out_csv;
  config.append_output = benchmark_append;
  config.has_expected_accepted_rows = has_expected_accepted;
  config.expected_accepted_rows = expected_accepted;

  if (load_mode == "direct") {
    std::vector<urbandrop::BenchmarkRunResult> results;
    std::string benchmark_error;
    if (!urbandrop::BenchmarkHarness::RunOptimized(config, &results, &benchmark_error)) {
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
              << " execution=" << execution << "\n";
    std::cout << "  avg_ingest_ms=" << (total_ingest / runs) << "\n";
    std::cout << "  avg_query_ms=" << (total_query / runs) << "\n";
    std::cout << "  avg_total_ms=" << (total_total / runs) << "\n";
    if (!benchmark_out_csv.empty()) {
      std::cout << "  output_csv=" << benchmark_out_csv << "\n";
    }
    return 0;
  }

  // row_convert path: baseline row ingest then convert to optimized columns.
  urbandrop::TrafficDataset row_dataset;
  urbandrop::TrafficLoadOptions options;
  options.print_progress = false;
  std::string error;
  if (!urbandrop::CSVReader::LoadTrafficCSV(traffic_csv, &row_dataset, options, nullptr, &error)) {
    std::cerr << "[ingest] failed: " << error << "\n";
    return 1;
  }
  urbandrop::TrafficDatasetOptimized dataset =
      urbandrop::TrafficDatasetOptimized::FromRowDataset(row_dataset);

  std::size_t result_count = 0;
  urbandrop::AggregateStats stats;

  if (query_type == "speed_below") {
    result_count = execution == "parallel"
                       ? urbandrop::OptimizedCongestionQuery::CountSpeedBelowParallel(
                             dataset, threshold_mph, effective_threads)
                       : urbandrop::OptimizedCongestionQuery::CountSpeedBelow(dataset, threshold_mph);
    stats = execution == "parallel"
                ? urbandrop::OptimizedTrafficAggregator::SummarizeSpeedBelowParallel(
                      dataset, threshold_mph, effective_threads)
                : urbandrop::OptimizedTrafficAggregator::SummarizeSpeedBelow(dataset, threshold_mph);
  } else if (query_type == "borough_speed_below") {
    const std::int16_t borough_code = urbandrop::ToInt(urbandrop::ParseBoroughCode(borough));
    result_count = execution == "parallel"
                       ? urbandrop::OptimizedCongestionQuery::CountBoroughAndSpeedBelowParallel(
                             dataset, borough_code, threshold_mph, effective_threads)
                       : urbandrop::OptimizedCongestionQuery::CountBoroughAndSpeedBelow(
                             dataset, borough_code, threshold_mph);
    stats = execution == "parallel"
                ? urbandrop::OptimizedTrafficAggregator::SummarizeBoroughAndSpeedBelowParallel(
                      dataset, borough_code, threshold_mph, effective_threads)
                : urbandrop::OptimizedTrafficAggregator::SummarizeBoroughAndSpeedBelow(
                      dataset, borough_code, threshold_mph);
  } else if (query_type == "time_window") {
    result_count = execution == "parallel"
                       ? urbandrop::OptimizedCongestionQuery::CountTimeWindowParallel(
                             dataset, start_epoch, end_epoch, effective_threads)
                       : urbandrop::OptimizedCongestionQuery::CountTimeWindow(dataset, start_epoch, end_epoch);
    stats = execution == "parallel"
                ? urbandrop::OptimizedTrafficAggregator::SummarizeTimeWindowParallel(
                      dataset, start_epoch, end_epoch, effective_threads)
                : urbandrop::OptimizedTrafficAggregator::SummarizeTimeWindow(dataset, start_epoch, end_epoch);
  } else if (query_type == "summary") {
    stats = execution == "parallel"
                ? urbandrop::OptimizedTrafficAggregator::SummarizeDatasetParallel(dataset, effective_threads)
                : urbandrop::OptimizedTrafficAggregator::SummarizeDataset(dataset);
    result_count = stats.record_count;
  } else if (query_type == "top_n_slowest") {
    const auto topn = execution == "parallel"
                          ? urbandrop::OptimizedCongestionQuery::TopNSlowestRecurringLinksParallel(
                                dataset, top_n, min_link_samples, effective_threads)
                          : urbandrop::OptimizedCongestionQuery::TopNSlowestRecurringLinks(
                                dataset, top_n, min_link_samples);
    result_count = topn.size();
  }

  std::cout << "[query] completed result_count=" << result_count;
  if (stats.record_count > 0) {
    std::cout << " avg_speed_mph=" << stats.average_speed_mph
              << " avg_travel_time_seconds=" << stats.average_travel_time_seconds;
  }
  std::cout << "\n";

  return 0;
}
