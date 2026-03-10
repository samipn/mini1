#include "data_model/TrafficColumns.hpp"

namespace urbandrop {

void TrafficColumns::Reserve(std::size_t size) {
  link_ids_.reserve(size);
  speeds_mph_.reserve(size);
  travel_times_seconds_.reserve(size);
  timestamps_epoch_seconds_.reserve(size);
  borough_codes_.reserve(size);
  link_name_ids_.reserve(size);
}

void TrafficColumns::Clear() {
  link_ids_.clear();
  speeds_mph_.clear();
  travel_times_seconds_.clear();
  timestamps_epoch_seconds_.clear();
  borough_codes_.clear();
  link_name_ids_.clear();
  link_name_dictionary_.clear();
  link_name_to_id_.clear();
}

std::size_t TrafficColumns::Size() const {
  return link_ids_.size();
}

bool TrafficColumns::Empty() const {
  return link_ids_.empty();
}

void TrafficColumns::AddRecord(std::int64_t link_id,
                               double speed_mph,
                               double travel_time_seconds,
                               std::int64_t timestamp_epoch_seconds,
                               std::int16_t borough_code,
                               const std::string& link_name) {
  const std::int32_t link_name_id = InternLinkName(link_name);
  link_ids_.push_back(link_id);
  speeds_mph_.push_back(speed_mph);
  travel_times_seconds_.push_back(travel_time_seconds);
  timestamps_epoch_seconds_.push_back(timestamp_epoch_seconds);
  borough_codes_.push_back(borough_code);
  link_name_ids_.push_back(link_name_id);
}

const std::vector<std::int64_t>& TrafficColumns::LinkIds() const {
  return link_ids_;
}

const std::vector<double>& TrafficColumns::SpeedsMph() const {
  return speeds_mph_;
}

const std::vector<double>& TrafficColumns::TravelTimesSeconds() const {
  return travel_times_seconds_;
}

const std::vector<std::int64_t>& TrafficColumns::TimestampsEpochSeconds() const {
  return timestamps_epoch_seconds_;
}

const std::vector<std::int16_t>& TrafficColumns::BoroughCodes() const {
  return borough_codes_;
}

const std::vector<std::int32_t>& TrafficColumns::LinkNameIds() const {
  return link_name_ids_;
}

const std::vector<std::string>& TrafficColumns::LinkNameDictionary() const {
  return link_name_dictionary_;
}

std::int32_t TrafficColumns::InternLinkName(const std::string& value) {
  const auto it = link_name_to_id_.find(value);
  if (it != link_name_to_id_.end()) {
    return it->second;
  }

  const std::int32_t id = static_cast<std::int32_t>(link_name_dictionary_.size());
  auto [map_it, inserted] = link_name_to_id_.emplace(value, id);
  (void)inserted;
  try {
    link_name_dictionary_.push_back(value);
  } catch (...) {
    link_name_to_id_.erase(map_it);
    throw;
  }
  return id;
}

}  // namespace urbandrop
