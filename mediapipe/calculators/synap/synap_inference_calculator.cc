// Copyright 2019 The MediaPipe Authors.
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


#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "mediapipe/calculators/synap/synap_inference_calculator.pb.h"
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/formats/tensor.h"
#include "mediapipe/framework/port/logging.h"
#include "mediapipe/framework/port/ret_check.h"
#include "mediapipe/util/synap/inc/synap/allocator.hpp"
#include "mediapipe/util/synap/inc/synap/network.hpp"
#include "mediapipe/util/synap/model.hpp"

namespace {
constexpr char kTensorsTag[] = "TENSORS";
}

// SynapInferenceCalculator File Layout:
//  * Header
//  * Core
//  * Aux
namespace mediapipe {

// Calculator Header Section

// Runs inference on the provided input tensors and NBG model.
//
// This calculator is designed to be used with the FloatsToTensors,
// to get the appropriate inputs.
//
// Input:
//  TENSORS - synaptics::synap:Tensors
//
// Output:
//  TENSORS - synaptics::synap::Tensors
//
// Example use:
// node {
//   calculator: "SynapInferenceCalculator"
//   input_stream: "TENSORS:tensor_image"
//   output_stream: "TENSORS:tensors"
//   options: {
//     [mediapipe.SynapInferenceCalculatorOptions.ext] {
//       model_path: "modelname.nbg"
//       metadata_path: "modelname.json"
//     }
//   }
// }
//
// IMPORTANT Notes:
//  Tensors are assumed to be ordered correctly (sequentially added to model).
//  Input tensors are assumed to be of the correct size and already normalized.
//  All output Tensors will be destroyed when the graph closes,
//  (i.e. after calling graph.WaitUntilDone()).
//  This calculator uses FixedSizeInputStreamHandler by default.
//

using namespace synaptics;
using synap::Allocator;
using synap::Network;

class SynapInferenceCalculator : public CalculatorBase {
 public:
  static absl::Status GetContract(CalculatorContract* cc);

  absl::Status Open(CalculatorContext* cc) override;
  absl::Status Process(CalculatorContext* cc) override;
  absl::Status Close(CalculatorContext* cc) override;

 private:
  absl::Status LoadModel(CalculatorContext* cc);
  absl::Status AllocateOutputs(CalculatorContext* cc);
  absl::Status ProcessInputs(CalculatorContext* cc);
  absl::Status ProcessOutputs(CalculatorContext* cc);

