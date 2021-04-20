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

licenses(["notice"])

package(default_visibility = [
    "//mediapipe:__subpackages__",
])

cc_library(
    name = "synap",
    srcs = glob(["lib/**"]),
    hdrs = glob(["inc/synap/*.hpp", "*.hpp"]),
    alwayslink = 1,
)

cc_library(
    name = "import",
    deps = [
        ":libvipbroker.so",
        ":libsynapnb.so",
    ],
)

cc_import(
    name = "libvipbroker.so",
    shared_library = "lib/armeabi-v7a/libVIPBroker.so",
    alwayslink = 1,
)

cc_import(
    name = "libsynapnb.so",
    shared_library = "lib/armeabi-v7a/libsynapnb.so",
    alwayslink = 1,
)
