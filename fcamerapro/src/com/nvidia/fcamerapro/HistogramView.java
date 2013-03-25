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

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Paint.Style;
import android.os.Handler;
import android.util.AttributeSet;
import android.view.View;
import android.view.View.OnClickListener;

/**
 * Custom UI component that displays image histogram. The data source can be
 * changed at run-time with {@link #setDataProvider(HistogramDataProvider)}
 * function call. The histogram component is refreshed either continuously (with
 * frequency defined by {@link Settings#UI_MAX_FPS}) or upon content change. The
 * refresh mode depends on {@code auto_refresh} attribute.
 */
public final class HistogramView extends View implements OnClickListener {
    final static private int DRAW_MODE_COARSE_ACCUM_COLUMNS = 4;

    /**
     * Histogram drawing modes.
     */
    private enum DrawMode {
        NORMAL, COARSE;
    }

    /**
     * Normalized histogram data
     */
    private final float[] mBinData = new float[256];

    /**
     * Current data provider
     */
    private HistogramDataProvider mDataProvider;

    private Paint mHistogramPaint, mBackgroundPaint;

    private DrawMode mDrawMode;
    final private boolean mAutoRefresh;

    /**
     * UI thread handler. Needed for posting UI update messages directly from
     * the UI thread.
     */
    private Handler mHandler = new Handler();

    /**
     * UI update event handler. Updates the histogram
     * {@link Settings#UI_MAX_FPS} per second. The event is added to event loop
     * in {@link #onAttachedToWindow()} and removed in
     * {@link #onDetachedFromWindow()}.
     */
    private Runnable mUpdateUITask = new Runnable() {
        public void run() {
            invalidate();
            mHandler.postDelayed(this, 1000 / Settings.UI_MAX_FPS);
        }
    };

    /**
     * Sets histogram data source.
     *
     * @param provider
     *            contains reference to histogram data source
     */
    public void setDataProvider(HistogramDataProvider provider) {
        mDataProvider = provider;
        if (provider == null) {
            for (int i = 0; i < mBinData.length; i++) {
                mBinData[i] = 0.0f;
            }
        }
        postInvalidate();
    }

    /**
     * Default histogram view constructor. The constructor checks for the value
     * of {@code auto_refresh} attribute to determine the refresh mode
     * (continuous or on demand).
     *
     * @param context
     *            contains parent context
     * @param attrs
     *            contains xml attribute set
     */
    public HistogramView(Context context, AttributeSet attrs) {
        super(context, attrs);

        mAutoRefresh = attrs.getAttributeBooleanValue(null, "auto_refresh", false);

        setOnClickListener(this);

        setBackgroundColor(Color.TRANSPARENT);

        mHistogramPaint = new Paint();
        mHistogramPaint.setColor(0xff707080);
        // mHistogramPaint.setAntiAlias(true);

        mBackgroundPaint = new Paint();
        mBackgroundPaint.setColor(0x40707080);
        mBackgroundPaint.setStyle(Style.STROKE);
        mBackgroundPaint.setStrokeWidth(2);

        mDrawMode = attrs.getAttributeBooleanValue(null, "draw_coarse", false) ? DrawMode.COARSE : DrawMode.NORMAL;
    }

    /**
     * Draws histogram view.
     */
    @Override
    protected void onDraw(Canvas canvas) {
        if (canvas == null) {
            return;
        }

        float width = canvas.getWidth();
        float height = canvas.getHeight();
        float ax = 0.0f;
        float dx = width / mBinData.length;

        // clear bg
        canvas.drawRect(0.0f, 0.0f, width, height, mBackgroundPaint);

        // query fcam
        if (mDataProvider != null) {
            mDataProvider.getHistogramData(mBinData);
        }

        // draw bars
        switch (mDrawMode) {
        case NORMAL:
            for (int i = 0; i < mBinData.length; i++) {
                canvas.drawRect(ax, height - height * mBinData[i], ax + dx, height, mHistogramPaint);
                ax += dx;
            }
            break;
        case COARSE:
            dx *= DRAW_MODE_COARSE_ACCUM_COLUMNS;
            for (int i = 0; i < mBinData.length; i += DRAW_MODE_COARSE_ACCUM_COLUMNS) {
                float avalue = mBinData[i];
                for (int j = 1; j < DRAW_MODE_COARSE_ACCUM_COLUMNS; j++) {
                    // TODO: check if max operator is the best for 'coarsening'
                    // of histogram data
                    if (mBinData[i + j] > avalue) {
                        avalue = mBinData[i + j];
                    }
                }
                canvas.drawRect(ax, height - height * avalue, ax + dx - 1, height, mHistogramPaint);
                ax += dx;
            }
            break;
        }
    }

    /**
     * Handles {@code onClick()} events and switches histogram drawing
     * mode.
     */
    public void onClick(View v) {
        switch (mDrawMode) {
        case NORMAL:
            mDrawMode = DrawMode.COARSE;
            break;
        case COARSE:
            mDrawMode = DrawMode.NORMAL;
            break;
        }

        invalidate();
    }

    /**
     * Called when component is attached to the parent. If the histogram view
     * has been created with continuous refresh set, then the function registers
     * UI refresh task.
     */
    @Override
    protected void onAttachedToWindow() {
        if (mAutoRefresh) {
            mHandler.postDelayed(mUpdateUITask, 0);
        }
    }

    /**
     * Called when component is detached from the parent. If the histogram view
     * has been created with continuous refresh set, then the function removes
     * UI refresh task.
     */
    @Override
    protected void onDetachedFromWindow() {
        if (mAutoRefresh) {
            mHandler.removeCallbacks(mUpdateUITask);
        }
    }
}
