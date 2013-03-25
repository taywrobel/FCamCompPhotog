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

import java.util.ArrayList;
import java.util.Vector;

/**
 * Interface between the java and native application parts. The communication
 * happens in two directions. The java code calls setter methods which are
 * serialized and passed to the native part, and the native code can notify FCam
 * event listeners with notify* methods.
 *
 * TODO: the interface works only as a singleton class. It is a bad design, but
 * we might be OK with it since camera access is exclusive anyway.
 */
public final class FCamInterface {
    static {
		System.loadLibrary("FCamTegraHal");
        System.loadLibrary("jni_fcamerapro");
    }

    /**
     * Private parameter ids. See ParamSetRequest.h for description
     */
    final static private int PARAM_SHOT = 0;
    final static private int PARAM_RESOLUTION = 1;
    final static private int PARAM_BURST_SIZE = 2;
    final static private int PARAM_OUTPUT_FORMAT = 3;
    final static private int PARAM_VIEWER_ACTIVE = 4;
    final static private int PARAM_OUTPUT_DIRECTORY = 5;
    final static private int PARAM_OUTPUT_FILE_ID = 6;
    final static private int PARAM_LUMINANCE_HISTOGRAM = 7;
    final static private int PARAM_PREVIEW_EXPOSURE = 8;
    final static private int PARAM_PREVIEW_FOCUS = 9;
    final static private int PARAM_PREVIEW_GAIN = 10;
    final static private int PARAM_PREVIEW_WB = 11;
    final static private int PARAM_PREVIEW_AUTO_EXPOSURE_ON = 12;
    final static private int PARAM_PREVIEW_AUTO_FOCUS_ON = 13;
    final static private int PARAM_PREVIEW_AUTO_GAIN_ON = 14;
    final static private int PARAM_PREVIEW_AUTO_WB_ON = 15;
    final static private int PARAM_CAPTURE_FPS = 16;
    final static private int PARAM_TAKE_PICTURE = 17;
    final static private int PARAM_FOCUS_ON_TOUCH = 18;
    final static private int PARAM_WB_ON_TOUCH = 19;
    final static private int PARAM_SELECT_CAMERA = 20;

    final static private int SHOT_PARAM_EXPOSURE = 0;
    final static private int SHOT_PARAM_FOCUS = 1;
    final static private int SHOT_PARAM_GAIN = 2;
    final static private int SHOT_PARAM_WB = 3;
    final static private int SHOT_PARAM_FLASH = 4;

    final static private int SELECT_FRONT_CAMERA = 0;
    final static private int SELECT_BACK_CAMERA = 1;
    final static private int SELECT_STEREO_CAMERA = 2;

    // ============================================================================
    // JAVA INTERFACE
    // ============================================================================

    /**
     * Native code event listeners
     */
    final private Vector<FCamInterfaceEventListener> mEventListenters = new Vector<FCamInterfaceEventListener>();

    /**
     * Contains preview image capture parameters
     */
    public enum PreviewParams {
        /**
         * Preview frame exposure
         */
        EXPOSURE,
        /**
         * Preview frame gain
         */
        GAIN,
        /**
         * Preview frame white point color temperature
         */
        WB,
        /**
         * Preview frame focus
         */
        FOCUS
    };

    public enum Cameras {
        /**
         * Selects front facing camera
         */
        FRONT,
        /**
         * Selects back facing camera
         */
        BACK,
        /**
         * Selects stereo camera
         */
        STEREO
    };

    // single-ton class model
    static private FCamInterface sInstance = new FCamInterface();

    /**
     * Returns the singleton instance of {@link FCamInterface}
     *
     * @return singleton instance of {@link FCamInterface}
     */
    static public FCamInterface GetInstance() {
        return sInstance;
    }

    /**
     * Private {@link FCamInterface} constructor.
     */
    private FCamInterface() {
        init();
    }

    /**
     * Registers a {@link FCamInterface} event listener.
     *
     * @param listener
     *            contains a reference to {@link FCamInterfaceEventListener}
     *            implementation.
     */
    public void addEventListener(FCamInterfaceEventListener listener) {
        if (mEventListenters.contains(listener)) {
            throw new IllegalArgumentException("listener already registered!");
        }

        mEventListenters.add(listener);
    }

    /**
     * Removes {@link FCamInterface} event listener.
     *
     * @param listener
     *            contains a reference to {@link FCamInterfaceEventListener}
     *            implementation.
     */
    public void removeEventListener(FCamInterfaceEventListener listener) {
        if (!mEventListenters.contains(listener)) {
            throw new IllegalArgumentException("listener not registered!");
        }

        mEventListenters.remove(listener);
    }

