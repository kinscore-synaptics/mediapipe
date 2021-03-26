package com.google.mediapipe.components;

import android.os.Bundle;
import android.util.Size;

public class Camera2TextureFrameSource extends ExternalTextureConverter {
    private TextureFrameHost host;
    private CameraHelper cameraHelper;
    private CameraHelper.CameraFacing cameraFacing;

    public Camera2TextureFrameSource(TextureFrameHost host) {
        super(host.getEglContext());
        this.host = host;
    }

    public Camera2TextureFrameSource(TextureFrameHost host, int numBuffers) {
        super(host.getEglContext(), numBuffers);
        this.host = host;
    }

    public static Camera2TextureFrameSource create(TextureFrameHost host, Bundle parameters) {
        Camera2TextureFrameSource c = parameters.containsKey("converterNumBuffers") ?
            new Camera2TextureFrameSource(host, parameters.getInt("converterNumBuffers")) :
            new Camera2TextureFrameSource(host);

        c.setParameters(parameters);

        return c;
    }

    @Override
    public void setParameters(Bundle parameters) {
        cameraFacing = parameters.containsKey("cameraFacingFront") ?
            (parameters.getBoolean("cameraFacingFront", false) ?
                CameraHelper.CameraFacing.FRONT :
                CameraHelper.CameraFacing.BACK) :
            CameraHelper.CameraFacing.ANY;

        super.setParameters(parameters);
    }

    @Override
    public void checkAndRequestPermissions() {
        PermissionHelper.checkAndRequestCameraPermissions(host.getActivity());
    }

    @Override
    public void start() {
        if (PermissionHelper.cameraPermissionsGranted(host.getActivity())) {
            cameraHelper = new Camera2PreviewHelper(host.getActivity());
            cameraHelper.setOnCameraStartedListener(
                    surfaceTexture -> {
                        host.setSurfaceTexture(surfaceTexture);
                    });
            cameraHelper.startCamera(host.getActivity(), cameraFacing, null);
        }
    }

    @Override
    public Size computeDisplaySizeFromViewSize(Size viewSize) {
        return cameraHelper.computeDisplaySizeFromViewSize(viewSize);
    }

    @Override
    public boolean isRotated() {
        return cameraHelper.isCameraRotated();
    }
}
