#pragma once

#include <cstddef>

#include "data_model/TrafficColumns.hpp"
#include "data_model/TrafficDataset.hpp"

namespace urbandrop {

class TrafficDatasetOptimized {
 public:
  void Reserve(std::size_t size);
  void Clear();
  void AddRecord(std::int64_t link_id,
                 double speed_mph,
                 double travel_time_seconds,
                 std::int64_t timestamp_epoch_seconds,
                 std::int16_t borough_code,
                 const std::string& link_name);

  bool Empty() const;
  std::size_t Size() const;

  const TrafficColumns& Columns() const;
  TrafficColumns& Columns();

  IngestionCounters& Counters();
  const IngestionCounters& Counters() const;

  static TrafficDatasetOptimized FromRowDataset(const TrafficDataset& row_dataset);

 private:
  TrafficColumns columns_;
  IngestionCounters counters_;
};

}  // namespace urbandrop
