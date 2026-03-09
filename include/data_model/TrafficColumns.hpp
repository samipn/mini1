#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace urbandrop {

class TrafficColumns {
 public:
  void Reserve(std::size_t size);
  void Clear();

  std::size_t Size() const;
  bool Empty() const;

  void AddRecord(std::int64_t link_id,
                 double speed_mph,
                 double travel_time_seconds,
                 std::int64_t timestamp_epoch_seconds,
                 std::int16_t borough_code,
                 const std::string& link_name);

  const std::vector<std::int64_t>& LinkIds() const;
  const std::vector<double>& SpeedsMph() const;
  const std::vector<double>& TravelTimesSeconds() const;
  const std::vector<std::int64_t>& TimestampsEpochSeconds() const;
  const std::vector<std::int16_t>& BoroughCodes() const;
  const std::vector<std::int32_t>& LinkNameIds() const;
  const std::vector<std::string>& LinkNameDictionary() const;

 private:
  std::int32_t InternLinkName(const std::string& value);

  std::vector<std::int64_t> link_ids_;
  std::vector<double> speeds_mph_;
  std::vector<double> travel_times_seconds_;
  std::vector<std::int64_t> timestamps_epoch_seconds_;
  std::vector<std::int16_t> borough_codes_;
  std::vector<std::int32_t> link_name_ids_;

  std::vector<std::string> link_name_dictionary_;
  std::unordered_map<std::string, std::int32_t> link_name_to_id_;
};

}  // namespace urbandrop
