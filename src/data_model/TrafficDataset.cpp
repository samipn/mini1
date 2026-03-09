#include "data_model/TrafficDataset.hpp"

#include "data_model/CommonCodes.hpp"

namespace urbandrop {

void TrafficDataset::AddRecord(const TrafficRecord& record) {
  TrafficRecord normalized = record;
  // Keep borough text/code dual representation consistent for direct inserts.
  if (normalized.borough_code < 0 && !normalized.borough.empty()) {
    normalized.borough_code = ToInt(ParseBoroughCode(normalized.borough));
  }
  records_.push_back(normalized);
  ++counters_.rows_accepted;
}

void TrafficDataset::Reserve(std::size_t size) { records_.reserve(size); }

void TrafficDataset::Clear() {
  records_.clear();
  counters_ = IngestionCounters{};
}

bool TrafficDataset::Empty() const { return records_.empty(); }

std::size_t TrafficDataset::Size() const { return records_.size(); }

const std::vector<TrafficRecord>& TrafficDataset::Records() const { return records_; }

const TrafficRecord* TrafficDataset::At(std::size_t index) const {
  if (index >= records_.size()) {
    return nullptr;
  }
  return &records_[index];
}

std::vector<TrafficRecord>::const_iterator TrafficDataset::begin() const { return records_.begin(); }

std::vector<TrafficRecord>::const_iterator TrafficDataset::end() const { return records_.end(); }

IngestionCounters& TrafficDataset::Counters() { return counters_; }

const IngestionCounters& TrafficDataset::Counters() const { return counters_; }

}  // namespace urbandrop
