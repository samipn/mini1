#pragma once

#include <cstdint>
#include <string>

#include "data_model/GeoPoint.hpp"

namespace urbandrop {

struct GarageRecord {
  std::string license_number;
  std::int64_t license_expiration_epoch_seconds = 0;
  std::int64_t license_creation_epoch_seconds = 0;
  std::string business_name;
  std::string address_building;
  std::string address_street_name;
  std::string address_street_name_2;
  std::string address_city;
  std::string address_state;
  std::string address_zip;
  std::string contact_phone;
  std::string borough;
  std::int16_t borough_code = -1;
  std::string detail;
  std::string nta;
  std::string community_board;
  std::string council_district;
  std::string bin;
  std::string bbl;
  std::string census_tract;
  GeoPoint location;
  bool has_location = false;
};

}  // namespace urbandrop
