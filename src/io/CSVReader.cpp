#include "io/CSVReader.hpp"
#include "io/CsvParseUtils.hpp"

#include <chrono>
#include <fstream>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

#include "data_model/TrafficRecord.hpp"
#include "data_model/CommonCodes.hpp"

namespace urbandrop {

bool CSVReader::LoadTrafficCSV(const std::string& csv_path,
                               TrafficDataset* dataset,
                               const TrafficLoadOptions& options,
                               std::ostream* log_stream,
                               std::string* error) {
  if (dataset == nullptr) {
    if (error != nullptr) {
      *error = "dataset pointer cannot be null";
    }
    return false;
  }

  dataset->Clear();

  std::ifstream input(csv_path);
  if (!input.is_open()) {
    if (error != nullptr) {
      *error = "unable to open CSV file: " + csv_path;
    }
    return false;
  }

  std::string header_line;
  if (!std::getline(input, header_line)) {
    if (error != nullptr) {
      *error = "CSV file is empty: " + csv_path;
    }
    return false;
  }

  const std::vector<std::string> header_fields = io::ParseCsvLine(header_line);
  std::unordered_map<std::string, std::size_t> columns;
  columns.reserve(header_fields.size());
  for (std::size_t i = 0; i < header_fields.size(); ++i) {
    columns[io::ToLower(header_fields[i])] = i;
  }

  std::size_t link_id_col = std::numeric_limits<std::size_t>::max();
  std::size_t speed_col = std::numeric_limits<std::size_t>::max();
  std::size_t travel_time_col = std::numeric_limits<std::size_t>::max();
  std::size_t timestamp_col = std::numeric_limits<std::size_t>::max();
  std::size_t borough_col = std::numeric_limits<std::size_t>::max();
  std::size_t link_name_col = std::numeric_limits<std::size_t>::max();

  const bool found_required =
      io::ResolveColumn(columns, {"link_id", "link id"}, &link_id_col) &&
      io::ResolveColumn(columns, {"speed"}, &speed_col) &&
      io::ResolveColumn(columns, {"travel_time", "travel time"}, &travel_time_col) &&
      io::ResolveColumn(columns, {"data_as_of", "timestamp", "datetime"}, &timestamp_col);

  io::ResolveColumn(columns, {"borough"}, &borough_col);
  io::ResolveColumn(columns, {"link_name", "link name"}, &link_name_col);

  if (!found_required) {
    if (error != nullptr) {
      *error = "CSV missing one or more required columns: link_id, speed, travel_time, data_as_of";
    }
    return false;
  }

  IngestionCounters& counters = dataset->Counters();
  std::string line;
  while (std::getline(input, line)) {
    if (line.empty()) {
      continue;
    }

    ++counters.rows_read;
    const std::vector<std::string> fields = io::ParseCsvLine(line);

    const auto has_col = [&fields](std::size_t idx) { return idx < fields.size(); };
    if (!has_col(link_id_col) || !has_col(speed_col) || !has_col(travel_time_col) || !has_col(timestamp_col)) {
      ++counters.rows_rejected;
      ++counters.malformed_rows;
      continue;
    }

    TrafficRecord record;
    if (!io::ParseInt64(fields[link_id_col], &record.link_id) ||
        !io::ParseDouble(fields[speed_col], &record.speed_mph) ||
        !io::ParseDouble(fields[travel_time_col], &record.travel_time_seconds)) {
      ++counters.rows_rejected;
      ++counters.malformed_rows;
      continue;
    }

    if (!io::ParseTimestamp(fields[timestamp_col], &record.timestamp_epoch_seconds)) {
      ++counters.rows_rejected;
      ++counters.missing_timestamp;
      continue;
    }

    if (has_col(borough_col)) {
      record.borough = fields[borough_col];
      record.borough_code = ToInt(ParseBoroughCode(record.borough));
    }
    if (has_col(link_name_col)) {
      record.link_name = fields[link_name_col];
    }

    if (record.speed_mph < 0.0 || record.speed_mph > 120.0) {
      ++counters.suspicious_speed;
    }
    if (record.travel_time_seconds < 0.0) {
      ++counters.suspicious_travel_time;
    }

    if (!record.HasValidCoreFields()) {
      ++counters.rows_rejected;
      ++counters.malformed_rows;
      continue;
    }

    dataset->AddRecord(record);

    if (options.print_progress && options.progress_every_rows > 0 &&
        counters.rows_read % options.progress_every_rows == 0 && log_stream != nullptr) {
      (*log_stream) << "[ingest] rows_read=" << counters.rows_read
                    << " accepted=" << counters.rows_accepted
                    << " rejected=" << counters.rows_rejected << '\n';
    }
  }

  return true;
}

bool CSVReader::LoadTrafficCSVOptimized(const std::string& csv_path,
                                        TrafficDatasetOptimized* dataset,
                                        const TrafficLoadOptions& options,
                                        std::ostream* log_stream,
                                        std::string* error) {
  if (dataset == nullptr) {
    if (error != nullptr) {
      *error = "optimized dataset pointer cannot be null";
    }
    return false;
  }

  dataset->Clear();

  std::ifstream input(csv_path);
  if (!input.is_open()) {
    if (error != nullptr) {
      *error = "unable to open CSV file: " + csv_path;
    }
    return false;
  }

  std::string header_line;
  if (!std::getline(input, header_line)) {
    if (error != nullptr) {
      *error = "CSV file is empty: " + csv_path;
    }
    return false;
  }

  const std::vector<std::string> header_fields = io::ParseCsvLine(header_line);
  std::unordered_map<std::string, std::size_t> columns;
  columns.reserve(header_fields.size());
  for (std::size_t i = 0; i < header_fields.size(); ++i) {
    columns[io::ToLower(header_fields[i])] = i;
  }

  std::size_t link_id_col = std::numeric_limits<std::size_t>::max();
  std::size_t speed_col = std::numeric_limits<std::size_t>::max();
  std::size_t travel_time_col = std::numeric_limits<std::size_t>::max();
  std::size_t timestamp_col = std::numeric_limits<std::size_t>::max();
  std::size_t borough_col = std::numeric_limits<std::size_t>::max();
  std::size_t link_name_col = std::numeric_limits<std::size_t>::max();

  const bool found_required =
      io::ResolveColumn(columns, {"link_id", "link id"}, &link_id_col) &&
      io::ResolveColumn(columns, {"speed"}, &speed_col) &&
      io::ResolveColumn(columns, {"travel_time", "travel time"}, &travel_time_col) &&
      io::ResolveColumn(columns, {"data_as_of", "timestamp", "datetime"}, &timestamp_col);

  io::ResolveColumn(columns, {"borough"}, &borough_col);
  io::ResolveColumn(columns, {"link_name", "link name"}, &link_name_col);

  if (!found_required) {
    if (error != nullptr) {
      *error = "CSV missing one or more required columns: link_id, speed, travel_time, data_as_of";
    }
    return false;
  }

  IngestionCounters& counters = dataset->Counters();
  std::string line;
  while (std::getline(input, line)) {
    if (line.empty()) {
      continue;
    }

    ++counters.rows_read;
    const std::vector<std::string> fields = io::ParseCsvLine(line);

    const auto has_col = [&fields](std::size_t idx) { return idx < fields.size(); };
    if (!has_col(link_id_col) || !has_col(speed_col) || !has_col(travel_time_col) ||
        !has_col(timestamp_col)) {
      ++counters.rows_rejected;
      ++counters.malformed_rows;
      continue;
    }

    std::int64_t link_id = -1;
    double speed_mph = 0.0;
    double travel_time_seconds = 0.0;
    std::int64_t timestamp_epoch_seconds = 0;

    if (!io::ParseInt64(fields[link_id_col], &link_id) || !io::ParseDouble(fields[speed_col], &speed_mph) ||
        !io::ParseDouble(fields[travel_time_col], &travel_time_seconds)) {
      ++counters.rows_rejected;
      ++counters.malformed_rows;
      continue;
    }

    if (!io::ParseTimestamp(fields[timestamp_col], &timestamp_epoch_seconds)) {
      ++counters.rows_rejected;
      ++counters.missing_timestamp;
      continue;
    }

    std::int16_t borough_code = -1;
    std::string link_name;
    if (has_col(borough_col)) {
      borough_code = ToInt(ParseBoroughCode(fields[borough_col]));
    }
    if (has_col(link_name_col)) {
      link_name = fields[link_name_col];
    }

    if (speed_mph < 0.0 || speed_mph > 120.0) {
      ++counters.suspicious_speed;
    }
    if (travel_time_seconds < 0.0) {
      ++counters.suspicious_travel_time;
    }
    if (link_id < 0 || speed_mph < 0.0 || travel_time_seconds < 0.0 || timestamp_epoch_seconds <= 0) {
      ++counters.rows_rejected;
      ++counters.malformed_rows;
      continue;
    }

    dataset->AddRecord(link_id,
                       speed_mph,
                       travel_time_seconds,
                       timestamp_epoch_seconds,
                       borough_code,
                       link_name);

    if (options.print_progress && options.progress_every_rows > 0 &&
        counters.rows_read % options.progress_every_rows == 0 && log_stream != nullptr) {
      (*log_stream) << "[ingest_optimized] rows_read=" << counters.rows_read
                    << " accepted=" << counters.rows_accepted
                    << " rejected=" << counters.rows_rejected << '\n';
    }
  }

  return true;
}

}  // namespace urbandrop
