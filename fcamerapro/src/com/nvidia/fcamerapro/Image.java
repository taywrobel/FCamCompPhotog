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

import java.io.File;

import org.xml.sax.Attributes;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;

/**
 * Container class for a captured image. It stores image thumbnail, capture
 * parameters and path to the full resolution image. The capture parameters are
 * read directly from xml descriptor file, specifically, from attributes of the
 * image node.
 */

public final class Image implements HistogramDataProvider {
    final private static int HISTOGRAM_SAMPLES = 32768;

    /**
     * thumbnail bitmap
     */
    private Bitmap mThumbnail;
    /**
     * thumbnail file name
     */
    final private String mThumbnailName;
    /**
     * full resolution file name
     */
    final private String mImageName;

    /**
     * capture parameters
     */
    final private boolean mFlashOn;
    final private int mGain;
    final private int mExposure;
    final private int mWb;
    final private float mFocus;

    /**
     * normalized histogram data
     */
    private float[] mHistogramData;

    /**
     * Image constructor
     *
     * @param galleryDir
     *            stores absolute location of directory with image data
     * @param attributes
     *            contains capture parameters. These can be read from xml
     *            descriptor file.
     */
    public Image(String galleryDir, Attributes attributes) {
        mImageName = galleryDir + File.separatorChar + attributes.getValue("name");
        mThumbnailName = galleryDir + File.separatorChar + attributes.getValue("thumbnail");

        mFlashOn = Integer.parseInt(attributes.getValue("flash")) != 0;
        mGain = Integer.parseInt(attributes.getValue("gain"));
        mExposure = Integer.parseInt(attributes.getValue("exposure"));
        mWb = Integer.parseInt(attributes.getValue("wb"));

        // XXX: need to check for presence of this tag for backwards compatibility
        String value = attributes.getValue("focus");
        mFocus = value != null ? Float.parseFloat(value) : 0.0f;
    }

    /**
     * Loads thumbnail image and computes its histogram data. If the thumbnail
     * has been already loaded, the method does nothing.
     *
     * @return true if thumbnail has been loaded successfully, false otherwise.
     */
    public boolean loadThumbnail() {
        if (mThumbnail == null) {
            BitmapFactory.Options opts = new BitmapFactory.Options();
            opts.inScaled = false;
            mThumbnail = BitmapFactory.decodeFile(mThumbnailName, opts);

            // get histogram data from thumbnail
            if (mThumbnail != null) {
                int[] accum = new int[256];
                int[] buffer = new int[mThumbnail.getWidth() * mThumbnail.getHeight()];
                mThumbnail.getPixels(buffer, 0, mThumbnail.getWidth(), 0, 0, mThumbnail.getWidth(), mThumbnail.getHeight());

                int tx = 0;
                int ax = (buffer.length << 12) / HISTOGRAM_SAMPLES;
                for (int i = 0; i < HISTOGRAM_SAMPLES; i++) {
                    int pixel = buffer[tx >>> 12];
                    // rgb2lum Y = 0.2126 R + 0.7152 G + 0.0722 B
                    // XXX: bug, srgb should be linearized before applying this
                    // formula
                    int y = (13932 * ((pixel >> 16) & 0xff) + 46971 * ((pixel >> 8) & 0xff) + 4733 * (pixel & 0xff)) >> 16;
                    accum[y]++;
                    tx += ax;
                }

                int maxv = 0;
                for (int i = 0; i < accum.length; i++) {
                    if (accum[i] > maxv) {
                        maxv = accum[i];
                    }
                }

                if (maxv > 0) {
                    float norm = 1.0f / maxv;
                    float[] data = new float[accum.length];
                    for (int i = 0; i < accum.length; i++) {
                        data[i] = accum[i] * norm;
                    }

                    mHistogramData = data;
                }
            }
        }

        return mThumbnail != null;
    }

    /**
     * Returns a reference to the thumbnail image.
     *
     * @return thumbnail image reference
     */
    public Bitmap getThumbnail() {
        return mThumbnail;
    }

    /**
     * Returns the thumbnail image file name.
     *
     * @return thumbnail image file name.
     */
    public String getThumbnailName() {
        return mThumbnailName;
    }

    /**
     * Returns a full resolution image file name.
     *
     * @return full resolution image file name
     */
    public String getName() {
        return mImageName;
    }

    /**
     * Returns a string with capture parameters. There is some UI code that
     * assumes this particular format of capture info, therefore, any change
     * here might brake the UI code in the image viewer.
     *
     * @return string with image capture parameters
     */
    public String getInfo() {
        String str = Utils.GetFileName(mImageName);
        str += "\n" + Utils.FormatExposure(mExposure);
        str += "\n" + Utils.FormatGain(mGain / 100);
        str += "\n" + Utils.FormatWhiteBalance(mWb);
        str += "\n" + Utils.FormatFocus(mFocus);
        str += "\n" + (mFlashOn ? "On" : "Off");

        return str;
    }

    /**
     * Returns the histogram data for this image. The data contains normalized
     * 256 bins histogram values.
     *
     * @param data
     *            reference to array holding 256 floats.
     */
    public void getHistogramData(float[] data) {
        if (mHistogramData != null) {
            System.arraycopy(mHistogramData, 0, data, 0, mHistogramData.length);
        }
    }

}
