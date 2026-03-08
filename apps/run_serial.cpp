#include <chrono>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

#include "benchmark/BenchmarkHarness.hpp"
#include "data_model/BuildingRecord.hpp"
#include "data_model/GarageRecord.hpp"
#include "data_model/TrafficDataset.hpp"
#include "io/BuildingLoader.hpp"
#include "io/CSVReader.hpp"
#include "io/GarageLoader.hpp"
#include "query/CongestionQuery.hpp"
#include "query/TimeWindowQuery.hpp"
#include "query/TrafficAggregator.hpp"

namespace {

using Clock = std::chrono::steady_clock;

void PrintUsage() {
  std::cout
      << "Usage: run_serial --traffic <path> [--garages <path>] [--bes <path>] "
         "[--progress-every <rows>] [--sample <count>] [--query <type>] "
         "[--benchmark-runs <n>] [--benchmark-out <path>]\n"
      << "\n"
      << "Query types and required params:\n"
      << "  speed_below            --threshold <mph>\n"
      << "  time_window            --start-epoch <sec> --end-epoch <sec>\n"
      << "  borough_speed_below    --borough <name> --threshold <mph>\n"
      << "  link_id                --link-id <id>\n"
      << "  link_range             --link-start <id> --link-end <id>\n"
      << "  top_n_slowest          --top-n <n> [--min-link-samples <n>]\n"
      << "  summary                (no extra params)\n";
  std::cout << "\nBenchmark mode options:\n"
            << "  --benchmark-runs <n>      run ingest+query benchmark n times\n"
            << "  --benchmark-out <path>    output CSV path (for example results/raw/serial.csv)\n"
            << "  --benchmark-append        append to existing CSV instead of overwrite\n"
            << "  --dataset-label <label>   metadata label for output rows\n"
            << "  --expect-accepted <n>     fail if accepted row count differs from n\n";
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
  std::string garages_csv;
  std::string bes_csv;
  std::string query_type;
  std::string borough;

  std::size_t progress_every = 500000;
  std::size_t sample_count = 0;
  std::size_t top_n = 0;
  std::size_t min_link_samples = 1;
  std::size_t benchmark_runs = 0;
  bool benchmark_append = false;
  std::string benchmark_out_csv;
  std::string dataset_label;
  std::size_t expected_accepted = 0;
  bool has_expected_accepted = false;

  double threshold_mph = 0.0;
  bool has_threshold = false;

  std::int64_t start_epoch = 0;
  std::int64_t end_epoch = 0;
  bool has_start_epoch = false;
  bool has_end_epoch = false;

  std::int64_t link_id = 0;
  bool has_link_id = false;

  std::int64_t link_start = 0;
  std::int64_t link_end = 0;
  bool has_link_start = false;
  bool has_link_end = false;

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
    } else if (arg == "--query" && i + 1 < argc) {
      query_type = argv[++i];
    } else if (arg == "--threshold" && i + 1 < argc) {
      has_threshold = ParseDouble(argv[++i], &threshold_mph);
    } else if (arg == "--start-epoch" && i + 1 < argc) {
      has_start_epoch = ParseInt64(argv[++i], &start_epoch);
    } else if (arg == "--end-epoch" && i + 1 < argc) {
      has_end_epoch = ParseInt64(argv[++i], &end_epoch);
    } else if (arg == "--borough" && i + 1 < argc) {
      borough = argv[++i];
    } else if (arg == "--link-id" && i + 1 < argc) {
      has_link_id = ParseInt64(argv[++i], &link_id);
    } else if (arg == "--link-start" && i + 1 < argc) {
      has_link_start = ParseInt64(argv[++i], &link_start);
    } else if (arg == "--link-end" && i + 1 < argc) {
      has_link_end = ParseInt64(argv[++i], &link_end);
    } else if (arg == "--top-n" && i + 1 < argc) {
      top_n = static_cast<std::size_t>(std::stoull(argv[++i]));
    } else if (arg == "--min-link-samples" && i + 1 < argc) {
      min_link_samples = static_cast<std::size_t>(std::stoull(argv[++i]));
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
    } else if (arg == "--help") {
      PrintUsage();
      return 0;
    } else {
      std::cerr << "Unknown or incomplete argument: " << arg << "\n";
      PrintUsage();
      return 2;
    }
  }

  if (traffic_csv.empty()) {
    PrintUsage();
    return 2;
  }

  if (benchmark_runs > 0) {
    urbandrop::BenchmarkConfig config;
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
    config.link_id = link_id;
    config.has_link_id = has_link_id;
    config.link_start = link_start;
    config.link_end = link_end;
    config.has_link_start = has_link_start;
    config.has_link_end = has_link_end;
    config.top_n = top_n;
    config.min_link_samples = min_link_samples;
    config.runs = benchmark_runs;
    config.output_csv_path = benchmark_out_csv;
    config.append_output = benchmark_append;
    config.enable_ingest_progress = false;
    config.progress_every_rows = progress_every;
    config.has_expected_accepted_rows = has_expected_accepted;
    config.expected_accepted_rows = expected_accepted;

    std::vector<urbandrop::BenchmarkRunResult> results;
    std::string benchmark_error;
    if (!urbandrop::BenchmarkHarness::RunSerial(config, &results, &benchmark_error)) {
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
              << " query_type=" << (query_type.empty() ? "ingest_only" : query_type) << "\n";
    std::cout << "  avg_ingest_ms=" << (total_ingest / runs) << "\n";
    std::cout << "  avg_query_ms=" << (total_query / runs) << "\n";
    std::cout << "  avg_total_ms=" << (total_total / runs) << "\n";
    if (!benchmark_out_csv.empty()) {
      std::cout << "  output_csv=" << benchmark_out_csv << "\n";
    }
    return 0;
  }

  urbandrop::TrafficDataset dataset;
  urbandrop::TrafficLoadOptions options;
  options.progress_every_rows = progress_every;
  options.print_progress = true;

  std::string error;
  std::cout << "[ingest] start path=" << traffic_csv << "\n";
  if (!urbandrop::CSVReader::LoadTrafficCSV(traffic_csv, &dataset, options, &std::cout, &error)) {
    std::cerr << "[ingest] failed: " << error << "\n";
    return 1;
  }

  const urbandrop::IngestionCounters& counters = dataset.Counters();
  std::cout << "[ingest] complete\n";
  std::cout << "  rows_read=" << counters.rows_read << "\n";
  std::cout << "  rows_accepted=" << counters.rows_accepted << "\n";
  std::cout << "  rows_rejected=" << counters.rows_rejected << "\n";
  std::cout << "  malformed_rows=" << counters.malformed_rows << "\n";
  std::cout << "  missing_timestamp=" << counters.missing_timestamp << "\n";
  std::cout << "  suspicious_speed=" << counters.suspicious_speed << "\n";
  std::cout << "  suspicious_travel_time=" << counters.suspicious_travel_time << "\n";
  if (has_expected_accepted && counters.rows_accepted != expected_accepted) {
    std::cerr << "[ingest] accepted row mismatch expected=" << expected_accepted
              << " actual=" << counters.rows_accepted << "\n";
    return 1;
  }

  if (sample_count > 0) {
    std::cout << "[sample] showing up to " << sample_count << " accepted rows\n";
    for (std::size_t i = 0; i < dataset.Size() && i < sample_count; ++i) {
      const auto* r = dataset.At(i);
      if (r == nullptr) {
        break;
      }
      std::cout << "  #" << i << " link_id=" << r->link_id << " speed_mph=" << r->speed_mph
                << " travel_time_seconds=" << r->travel_time_seconds
                << " timestamp_epoch_seconds=" << r->timestamp_epoch_seconds
                << " borough=\"" << r->borough << "\""
                << " link_name=\"" << r->link_name << "\"\n";
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
              << " accepted=" << stats.rows_accepted << " rejected=" << stats.rows_rejected
              << "\n";
  }

  if (!bes_csv.empty()) {
    std::vector<urbandrop::BuildingRecord> buildings;
    urbandrop::LoaderStats stats;
    if (!urbandrop::BuildingLoader::LoadCSV(bes_csv, &buildings, &stats, &error)) {
      std::cerr << "[bes] failed: " << error << "\n";
      return 1;
    }
    std::cout << "[bes] complete rows_read=" << stats.rows_read << " accepted=" << stats.rows_accepted
              << " rejected=" << stats.rows_rejected << "\n";
  }

  if (query_type.empty()) {
    return 0;
  }

  const auto query_started = Clock::now();
  std::cout << "[query] start type=" << query_type << "\n";

  if (query_type == "speed_below") {
    if (!has_threshold) {
      std::cerr << "speed_below requires --threshold\n";
      return 2;
    }
    const auto matches = urbandrop::CongestionQuery::FilterSpeedBelow(dataset, threshold_mph);
    const auto stats = urbandrop::TrafficAggregator::Summarize(matches);
    std::cout << "[query] matched_records=" << stats.record_count
              << " avg_speed_mph=" << stats.average_speed_mph
              << " avg_travel_time_seconds=" << stats.average_travel_time_seconds << "\n";
  } else if (query_type == "time_window") {
    if (!has_start_epoch || !has_end_epoch) {
      std::cerr << "time_window requires --start-epoch and --end-epoch\n";
      return 2;
    }
    const auto matches =
        urbandrop::TimeWindowQuery::FilterByEpochRange(dataset, start_epoch, end_epoch);
    const auto stats = urbandrop::TrafficAggregator::Summarize(matches);
    std::cout << "[query] matched_records=" << stats.record_count
              << " avg_speed_mph=" << stats.average_speed_mph
              << " avg_travel_time_seconds=" << stats.average_travel_time_seconds << "\n";
  } else if (query_type == "borough_speed_below") {
    if (borough.empty() || !has_threshold) {
      std::cerr << "borough_speed_below requires --borough and --threshold\n";
      return 2;
    }
    const auto matches =
        urbandrop::CongestionQuery::FilterBoroughAndSpeedBelow(dataset, borough, threshold_mph);
    const auto stats = urbandrop::TrafficAggregator::Summarize(matches);
    std::cout << "[query] matched_records=" << stats.record_count
              << " avg_speed_mph=" << stats.average_speed_mph
              << " avg_travel_time_seconds=" << stats.average_travel_time_seconds << "\n";
  } else if (query_type == "link_id") {
    if (!has_link_id) {
      std::cerr << "link_id requires --link-id\n";
      return 2;
    }
    const auto matches = urbandrop::CongestionQuery::FindByLinkId(dataset, link_id);
    const auto stats = urbandrop::TrafficAggregator::Summarize(matches);
    std::cout << "[query] matched_records=" << stats.record_count
              << " avg_speed_mph=" << stats.average_speed_mph
              << " avg_travel_time_seconds=" << stats.average_travel_time_seconds << "\n";
  } else if (query_type == "link_range") {
    if (!has_link_start || !has_link_end) {
      std::cerr << "link_range requires --link-start and --link-end\n";
      return 2;
    }
    const auto matches = urbandrop::CongestionQuery::FindByLinkRange(dataset, link_start, link_end);
    const auto stats = urbandrop::TrafficAggregator::Summarize(matches);
    std::cout << "[query] matched_records=" << stats.record_count
              << " avg_speed_mph=" << stats.average_speed_mph
              << " avg_travel_time_seconds=" << stats.average_travel_time_seconds << "\n";
  } else if (query_type == "top_n_slowest") {
    if (top_n == 0) {
      std::cerr << "top_n_slowest requires --top-n > 0\n";
      return 2;
    }
    const auto stats =
        urbandrop::CongestionQuery::TopNSlowestRecurringLinks(dataset, top_n, min_link_samples);
    std::cout << "[query] ranked_links=" << stats.size() << "\n";
    for (std::size_t i = 0; i < stats.size(); ++i) {
      const auto& s = stats[i];
      std::cout << "  rank=" << (i + 1) << " link_id=" << s.link_id
                << " samples=" << s.record_count << " avg_speed_mph=" << s.average_speed_mph
                << "\n";
    }
  } else if (query_type == "summary") {
    const auto stats = urbandrop::TrafficAggregator::SummarizeDataset(dataset);
    std::cout << "[query] matched_records=" << stats.record_count
              << " avg_speed_mph=" << stats.average_speed_mph
              << " avg_travel_time_seconds=" << stats.average_travel_time_seconds << "\n";
  } else {
    std::cerr << "unknown --query type: " << query_type << "\n";
    PrintUsage();
    return 2;
  }

  const auto query_ended = Clock::now();
  const auto elapsed_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(query_ended - query_started).count();
  std::cout << "[query] complete elapsed_ms=" << elapsed_ms << "\n";

  return 0;
}
