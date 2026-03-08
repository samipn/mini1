#include "io/BuildingLoader.hpp"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

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

bool ParseInt32(const std::string& value, std::int32_t* out) {
  if (value.empty()) {
    return false;
  }
  errno = 0;
  char* end = nullptr;
  const long parsed = std::strtol(value.c_str(), &end, 10);
  if (errno != 0 || end == value.c_str() || *end != '\0') {
    return false;
  }
  *out = static_cast<std::int32_t>(parsed);
  return true;
}

bool ParseDouble(const std::string& value, double* out) {
  if (value.empty()) {
    return false;
  }
  errno = 0;
  char* end = nullptr;
  const double parsed = std::strtod(value.c_str(), &end);
  if (errno != 0 || end == value.c_str() || *end != '\0') {
    return false;
  }
  *out = parsed;
  return true;
}

bool ResolveColumn(const std::unordered_map<std::string, std::size_t>& columns,
                   const std::vector<std::string>& aliases,
                   std::size_t* index) {
  for (const auto& alias : aliases) {
    const auto it = columns.find(alias);
    if (it != columns.end()) {
      *index = it->second;
      return true;
    }
  }
  return false;
}

}  // namespace

bool BuildingLoader::LoadCSV(const std::string& csv_path,
                             std::vector<BuildingRecord>* out_records,
                             LoaderStats* out_stats,
                             std::string* error) {
  if (out_records == nullptr || out_stats == nullptr) {
    if (error != nullptr) {
      *error = "building loader output pointers cannot be null";
    }
    return false;
  }

  std::ifstream input(csv_path);
  if (!input.is_open()) {
    if (error != nullptr) {
      *error = "unable to open BES CSV: " + csv_path;
    }
    return false;
  }

  std::string header_line;
  if (!std::getline(input, header_line)) {
    if (error != nullptr) {
      *error = "BES CSV is empty: " + csv_path;
    }
    return false;
  }

  const auto headers = ParseCsvLine(header_line);
  std::unordered_map<std::string, std::size_t> cols;
  for (std::size_t i = 0; i < headers.size(); ++i) {
    cols[ToLower(headers[i])] = i;
  }

  std::size_t bbl_col = static_cast<std::size_t>(-1);
  std::size_t subgrade_col = static_cast<std::size_t>(-1);
  std::size_t elevation_col = static_cast<std::size_t>(-1);
  std::size_t lat_col = static_cast<std::size_t>(-1);
  std::size_t lon_col = static_cast<std::size_t>(-1);

  ResolveColumn(cols, {"bbl"}, &bbl_col);
  ResolveColumn(cols, {"subgrade_level", "subgrade", "subgrade_ft"}, &subgrade_col);
  ResolveColumn(cols, {"elevation_feet", "elevation", "elevation_ft"}, &elevation_col);
  ResolveColumn(cols, {"latitude", "lat"}, &lat_col);
  ResolveColumn(cols, {"longitude", "lon", "lng"}, &lon_col);

  std::string line;
  while (std::getline(input, line)) {
    if (line.empty()) {
      continue;
    }

    ++out_stats->rows_read;
    const auto fields = ParseCsvLine(line);

    BuildingRecord record;
    bool ok = true;

    if (bbl_col < fields.size()) {
      record.bbl = fields[bbl_col];
    }
    if (subgrade_col < fields.size() && !fields[subgrade_col].empty()) {
      ok = ParseInt32(fields[subgrade_col], &record.subgrade_level);
    }
    if (ok && elevation_col < fields.size() && !fields[elevation_col].empty()) {
      ok = ParseDouble(fields[elevation_col], &record.elevation_feet);
    }
    if (ok && lat_col < fields.size() && lon_col < fields.size() &&
        !fields[lat_col].empty() && !fields[lon_col].empty()) {
      ok = ParseDouble(fields[lat_col], &record.location.latitude) &&
           ParseDouble(fields[lon_col], &record.location.longitude);
    }

    if (!ok) {
      ++out_stats->rows_rejected;
      continue;
    }

    out_records->push_back(record);
    ++out_stats->rows_accepted;
  }

  return true;
}

}  // namespace urbandrop
