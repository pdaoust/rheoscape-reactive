#pragma once

#include <string>

namespace rheo::mqtt {

  enum QoS {
    AtMostOnce = 0,
    AtLeastOnce = 1,
    ExactlyOnce = 2,
  };

  struct MessageIn {
    std::string topic;
    std::string payload;
    QoS qos;
  };

  struct MessageOut : MessageIn {
    bool persist;
  };

}