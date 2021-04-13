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

package com.google.mediapipe.apps.posetracking;

import android.os.Bundle;
import android.util.Log;
import com.google.mediapipe.formats.proto.LandmarkProto.NormalizedLandmark;
import com.google.mediapipe.formats.proto.LandmarkProto.NormalizedLandmarkList;
import com.google.mediapipe.framework.PacketGetter;
import com.google.protobuf.InvalidProtocolBufferException;
import java.io.File;
import java.io.FileWriter;
import java.util.List;

/** Main activity of MediaPipe pose tracking app. */
public class MainActivity extends com.google.mediapipe.apps.basic.MainActivity {
  private static final String TAG = "MainActivity";

  private static final String OUTPUT_LANDMARKS_STREAM_NAME = "pose_landmarks";

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);

    File logFile = new File("/sdcard/" + options.getString("binaryGraphName") + ".landmarks.txt");

    // To show verbose logging, run:
    // adb shell setprop log.tag.MainActivity VERBOSE
    if (Log.isLoggable(TAG, Log.VERBOSE)) {
      processor.addPacketCallback(
          OUTPUT_LANDMARKS_STREAM_NAME,
          (packet) -> {
            Log.v(TAG, "Received pose landmarks packet.");
            try {
              byte[] poseLandmarks =
                  PacketGetter.getProtoBytes(packet);

              FileWriter w = new FileWriter(logFile);
              for (byte b : poseLandmarks) {
                  w.append(b + "\n");
              }
              w.flush();
              w.close();

              Log.v(
                  TAG,
                  "[TS:"
                      + packet.getTimestamp()
                      + "] "
                      + getPoseLandmarksDebugString(poseLandmarks));
            } catch (Exception exception) {
              Log.e(TAG, "Failed to get proto.", exception);
            }
          });
    }
  }

  private static String getPoseLandmarksDebugString(byte[] poseLandmarks) {
    String poseLandmarkStr = "Pose landmarks: " + poseLandmarks.length + "\n";
    for (byte b : poseLandmarks) {
        poseLandmarkStr += b + " ";
    }
    return poseLandmarkStr;
  }

  private static String getPoseLandmarksDebugString(NormalizedLandmarkList poseLandmarks) {
    String poseLandmarkStr = "Pose landmarks: " + poseLandmarks.getLandmarkCount() + "\n";
    int landmarkIndex = 0;
    for (NormalizedLandmark landmark : poseLandmarks.getLandmarkList()) {
      poseLandmarkStr +=
          "\tLandmark ["
              + landmarkIndex
              + "]: ("
              + landmark.getX()
              + ", "
              + landmark.getY()
              + ", "
              + landmark.getZ()
              + ")\n";
      ++landmarkIndex;
    }
    return poseLandmarkStr;
  }
}
