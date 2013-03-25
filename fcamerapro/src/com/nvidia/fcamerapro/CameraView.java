/* Copyright (c) 2011-2012, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
package com.nvidia.fcamerapro;

import java.nio.ByteBuffer;

import java.nio.ByteOrder;
import java.nio.FloatBuffer;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import com.nvidia.fcamerapro.FCamInterface.PreviewParams;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.opengl.GLSurfaceView;
import android.opengl.GLUtils;
import android.os.Handler;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnTouchListener;

/**
 * OpenGL surface that displays camera preview stream. The rendering code runs
 * in a separate thread and for each frame queries the native code for an OpenGL
 * texture handler with current frame data. The frame is rendered as textured
 * quad with OpenGL ES 1.1 draw calls. Additionally, {@code CameraView} passes
 * touch events and their positions in normalized coordinates to the native code
 * to enable local touch to focus and touch to white balance functionality.
 */
public final class CameraView extends GLSurfaceView implements OnTouchListener {
    /**
     * Touch event modes.
     */
    public enum TouchModes {
        /**
         * Touch to auto focus mode
         */
        TOUCH_TO_FOCUS,
        /**
         * Touch to auto white balance mode
         */
        TOUCH_TO_WHITE_BALANCE
    };

    /**
     * Camera component aspect ratio (X/Y)
     */
    final private int mAspectRatioX, mAspectRatioY;
    final private boolean mScaleToWidth;

    /**
     * Current touch mode.
     */
    private TouchModes mTouchMode;

    /**
     * OpenGL renderer instance
     */
    final private CameraViewRenderer mRenderer;

    /**
     * UI thread handler. Needed for posting UI update messages directly from
     * the UI thread.
     */
    final private Handler mLongPressHandler = new Handler();

    /**
     * Called from UI thread after the user holds the finger on the preview
     * screen for more than 1000ms. Then a top-layer component is shown which
     * allows the user to select the touch mode.
     */
    final private Runnable mOnLongPress = new Runnable() {
        public void run() {
            // TODO: add ui for "touch to ..." selector
            System.out.println("long press!");
        }
    };

    /**
     * Camera view default constructor
     *
     * @param context
     *            parent component context
     * @param attrs
     *            component attribute
     */
    public CameraView(Context context, AttributeSet attrs) {
        super(context, attrs);
        setEGLConfigChooser(8, 8, 8, 8, 0, 0);

        // TODO: make the attributes type safe and constrained to only small set of values
        String str = attrs.getAttributeValue(null, "aspect_ratio");
        int x = 0, y = 0;
        if (str != null) {
            int sindex = str.lastIndexOf(':');
            if (sindex >= 0) {
                try {
                    x = Integer.parseInt(str.substring(0, sindex));
                    y = Integer.parseInt(str.substring(sindex + 1));
                }

                catch (NumberFormatException e) {
                }
            }
        }

        mAspectRatioX = x;
        mAspectRatioY = y;

        // TODO: make the attributes type safe and constrained to only small set of values
        str = attrs.getAttributeValue(null, "scale_to");
        mScaleToWidth = str != null && str.equals("width") ? true : false;

        mRenderer = new CameraViewRenderer(context);
        setRenderer(mRenderer);

        setOnTouchListener(this);
        mTouchMode = TouchModes.TOUCH_TO_FOCUS;
    }

    /**
     * Returns current OpenGL renderer instance.
     *
     * @return current OpenGL renderer instance
     */
    public CameraViewRenderer getRenderer() {
        return mRenderer;
    }

    /**
     * Sets current touch mode
     *
     * @param tm
     *            touch mode
     */
    public void setTouchMode(TouchModes tm) {
        mTouchMode = tm;
    }

    /**
     * Computes visible dimensions of a component.
     */
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        int height, width;

        if (mScaleToWidth) {
            width = MeasureSpec.getSize(widthMeasureSpec);
            if (mAspectRatioX != 0) {
                height = width * mAspectRatioY / mAspectRatioX;
            } else {
                height = MeasureSpec.getSize(heightMeasureSpec);
            }
        } else {
            height = MeasureSpec.getSize(heightMeasureSpec);
            if (mAspectRatioX != 0) {
                width = height * mAspectRatioX / mAspectRatioY;
            } else {
                width = MeasureSpec.getSize(widthMeasureSpec);
            }
        }

        // max width constraint
//        int mwidth = (int)(MeasureSpec.getSize(widthMeasureSpec) * 0.7f);
//        if (width > mwidth) {
//            height *= (float)mwidth / width;
//            width = mwidth;
//        }

