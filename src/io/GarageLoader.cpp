#include "io/GarageLoader.hpp"
#include "io/CsvParseUtils.hpp"

#include <fstream>
#include <string>
#include <utility>
#include <unordered_map>
#include <vector>

#include "data_model/CommonCodes.hpp"

namespace urbandrop {

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

  const auto headers = io::ParseCsvLine(header_line);
  std::unordered_map<std::string, std::size_t> cols;
  for (std::size_t i = 0; i < headers.size(); ++i) {
    cols[io::ToLower(headers[i])] = i;
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

  io::ResolveColumn(cols, {"license_nbr", "license_number", "license number"}, &license_col);
  io::ResolveColumn(cols, {"lic_expir_dd", "expiration_date"}, &expiration_col);
  io::ResolveColumn(cols, {"license_creation_date", "initial_issuance_date"}, &creation_col);
  io::ResolveColumn(cols, {"business_name", "entity_name", "name"}, &business_name_col);
  io::ResolveColumn(cols, {"address_building", "building_number"}, &address_building_col);
  io::ResolveColumn(cols, {"address_street_name", "street1"}, &address_street_name_col);
  io::ResolveColumn(cols, {"address_street_name_2", "street2"}, &address_street_name_2_col);
  io::ResolveColumn(cols, {"address_city", "city"}, &address_city_col);
  io::ResolveColumn(cols, {"address_state", "state"}, &address_state_col);
  io::ResolveColumn(cols, {"address_zip", "zip_code"}, &address_zip_col);
  io::ResolveColumn(cols, {"contact_phone"}, &contact_phone_col);
  io::ResolveColumn(cols, {"address_borough", "borough"}, &borough_col);
  io::ResolveColumn(cols, {"detail", "details"}, &detail_col);
  io::ResolveColumn(cols, {"nta"}, &nta_col);
  io::ResolveColumn(cols, {"community_board"}, &community_board_col);
  io::ResolveColumn(cols, {"council_district"}, &council_district_col);
  io::ResolveColumn(cols, {"bin"}, &bin_col);
  io::ResolveColumn(cols, {"bbl"}, &bbl_col);
  io::ResolveColumn(cols, {"census_tract", "census_tract_2010"}, &census_tract_col);
  io::ResolveColumn(cols, {"latitude", "lat"}, &lat_col);
  io::ResolveColumn(cols, {"longitude", "lon", "lng"}, &lon_col);

  std::vector<GarageRecord> records;

  std::string line;
  while (std::getline(input, line)) {
    if (line.empty()) {
      continue;
    }

    ++out_stats->rows_read;
    const auto fields = io::ParseCsvLine(line);

    GarageRecord record;
    bool ok = true;

    if (license_col < fields.size()) {
      record.license_number = fields[license_col];
    }
    if (expiration_col < fields.size() && !fields[expiration_col].empty()) {
      ok = io::ParseTimestamp(fields[expiration_col], &record.license_expiration_epoch_seconds);
    }
    if (ok && creation_col < fields.size() && !fields[creation_col].empty()) {
      ok = io::ParseTimestamp(fields[creation_col], &record.license_creation_epoch_seconds);
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
      if (io::ParseDouble(fields[lat_col], &lat) && io::ParseDouble(fields[lon_col], &lon)) {
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

    records.push_back(record);
    ++out_stats->rows_accepted;
  }

  *out_records = std::move(records);
  return true;
}

}  // namespace urbandrop
