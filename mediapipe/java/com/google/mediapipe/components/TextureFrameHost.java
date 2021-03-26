package com.google.mediapipe.components;

import android.app.Activity;
import android.graphics.SurfaceTexture;
import javax.microedition.khronos.egl.EGLContext;

public interface TextureFrameHost {
    EGLContext getEglContext();
    Activity getActivity();
    void setSurfaceTexture(SurfaceTexture surfaceTexture);
}
