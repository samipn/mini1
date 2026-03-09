#include "data_model/TrafficDatasetOptimized.hpp"

namespace urbandrop {

void TrafficDatasetOptimized::Reserve(std::size_t size) {
  columns_.Reserve(size);
}

void TrafficDatasetOptimized::Clear() {
  columns_.Clear();
  counters_ = IngestionCounters{};
}

void TrafficDatasetOptimized::AddRecord(std::int64_t link_id,
                                        double speed_mph,
                                        double travel_time_seconds,
                                        std::int64_t timestamp_epoch_seconds,
                                        std::int16_t borough_code,
                                        const std::string& link_name) {
  columns_.AddRecord(link_id,
                     speed_mph,
                     travel_time_seconds,
                     timestamp_epoch_seconds,
                     borough_code,
                     link_name);
  ++counters_.rows_accepted;
}

bool TrafficDatasetOptimized::Empty() const {
  return columns_.Empty();
}

std::size_t TrafficDatasetOptimized::Size() const {
  return columns_.Size();
}

const TrafficColumns& TrafficDatasetOptimized::Columns() const {
  return columns_;
}

TrafficColumns& TrafficDatasetOptimized::Columns() {
  return columns_;
}

IngestionCounters& TrafficDatasetOptimized::Counters() {
  return counters_;
}

const IngestionCounters& TrafficDatasetOptimized::Counters() const {
  return counters_;
}

TrafficDatasetOptimized TrafficDatasetOptimized::FromRowDataset(const TrafficDataset& row_dataset) {
  TrafficDatasetOptimized optimized;
  optimized.Reserve(row_dataset.Size());
  optimized.counters_ = row_dataset.Counters();

  for (const auto& row : row_dataset.Records()) {
    optimized.columns_.AddRecord(row.link_id,
                                 row.speed_mph,
                                 row.travel_time_seconds,
                                 row.timestamp_epoch_seconds,
                                 row.borough_code,
                                 row.link_name);
  }

  return optimized;
}

}  // namespace urbandrop
