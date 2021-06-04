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
//
// A simple main function to run a MediaPipe graph.
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/port/file_helpers.h"
#include "mediapipe/framework/port/map_util.h"
#include "mediapipe/framework/port/parse_text_proto.h"
#include "mediapipe/framework/port/ret_check.h"
#include "mediapipe/framework/port/status.h"
#include "mediapipe/framework/port/statusor.h"

ABSL_FLAG(std::string, calculator_graph_config_file, "",
          "Name of file containing text format CalculatorGraphConfig proto.");

ABSL_FLAG(std::string, input_side_packets, "",
          "Comma-separated list of key=value pairs specifying side packets "
          "for the CalculatorGraph. All values will be treated as the "
          "string type even if they represent doubles, floats, etc.");

ABSL_FLAG(std::string, input_stream_files, "",
          "Comma-seperated list of input_stream_name=TYPE:file_path");

// Local file output flags.
// Output stream
ABSL_FLAG(std::string, output_stream_files, "",
          "Comma-seperated list of output_stream_name=file_path");
ABSL_FLAG(bool, strip_timestamps, false,
          "If true, only the packet contents (without timestamps) will be "
          "written into the local file.");
// Output side packets
ABSL_FLAG(std::string, output_side_packets, "",
          "A CSV of output side packets to output to local file.");
ABSL_FLAG(std::string, output_side_packets_file, "",
          "The name of the local file to output all side packets specified "
          "with --output_side_packets. ");

ABSL_FLAG(bool, wait_until_idle, false,
          "If true, graph will be stopped when it is idle; "
          "If false, graph will be stopped when it is done.");

template <typename T>
absl::StatusOr<std::vector<T>> GetInputData(std::string& file) {
    std::ifstream f(file, std::ios::binary);
    RET_CHECK(f.good());
    T k;
    std::vector<T> file_content;
    while (f.read(reinterpret_cast<char*>(&k), sizeof(T))) {
        file_content.emplace_back(k);
    }
    LOG(INFO) << "Read " << file_content.size() << " records from file '" << file << "'";
    return file_content;
}

absl::StatusOr<mediapipe::Packet> MakePacket(std::string& type, std::string& file) {
  if (type == "float") {
    ASSIGN_OR_RETURN(std::vector<float> data, GetInputData<float>(file));
    return mediapipe::MakePacket<std::vector<float>>(data);
  }
  RET_CHECK_FAIL() << "unsupported type: " << type;
}

absl::Status AddInputStreams(mediapipe::CalculatorGraph& graph) {
  if (!absl::GetFlag(FLAGS_input_stream_files).empty()) {
      LOG(INFO) << "Adding input streams.";
      std::vector<std::string> kv_pairs =
          absl::StrSplit(absl::GetFlag(FLAGS_input_stream_files), ',');
      for (const std::string& kv_pair : kv_pairs) {
        std::vector<std::string> name_and_value = absl::StrSplit(kv_pair, '=');
        RET_CHECK(name_and_value.size() == 2);
        std::string& name = name_and_value[0];
        std::string& value = name_and_value[1];
        std::vector<std::string> type_and_file = absl::StrSplit(value, ':');
        RET_CHECK(type_and_file.size() == 2);
        std::string& type = type_and_file[0];
        std::string& file = type_and_file[1];
        LOG(INFO) << "Adding input stream '" << name
                  << "' from file '" << file << "' type " << type;
        ASSIGN_OR_RETURN(mediapipe::Packet packet,
                MakePacket(type, file));
        graph.AddPacketToInputStream(name, packet.At(mediapipe::Timestamp(1)));
      }
      graph.CloseAllInputStreams();
  }
  return absl::OkStatus();
}

absl::Status OutputStreamToLocalFile(const std::string& file, const std::string& name, mediapipe::OutputStreamPoller *poller) {
  LOG(INFO) << "Logging stream '" << name << "' to file '" << file << "'";
  std::ofstream f;
  f.open(file);
  RET_CHECK(f.good());
  mediapipe::Packet packet;
  int i = 0;
  while (poller->Next(&packet)) {
    ++i;
    LOG(INFO) << "\tpacket #" << i << "...";
    std::string output_data;
    if (!absl::GetFlag(FLAGS_strip_timestamps)) {
      absl::StrAppend(&output_data, packet.Timestamp().Value(), ",");
      LOG(INFO) << "\t" << output_data;
    }
    //absl::StrAppend(&output_data, packet.Get<std::string>(), "\n");
    LOG(INFO) << "\t" << packet;
    f << output_data;
  }
  f.close();
  LOG(INFO) << "\tWrote " << i << " packets.";
  return absl::OkStatus();
}

