#include "io/CSVReader.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "data_model/TrafficRecord.hpp"
#include "data_model/CommonCodes.hpp"

namespace urbandrop {
namespace {

std::string Trim(const std::string& input) {
  std::size_t start = 0;
  while (start < input.size() && std::isspace(static_cast<unsigned char>(input[start])) != 0) {
    ++start;
  }

  std::size_t end = input.size();
  while (end > start && std::isspace(static_cast<unsigned char>(input[end - 1])) != 0) {
    --end;
  }

  return input.substr(start, end - start);
}

std::string ToLower(std::string input) {
  std::transform(input.begin(), input.end(), input.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return input;
}

std::vector<std::string> ParseCsvLine(const std::string& line) {
  std::vector<std::string> fields;
  std::string current;
  bool in_quotes = false;

  for (std::size_t i = 0; i < line.size(); ++i) {
    const char c = line[i];

    if (c == '"') {
      if (in_quotes && i + 1 < line.size() && line[i + 1] == '"') {
        current.push_back('"');
        ++i;
      } else {
        in_quotes = !in_quotes;
      }
      continue;
    }

    if (c == ',' && !in_quotes) {
      fields.push_back(Trim(current));
      current.clear();
      continue;
    }

    current.push_back(c);
  }

  fields.push_back(Trim(current));
  return fields;
}

bool ParseInt64(const std::string& value, std::int64_t* out) {
  if (value.empty()) {
    return false;
  }
  char* end = nullptr;
  errno = 0;
  const long long parsed = std::strtoll(value.c_str(), &end, 10);
  if (errno != 0 || end == value.c_str() || *end != '\0') {
    return false;
  }
  *out = static_cast<std::int64_t>(parsed);
  return true;
}

bool ParseDouble(const std::string& value, double* out) {
  if (value.empty()) {
    return false;
  }
  char* end = nullptr;
  errno = 0;
  const double parsed = std::strtod(value.c_str(), &end);
  if (errno != 0 || end == value.c_str() || *end != '\0') {
    return false;
  }
  *out = parsed;
  return true;
}

std::string NormalizeTimestamp(const std::string& value) {
  std::string cleaned = Trim(value);
  if (cleaned.empty()) {
    return cleaned;
  }

  if (!cleaned.empty() && cleaned.back() == 'Z') {
    cleaned.pop_back();
  }

  const std::size_t dot_pos = cleaned.find('.');
  if (dot_pos != std::string::npos) {
    cleaned = cleaned.substr(0, dot_pos);
  }

  return cleaned;
}

bool ParseTimestamp(const std::string& value, std::int64_t* out_epoch_seconds) {
  const std::string normalized = NormalizeTimestamp(value);
  if (normalized.empty()) {
    return false;
  }

  std::tm tm = {};
  std::istringstream stream(normalized);
  stream >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
  if (stream.fail()) {
    stream.clear();
    stream.str(normalized);
    stream >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    if (stream.fail()) {
      return false;
    }
  }

#if defined(_WIN32)
  const std::time_t parsed_time = _mkgmtime(&tm);
#else
  const std::time_t parsed_time = timegm(&tm);
#endif

  if (parsed_time < 0) {
    return false;
  }

  *out_epoch_seconds = static_cast<std::int64_t>(parsed_time);
  return true;
}

bool ResolveColumn(const std::unordered_map<std::string, std::size_t>& columns,
                   const std::vector<std::string>& aliases,
                   std::size_t* index) {
  for (const std::string& alias : aliases) {
    const auto it = columns.find(alias);
    if (it != columns.end()) {
      *index = it->second;
      return true;
    }
  }
  return false;
}

}  // namespace

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

  const std::vector<std::string> header_fields = ParseCsvLine(header_line);
  std::unordered_map<std::string, std::size_t> columns;
  columns.reserve(header_fields.size());
  for (std::size_t i = 0; i < header_fields.size(); ++i) {
    columns[ToLower(header_fields[i])] = i;
  }

  std::size_t link_id_col = std::numeric_limits<std::size_t>::max();
  std::size_t speed_col = std::numeric_limits<std::size_t>::max();
  std::size_t travel_time_col = std::numeric_limits<std::size_t>::max();
  std::size_t timestamp_col = std::numeric_limits<std::size_t>::max();
  std::size_t borough_col = std::numeric_limits<std::size_t>::max();
  std::size_t link_name_col = std::numeric_limits<std::size_t>::max();

  const bool found_required =
      ResolveColumn(columns, {"link_id", "link id"}, &link_id_col) &&
      ResolveColumn(columns, {"speed"}, &speed_col) &&
      ResolveColumn(columns, {"travel_time", "travel time"}, &travel_time_col) &&
      ResolveColumn(columns, {"data_as_of", "timestamp", "datetime"}, &timestamp_col);

  ResolveColumn(columns, {"borough"}, &borough_col);
  ResolveColumn(columns, {"link_name", "link name"}, &link_name_col);

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
    const std::vector<std::string> fields = ParseCsvLine(line);

    const auto has_col = [&fields](std::size_t idx) { return idx < fields.size(); };
    if (!has_col(link_id_col) || !has_col(speed_col) || !has_col(travel_time_col) || !has_col(timestamp_col)) {
      ++counters.rows_rejected;
      ++counters.malformed_rows;
      continue;
    }

    TrafficRecord record;
    if (!ParseInt64(fields[link_id_col], &record.link_id) ||
        !ParseDouble(fields[speed_col], &record.speed_mph) ||
        !ParseDouble(fields[travel_time_col], &record.travel_time_seconds)) {
      ++counters.rows_rejected;
      ++counters.malformed_rows;
      continue;
    }

    if (!ParseTimestamp(fields[timestamp_col], &record.timestamp_epoch_seconds)) {
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

}  // namespace urbandrop