    /**
     * Emits {@link FCamInterfaceEventListener#onCaptureStart()} event to all
     * {@link FCamInterface} listeners.
     */
    private void notifyCaptureStart() {
        for (FCamInterfaceEventListener listener : mEventListenters) {
            listener.onCaptureStart();
        }
    }

    /**
     * Emits {@link FCamInterfaceEventListener#onCaptureComplete()} event to all
     * {@link FCamInterface} listeners.
     */
    private void notifyCaptureComplete() {
        for (FCamInterfaceEventListener listener : mEventListenters) {
            listener.onCaptureComplete();
        }
    }

    /**
     * Emits {@link FCamInterfaceEventListener#onFileSystemChange()} event to
     * all {@link FCamInterface} listeners.
     */
    private void notifyFileSystemChange() {
        for (FCamInterfaceEventListener listener : mEventListenters) {
            listener.onFileSystemChange();
        }
    }

    /**
     * Emits
     * {@link FCamInterfaceEventListener#onPreviewParamChange(PreviewParams)}
     * event to all {@link FCamInterface} listeners.
     *
     * @param shotParamId
     *            contains modified parameter id
     */
    private void notifyPreviewParamChange(int paramId) {
        switch (paramId) {
        case PARAM_PREVIEW_EXPOSURE:
            for (FCamInterfaceEventListener listener : mEventListenters) {
                listener.onPreviewParamChange(PreviewParams.EXPOSURE);
            }
            break;
        case PARAM_PREVIEW_FOCUS:
            for (FCamInterfaceEventListener listener : mEventListenters) {
                listener.onPreviewParamChange(PreviewParams.FOCUS);
            }
            break;
        case PARAM_PREVIEW_GAIN:
            for (FCamInterfaceEventListener listener : mEventListenters) {
                listener.onPreviewParamChange(PreviewParams.GAIN);
            }
            break;
        case PARAM_PREVIEW_WB:
            for (FCamInterfaceEventListener listener : mEventListenters) {
                listener.onPreviewParamChange(PreviewParams.WB);
            }
            break;
        }
    }

    /**
     * Returns the histogram data for current preview frame. The data contains
     * normalized 256 bins histogram values.
     *
     * @param bins
     *            reference to array holding 256 floats.
     */
    public void getHistogramData(float[] bins) {
        getParamFloatArray(PARAM_LUMINANCE_HISTOGRAM, bins);
    }

    /**
     * Selects camera used for capture and preview. The change is immediate.
     *
     * @param camera
     */
    public void selectCamera(Cameras camera) {
        switch (camera) {
        case FRONT:
            setParamInt(PARAM_SELECT_CAMERA, SELECT_FRONT_CAMERA);
            break;
        case BACK:
            setParamInt(PARAM_SELECT_CAMERA, SELECT_BACK_CAMERA);
            break;
        case STEREO:
            setParamInt(PARAM_SELECT_CAMERA, SELECT_STEREO_CAMERA);
            break;
        }
    }

    /**
     * Returns currently selected camera
     *
     * @return currently selected camera
     */
    public Cameras getSelectedCamera() {
        int camera = getParamInt(PARAM_SELECT_CAMERA);
        switch (camera) {
        case SELECT_FRONT_CAMERA:
            return Cameras.FRONT;
        case SELECT_BACK_CAMERA:
            return Cameras.BACK;
        default:
            return Cameras.STEREO;
        }
    }

    /**
     * Enables/disables auto-evaluation of preview frame capture parameters.
     *
     * @param previewParamId
     *            contains capture parameter id
     * @param value
     *            defines parameter state. Set to true to enable or false to
     *            disable auto-evaluation.
     */
    public void enablePreviewParamEvaluator(PreviewParams previewParamId, boolean value) {
        int pvalue = value ? 1 : 0;
        switch (previewParamId) {
        case EXPOSURE:
            setParamInt(PARAM_PREVIEW_AUTO_EXPOSURE_ON, pvalue);
            break;
        case GAIN:
            setParamInt(PARAM_PREVIEW_AUTO_GAIN_ON, pvalue);
            break;
        case WB:
            setParamInt(PARAM_PREVIEW_AUTO_WB_ON, pvalue);
            break;
        case FOCUS:
            setParamInt(PARAM_PREVIEW_AUTO_FOCUS_ON, pvalue);
            break;
        }
    }