  Packet model_packet_;
  std::unique_ptr<Network> network_;
  Allocator* allocator_;
  std::vector<Tensor> output_tensors_;
};
REGISTER_CALCULATOR(SynapInferenceCalculator);

// Calculator Core Section

absl::Status SynapInferenceCalculator::GetContract(CalculatorContract* cc) {
  LOG(INFO) << "SynapInferenceCalculator::GetContract()";
  RET_CHECK(cc->Inputs().HasTag(kTensorsTag));
  RET_CHECK(cc->Outputs().HasTag(kTensorsTag));

  cc->InputSidePackets().Tag("MODEL_BLOB").Set<std::string>();
  cc->InputSidePackets().Tag("METADATA_BLOB").Set<std::string>();

  if (cc->Inputs().HasTag(kTensorsTag))
    cc->Inputs().Tag(kTensorsTag).Set<std::vector<Tensor>>();
  if (cc->Outputs().HasTag(kTensorsTag))
    cc->Outputs().Tag(kTensorsTag).Set<std::vector<Tensor>>();

  // Assign this calculator's default InputStreamHandler.
  cc->SetInputStreamHandler("FixedSizeInputStreamHandler");

  LOG(INFO) << "SynapInferenceCalculator::GetContract() => OK";
  return absl::OkStatus();
}

absl::Status SynapInferenceCalculator::Open(CalculatorContext* cc) {
  LOG(INFO) << "SynapInferenceCalculator::Open()";
  cc->SetOffset(TimestampDiff(0));

  const auto& options =
      cc->Options<::mediapipe::SynapInferenceCalculatorOptions>();

  allocator_ = options.use_cma() ? synap::cma_allocator() : synap::std_allocator();

  MP_RETURN_IF_ERROR(LoadModel(cc));
  MP_RETURN_IF_ERROR(AllocateOutputs(cc));

  LOG(INFO) << "SynapInferenceCalculator::Open() => OK";
  return absl::OkStatus();
}

absl::Status SynapInferenceCalculator::Process(CalculatorContext* cc) {
  LOG(INFO) << "SynapInferenceCalculator::Process()";
  // 1. Receive pre-processed tensor inputs.
  MP_RETURN_IF_ERROR(ProcessInputs(cc));

  // 2. Do inference
  RET_CHECK(network_->predict());

  // 3. Output processed tensors.
  MP_RETURN_IF_ERROR(ProcessOutputs(cc));

  LOG(INFO) << "SynapInferenceCalculator::Process() => OK";
  return absl::OkStatus();
}

absl::Status SynapInferenceCalculator::Close(CalculatorContext* cc) {
  LOG(INFO) << "SynapInferenceCalculator::Close()";
  LOG(INFO) << "SynapInferenceCalculator::Close() => OK";
  return absl::OkStatus();
}

// Calculator Auxiliary Section

absl::Status SynapInferenceCalculator::ProcessInputs(CalculatorContext* cc) {
  LOG(INFO) << "SynapInferenceCalculator::ProcessInputs()";
  if (cc->Inputs().Tag(kTensorsTag).IsEmpty()) {
  LOG(INFO) << "SynapInferenceCalculator::ProcessInputs() => " << kTensorsTag << " is empty; nothing to do";
  LOG(INFO) << "SynapInferenceCalculator::ProcessInputs() => OK";
    return absl::OkStatus();
  }
  // Read input into tensors.
  const auto& input_tensors =
      cc->Inputs().Tag(kTensorsTag).Get<std::vector<Tensor>>();
  RET_CHECK_EQ(input_tensors.size(), network_->inputs.size());
  for (int i = 0; i < input_tensors.size(); ++i) {
    auto view = input_tensors[i].GetCpuReadView();
    auto raw_floats = view.buffer<float>();
    int num_values = input_tensors[i].shape().num_elements();
    auto vec_bytes = std::vector<uint8_t>(raw_floats, raw_floats + num_values);
    RET_CHECK(network_->inputs[i].assign(std::move(vec_bytes)));
  }

  LOG(INFO) << "SynapInferenceCalculator::ProcessInputs() => OK";
  return absl::OkStatus();
}

absl::Status SynapInferenceCalculator::ProcessOutputs(CalculatorContext* cc) {
  LOG(INFO) << "SynapInferenceCalculator::ProcessOutputs()";
  auto out = absl::make_unique<std::vector<Tensor>>();
  for (synap::Tensor& tensor : network_->outputs) {
    Tensor t(Tensor::ElementType::kFloat32, Tensor::Shape(tensor.shape()));
    float* f = t.GetCpuWriteView().buffer<float>();
    memcpy(f, tensor.as_float(), tensor.size());
    out->push_back(std::move(t));
  }

  cc->Outputs().Tag(kTensorsTag).Add(out.release(), cc->InputTimestamp());

  LOG(INFO) << "SynapInferenceCalculator::ProcessOutputs() => OK";
  return absl::OkStatus();
}

absl::Status SynapInferenceCalculator::LoadModel(CalculatorContext* cc) {
  LOG(INFO) << "SynapInferenceCalculator::LoadModel()";
  const auto& options =
      cc->Options<::mediapipe::SynapInferenceCalculatorOptions>();

  LOG(INFO) << "SynapInferenceCalculator::LoadModel() => get input...";

  const Packet& model_packet = cc->InputSidePackets().Tag("MODEL_BLOB");
  const Packet& metadata_packet = cc->InputSidePackets().Tag("METADATA_BLOB");
  const std::string& model_blob = model_packet.Get<std::string>();
  const std::string& metadata_blob = metadata_packet.Get<std::string>();

  LOG(INFO) << "SynapInferenceCalculator::LoadModel() => get input done";

  LOG(INFO) << "SynapInferenceCalculator::LoadModel() => checking model/metadata...";

  RET_CHECK(model_blob.data());
  RET_CHECK(metadata_blob.c_str());

  LOG(INFO) << "SynapInferenceCalculator::LoadModel() => checking model/metadata done";

  network_ = absl::make_unique<Network>();

  LOG(INFO) << "SynapInferenceCalculator::LoadModel() => network_->load_model()...";
  RET_CHECK(network_->load_model(model_blob.data(), model_blob.size(), metadata_blob.c_str()));
  LOG(INFO) << "SynapInferenceCalculator::LoadModel() => network_->load_model() done";

  LOG(INFO) << "SynapInferenceCalculator::LoadModel() => OK";
  return absl::OkStatus();
}

absl::Status SynapInferenceCalculator::AllocateOutputs(CalculatorContext* cc) {
  LOG(INFO) << "SynapInferenceCalculator::AllocateOutputs()";
  for (synap::Tensor& tensor : network_->outputs) {
      tensor.buffer()->set_allocator(allocator_);
  }
  LOG(INFO) << "SynapInferenceCalculator::AllocateOutputs() => OK";
  return absl::OkStatus();
}

}  // namespace mediapipe
