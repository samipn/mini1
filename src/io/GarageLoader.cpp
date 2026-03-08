#include "io/GarageLoader.hpp"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <ctime>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

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

  *out_stats = LoaderStats{};

  std::size_t license_col = static_cast<std::size_t>(-1);
  std::size_t expiration_col = static_cast<std::size_t>(-1);
  std::size_t creation_col = static_cast<std::size_t>(-1);
  std::size_t business_name_col = static_cast<std::size_t>(-1);
  std::size_t address_building_col = static_cast<std::size_t>(-1);
  std::size_t address_street_name_col = static_cast<std::size_t>(-1);
  std::size_t address_street_name_2_col = static_cast<std::size_t>(-1);
  std::size_t address_city_col = static_cast<std::size_t>(-1);
  std::size_t address_state_col = static_cast<std::size_t>(-1);
  std::size_t address_zip_col = static_cast<std::size_t>(-1);
  std::size_t contact_phone_col = static_cast<std::size_t>(-1);
  std::size_t borough_col = static_cast<std::size_t>(-1);
  std::size_t detail_col = static_cast<std::size_t>(-1);
  std::size_t nta_col = static_cast<std::size_t>(-1);
  std::size_t community_board_col = static_cast<std::size_t>(-1);
  std::size_t council_district_col = static_cast<std::size_t>(-1);
  std::size_t bin_col = static_cast<std::size_t>(-1);
  std::size_t bbl_col = static_cast<std::size_t>(-1);
  std::size_t census_tract_col = static_cast<std::size_t>(-1);
  std::size_t lat_col = static_cast<std::size_t>(-1);
  std::size_t lon_col = static_cast<std::size_t>(-1);

  ResolveColumn(cols, {"license_nbr", "license_number", "license number"}, &license_col);
  ResolveColumn(cols, {"lic_expir_dd", "expiration_date"}, &expiration_col);
  ResolveColumn(cols, {"license_creation_date", "initial_issuance_date"}, &creation_col);
  ResolveColumn(cols, {"business_name", "entity_name", "name"}, &business_name_col);
  ResolveColumn(cols, {"address_building", "building_number"}, &address_building_col);
  ResolveColumn(cols, {"address_street_name", "street1"}, &address_street_name_col);
  ResolveColumn(cols, {"address_street_name_2", "street2"}, &address_street_name_2_col);
  ResolveColumn(cols, {"address_city", "city"}, &address_city_col);
  ResolveColumn(cols, {"address_state", "state"}, &address_state_col);
  ResolveColumn(cols, {"address_zip", "zip_code"}, &address_zip_col);
  ResolveColumn(cols, {"contact_phone"}, &contact_phone_col);
  ResolveColumn(cols, {"address_borough", "borough"}, &borough_col);
  ResolveColumn(cols, {"detail", "details"}, &detail_col);
  ResolveColumn(cols, {"nta"}, &nta_col);
  ResolveColumn(cols, {"community_board"}, &community_board_col);
  ResolveColumn(cols, {"council_district"}, &council_district_col);
  ResolveColumn(cols, {"bin"}, &bin_col);
  ResolveColumn(cols, {"bbl"}, &bbl_col);
  ResolveColumn(cols, {"census_tract", "census_tract_2010"}, &census_tract_col);
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
    if (expiration_col < fields.size() && !fields[expiration_col].empty()) {
      ok = ParseTimestamp(fields[expiration_col], &record.license_expiration_epoch_seconds);
    }
    if (ok && creation_col < fields.size() && !fields[creation_col].empty()) {
      ok = ParseTimestamp(fields[creation_col], &record.license_creation_epoch_seconds);
    }
    if (business_name_col < fields.size()) {
      record.business_name = fields[business_name_col];
    }
    if (address_building_col < fields.size()) {
      record.address_building = fields[address_building_col];
    }
    if (address_street_name_col < fields.size()) {
      record.address_street_name = fields[address_street_name_col];
    }
    if (address_street_name_2_col < fields.size()) {
      record.address_street_name_2 = fields[address_street_name_2_col];
    }
    if (address_city_col < fields.size()) {
      record.address_city = fields[address_city_col];
    }
    if (address_state_col < fields.size()) {
      record.address_state = fields[address_state_col];
    }
    if (address_zip_col < fields.size()) {
      record.address_zip = fields[address_zip_col];
    }
    if (contact_phone_col < fields.size()) {
      record.contact_phone = fields[contact_phone_col];
    }
    if (borough_col < fields.size()) {
      record.borough = fields[borough_col];
      record.borough_code = ToInt(ParseBoroughCode(record.borough));
    }
    if (detail_col < fields.size()) {
      record.detail = fields[detail_col];
    }
    if (nta_col < fields.size()) {
      record.nta = fields[nta_col];
    }
    if (community_board_col < fields.size()) {
      record.community_board = fields[community_board_col];
    }
    if (council_district_col < fields.size()) {
      record.council_district = fields[council_district_col];
    }
    if (bin_col < fields.size()) {
      record.bin = fields[bin_col];
    }
    if (bbl_col < fields.size()) {
      record.bbl = fields[bbl_col];
    }
    if (census_tract_col < fields.size()) {
      record.census_tract = fields[census_tract_col];
    }

    if (lat_col < fields.size() && lon_col < fields.size() && !fields[lat_col].empty() &&
        !fields[lon_col].empty()) {
      double lat = 0.0;
      double lon = 0.0;
      if (ParseDouble(fields[lat_col], &lat) && ParseDouble(fields[lon_col], &lon)) {
        record.location.latitude = lat;
        record.location.longitude = lon;
        record.has_location = true;
      }
    }

    if (record.license_number.empty() && record.business_name.empty()) {
      ok = false;
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
