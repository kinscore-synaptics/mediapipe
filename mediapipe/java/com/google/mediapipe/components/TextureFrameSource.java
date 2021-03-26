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

package com.google.mediapipe.components;

import android.graphics.SurfaceTexture;
import android.os.Bundle;
import android.util.Log;
import android.util.Size;

public interface TextureFrameSource extends TextureFrameProducer {
  default void checkAndRequestPermissions() {}
  default void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults)
  {
      PermissionHelper.onRequestPermissionsResult(requestCode, permissions, grantResults);
  }
  void start();
  void stop();
  Size computeDisplaySizeFromViewSize(Size viewSize);
  boolean isRotated();
  void attach(SurfaceTexture surfaceTexture, int width, int height);
}