        setMeasuredDimension(width, height);
    }

    /**
     * Called when component is attached to the parent. Starts FCam native
     * preview capture and registers this view renderer instance as a
     * {@link FCamInterface} event listener.
     */
    @Override
    protected void onAttachedToWindow() {
        if (!isInEditMode()) {
            FCamInterface instance = FCamInterface.GetInstance();
            // if picture grab was completed while in the viewer window, notify
            // the
            // renderer
            if (!instance.isCapturing()) {
                mRenderer.onCaptureComplete();
            }

            instance.setPreviewActive(true);
            instance.addEventListener(mRenderer);
        }

        super.onAttachedToWindow();
    }

    /**
     * Called when component is detached from the parent. Stops FCam native
     * preview capture and removes view renderer instance from
     * {@link FCamInterface} event listeners.
     */
    @Override
    protected void onDetachedFromWindow() {
        if (!isInEditMode()) {
            FCamInterface instance = FCamInterface.GetInstance();
            instance.setPreviewActive(false);
            instance.removeEventListener(mRenderer);
        }

        super.onDetachedFromWindow();
    }

    /**
     * Handles {@code onTouch()} events of the camera view.
     */
    public boolean onTouch(View v, MotionEvent event) {
        float x, y;

        switch (event.getActionMasked()) {
        case MotionEvent.ACTION_DOWN:
            x = event.getX() / getWidth();
            y = event.getY() / getHeight();
            if (mTouchMode == TouchModes.TOUCH_TO_FOCUS) {
                FCamInterface.GetInstance().touchToFocus(x, y);
            } else {
                FCamInterface.GetInstance().touchToWhiteBalance(x, y);
            }

            mLongPressHandler.postDelayed(mOnLongPress, 1000);
            return true;

        case MotionEvent.ACTION_UP:
            mLongPressHandler.removeCallbacks(mOnLongPress);
            return true;
        }

        return false;
    }
}

/**
 * Camera view renderer class. The rendering is done through
 * {@link android.opengl.GLSurfaceView} interface and runs in a separate thread.
 */
final class CameraViewRenderer implements GLSurfaceView.Renderer, FCamInterfaceEventListener {
    /**
     * Float buffers for vertex and texture coordinates
     */
    final private FloatBuffer mVertexBuffer, mTextureBuffer;
    final private Context mContext;

    /**
     * Specifies the animation start time of busy icon. If negative then busy
     * icon is not visible
     */
    private long mBusyTimerStart = -1;

    /**
     * Busy icon texture handler
     */
    private int mBusyTextureId = -1;

    /**
     * Surface dimensions
     */
    private int mSurfaceWidth, mSurfaceHeight;

    /**
     * Quad vertex coordinates
     */
    final static private float sVertexCoords[] = { 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f };
    /**
     * Quad texture coordinates
     */
    final static private float sTextureCoords[] = { 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f };

    /**
     * Default camera view renderer constructor
     *
     * @param context
     *            parent component context
     */
    CameraViewRenderer(Context context) {
        mContext = context;

        ByteBuffer byteBuf = ByteBuffer.allocateDirect(sVertexCoords.length * 4);
        byteBuf.order(ByteOrder.nativeOrder());
        mVertexBuffer = byteBuf.asFloatBuffer();
        mVertexBuffer.put(sVertexCoords);
        mVertexBuffer.position(0);

        byteBuf = ByteBuffer.allocateDirect(sTextureCoords.length * 4);
        byteBuf.order(ByteOrder.nativeOrder());
        mTextureBuffer = byteBuf.asFloatBuffer();
        mTextureBuffer.put(sTextureCoords);
        mTextureBuffer.position(0);
    }

