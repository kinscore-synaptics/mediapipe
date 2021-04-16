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
#include "mediapipe/framework/port/ret_check.h"
#include "mediapipe/util/synap/inc/synap/allocator.hpp"
#include "mediapipe/util/synap/inc/synap/network.hpp"

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
  RET_CHECK(cc->Inputs().HasTag(kTensorsTag));
  RET_CHECK(cc->Outputs().HasTag(kTensorsTag));

  const auto& options =
      cc->Options<::mediapipe::SynapInferenceCalculatorOptions>();
  RET_CHECK(!options.model_path().empty() ||
            !options.metadata_path().empty())
      << "Model path and Metadata path in options are required.";

  if (cc->Inputs().HasTag(kTensorsTag))
    cc->Inputs().Tag(kTensorsTag).Set<std::vector<Tensor>>();
  if (cc->Outputs().HasTag(kTensorsTag))
    cc->Outputs().Tag(kTensorsTag).Set<std::vector<Tensor>>();

  // Assign this calculator's default InputStreamHandler.
  cc->SetInputStreamHandler("FixedSizeInputStreamHandler");

  return absl::OkStatus();
}

absl::Status SynapInferenceCalculator::Open(CalculatorContext* cc) {
  cc->SetOffset(TimestampDiff(0));

  const auto& options =
      cc->Options<::mediapipe::SynapInferenceCalculatorOptions>();

  network_ = absl::make_unique<Network>();
  allocator_ = options.use_cma() ? synap::cma_allocator() : synap::std_allocator();

  MP_RETURN_IF_ERROR(LoadModel(cc));
  MP_RETURN_IF_ERROR(AllocateOutputs(cc));

  return absl::OkStatus();
}

absl::Status SynapInferenceCalculator::Process(CalculatorContext* cc) {
  // 1. Receive pre-processed tensor inputs.
  MP_RETURN_IF_ERROR(ProcessInputs(cc));

  // 2. Do inference
  RET_CHECK(network_->predict());

  // 3. Output processed tensors.
  MP_RETURN_IF_ERROR(ProcessOutputs(cc));

  return absl::OkStatus();
}

absl::Status SynapInferenceCalculator::Close(CalculatorContext* cc) {

  return absl::OkStatus();
}

// Calculator Auxiliary Section

absl::Status SynapInferenceCalculator::ProcessInputs(CalculatorContext* cc) {
  if (cc->Inputs().Tag(kTensorsTag).IsEmpty()) {
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

  return absl::OkStatus();
}

absl::Status SynapInferenceCalculator::ProcessOutputs(CalculatorContext* cc) {
  auto out = absl::make_unique<std::vector<Tensor>>();
  for (synap::Tensor& tensor : network_->outputs) {
    Tensor t(Tensor::ElementType::kFloat32, Tensor::Shape(tensor.shape()));
    float* f = t.GetCpuWriteView().buffer<float>();
    memcpy(f, tensor.as_float(), tensor.size());
    out->push_back(std::move(t));
  }

  cc->Outputs().Tag(kTensorsTag).Add(out.release(), cc->InputTimestamp());

  return absl::OkStatus();
}

absl::Status SynapInferenceCalculator::LoadModel(CalculatorContext* cc) {
  const auto& options =
      cc->Options<::mediapipe::SynapInferenceCalculatorOptions>();

  RET_CHECK(network_->load_model(options.model_path(), options.metadata_path()));

  return absl::OkStatus();
}

absl::Status SynapInferenceCalculator::AllocateOutputs(CalculatorContext* cc) {
  for (synap::Tensor& tensor : network_->outputs) {
      tensor.buffer()->set_allocator(allocator_);
  }
}

}  // namespace mediapipe
