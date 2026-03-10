#pragma once

#include <cstddef>
#include <iosfwd>
#include <string>

#include "data_model/TrafficDataset.hpp"
#include "data_model/TrafficDatasetOptimized.hpp"

namespace urbandrop {

struct TrafficLoadOptions {
  std::size_t progress_every_rows = 500000;
  bool print_progress = true;
  // When > 1, the loader uses a parallel mmap-based strategy (Linux only).
  // 0 means "use the serial loader regardless".
  std::size_t num_threads = 0;
};

class CSVReader {
 public:
  static bool LoadTrafficCSV(const std::string& csv_path,
                             TrafficDataset* dataset,
                             const TrafficLoadOptions& options,
                             std::ostream* log_stream,
                             std::string* error);

  static bool LoadTrafficCSVOptimized(const std::string& csv_path,
                                      TrafficDatasetOptimized* dataset,
                                      const TrafficLoadOptions& options,
                                      std::ostream* log_stream,
                                      std::string* error);
};

}  // namespace urbandrop