    /**
     * Gets auto-evaluation state of <b>previously captured</b> preview frame.
     *
     * @param previewParamId
     *            contains capture parameter id
     * @return auto-evaluation state, true if enabled, false otherwise
     */
    public boolean isPreviewParamEvaluatorEnabled(PreviewParams previewParamId) {
        switch (previewParamId) {
        case EXPOSURE:
            return getParamInt(PARAM_PREVIEW_AUTO_EXPOSURE_ON) != 0;
        case GAIN:
            return getParamInt(PARAM_PREVIEW_AUTO_GAIN_ON) != 0;
        case WB:
            return getParamInt(PARAM_PREVIEW_AUTO_WB_ON) != 0;
        case FOCUS:
            return getParamInt(PARAM_PREVIEW_AUTO_FOCUS_ON) != 0;
        }

        return false;
    }

    /**
     * Gets a capture parameter of the <b>previously captured</b> preview frame.
     * If auto-evaluation was enabled (see
     * {@link #enablePreviewParamEvaluator(PreviewParams, boolean)}) then
     * auto-evaluated parameter value is returned.
     *
     * @param previewParamId
     *            contains capture parameter id
     * @return parameter value
     */
    public double getPreviewParam(PreviewParams previewParamId) {
        switch (previewParamId) {
        case EXPOSURE:
            return getParamFloat(PARAM_PREVIEW_EXPOSURE);
        case GAIN:
            return getParamFloat(PARAM_PREVIEW_GAIN);
        case WB:
            return getParamFloat(PARAM_PREVIEW_WB);
        case FOCUS:
            return getParamFloat(PARAM_PREVIEW_FOCUS);
        }

        return 0;
    }

    /**
     * Sets the capture parameter of a preview frame. If auto-evaluation is
     * enabled (see {@link #enablePreviewParamEvaluator(PreviewParams, boolean)}
     * ) then the changes made through this function do not affect the preview
     * capture.
     *
     * @param previewParamId
     *            contains capture parameter id
     * @param value
     *            stores capture parameter value
     */
    public void setPreviewParam(PreviewParams previewParamId, double value) {
        switch (previewParamId) {
        case EXPOSURE:
            setParamFloat(PARAM_PREVIEW_EXPOSURE, (float) value);
            break;
        case GAIN:
            setParamFloat(PARAM_PREVIEW_GAIN, (float) value);
            break;
        case WB:
            setParamFloat(PARAM_PREVIEW_WB, (float) value);
            break;
        case FOCUS:
            setParamFloat(PARAM_PREVIEW_FOCUS, (float) value);
            break;
        }
    }

    /**
     * Returns preview capture speed in frames per second.
     *
     * @return preview capture speed in frames per second.
     */
    public float getCaptureFps() {
        return getParamFloat(PARAM_CAPTURE_FPS);
    }

    /**
     * Gets preview capture state. If true then preview is active and capturing
     * frames.
     *
     * @return preview capture activity.
     */
    public boolean isPreviewActive() {
        return getParamInt(PARAM_VIEWER_ACTIVE) != 0;
    }

    /**
     * Sets preview capture state. Setting to true activates the preview
     * capture.
     *
     * @param enabled
     *            true/false boolean controlling capture activity
     */
    public void setPreviewActive(boolean enabled) {
        setParamInt(PARAM_VIEWER_ACTIVE, enabled ? 1 : 0);
    }

    /**
     * Issues image capture command. The native code stops updating preview and
     * captures a series of images which are subsequently compressed and dumped
     * to the storage. A number of events indicating capture state or file
     * system changes is sent to event listeners of {@link FCamInterface}. After
     * capture is complete, the native part resumes preview capture (if active).
     *
     * @param shots
     *            stores an array of {@link FCamShot} with shot parameters of
     *            images to be captured.
     */
    public void capture(ArrayList<FCamShot> shots) {
        float[] shotArray = new float[5];
        setParamInt(PARAM_BURST_SIZE, shots.size());
        for (int i = 0; i < shots.size(); i++) {
            FCamShot shot = shots.get(i);
            shotArray[SHOT_PARAM_EXPOSURE] = (float) shot.exposure;
            shotArray[SHOT_PARAM_FOCUS] = (float) shot.focus;
            shotArray[SHOT_PARAM_GAIN] = (float) shot.gain;
            shotArray[SHOT_PARAM_WB] = (float) shot.wb;
            shotArray[SHOT_PARAM_FLASH] = shot.flashOn ? 1 : 0;
            setParamFloatArray(PARAM_SHOT | (i << 16), shotArray);
        }

        setParamInt(PARAM_TAKE_PICTURE, 1);
    }

    /**
     * Gets the capture state. If true then we are still capturing images.
     *
     * @return capture state
     */
    public boolean isCapturing() {
        return getParamInt(PARAM_TAKE_PICTURE) != 0;
    }

    /**
     * Sets the location where captured images are written.
     *
     * @param dir
     *            contains absolute path of image storage directory
     */
    public void setStorageDirectory(String dir) {
        setParamString(PARAM_OUTPUT_DIRECTORY, dir);
    }

