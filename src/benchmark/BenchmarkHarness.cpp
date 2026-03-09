#include "benchmark/BenchmarkHarness.hpp"

#include <chrono>
#include <cmath>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#if defined(_OPENMP)
#include <omp.h>
#endif

#include "data_model/TrafficDataset.hpp"
#include "io/CSVReader.hpp"
#include "query/CongestionQuery.hpp"
#include "query/OptimizedCongestionQuery.hpp"
#include "query/OptimizedTrafficAggregator.hpp"
#include "query/ParallelCongestionQuery.hpp"
#include "query/ParallelTrafficAggregator.hpp"
#include "query/TimeWindowQuery.hpp"
#include "query/TrafficAggregator.hpp"
#include "data_model/CommonCodes.hpp"

namespace urbandrop {
namespace {

using Clock = std::chrono::steady_clock;

struct QueryExecutionResult {
  std::size_t result_count = 0;
  bool has_aggregate = false;
  AggregateStats aggregate;
  bool has_topn = false;
  std::vector<LinkSpeedStat> topn;
};

static inline double cpu_now_ms() {
  return static_cast<double>(std::clock()) * 1000.0 / CLOCKS_PER_SEC;
}

static long read_peak_rss_kb() {
  std::ifstream f("/proc/self/status");
  std::string line;
  while (std::getline(f, line)) {
    if (line.rfind("VmHWM:", 0) == 0) {
      long kb = 0;
      std::sscanf(line.c_str(), "VmHWM: %ld kB", &kb);
      return kb;
    }
  }
  return -1;
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

std::string NowUtcIso8601() {
  const auto now = std::chrono::system_clock::now();
  const std::time_t now_c = std::chrono::system_clock::to_time_t(now);
  std::tm tm = {};
#if defined(_WIN32)
  gmtime_s(&tm, &now_c);
#else
  gmtime_r(&now_c, &tm);
#endif
  std::ostringstream oss;
  oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
  return oss.str();
}

bool NearlyEqual(double lhs, double rhs) {
  return std::fabs(lhs - rhs) < 1e-9;
}

std::string ExecutionModeString(BenchmarkExecutionMode mode) {
  if (mode == BenchmarkExecutionMode::kParallel) {
    return "parallel";
  }
  if (mode == BenchmarkExecutionMode::kOptimized) {
    return "optimized";
  }
  return "serial";
}

bool ExecuteQuerySerial(const BenchmarkConfig& config,
                        const TrafficDataset& dataset,
                        QueryExecutionResult* out,
                        std::string* error) {
  if (out == nullptr) {
    if (error != nullptr) {
      *error = "out cannot be null";
    }
    return false;
  }

  QueryExecutionResult result;

  if (config.query_type.empty() || config.query_type == "ingest_only") {
    result.result_count = dataset.Size();
    *out = result;
    return true;
  }

  if (config.query_type == "speed_below") {
    if (!config.has_threshold) {
      if (error != nullptr) {
        *error = "speed_below requires threshold";
      }
      return false;
    }
    const auto rows = CongestionQuery::FilterSpeedBelow(dataset, config.threshold_mph);
    result.result_count = rows.size();
    result.has_aggregate = true;
    result.aggregate = TrafficAggregator::Summarize(rows);
    *out = result;
    return true;
  }

  if (config.query_type == "time_window") {
    if (!config.has_start_epoch || !config.has_end_epoch) {
      if (error != nullptr) {
        *error = "time_window requires start and end epoch";
      }
      return false;
    }
    const auto rows = TimeWindowQuery::FilterByEpochRange(dataset, config.start_epoch, config.end_epoch);
    result.result_count = rows.size();
    result.has_aggregate = true;
    result.aggregate = TrafficAggregator::Summarize(rows);
    *out = result;
    return true;
  }

  if (config.query_type == "borough_speed_below") {
    if (config.borough.empty() || !config.has_threshold) {
      if (error != nullptr) {
        *error = "borough_speed_below requires borough and threshold";
      }
      return false;
    }
    const auto rows = CongestionQuery::FilterBoroughAndSpeedBelow(dataset, config.borough, config.threshold_mph);
    result.result_count = rows.size();
    result.has_aggregate = true;
    result.aggregate = TrafficAggregator::Summarize(rows);
    *out = result;
    return true;
  }

  if (config.query_type == "link_id") {
    if (!config.has_link_id) {
      if (error != nullptr) {
        *error = "link_id requires link_id";
      }
      return false;
    }
    const auto rows = CongestionQuery::FindByLinkId(dataset, config.link_id);
    result.result_count = rows.size();
    result.has_aggregate = true;
    result.aggregate = TrafficAggregator::Summarize(rows);
    *out = result;
    return true;
  }

  if (config.query_type == "link_range") {
    if (!config.has_link_start || !config.has_link_end) {
      if (error != nullptr) {
        *error = "link_range requires link_start and link_end";
      }
      return false;
    }
    const auto rows = CongestionQuery::FindByLinkRange(dataset, config.link_start, config.link_end);
    result.result_count = rows.size();
    result.has_aggregate = true;
    result.aggregate = TrafficAggregator::Summarize(rows);
    *out = result;
    return true;
  }

  if (config.query_type == "top_n_slowest") {
    if (config.top_n == 0) {
      if (error != nullptr) {
        *error = "top_n_slowest requires top_n > 0";
      }
      return false;
    }
    result.has_topn = true;
    result.topn =
        CongestionQuery::TopNSlowestRecurringLinks(dataset, config.top_n, config.min_link_samples);
    result.result_count = result.topn.size();
    *out = result;
    return true;
  }

  if (config.query_type == "summary") {
    result.has_aggregate = true;
    result.aggregate = TrafficAggregator::SummarizeDataset(dataset);
    result.result_count = result.aggregate.record_count;
    *out = result;
    return true;
  }

  if (error != nullptr) {
    *error = "unknown query_type: " + config.query_type;
  }
  return false;
}

bool ExecuteQueryParallel(const BenchmarkConfig& config,
                          const TrafficDataset& dataset,
                          std::size_t thread_count,
                          QueryExecutionResult* out,
                          std::string* error) {
  if (out == nullptr) {
    if (error != nullptr) {
      *error = "out cannot be null";
    }
    return false;
  }

  QueryExecutionResult result;

  if (config.query_type.empty() || config.query_type == "ingest_only") {
    result.result_count = dataset.Size();
    *out = result;
    return true;
  }

  if (config.query_type == "speed_below") {
    if (!config.has_threshold) {
      if (error != nullptr) {
        *error = "speed_below requires threshold";
      }
      return false;
    }
    result.result_count = ParallelCongestionQuery::CountSpeedBelow(dataset, config.threshold_mph, thread_count);
    result.has_aggregate = true;
    result.aggregate =
        ParallelTrafficAggregator::SummarizeSpeedBelow(dataset, config.threshold_mph, thread_count);
    *out = result;
    return true;
  }

  if (config.query_type == "time_window") {
    if (!config.has_start_epoch || !config.has_end_epoch) {
      if (error != nullptr) {
        *error = "time_window requires start and end epoch";
      }
      return false;
    }
    result.result_count =
        ParallelCongestionQuery::CountTimeWindow(dataset, config.start_epoch, config.end_epoch, thread_count);
    result.has_aggregate = true;
    result.aggregate = ParallelTrafficAggregator::SummarizeTimeWindow(
        dataset, config.start_epoch, config.end_epoch, thread_count);
    *out = result;
    return true;
  }

  if (config.query_type == "borough_speed_below") {
    if (config.borough.empty() || !config.has_threshold) {
      if (error != nullptr) {
        *error = "borough_speed_below requires borough and threshold";
      }
      return false;
    }
    result.result_count = ParallelCongestionQuery::CountBoroughAndSpeedBelow(
        dataset, config.borough, config.threshold_mph, thread_count);
    result.has_aggregate = true;
    result.aggregate = ParallelTrafficAggregator::SummarizeBoroughAndSpeedBelow(
        dataset, config.borough, config.threshold_mph, thread_count);
    *out = result;
    return true;
  }

  if (config.query_type == "top_n_slowest") {
    if (config.top_n == 0) {
      if (error != nullptr) {
        *error = "top_n_slowest requires top_n > 0";
      }
      return false;
    }
    result.has_topn = true;
    result.topn = ParallelCongestionQuery::TopNSlowestRecurringLinks(
        dataset, config.top_n, config.min_link_samples, thread_count);
    result.result_count = result.topn.size();
    *out = result;
    return true;
  }

  if (config.query_type == "summary") {
    result.has_aggregate = true;
    result.aggregate = ParallelTrafficAggregator::SummarizeDataset(dataset, thread_count);
    result.result_count = result.aggregate.record_count;
    *out = result;
    return true;
  }

  if (error != nullptr) {
    *error = "query_type not supported in parallel mode: " + config.query_type;
  }
  return false;
}

bool ExecuteQueryOptimized(const BenchmarkConfig& config,
                           const TrafficDatasetOptimized& dataset,
                           std::size_t thread_count,
                           QueryExecutionResult* out,
                           std::string* error) {
  if (out == nullptr) {
    if (error != nullptr) {
      *error = "out cannot be null";
    }
    return false;
  }

  QueryExecutionResult result;
  const bool use_parallel = thread_count > 1;

  if (config.query_type.empty() || config.query_type == "ingest_only") {
    result.result_count = dataset.Size();
    *out = result;
    return true;
  }

  if (config.query_type == "speed_below") {
    if (!config.has_threshold) {
      if (error != nullptr) {
        *error = "speed_below requires threshold";
      }
      return false;
    }
    result.result_count = use_parallel
                              ? OptimizedCongestionQuery::CountSpeedBelowParallel(dataset,
                                                                                  config.threshold_mph,
                                                                                  thread_count)
                              : OptimizedCongestionQuery::CountSpeedBelow(dataset, config.threshold_mph);
    result.has_aggregate = true;
    result.aggregate = use_parallel
                           ? OptimizedTrafficAggregator::SummarizeSpeedBelowParallel(dataset,
                                                                                      config.threshold_mph,
                                                                                      thread_count)
                           : OptimizedTrafficAggregator::SummarizeSpeedBelow(dataset, config.threshold_mph);
    *out = result;
    return true;
  }

  if (config.query_type == "time_window") {
    if (!config.has_start_epoch || !config.has_end_epoch) {
      if (error != nullptr) {
        *error = "time_window requires start and end epoch";
      }
      return false;
    }
    result.result_count =
        use_parallel
            ? OptimizedCongestionQuery::CountTimeWindowParallel(dataset,
                                                                config.start_epoch,
                                                                config.end_epoch,
                                                                thread_count)
            : OptimizedCongestionQuery::CountTimeWindow(dataset, config.start_epoch, config.end_epoch);
    result.has_aggregate = true;
    result.aggregate = use_parallel
                           ? OptimizedTrafficAggregator::SummarizeTimeWindowParallel(dataset,
                                                                                      config.start_epoch,
                                                                                      config.end_epoch,
                                                                                      thread_count)
                           : OptimizedTrafficAggregator::SummarizeTimeWindow(dataset,
                                                                             config.start_epoch,
                                                                             config.end_epoch);
    *out = result;
    return true;
  }

  if (config.query_type == "borough_speed_below") {
    if (config.borough.empty() || !config.has_threshold) {
      if (error != nullptr) {
        *error = "borough_speed_below requires borough and threshold";
      }
      return false;
    }
    const std::int16_t borough_code = ToInt(ParseBoroughCode(config.borough));
    result.result_count = use_parallel
                              ? OptimizedCongestionQuery::CountBoroughAndSpeedBelowParallel(
                                    dataset, borough_code, config.threshold_mph, thread_count)
                              : OptimizedCongestionQuery::CountBoroughAndSpeedBelow(dataset,
                                                                                    borough_code,
                                                                                    config.threshold_mph);
    result.has_aggregate = true;
    result.aggregate = use_parallel
                           ? OptimizedTrafficAggregator::SummarizeBoroughAndSpeedBelowParallel(
                                 dataset, borough_code, config.threshold_mph, thread_count)
                           : OptimizedTrafficAggregator::SummarizeBoroughAndSpeedBelow(dataset,
                                                                                       borough_code,
                                                                                       config.threshold_mph);
    *out = result;
    return true;
  }

  if (config.query_type == "top_n_slowest") {
    if (config.top_n == 0) {
      if (error != nullptr) {
        *error = "top_n_slowest requires top_n > 0";
      }
      return false;
    }
    result.has_topn = true;
    result.topn = use_parallel
                      ? OptimizedCongestionQuery::TopNSlowestRecurringLinksParallel(
                            dataset, config.top_n, config.min_link_samples, thread_count)
                      : OptimizedCongestionQuery::TopNSlowestRecurringLinks(dataset,
                                                                            config.top_n,
                                                                            config.min_link_samples);
    result.result_count = result.topn.size();
    *out = result;
    return true;
  }

  if (config.query_type == "summary") {
    result.has_aggregate = true;
    result.aggregate = use_parallel ? OptimizedTrafficAggregator::SummarizeDatasetParallel(dataset, thread_count)
                                    : OptimizedTrafficAggregator::SummarizeDataset(dataset);
    result.result_count = result.aggregate.record_count;
    *out = result;
    return true;
  }

  if (error != nullptr) {
    *error = "query_type not supported in optimized mode: " + config.query_type;
  }
  return false;
}

bool CompareResults(const QueryExecutionResult& serial,
                    const QueryExecutionResult& parallel,
                    std::string* mismatch) {
  if (serial.result_count != parallel.result_count) {
    if (mismatch != nullptr) {
      *mismatch = "result_count mismatch serial=" + std::to_string(serial.result_count) +
                  " parallel=" + std::to_string(parallel.result_count);
    }
    return false;
  }

  if (serial.has_aggregate != parallel.has_aggregate) {
    if (mismatch != nullptr) {
      *mismatch = "aggregate presence mismatch";
    }
    return false;
  }

  if (serial.has_aggregate) {
    if (!NearlyEqual(serial.aggregate.average_speed_mph, parallel.aggregate.average_speed_mph) ||
        !NearlyEqual(serial.aggregate.average_travel_time_seconds,
                     parallel.aggregate.average_travel_time_seconds)) {
      if (mismatch != nullptr) {
        *mismatch = "aggregate mismatch";
      }
      return false;
    }
  }

  if (serial.has_topn != parallel.has_topn) {
    if (mismatch != nullptr) {
      *mismatch = "topn presence mismatch";
    }
    return false;
  }

  if (serial.has_topn) {
    if (serial.topn.size() != parallel.topn.size()) {
      if (mismatch != nullptr) {
        *mismatch = "topn size mismatch";
      }
      return false;
    }
    for (std::size_t i = 0; i < serial.topn.size(); ++i) {
      const auto& lhs = serial.topn[i];
      const auto& rhs = parallel.topn[i];
      if (lhs.link_id != rhs.link_id || lhs.record_count != rhs.record_count ||
          !NearlyEqual(lhs.average_speed_mph, rhs.average_speed_mph)) {
        if (mismatch != nullptr) {
          *mismatch = "topn row mismatch at index " + std::to_string(i);
        }
        return false;
      }
    }
  }

  return true;
}

bool WriteCsvRows(const BenchmarkConfig& config,
                  const std::vector<BenchmarkRunResult>& results,
                  std::string* error) {
  if (config.output_csv_path.empty()) {
    return true;
  }

  const std::filesystem::path output_path(config.output_csv_path);
  if (output_path.has_parent_path()) {
    std::error_code ec;
    std::filesystem::create_directories(output_path.parent_path(), ec);
    if (ec) {
      if (error != nullptr) {
        *error = "failed to create output directory: " + ec.message();
      }
      return false;
    }
  }

  const bool file_exists = std::filesystem::exists(output_path);
  const bool write_header = !file_exists || !config.append_output || std::filesystem::file_size(output_path) == 0;

  std::ofstream out;
  if (config.append_output) {
    out.open(config.output_csv_path, std::ios::app);
  } else {
    out.open(config.output_csv_path, std::ios::trunc);
  }
  if (!out.is_open()) {
    if (error != nullptr) {
      *error = "failed to open output CSV: " + config.output_csv_path;
    }
    return false;
  }

  if (write_header) {
    out << "timestamp_utc,binary_name,execution_mode,thread_count,serial_validation_enabled,serial_match,compiler_id,compiler_version,dataset_label,dataset_path,query_type,run_number,ingest_ms,query_ms,total_ms,rows_read,rows_accepted,rows_rejected,result_count,has_aggregate,average_speed_mph,average_travel_time_seconds,validation_ms,validation_ingest_ms,cpu_ms,peak_rss_kb\n";
  }

#if defined(__clang__)
  const std::string compiler_id = "clang";
#elif defined(__GNUC__)
  const std::string compiler_id = "gcc";
#else
  const std::string compiler_id = "unknown";
#endif
  const std::string compiler_version = __VERSION__;

  const std::string query_label = config.query_type.empty() ? "ingest_only" : config.query_type;
  const std::string dataset_label = config.dataset_label.empty() ? config.dataset_path : config.dataset_label;

  for (const auto& row : results) {
    out << NowUtcIso8601() << ',' << EscapeCsv(config.binary_name) << ','
        << ExecutionModeString(row.execution_mode) << ',' << row.thread_count << ','
        << (config.validate_parallel_against_serial ? "true" : "false") << ','
        << (row.serial_match ? "true" : "false") << ',' << compiler_id << ','
        << EscapeCsv(compiler_version) << ',' << EscapeCsv(dataset_label) << ','
        << EscapeCsv(config.dataset_path) << ',' << EscapeCsv(query_label) << ',' << row.run_number << ','
        << std::fixed << std::setprecision(3) << row.ingest_ms << ',' << row.query_ms << ',' << row.total_ms
        << ',' << row.rows_read << ',' << row.rows_accepted << ',' << row.rows_rejected << ','
        << row.result_count << ',' << (row.has_aggregate ? "true" : "false") << ','
        << row.average_speed_mph << ',' << row.average_travel_time_seconds << ','
        << row.validation_ms << ',' << row.validation_ingest_ms << ','
        << row.cpu_ms << ',' << row.peak_rss_kb << '\n';
  }

  return true;
}

bool RunInternal(const BenchmarkConfig& config,
                 BenchmarkExecutionMode mode,
                 std::vector<BenchmarkRunResult>* out_results,
                 std::string* error) {
  if (config.dataset_path.empty()) {
    if (error != nullptr) {
      *error = "dataset_path cannot be empty";
    }
    return false;
  }

  if (config.runs == 0) {
    if (error != nullptr) {
      *error = "runs must be > 0";
    }
    return false;
  }

  std::size_t effective_threads = 1;
  if (mode == BenchmarkExecutionMode::kParallel || mode == BenchmarkExecutionMode::kOptimized) {
    effective_threads = config.thread_count;
    if (effective_threads == 0) {
#if defined(_OPENMP)
      effective_threads = static_cast<std::size_t>(omp_get_max_threads());
#else
      effective_threads = 1;
#endif
    }
  }

  std::vector<BenchmarkRunResult> local_results;
  local_results.reserve(config.runs);

  bool expected_set = false;
  std::size_t expected_accepted = 0;

  for (std::size_t run = 1; run <= config.runs; ++run) {
    const auto total_started = Clock::now();

    TrafficDataset dataset;
    TrafficDatasetOptimized optimized_dataset;
    TrafficLoadOptions options;
    options.print_progress = config.enable_ingest_progress;
    options.progress_every_rows = config.progress_every_rows;

    const auto ingest_started = Clock::now();
    std::string ingest_error;
    const bool ingest_ok =
        mode == BenchmarkExecutionMode::kOptimized
            ? CSVReader::LoadTrafficCSVOptimized(
                  config.dataset_path, &optimized_dataset, options, nullptr, &ingest_error)
            : CSVReader::LoadTrafficCSV(config.dataset_path, &dataset, options, nullptr, &ingest_error);
    if (!ingest_ok) {
      if (error != nullptr) {
        *error = "ingest failed on run " + std::to_string(run) + ": " + ingest_error;
      }
      return false;
    }
    const auto ingest_finished = Clock::now();

    const std::size_t accepted_rows = mode == BenchmarkExecutionMode::kOptimized
                                          ? optimized_dataset.Counters().rows_accepted
                                          : dataset.Counters().rows_accepted;
    if (config.has_expected_accepted_rows && accepted_rows != config.expected_accepted_rows) {
      if (error != nullptr) {
        *error = "accepted row count mismatch on run " + std::to_string(run) +
                 " expected=" + std::to_string(config.expected_accepted_rows) +
                 " actual=" + std::to_string(accepted_rows);
      }
      return false;
    }

    if (config.enforce_consistent_accepted_rows) {
      if (!expected_set) {
        expected_accepted = accepted_rows;
        expected_set = true;
      } else if (accepted_rows != expected_accepted) {
        if (error != nullptr) {
          *error = "accepted row count drift across runs: baseline=" + std::to_string(expected_accepted) +
                   " run=" + std::to_string(run) + " actual=" + std::to_string(accepted_rows);
        }
        return false;
      }
    }

    const double cpu_query_start = cpu_now_ms();
    const auto query_started = Clock::now();
    QueryExecutionResult query_result;
    std::string query_error;
    bool query_ok = false;
    if (mode == BenchmarkExecutionMode::kParallel) {
      query_ok = ExecuteQueryParallel(config, dataset, effective_threads, &query_result, &query_error);
    } else if (mode == BenchmarkExecutionMode::kOptimized) {
      query_ok =
          ExecuteQueryOptimized(config, optimized_dataset, effective_threads, &query_result, &query_error);
    } else {
      query_ok = ExecuteQuerySerial(config, dataset, &query_result, &query_error);
    }
    if (!query_ok) {
      if (error != nullptr) {
        *error = "query failed on run " + std::to_string(run) + ": " + query_error;
      }
      return false;
    }

    const auto query_finished = Clock::now();
    const double cpu_query_end = cpu_now_ms();

    bool serial_match = true;
    double validation_ms = 0.0;
    double validation_ingest_ms = 0.0;
    if ((mode == BenchmarkExecutionMode::kParallel || mode == BenchmarkExecutionMode::kOptimized) &&
        config.validate_parallel_against_serial) {
      const auto validation_started = Clock::now();
      QueryExecutionResult serial_result;
      std::string serial_error;
      if (mode == BenchmarkExecutionMode::kOptimized) {
        TrafficDataset serial_dataset;
        std::string serial_ingest_error;
        const auto serial_validation_ingest_started = Clock::now();
        if (!CSVReader::LoadTrafficCSV(
                config.dataset_path, &serial_dataset, options, nullptr, &serial_ingest_error)) {
          if (error != nullptr) {
            *error = "serial validation ingest failed on run " + std::to_string(run) + ": " +
                     serial_ingest_error;
          }
          return false;
        }
        const auto serial_validation_ingest_finished = Clock::now();
        validation_ingest_ms =
            std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
                serial_validation_ingest_finished - serial_validation_ingest_started)
                .count();
        if (!ExecuteQuerySerial(config, serial_dataset, &serial_result, &serial_error)) {
          if (error != nullptr) {
            *error = "serial validation failed on run " + std::to_string(run) + ": " + serial_error;
          }
          return false;
        }
      } else if (!ExecuteQuerySerial(config, dataset, &serial_result, &serial_error)) {
        if (error != nullptr) {
          *error = "serial validation failed on run " + std::to_string(run) + ": " + serial_error;
        }
        return false;
      }
      std::string mismatch;
      serial_match = CompareResults(serial_result, query_result, &mismatch);
      if (!serial_match) {
        if (error != nullptr) {
          *error = "serial validation mismatch on run " + std::to_string(run) + ": " + mismatch;
        }
        return false;
      }
      const auto validation_finished = Clock::now();
      validation_ms =
          std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
              validation_finished - validation_started)
              .count();
    }
    const auto total_finished = Clock::now();

    BenchmarkRunResult row;
    row.run_number = run;
    row.execution_mode = mode;
    row.thread_count =
        (mode == BenchmarkExecutionMode::kParallel || mode == BenchmarkExecutionMode::kOptimized)
            ? effective_threads
            : 1;
    row.ingest_ms =
        std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(ingest_finished - ingest_started)
            .count();
    row.query_ms =
        std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(query_finished - query_started)
            .count();
    row.total_ms =
        std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(total_finished - total_started)
            .count();
    if (mode == BenchmarkExecutionMode::kOptimized) {
      row.rows_read = optimized_dataset.Counters().rows_read;
      row.rows_accepted = optimized_dataset.Counters().rows_accepted;
      row.rows_rejected = optimized_dataset.Counters().rows_rejected;
    } else {
      row.rows_read = dataset.Counters().rows_read;
      row.rows_accepted = dataset.Counters().rows_accepted;
      row.rows_rejected = dataset.Counters().rows_rejected;
    }
    row.result_count = query_result.result_count;
    row.has_aggregate = query_result.has_aggregate;
    row.average_speed_mph = query_result.aggregate.average_speed_mph;
    row.average_travel_time_seconds = query_result.aggregate.average_travel_time_seconds;
    row.serial_match = serial_match;
    row.validation_ms = validation_ms;
    row.validation_ingest_ms = validation_ingest_ms;
    row.cpu_ms = cpu_query_end - cpu_query_start;
    row.peak_rss_kb = read_peak_rss_kb();

    local_results.push_back(row);
  }

  if (!WriteCsvRows(config, local_results, error)) {
    return false;
  }

  if (out_results != nullptr) {
    *out_results = local_results;
  }

  return true;
}

}  // namespace

bool BenchmarkHarness::RunSerial(const BenchmarkConfig& config,
                                 std::vector<BenchmarkRunResult>* out_results,
                                 std::string* error) {
  return RunInternal(config, BenchmarkExecutionMode::kSerial, out_results, error);
}

bool BenchmarkHarness::RunParallel(const BenchmarkConfig& config,
                                   std::vector<BenchmarkRunResult>* out_results,
                                   std::string* error) {
  return RunInternal(config, BenchmarkExecutionMode::kParallel, out_results, error);
}

bool BenchmarkHarness::RunOptimized(const BenchmarkConfig& config,
                                    std::vector<BenchmarkRunResult>* out_results,
                                    std::string* error) {
  return RunInternal(config, BenchmarkExecutionMode::kOptimized, out_results, error);
}

}  // namespace urbandrop
