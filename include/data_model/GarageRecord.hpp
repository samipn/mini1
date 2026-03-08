#pragma once

#include <string>

#include "data_model/GeoPoint.hpp"

namespace urbandrop {

struct GarageRecord {
  std::string license_number;
  std::string name;
  std::string borough;
  std::string address;
  GeoPoint location;
};

}  // namespace urbandrop
