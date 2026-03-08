#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "data_model/GarageRecord.hpp"

namespace urbandrop {

struct LoaderStats {
  std::size_t rows_read = 0;
  std::size_t rows_accepted = 0;
  std::size_t rows_rejected = 0;
};

class GarageLoader {
 public:
  static bool LoadCSV(const std::string& csv_path,
                      std::vector<GarageRecord>* out_records,
                      LoaderStats* out_stats,
                      std::string* error);
};

}  // namespace urbandrop
