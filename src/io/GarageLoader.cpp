#include "io/GarageLoader.hpp"

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

bool GarageLoader::LoadCSV(const std::string& csv_path,
                           std::vector<GarageRecord>* out_records,
                           LoaderStats* out_stats,
                           std::string* error) {
  if (out_records == nullptr || out_stats == nullptr) {
    if (error != nullptr) {
      *error = "garage loader output pointers cannot be null";
    }
    return false;
  }

  std::ifstream input(csv_path);
  if (!input.is_open()) {
    if (error != nullptr) {
      *error = "unable to open garages CSV: " + csv_path;
    }
    return false;
  }

  std::string header_line;
  if (!std::getline(input, header_line)) {
    if (error != nullptr) {
      *error = "garages CSV is empty: " + csv_path;
    }
    return false;
  }

  const auto headers = ParseCsvLine(header_line);
  std::unordered_map<std::string, std::size_t> cols;
  for (std::size_t i = 0; i < headers.size(); ++i) {
    cols[ToLower(headers[i])] = i;
  }

  std::size_t license_col = static_cast<std::size_t>(-1);
  std::size_t name_col = static_cast<std::size_t>(-1);
  std::size_t borough_col = static_cast<std::size_t>(-1);
  std::size_t address_col = static_cast<std::size_t>(-1);
  std::size_t lat_col = static_cast<std::size_t>(-1);
  std::size_t lon_col = static_cast<std::size_t>(-1);

  ResolveColumn(cols, {"license_number", "license number"}, &license_col);
  ResolveColumn(cols, {"business_name", "entity_name", "name"}, &name_col);
  ResolveColumn(cols, {"borough"}, &borough_col);
  ResolveColumn(cols, {"address", "street_address"}, &address_col);
  ResolveColumn(cols, {"latitude", "lat"}, &lat_col);
  ResolveColumn(cols, {"longitude", "lon", "lng"}, &lon_col);

  std::string line;
  while (std::getline(input, line)) {
    if (line.empty()) {
      continue;
    }

    ++out_stats->rows_read;
    const auto fields = ParseCsvLine(line);

    GarageRecord record;
    bool ok = true;

    if (license_col < fields.size()) {
      record.license_number = fields[license_col];
    }
    if (name_col < fields.size()) {
      record.name = fields[name_col];
    }
    if (borough_col < fields.size()) {
      record.borough = fields[borough_col];
    }
    if (address_col < fields.size()) {
      record.address = fields[address_col];
    }

    if (lat_col < fields.size() && lon_col < fields.size()) {
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
