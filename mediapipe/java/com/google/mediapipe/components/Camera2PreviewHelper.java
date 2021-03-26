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

import android.app.Activity;
import android.content.Context;
import android.graphics.SurfaceTexture;
import android.hardware.camera2.*;
import android.hardware.camera2.params.StreamConfigurationMap;
import android.media.ImageReader;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Log;
import android.util.Size;
import android.view.Surface;
import com.google.mediapipe.glutil.DetachedSurfaceTexture;
import java.util.Arrays;
import javax.annotation.Nonnull;
import javax.annotation.Nullable;

/**
 * Uses Camera2 APIs for camera setup and access.
 *
 * <p>{@link Camera2} connects to the camera and provides video frames.
 */
public class Camera2PreviewHelper extends CameraHelper {
  private static final String TAG = "Camera2PreviewHelper";

  private String cameraId;
  private CameraCharacteristics cameraCharacteristics;
  private CameraDevice cameraDevice;
  private CameraCaptureSession captureSession;

  private CaptureRequest.Builder captureRequestBuilder;
  private Size imageDimensions;
  private ImageReader imageReader;

  private Handler backgroundHandler;
  private HandlerThread backgroundThread;
  private SurfaceTexture outputSurface;
  private Size frameSize;
  private int frameRotation;
  private CameraHelper.CameraFacing cameraFacing;
  private Context context;

  private static Integer getLensFacing(CameraFacing cameraFacing) {
      switch (cameraFacing) {
          case FRONT: return CameraMetadata.LENS_FACING_FRONT;
          case BACK: return CameraMetadata.LENS_FACING_BACK;
          case EXTERNAL: return CameraMetadata.LENS_FACING_EXTERNAL;
      }
      return null;
  }

  public Camera2PreviewHelper(Context context) {
      this(context, null);
  }

  public Camera2PreviewHelper(Context context, SurfaceTexture surfaceTexture) {
      this.context = context;
      this.outputSurface = surfaceTexture;
      //frameRotation = getWindowManager().getDefaultDisplay().getRotation();
  }

  @Override
  public void startCamera(Activity activity, CameraFacing cameraFacing, @Nullable SurfaceTexture surfaceTexture) {
      this.cameraFacing = cameraFacing;
      closeCamera();
      startBackgroundThread();
      openCamera();
  }

  @Override
  public Size computeDisplaySizeFromViewSize(Size viewSize) {
      if (viewSize == null || frameSize == null) {
          return null;
      }

      float frameAspectRatio =
          frameRotation == 90 || frameRotation == 270 ?
            frameSize.getHeight() / (float) frameSize.getWidth() :
            frameSize.getWidth() / (float) frameSize.getHeight();

      float viewAspectRatio = viewSize.getWidth() / (float) viewSize.getHeight();

      int scaledWidth;
      int scaledHeight;
      if (frameAspectRatio < viewAspectRatio) {
          scaledWidth = viewSize.getWidth();
          scaledHeight = Math.round(viewSize.getWidth() / frameAspectRatio);
      } else {
          scaledHeight = viewSize.getHeight();
          scaledWidth = Math.round(viewSize.getHeight() * frameAspectRatio);
      }

      return new Size(scaledWidth, scaledHeight);
  }

  @Override
  public boolean isCameraRotated() {
      Integer cameraOrientation = cameraCharacteristics.get(CameraCharacteristics.SENSOR_ORIENTATION);
      return cameraOrientation != 90 && cameraOrientation != 270;
  }

  /*
  final CameraCaptureSession.CaptureCallback captureCallbackListener = new CameraCaptureSession.CaptureCallback() {
      (CameraCaptureSession session, CaptureRequest request, TotalCaptureResult result) -> {
          super.onCaptureCompleted(session, request, result);
          createCameraPreview();
      };
      */

