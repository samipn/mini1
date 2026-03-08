#pragma once

#include <cstddef>
#include <vector>

#include "data_model/TrafficRecord.hpp"

namespace urbandrop {

struct IngestionCounters {
  std::size_t rows_read = 0;
  std::size_t rows_accepted = 0;
  std::size_t rows_rejected = 0;
  std::size_t malformed_rows = 0;
  std::size_t missing_timestamp = 0;
  std::size_t suspicious_speed = 0;
  std::size_t suspicious_travel_time = 0;
};

class TrafficDataset {
 public:
  void AddRecord(const TrafficRecord& record);
  void Reserve(std::size_t size);

  bool Empty() const;
  std::size_t Size() const;
  const std::vector<TrafficRecord>& Records() const;

  IngestionCounters& Counters();
  const IngestionCounters& Counters() const;

 private:
  std::vector<TrafficRecord> records_;
  IngestionCounters counters_;
};

}  // namespace urbandrop