    /**
     * Sets the integer file id of output image. Each captured image is written
     * to a file with name ending with 4-digit postfix. The id is incremented
     * automatically and by default start from 0. The code responsible for
     * outputting the image never overwrites existing files. Instead, he will
     * try to find a non-existing id and dump the file.
     *
     * @param id
     *            integer file id
     */
    public void setOutputFileId(int id) {
        setParamInt(PARAM_OUTPUT_FILE_ID, id);
    }

    /**
     * Sends touch to focus event to the native code. The touch position is
     * expressed in normalized (0..1) coordinates inside the preview frame. This
     * event will start auto-focus procedure only if auto-evaluation of preview
     * frame parameters is enabled (see
     * {@link #enablePreviewParamEvaluator(PreviewParams, boolean)}).
     *
     * @param x
     *            normalized touch x position
     * @param y
     *            normalized touch y position
     */
    public void touchToFocus(float x, float y) {
        float[] parray = new float[2];
        parray[0] = x;
        parray[1] = y;
        setParamFloatArray(PARAM_FOCUS_ON_TOUCH, parray);
    }

    /**
     * Sends touch to white balance event to the native code. The touch position
     * is expressed in normalized (0..1) coordinates inside the preview frame.
     * This event will start auto-focus procedure only if auto-evaluation of
     * preview frame parameters is enabled (see
     * {@link #enablePreviewParamEvaluator(PreviewParams, boolean)}).
     *
     * @param x
     *            normalized touch x position
     * @param y
     *            normalized touch y position
     */
    public void touchToWhiteBalance(float x, float y) {
        float[] parray = new float[2];
        parray[0] = x;
        parray[1] = y;
        setParamFloatArray(PARAM_WB_ON_TOUCH, parray);
    }

    // ============================================================================
    // NATIVE INTERFACE
    // ============================================================================

    /**
     * Performs initialization of the native code. Should be invoked only once.
     */
    private native void init();

    /**
     * Gets preview frame texture target type.
     *
     * @return preview frame texture target type
     */
    public native int getViewerTextureTarget();

    /**
     * Acquires preview frame texture id. The texture binding target can be
     * determined with {@link #getViewerTextureTarget()}. After drawing finishes
     * the texture ownership should be transferred back to the native code by
     * calling {@link #unlockViewerTexture()}.
     *
     * @return preview frame texture id
     */
    public native int lockViewerTexture();

    /**
     * Releases preview texture id. The application should not use the texture
     * after this function has returned.
     */
    public native void unlockViewerTexture();

    /**
     * Sends parameter set command to the message queue of the native code.
     *
     * @param param
     *            is the parameter id ({@code PARAM_*})
     * @param value
     *            contains parameter value
     */
    private native void setParamInt(int param, int value);

    /**
     * Sends parameter set command to the message queue of the native code.
     *
     * @param param
     *            is the parameter id ({@code PARAM_*})
     * @param value
     *            contains parameter value
     */
    private native void setParamFloat(int param, float value);

    /**
     * Sends parameter set command to the message queue of the native code.
     *
     * @param param
     *            is the parameter id ({@code PARAM_*})
     * @param value
     *            contains parameter value
     */
    private native void setParamIntArray(int param, int[] value);

    /**
     * Sends parameter set command to the message queue of the native code.
     *
     * @param param
     *            is the parameter id ({@code PARAM_*})
     * @param value
     *            contains parameter value
     */
    private native void setParamFloatArray(int param, float[] value);

    /**
     * Sends parameter set command to the message queue of the native code.
     *
     * @param param
     *            is the parameter id ({@code PARAM_*})
     * @param value
     *            contains parameter value
     */
    private native void setParamString(int param, String value);

    /**
     * Gets the value of a given parameter.
     *
     * @param param
     *            is the parameter id ({@code PARAM_*})
     * @return parameter value
     */
    private native int getParamInt(int param);

    /**
     * Gets the value of a given parameter.
     *
     * @param param
     *            is the parameter id ({@code PARAM_*})
     * @return parameter value
     */
    private native float getParamFloat(int param);

    /**
     * Gets the value of a given parameter.
     *
     * @param param
     *            is the parameter id ({@code PARAM_*})
     * @param value
     *            contains reference to parameter value
     */
    private native void getParamIntArray(int param, float[] value);

    /**
     * Gets the value of a given parameter.
     *
     * @param param
     *            is the parameter id ({@code PARAM_*})
     * @param value
     *            contains reference to parameter value
     */
    private native void getParamFloatArray(int param, float[] value);

    /**
     * Gets the value of a given parameter.
     *
     * @param param
     *            is the parameter id ({@code PARAM_*})
     * @return parameter value
     */
    private native String getParamString(int param);
}
