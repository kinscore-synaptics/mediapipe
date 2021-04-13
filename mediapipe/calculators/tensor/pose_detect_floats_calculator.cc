#include "mediapipe/calculators/tensor/pose_detect_floats_calculator_options.pb.h"
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/port/ret_check.h"
#include "mediapipe/framework/port/status.h"
#include "mediapipe/framework/formats/tensor.h"

namespace mediapipe {

namespace {
auto& INPUT_1D = PoseDetectFloatsCalculatorOptions::INPUT_1D;
auto& INPUT_2D = PoseDetectFloatsCalculatorOptions::INPUT_2D;
}  // namespace

constexpr char kInFloats[] = "FLOATS";
constexpr char kOutTensors[] = "TENSORS";

// The calculator expects one input (a packet containing a vector<float> 
// containing pose detection inference results and generates one output 
// (a packet containing a Tensor containing the same data, but formatted
// for input to the TensorsToDetections calculator).
//
// Example config:
// node {
//   calculator: "PoseDetectFloatsCalculator"
//   input_stream: "vector_float_detections"
//   output_stream: "tensor_detections"
// }
class PoseDetectFloatsCalculator : public CalculatorBase {
 public:
  static absl::Status GetContract(CalculatorContract* cc);

  absl::Status Open(CalculatorContext* cc) override;
  absl::Status Process(CalculatorContext* cc) override;

 private:
  PoseDetectFloatsCalculatorOptions options_;
};
REGISTER_CALCULATOR(PoseDetectFloatsCalculator);

absl::Status PoseDetectFloatsCalculator::GetContract(
    CalculatorContract* cc) {
  const auto& options = cc->Options<PoseDetectFloatsCalculatorOptions>();
  // Start with only one input packet.
  RET_CHECK_EQ(cc->Inputs().NumEntries(), 1)
      << "Only one input stream is supported.";
  if (options.input_size() == INPUT_1D) {
    cc->Inputs().Tag(kInFloats).Set<std::vector<float>>(
        // Output vector<float>.
    );
  } else {
    LOG(FATAL) << "input size not supported";
  }
  RET_CHECK_EQ(cc->Outputs().NumEntries(), 1)
      << "Only one output stream is supported.";
  cc->Outputs().Tag(kOutTensors).Set<std::vector<Tensor>>(
      // Output stream with data as tf::Tensor and the same TimeSeriesHeader.
  );
  return absl::OkStatus();
}

absl::Status PoseDetectFloatsCalculator::Open(CalculatorContext* cc) {
  options_ = cc->Options<PoseDetectFloatsCalculatorOptions>();
  cc->SetOffset(0);
  return absl::OkStatus();
}

absl::Status PoseDetectFloatsCalculator::Process(CalculatorContext* cc) {
  if (options_.input_size() == INPUT_1D) {
    const std::vector<float>& input =
        cc->Inputs().Tag(kInFloats).Value().Get<std::vector<float>>();
    RET_CHECK_GE(input.size(), 1);
    auto output_tensors = absl::make_unique<std::vector<Tensor>>();
    const auto num_outputs = 2;
    output_tensors->reserve(num_outputs);
    // FIXME: add calculator options for hard-coded paramters
    for (int32 i = 0; i < num_outputs; i++) {
        if (i > 0) {
            auto length = 896 * sizeof(float);
            output_tensors->emplace_back(Tensor::ElementType::kFloat32, Tensor::Shape({1, 896, 1}));
            auto cpu_view = output_tensors->back().GetCpuWriteView();
            std::memcpy(cpu_view.buffer<float>(), input.data() + 896 * 12, length);
        } else {
            auto length = 896 * 12 * sizeof(float);
            output_tensors->emplace_back(Tensor::ElementType::kFloat32, Tensor::Shape({1, 896, 12}));
            auto cpu_view = output_tensors->back().GetCpuWriteView();
            std::memcpy(cpu_view.buffer<float>(), input.data(), length);
        }
    }

    cc->Outputs().Tag(kOutTensors).Add(output_tensors.release(), cc->InputTimestamp());
  } else {
    LOG(FATAL) << "input size not supported";
  }
  return absl::OkStatus();
}

}  // namespace mediapipe
