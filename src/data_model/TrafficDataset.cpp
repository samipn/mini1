#include "data_model/TrafficDataset.hpp"

namespace urbandrop {

void TrafficDataset::AddRecord(const TrafficRecord& record) {
  records_.push_back(record);
  ++counters_.rows_accepted;
}

void TrafficDataset::Reserve(std::size_t size) { records_.reserve(size); }

bool TrafficDataset::Empty() const { return records_.empty(); }

std::size_t TrafficDataset::Size() const { return records_.size(); }

const std::vector<TrafficRecord>& TrafficDataset::Records() const { return records_; }

IngestionCounters& TrafficDataset::Counters() { return counters_; }

const IngestionCounters& TrafficDataset::Counters() const { return counters_; }

}  // namespace urbandrop
