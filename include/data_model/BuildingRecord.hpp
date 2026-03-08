#pragma once

#include <cstdint>
#include <string>

#include "data_model/GeoPoint.hpp"

namespace urbandrop {

struct BuildingRecord {
  std::string bbl;
  std::int32_t subgrade_level = 0;
  double elevation_feet = 0.0;
  GeoPoint location;
};

}  // namespace urbandrop
