#include "mediapipe/calculators/tensor/pose_landmark_floats_calculator_options.pb.h"
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/port/ret_check.h"
#include "mediapipe/framework/port/status.h"
#include "mediapipe/framework/formats/tensor.h"

namespace mediapipe {

namespace {
auto& INPUT_1D = PoseLandmarkFloatsCalculatorOptions::INPUT_1D;
auto& INPUT_2D = PoseLandmarkFloatsCalculatorOptions::INPUT_2D;
}  // namespace

constexpr char kInFloats[] = "FLOATS";
constexpr char kOutTensors[] = "TENSORS";

// The calculator expects one input (a packet containing a vector<float> 
// containing pose landmark inference results and generates one output 
// (a packet containing a Tensor containing the same data, but formatted
// for input to the TensorsToLandmarks calculator).
//
// Example config:
// node {
//   calculator: "PoseLandmarkFloatsCalculator"
//   input_stream: "vector_float_landmarks"
//   output_stream: "tensor_landmarks"
// }
class PoseLandmarkFloatsCalculator : public CalculatorBase {
 public:
  static absl::Status GetContract(CalculatorContract* cc);

  absl::Status Open(CalculatorContext* cc) override;
  absl::Status Process(CalculatorContext* cc) override;

 private:
  PoseLandmarkFloatsCalculatorOptions options_;
};
REGISTER_CALCULATOR(PoseLandmarkFloatsCalculator);

absl::Status PoseLandmarkFloatsCalculator::GetContract(
    CalculatorContract* cc) {
  const auto& options = cc->Options<PoseLandmarkFloatsCalculatorOptions>();
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

absl::Status PoseLandmarkFloatsCalculator::Open(CalculatorContext* cc) {
  options_ = cc->Options<PoseLandmarkFloatsCalculatorOptions>();
  cc->SetOffset(0);
  return absl::OkStatus();
}

absl::Status PoseLandmarkFloatsCalculator::Process(CalculatorContext* cc) {
  if (options_.input_size() == INPUT_1D) {
    const std::vector<float>& input =
        cc->Inputs().Tag(kInFloats).Value().Get<std::vector<float>>();
    RET_CHECK_GE(input.size(), 1);
    auto output_tensors = absl::make_unique<std::vector<Tensor>>();
    output_tensors->reserve(1);
    const int32 length = 195 * sizeof(float);
    auto tensor_shape = Tensor::Shape({195});
    output_tensors->emplace_back(Tensor::ElementType::kFloat32, tensor_shape);
    auto cpu_view = output_tensors->back().GetCpuWriteView();
    std::memcpy(cpu_view.buffer<float>(), input.data() + 1, length);
    cc->Outputs().Tag(kOutTensors).Add(output_tensors.release(), cc->InputTimestamp());
  } else {
    LOG(FATAL) << "input size not supported";
  }
  return absl::OkStatus();
}

}  // namespace mediapipe