absl::Status OutputSidePacketsToLocalFile(mediapipe::CalculatorGraph& graph) {
  if (!absl::GetFlag(FLAGS_output_side_packets).empty() &&
      !absl::GetFlag(FLAGS_output_side_packets_file).empty()) {
    LOG(INFO) << "Collecting output side packets.";
    std::ofstream file;
    file.open(absl::GetFlag(FLAGS_output_side_packets_file));
    std::vector<std::string> side_packet_names =
        absl::StrSplit(absl::GetFlag(FLAGS_output_side_packets), ',');
    for (const std::string& side_packet_name : side_packet_names) {
      ASSIGN_OR_RETURN(auto status_or_packet,
                       graph.GetOutputSidePacket(side_packet_name));
      file << absl::StrCat(side_packet_name, ":",
                           status_or_packet.Get<std::string>(), "\n");
    }
    file.close();
  } else {
    RET_CHECK(absl::GetFlag(FLAGS_output_side_packets).empty() &&
              absl::GetFlag(FLAGS_output_side_packets_file).empty())
        << "--output_side_packets and --output_side_packets_file should be "
           "specified in pair.";
  }
  return absl::OkStatus();
}

absl::Status RunMPPGraph() {
  std::string calculator_graph_config_file = absl::GetFlag(FLAGS_calculator_graph_config_file);
  RET_CHECK(!calculator_graph_config_file.empty());
  std::string calculator_graph_config_contents;
  MP_RETURN_IF_ERROR(mediapipe::file::GetContents(
      calculator_graph_config_file,
      &calculator_graph_config_contents));
  mediapipe::CalculatorGraphConfig config =
      mediapipe::ParseTextProtoOrDie<mediapipe::CalculatorGraphConfig>(
          calculator_graph_config_contents);
  std::map<std::string, mediapipe::Packet> input_side_packets;
  if (!absl::GetFlag(FLAGS_input_side_packets).empty()) {
    std::vector<std::string> kv_pairs =
        absl::StrSplit(absl::GetFlag(FLAGS_input_side_packets), ',');
    for (const std::string& kv_pair : kv_pairs) {
      std::vector<std::string> name_and_value = absl::StrSplit(kv_pair, '=');
      RET_CHECK(name_and_value.size() == 2);
      RET_CHECK(!mediapipe::ContainsKey(input_side_packets, name_and_value[0]));
      input_side_packets[name_and_value[0]] =
          mediapipe::MakePacket<std::string>(name_and_value[1]);
    }
  }
  LOG(INFO) << "Initializing the calculator graph...";
  mediapipe::CalculatorGraph graph;
  MP_RETURN_IF_ERROR(graph.Initialize(config, input_side_packets));
  if (!absl::GetFlag(FLAGS_output_stream_files).empty()) {
    LOG(INFO) << "Adding output stream poller(s).";
    std::map<std::string, std::pair<std::string, mediapipe::OutputStreamPoller*>> output_stream_pollers;
    std::vector<std::string> kv_pairs =
        absl::StrSplit(absl::GetFlag(FLAGS_output_stream_files), ',');
    for (const std::string& kv_pair : kv_pairs) {
      std::vector<std::string> name_and_value = absl::StrSplit(kv_pair, '=');
      RET_CHECK(name_and_value.size() == 2);
      std::string& name = name_and_value[0];
      std::string& file = name_and_value[1];
      LOG(INFO) << "    Stream '" << name << "' to file '" << file << "'";
      ASSIGN_OR_RETURN(auto poller, graph.AddOutputStreamPoller(
                                        name));
      RET_CHECK(!mediapipe::ContainsKey(output_stream_pollers, file));
      output_stream_pollers[file] = std::pair(name, &poller);
    }
    LOG(INFO) << "Start running the calculator graph.";
    MP_RETURN_IF_ERROR(graph.StartRun({}));
    MP_RETURN_IF_ERROR(AddInputStreams(graph));
    for (auto it = output_stream_pollers.begin(); it != output_stream_pollers.end(); ++it) {
      MP_RETURN_IF_ERROR(OutputStreamToLocalFile(it->first, it->second.first, it->second.second));
    }
  } else {
    LOG(INFO) << "No output streams polled.";
    LOG(INFO) << "Start running the calculator graph.";
    MP_RETURN_IF_ERROR(graph.StartRun({}));
    MP_RETURN_IF_ERROR(AddInputStreams(graph));
  }
  LOG(INFO) << "Waiting for graph to finish.";
  MP_RETURN_IF_ERROR(absl::GetFlag(FLAGS_wait_until_idle) ? graph.WaitUntilIdle() : graph.WaitUntilDone());
  LOG(INFO) << "Graph finished.";
  return OutputSidePacketsToLocalFile(graph);
}

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  LOG(INFO) << "Parsing command line.";
  absl::ParseCommandLine(argc, argv);
  LOG(INFO) << "Running graph.";
  absl::Status run_status = RunMPPGraph();
  if (!run_status.ok()) {
    LOG(ERROR) << "Failed to run the graph: " << run_status.message();
    return EXIT_FAILURE;
  } else {
    LOG(INFO) << "Success!";
  }
  return EXIT_SUCCESS;
}
