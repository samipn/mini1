#include "io/BuildingLoader.hpp"
#include "io/CsvParseUtils.hpp"

#include <fstream>
#include <string>
#include <utility>
#include <unordered_map>
#include <vector>

namespace urbandrop {

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

  const auto headers = io::ParseCsvLine(header_line);
  std::unordered_map<std::string, std::size_t> cols;
  for (std::size_t i = 0; i < headers.size(); ++i) {
    cols[io::ToLower(headers[i])] = i;
  }

  *out_stats = LoaderStats{};

  std::size_t bbl_col = static_cast<std::size_t>(-1);
  std::size_t subgrade_col = static_cast<std::size_t>(-1);
  std::size_t elevation_col = static_cast<std::size_t>(-1);
  std::size_t lat_col = static_cast<std::size_t>(-1);
  std::size_t lon_col = static_cast<std::size_t>(-1);

  io::ResolveColumn(cols, {"bbl"}, &bbl_col);
  io::ResolveColumn(cols, {"subgrade_level", "subgrade", "subgrade_ft"}, &subgrade_col);
  io::ResolveColumn(cols, {"elevation_feet", "elevation", "elevation_ft"}, &elevation_col);
  io::ResolveColumn(cols, {"latitude", "lat"}, &lat_col);
  io::ResolveColumn(cols, {"longitude", "lon", "lng"}, &lon_col);

  std::vector<BuildingRecord> records;

  std::string line;
  while (std::getline(input, line)) {
    if (line.empty()) {
      continue;
    }

    ++out_stats->rows_read;
    const auto fields = io::ParseCsvLine(line);

    BuildingRecord record;
    bool ok = true;

    if (bbl_col < fields.size()) {
      record.bbl = fields[bbl_col];
    }
    if (subgrade_col < fields.size() && !fields[subgrade_col].empty()) {
      ok = io::ParseInt32(fields[subgrade_col], &record.subgrade_level);
    }
    if (ok && elevation_col < fields.size() && !fields[elevation_col].empty()) {
      ok = io::ParseDouble(fields[elevation_col], &record.elevation_feet);
    }
    if (ok && lat_col < fields.size() && lon_col < fields.size() &&
        !fields[lat_col].empty() && !fields[lon_col].empty()) {
      ok = io::ParseDouble(fields[lat_col], &record.location.latitude) &&
           io::ParseDouble(fields[lon_col], &record.location.longitude);
    }

    if (!ok) {
      ++out_stats->rows_rejected;
      continue;
    }

    records.push_back(record);
    ++out_stats->rows_accepted;
  }

  *out_records = std::move(records);
  return true;
}

}  // namespace urbandrop
