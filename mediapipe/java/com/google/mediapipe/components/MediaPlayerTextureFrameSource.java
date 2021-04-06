package com.google.mediapipe.components;

import android.graphics.SurfaceTexture;
import android.Manifest.permission;
import android.media.MediaPlayer;
import android.net.Uri;
import android.os.Bundle;
import android.util.Size;
import android.view.Surface;
import com.google.mediapipe.glutil.DetachedSurfaceTexture;

public class MediaPlayerTextureFrameSource extends ExternalTextureConverter {
    private static final String[] permissions = new String[] { permission.READ_EXTERNAL_STORAGE };
    private TextureFrameHost host;
    private MediaPlayer player;
    private boolean loop;
    private String file;
    private SurfaceTexture outputSurface;

    public MediaPlayerTextureFrameSource(TextureFrameHost host) {
        super(host.getEglContext());
        this.host = host;
    }

    public MediaPlayerTextureFrameSource(TextureFrameHost host, int numBuffers) {
        super(host.getEglContext(), numBuffers);
        this.host = host;
    }

    public static MediaPlayerTextureFrameSource create(TextureFrameHost host, Bundle parameters) {
        MediaPlayerTextureFrameSource c = parameters.containsKey("converterNumBuffers") ?
            new MediaPlayerTextureFrameSource(host, parameters.getInt("converterNumBuffers")) :
            new MediaPlayerTextureFrameSource(host);

        c.setParameters(parameters);

        return c;
    }

    public void setParameters(Bundle parameters) {
        file = parameters.getString("mediaFile");
        loop = parameters.getBoolean("mediaLoop", false);
    }

    @Override
    public void checkAndRequestPermissions() {
        PermissionHelper.checkAndRequestPermissions(host.getActivity(), permissions);
    }

    @Override
    public void start() {
        if (PermissionHelper.permissionsGranted(host.getActivity(), permissions)) {
            if (outputSurface == null) {
                outputSurface = new DetachedSurfaceTexture(0);
            }
            if (player == null) {
                player = MediaPlayer.create(host.getActivity(), Uri.parse(file));
                player.setSurface(new Surface(outputSurface));
                player.setLooping(loop);
            }
            host.setSurfaceTexture(outputSurface);
            player.start();
        }
    }

    @Override
    public void stop() {
        if (player != null) {
            player.stop();
        }
        super.stop();
    }

    @Override
    public Size computeDisplaySizeFromViewSize(Size viewSize) {
        return viewSize;
    }

    @Override
    public boolean isRotated() {
        return false;
    }
}
