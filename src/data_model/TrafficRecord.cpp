#include "data_model/TrafficRecord.hpp"

namespace urbandrop {

bool TrafficRecord::HasValidCoreFields() const {
  return link_id >= 0 && speed_mph >= 0.0 && travel_time_seconds >= 0.0 &&
         timestamp_epoch_seconds > 0;
}

}  // namespace urbandrop
