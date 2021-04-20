#include "mediapipe/util/synap/inc/synap/network.hpp"

namespace mediapipe {
  using synaptics::synap::Network;
  using SynapModelPtr =
      std::unique_ptr<Network,
                      std::function<void(Network*)>>;
}
