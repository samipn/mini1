#pragma once

#include <cstdint>
#include <string>

namespace urbandrop {

struct TrafficRecord {
  std::int64_t link_id = -1;
  double speed_mph = 0.0;
  double travel_time_seconds = 0.0;
  std::int64_t timestamp_epoch_seconds = 0;
  std::string borough;
  std::int16_t borough_code = -1;
  std::string link_name;

  bool HasValidCoreFields() const;
};

}  // namespace urbandrop
