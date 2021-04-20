// Copyright 2020 The MediaPipe Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <functional>
#include <memory>
#include <string>

#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/packet.h"
#include "mediapipe/framework/port/ret_check.h"
#include "mediapipe/util/synap/inc/synap/network.hpp"
#include "mediapipe/util/synap/model.hpp"

namespace mediapipe {

// Loads Synap model from model blob specified as input side packet and outputs
// corresponding side packet.
//
// Input side packets:
//   MODEL_BLOB - Synap model blob/file-contents (std::string). You can read
//                model blob from file (using whatever APIs you have) and pass
//                it to the graph as input side packet or you can use some of
//                calculators like LocalFileContentsCalculator to get model
//                blob and use it as input here.
//
// Output side packets:
//   MODEL - Synap model. (std::unique_ptr<tflite::FlatBufferModel,
//           std::function<void(tflite::FlatBufferModel*)>>)
//
// Example use:
//
// node {
//   calculator: "SynapModelCalculator"
//   input_side_packet: "MODEL_BLOB:model_blob"
//   output_side_packet: "MODEL:model"
// }
//
class SynapModelCalculator : public CalculatorBase {
 public:

  static absl::Status GetContract(CalculatorContract* cc) {
    cc->InputSidePackets().Tag("MODEL_BLOB").Set<std::string>();
    cc->InputSidePackets().Tag("METADATA_BLOB").Set<std::string>();
    cc->OutputSidePackets().Tag("MODEL").Set<SynapModelPtr>();
    return absl::OkStatus();
  }

  absl::Status Open(CalculatorContext* cc) override {
    const Packet& model_packet = cc->InputSidePackets().Tag("MODEL_BLOB");
    const Packet& metadata_packet = cc->InputSidePackets().Tag("METADATA_BLOB");
    const std::string& model_blob = model_packet.Get<std::string>();
    const std::string& metadata_blob = metadata_packet.Get<std::string>();
    std::unique_ptr<Network> model = make_unique<Network>();

    RET_CHECK(model->load_model(model_blob.data(), model_blob.size(), metadata_blob.c_str()));

    cc->OutputSidePackets().Tag("MODEL").Set(
        MakePacket<SynapModelPtr>(SynapModelPtr(
            model.release(), [model_packet](Network* model) {
              // Keeping model_packet in order to keep underlying model blob
              // which can be released only after Synap model is not needed
              // anymore (deleted).
              delete model;
            })));

    return absl::OkStatus();
  }

  absl::Status Process(CalculatorContext* cc) override {
    return absl::OkStatus();
  }
};
REGISTER_CALCULATOR(SynapModelCalculator);

}  // namespace mediapipe