  private void closeCamera() {
      try {
          stopBackgroundThread();
          if (cameraDevice != null) {
              cameraDevice.close();
              cameraDevice = null;
          }
          if (imageReader != null) {
              imageReader.close();
              imageReader = null;
          }
      } catch (Exception e) {
          Log.e(TAG, e.toString());
      }
  }

  private void openCamera() {
      CameraManager manager = (CameraManager) context.getSystemService(Context.CAMERA_SERVICE);
      try {
          if (cameraId == null) {
              final Integer lensFacing = getLensFacing(cameraFacing);
              final String[] cameraIdList = manager.getCameraIdList();
              for (String id : cameraIdList) {
                  cameraId = id;
                  Log.d(TAG, "Camera ID: " + cameraId);
                  cameraCharacteristics = manager.getCameraCharacteristics(id);
                  if (lensFacing == null || lensFacing == cameraCharacteristics.get(CameraCharacteristics.LENS_FACING)) {
                      break;
                  }
              }
          }
          Log.d(TAG, "Camera ID: " + cameraId);
          final StreamConfigurationMap map = cameraCharacteristics.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
          imageDimensions = map.getOutputSizes(SurfaceTexture.class)[0];
          manager.openCamera(cameraId, stateCallback, null);
      } catch (CameraAccessException e) {
          Log.e(TAG, e.toString());
      }
  }

  private final CameraDevice.StateCallback stateCallback = new CameraDevice.StateCallback() {
      @Override
      public void onOpened(CameraDevice camera) {
          cameraDevice = camera;
          createCameraPreview();
          if (onCameraStartedListener != null) {
              onCameraStartedListener.onCameraStarted(outputSurface);
          }
      }

      @Override
      public void onDisconnected(CameraDevice camera) {
          cameraDevice.close();
      }

      @Override
      public void onError(CameraDevice camera, int error) {
          try {
              Log.e(TAG, "CameraDevice error: " + error);
              cameraDevice.close();
              cameraDevice = null;
          } catch (Exception e) {
              Log.e(TAG, e.toString());
          }
      }
  };

  protected void createCameraPreview() {
      try {
          if (outputSurface == null) {
              outputSurface = new DetachedSurfaceTexture(0);
          }
          outputSurface.setDefaultBufferSize(imageDimensions.getWidth(), imageDimensions.getHeight());
          frameSize = imageDimensions;
          Surface surface = new Surface(outputSurface);
          captureRequestBuilder = cameraDevice.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW);
          captureRequestBuilder.addTarget(surface);
          cameraDevice.createCaptureSession(Arrays.asList(surface), new CameraCaptureSession.StateCallback() {
              @Override
              public void onConfigured(@Nonnull CameraCaptureSession cameraCaptureSession) {
                  if (cameraDevice == null) {
                      return;
                  }

                  captureSession = cameraCaptureSession;
                  updatePreview();
              }

              @Override
              public void onConfigureFailed(@Nonnull CameraCaptureSession cameraCaptureSession) {
                  Log.e(TAG, "Camera configuration failed");
              }
          }, null);
      } catch (CameraAccessException e) {
          Log.e(TAG, e.toString());
      }
  }

  protected void updatePreview() {
      if (cameraDevice == null) {
          Log.e(TAG, "updatePreview() called with no cameraDevice");
      }
      captureRequestBuilder.set(CaptureRequest.CONTROL_MODE, CameraMetadata.CONTROL_MODE_AUTO);
      try {
          captureSession.setRepeatingRequest(captureRequestBuilder.build(), null, backgroundHandler);
      } catch (CameraAccessException e) {
          Log.e(TAG, e.toString());
      }
  }

  protected void startBackgroundThread() {
      backgroundThread = new HandlerThread("Camera Background");
      backgroundThread.start();
      backgroundHandler = new Handler(backgroundThread.getLooper());
  }

  protected void stopBackgroundThread() {
      backgroundThread.quitSafely();
      try {
          backgroundThread.join();
          backgroundThread = null;
          backgroundHandler = null;
      } catch (InterruptedException e) {
          Log.e(TAG, e.toString());
      }
  }
}