    /**
     * Draws current frame data. Called continuously by {@link GLSurfaceView}.
     * The function draws a textured quad with current frame and in case the
     * capture is in progress it also draws a busy icon.
     */
    @Override
    public void onDrawFrame(GL10 gl) {
        FCamInterface iface = FCamInterface.GetInstance();

        if (iface.isPreviewActive()) {
            // lock frame texture
            int texId = iface.lockViewerTexture();
            if (texId >= 0) {
                int texTarget = iface.getViewerTextureTarget();

                // bind texture
                gl.glBindTexture(texTarget, texId);

                // draw textured quad
                gl.glEnableClientState(GL10.GL_VERTEX_ARRAY);
                gl.glEnableClientState(GL10.GL_TEXTURE_COORD_ARRAY);

                gl.glVertexPointer(2, GL10.GL_FLOAT, 0, mVertexBuffer);
                gl.glTexCoordPointer(2, GL10.GL_FLOAT, 0, mTextureBuffer);

                gl.glLoadIdentity();
                gl.glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
                gl.glEnable(texTarget);
                gl.glDrawArrays(GL10.GL_TRIANGLE_STRIP, 0, 4);
                gl.glDisable(texTarget);

                // unbind texture
                gl.glBindTexture(texTarget, 0);

                // draw busy animation
                if (mBusyTimerStart > -1) {
                    float aratio = (float) mSurfaceWidth / mSurfaceHeight;

                    gl.glEnable(GL10.GL_BLEND);
                    gl.glBlendFunc(GL10.GL_SRC_ALPHA, GL10.GL_ONE);

                    gl.glEnable(GL10.GL_TEXTURE_2D);
                    gl.glBindTexture(GL10.GL_TEXTURE_2D, mBusyTextureId);

                    gl.glTranslatef(0.5f, 0.5f, 0.0f);
                    gl.glScalef(Settings.BUSY_ICON_SCALE, Settings.BUSY_ICON_SCALE * aratio, 1.0f);
                    gl.glRotatef((System.currentTimeMillis() - mBusyTimerStart) * Settings.BUSY_ICON_SPEED, 0.0f, 0.0f, -1.0f);
                    gl.glTranslatef(-0.5f, -0.5f, 0.0f);

                    gl.glColor4f(1.0f, 1.0f, 1.0f, 0.5f);

                    gl.glDrawArrays(GL10.GL_TRIANGLE_STRIP, 0, 4);

                    gl.glDisable(GL10.GL_BLEND);
                }

                gl.glDisableClientState(GL10.GL_TEXTURE_COORD_ARRAY);
                gl.glDisableClientState(GL10.GL_VERTEX_ARRAY);

                // flush render calls
                gl.glFlush();
                gl.glFinish();

                // unlock frame texture
                iface.unlockViewerTexture();
            }
        }
    }

    /**
     * Loads and creates OpenGL texture from android resource.
     *
     * @param gl
     *            specifies GL context
     * @param resourceId
     *            points to a particular file resource
     * @return OpenGL texture id
     */
    private int loadTextureFromResource(GL10 gl, int resourceId) {
        // load the resource
        BitmapFactory.Options opts = new BitmapFactory.Options();
        opts.inScaled = false;
        Bitmap bitmap = BitmapFactory.decodeResource(mContext.getResources(), resourceId, opts);

        // create texture
        int[] tid = new int[1];
        gl.glGenTextures(1, tid, 0);
        gl.glBindTexture(GL10.GL_TEXTURE_2D, tid[0]);
        gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_MIN_FILTER, GL10.GL_LINEAR);
        gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_MAG_FILTER, GL10.GL_LINEAR);
        gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_WRAP_S, GL10.GL_CLAMP_TO_EDGE);
        gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_WRAP_T, GL10.GL_CLAMP_TO_EDGE);

        GLUtils.texImage2D(GL10.GL_TEXTURE_2D, 0, GL10.GL_RGBA, bitmap, 0);

        bitmap.recycle();

        return tid[0];
    }

    /**
     * Called when OpenGL surface size is changed.
     */
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        mSurfaceWidth = width;
        mSurfaceHeight = height;

        gl.glViewport(0, 0, width, height);

        gl.glMatrixMode(GL10.GL_PROJECTION);
        gl.glLoadIdentity();
        gl.glOrthof(0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f);

        gl.glMatrixMode(GL10.GL_MODELVIEW);
        gl.glLoadIdentity();
    }

    /**
     * Called when OpenGL surface is created.
     */
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        mBusyTextureId = loadTextureFromResource(gl, R.drawable.busy);
    }

    /**
     * Turns on the busy icon animation when
     * {@link FCamInterfaceEventListener#onCaptureStart()} event is received
     * from {@link FCamInterface}
     */
    public void onCaptureStart() {
        mBusyTimerStart = System.currentTimeMillis();
    }

    /**
     * Turns off the busy icon animation when capture is finished
     */
    public void onCaptureComplete() {
        mBusyTimerStart = -1;
    }

    /**
     * NOP
     */
    public void onFileSystemChange() {
    }

    /**
     * NOP
     */
    public void onPreviewParamChange(PreviewParams paramId) {
    }
}
