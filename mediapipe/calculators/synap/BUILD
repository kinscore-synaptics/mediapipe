# Copyright 2019 The MediaPipe Authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

load("//mediapipe/framework/port:build_config.bzl", "mediapipe_proto_library")
load("@bazel_skylib//lib:selects.bzl", "selects")

licenses(["notice"])

package(default_visibility = ["//visibility:private"])

mediapipe_proto_library(
    name = "synap_inference_calculator_proto",
    srcs = ["synap_inference_calculator.proto"],
    visibility = ["//visibility:public"],
    deps = [
        "//mediapipe/framework:calculator_options_proto",
        "//mediapipe/framework:calculator_proto",
    ],
)

cc_library(
    name = "synap_inference_calculator",
    srcs = ["synap_inference_calculator.cc"],
    copts = select({
        "//mediapipe:ios": [
            "-x objective-c++",
            "-fobjc-arc",  # enable reference-counting
        ],
        "//conditions:default": [],
    }),
    linkopts = select({
        "//mediapipe:ios": [
            "-framework CoreVideo",
            "-framework MetalKit",
        ],
        "//conditions:default": [],
    }),
    visibility = ["//visibility:public"],
    deps = [
        ":synap_inference_calculator_cc_proto",
        "//mediapipe/framework:calculator_framework",
        "//mediapipe/framework/formats:tensor",
        "//mediapipe/framework/stream_handler:fixed_size_input_stream_handler",
        "//mediapipe/framework/port:ret_check",
        "//mediapipe/util/synap:synap",
    ] + select({
        "//conditions:default": [],
        "//mediapipe:ios": [
            "//mediapipe/objc:mediapipe_framework_ios",
        ],
    }) + select({
        "//conditions:default": [],
        "//mediapipe:android": [
            "//mediapipe/util/android/file/base",
        ],
    }) + select({
        "//conditions:default": [
            "//mediapipe/util:cpu_util",
        ],
    }),
    alwayslink = 1,
)

cc_test(
    name = "synap_inference_calculator_test",
    srcs = [
        "synap_inference_calculator_test.cc",
    ],
    data = ["testdata/add.bin"],
    linkstatic = 1,
    deps = [
        ":synap_inference_calculator",
        ":synap_inference_calculator_cc_proto",
        "//mediapipe/calculators/core:constant_side_packet_calculator",
        "//mediapipe/calculators/util:local_file_contents_calculator",
        "//mediapipe/framework:calculator_framework",
        "//mediapipe/framework:calculator_runner",
        "//mediapipe/framework/deps:file_path",
        "//mediapipe/framework/port:gtest_main",
        "//mediapipe/framework/port:integral_types",
        "//mediapipe/framework/port:parse_text_proto",
        "//mediapipe/framework/tool:validate_type",
    ],
)
