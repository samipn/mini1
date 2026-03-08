#include "benchmark/BenchmarkHarness.hpp"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "data_model/TrafficDataset.hpp"
#include "io/CSVReader.hpp"
#include "query/CongestionQuery.hpp"
#include "query/TimeWindowQuery.hpp"
#include "query/TrafficAggregator.hpp"

namespace urbandrop {
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

bool ExecuteConfiguredQuery(const BenchmarkConfig& config,
                            const TrafficDataset& dataset,
                            std::size_t* out_result_count,
                            std::string* error) {
  if (out_result_count == nullptr) {
    if (error != nullptr) {
      *error = "out_result_count cannot be null";
    }
    return false;
  }

  if (config.query_type.empty() || config.query_type == "ingest_only") {
    *out_result_count = dataset.Size();
    return true;
  }

  if (config.query_type == "speed_below") {
    if (!config.has_threshold) {
      if (error != nullptr) {
        *error = "speed_below requires threshold";
      }
      return false;
    }
    *out_result_count = CongestionQuery::FilterSpeedBelow(dataset, config.threshold_mph).size();
    return true;
  }

  if (config.query_type == "time_window") {
    if (!config.has_start_epoch || !config.has_end_epoch) {
      if (error != nullptr) {
        *error = "time_window requires start and end epoch";
      }
      return false;
    }
    *out_result_count =
        TimeWindowQuery::FilterByEpochRange(dataset, config.start_epoch, config.end_epoch).size();
    return true;
  }

  if (config.query_type == "borough_speed_below") {
    if (config.borough.empty() || !config.has_threshold) {
      if (error != nullptr) {
        *error = "borough_speed_below requires borough and threshold";
      }
      return false;
    }
    *out_result_count =
        CongestionQuery::FilterBoroughAndSpeedBelow(dataset, config.borough, config.threshold_mph).size();
    return true;
  }

  if (config.query_type == "link_id") {
    if (!config.has_link_id) {
      if (error != nullptr) {
        *error = "link_id requires link_id";
      }
      return false;
    }
    *out_result_count = CongestionQuery::FindByLinkId(dataset, config.link_id).size();
    return true;
  }

  if (config.query_type == "link_range") {
    if (!config.has_link_start || !config.has_link_end) {
      if (error != nullptr) {
        *error = "link_range requires link_start and link_end";
      }
      return false;
    }
    *out_result_count = CongestionQuery::FindByLinkRange(dataset, config.link_start, config.link_end).size();
    return true;
  }

  if (config.query_type == "top_n_slowest") {
    if (config.top_n == 0) {
      if (error != nullptr) {
        *error = "top_n_slowest requires top_n > 0";
      }
      return false;
    }
    *out_result_count =
        CongestionQuery::TopNSlowestRecurringLinks(dataset, config.top_n, config.min_link_samples).size();
    return true;
  }

  if (config.query_type == "summary") {
    *out_result_count = TrafficAggregator::SummarizeDataset(dataset).record_count;
    return true;
  }

  if (error != nullptr) {
    *error = "unknown query_type: " + config.query_type;
  }
  return false;
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
    out << "timestamp_utc,binary_name,compiler_id,compiler_version,dataset_label,dataset_path,query_type,run_number,ingest_ms,query_ms,total_ms,rows_read,rows_accepted,rows_rejected,result_count\n";
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
    out << NowUtcIso8601() << ',' << EscapeCsv(config.binary_name) << ',' << compiler_id << ','
        << EscapeCsv(compiler_version) << ',' << EscapeCsv(dataset_label) << ','
        << EscapeCsv(config.dataset_path) << ',' << EscapeCsv(query_label) << ',' << row.run_number << ','
        << std::fixed << std::setprecision(3) << row.ingest_ms << ',' << row.query_ms << ',' << row.total_ms
        << ',' << row.rows_read << ',' << row.rows_accepted << ',' << row.rows_rejected << ','
        << row.result_count << '\n';
  }

  return true;
}

}  // namespace

bool BenchmarkHarness::RunSerial(const BenchmarkConfig& config,
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

  std::vector<BenchmarkRunResult> local_results;
  local_results.reserve(config.runs);

  bool expected_set = false;
  std::size_t expected_accepted = 0;

  for (std::size_t run = 1; run <= config.runs; ++run) {
    const auto total_started = Clock::now();

    TrafficDataset dataset;
    TrafficLoadOptions options;
    options.print_progress = config.enable_ingest_progress;
    options.progress_every_rows = config.progress_every_rows;

    const auto ingest_started = Clock::now();
    std::string ingest_error;
    if (!CSVReader::LoadTrafficCSV(config.dataset_path, &dataset, options, nullptr, &ingest_error)) {
      if (error != nullptr) {
        *error = "ingest failed on run " + std::to_string(run) + ": " + ingest_error;
      }
      return false;
    }
    const auto ingest_finished = Clock::now();

    const std::size_t accepted_rows = dataset.Counters().rows_accepted;
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
          *error = "accepted row count drift across runs: baseline=" +
                   std::to_string(expected_accepted) + " run=" + std::to_string(run) +
                   " actual=" + std::to_string(accepted_rows);
        }
        return false;
      }
    }

    const auto query_started = Clock::now();
    std::size_t result_count = 0;
    std::string query_error;
    if (!ExecuteConfiguredQuery(config, dataset, &result_count, &query_error)) {
      if (error != nullptr) {
        *error = "query failed on run " + std::to_string(run) + ": " + query_error;
      }
      return false;
    }
    const auto query_finished = Clock::now();
    const auto total_finished = Clock::now();

    BenchmarkRunResult row;
    row.run_number = run;
    row.ingest_ms =
        std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(ingest_finished - ingest_started)
            .count();
    row.query_ms =
        std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(query_finished - query_started)
            .count();
    row.total_ms =
        std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(total_finished - total_started)
            .count();
    row.rows_read = dataset.Counters().rows_read;
    row.rows_accepted = dataset.Counters().rows_accepted;
    row.rows_rejected = dataset.Counters().rows_rejected;
    row.result_count = result_count;

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

}  // namespace urbandrop
