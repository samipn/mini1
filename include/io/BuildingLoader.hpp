#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "data_model/BuildingRecord.hpp"
#include "io/GarageLoader.hpp"

namespace urbandrop {

class BuildingLoader {
 public:
  static bool LoadCSV(const std::string& csv_path,
                      std::vector<BuildingRecord>* out_records,
                      LoaderStats* out_stats,
                      std::string* error);
};

}  // namespace urbandrop
