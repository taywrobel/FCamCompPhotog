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

import com.nvidia.fcamerapro.FCamInterface.Cameras;

import android.app.Activity;
import android.app.Fragment;
import android.os.Bundle;
import android.os.Handler;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.View.OnClickListener;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.SeekBar;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.SeekBar.OnSeekBarChangeListener;

/**
 * UI component implementing and controlling the camera functionality.
 */
public final class CameraFragment extends Fragment implements OnClickListener, OnSeekBarChangeListener, HistogramDataProvider, OnItemSelectedListener {

    /**
     * References to UI components used to control picture capture parameters
     */
    private CheckBox mAutoWBCheckBox, mAutoFocusCheckBox, mAutoExposureCheckBox, mAutoGainCheckBox;
    private SeekBar mWBSeekBar, mFocusSeekBar, mExposureSeekBar, mGainSeekBar, mBrackSeekBar;
    private TextView mWbTextView, mFocusTextView, mExposureTextView, mGainTextView, mBrackTextView;
    private Spinner mOutputFormatSpinner, mShootingModeSpinner, mFlashModeSpinner;
    private Button mCaptureButton;

    /**
     * Reference to histogram view subcomponent
     */
    private HistogramView mHistogramView;

    /**
     * Id of the camera used for capture and preview
     */
    private Cameras mCurrentCamera = Cameras.BACK;

    /**
     * Reference to camera fragment view component created by
     * {@link #initContentView()}
     */
    private View mContentView;

    /**
     * UI thread handler. Needed for posting UI update messages directly from
     * the UI thread.
     */
    private Handler mHandler = new Handler();

    /**
     * UI update event handler. Updates the positions of sliders
     * {@link Settings#UI_MAX_FPS} per second. The event is added to event loop
     * in {@link #onCreateView(LayoutInflater, ViewGroup, Bundle)} and removed
     * in {@link #onDestroyView()}.
     */
    private Runnable mUpdateUITask = new Runnable() {
        public void run() {
            // update sliders
            updateSeekBarValues();
            mHandler.postDelayed(this, 1000 / Settings.UI_MAX_FPS);
        }
    };

    /**
     * Creates the camera fragment view and initializes UI components. Because
     * the android API tries to to destroy fragments between tab switches, we
     * create the view once and reuse it in later calls to
     * {@link #onCreateView(LayoutInflater, ViewGroup, Bundle)} or
     * {@link #onCreate(Bundle)}. This way we preserve the UI state (seek bar
     * positions, button states) between view changes.
     */
    private void initContentView() {
        if (mContentView != null) {
            return;
        }

        Activity activity = getActivity();
        mContentView = activity.getLayoutInflater().inflate(R.layout.camera, null);
        setHasOptionsMenu(true);

        ArrayAdapter<CharSequence> adapter;

        // image output
        mOutputFormatSpinner = (Spinner) mContentView.findViewById(R.id.spinner_output_format);
        adapter = ArrayAdapter.createFromResource(activity, R.array.output_format_array, android.R.layout.simple_spinner_item);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        mOutputFormatSpinner.setAdapter(adapter);
        mOutputFormatSpinner.setEnabled(false);

        // shooting mode
        mShootingModeSpinner = (Spinner) mContentView.findViewById(R.id.spinner_shooting_mode);
        adapter = ArrayAdapter.createFromResource(activity, R.array.shooting_mode_array, android.R.layout.simple_spinner_item);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        mShootingModeSpinner.setAdapter(adapter);
        mShootingModeSpinner.setOnItemSelectedListener(this);

        // flash mode
        mFlashModeSpinner = (Spinner) mContentView.findViewById(R.id.spinner_flash_mode);
        adapter = ArrayAdapter.createFromResource(activity, R.array.flash_mode_array, android.R.layout.simple_spinner_item);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        mFlashModeSpinner.setAdapter(adapter);

        // auto checkboxes
        mAutoExposureCheckBox = (CheckBox) mContentView.findViewById(R.id.cb_auto_exposure);
        mAutoExposureCheckBox.setOnClickListener(this);
        mAutoFocusCheckBox = (CheckBox) mContentView.findViewById(R.id.cb_auto_focus);
        mAutoFocusCheckBox.setOnClickListener(this);
        mAutoGainCheckBox = (CheckBox) mContentView.findViewById(R.id.cb_auto_gain);
        mAutoGainCheckBox.setOnClickListener(this);
        mAutoWBCheckBox = (CheckBox) mContentView.findViewById(R.id.cb_auto_wb);
        mAutoWBCheckBox.setOnClickListener(this);

        // param values
        mExposureTextView = (TextView) mContentView.findViewById(R.id.tv_exposure);
        mFocusTextView = (TextView) mContentView.findViewById(R.id.tv_focus);
        mGainTextView = (TextView) mContentView.findViewById(R.id.tv_gain);
        mWbTextView = (TextView) mContentView.findViewById(R.id.tv_wb);
        mBrackTextView = (TextView) mContentView.findViewById(R.id.tv_brack);

        // seek bars
        mExposureSeekBar = (SeekBar) mContentView.findViewById(R.id.sb_exposure);
        mExposureSeekBar.setMax(Settings.SEEK_BAR_PRECISION);
        mExposureSeekBar.setOnSeekBarChangeListener(this);
        mFocusSeekBar = (SeekBar) mContentView.findViewById(R.id.sb_focus);
        mFocusSeekBar.setMax(Settings.SEEK_BAR_PRECISION);
        mFocusSeekBar.setOnSeekBarChangeListener(this);
        mGainSeekBar = (SeekBar) mContentView.findViewById(R.id.sb_gain);
        mGainSeekBar.setMax(Settings.SEEK_BAR_PRECISION);
        mGainSeekBar.setOnSeekBarChangeListener(this);
        mWBSeekBar = (SeekBar) mContentView.findViewById(R.id.sb_wb);
        mWBSeekBar.setMax(Settings.SEEK_BAR_PRECISION);
        mWBSeekBar.setOnSeekBarChangeListener(this);
        mBrackSeekBar = (SeekBar) mContentView.findViewById(R.id.sb_brack);
        mBrackSeekBar.setOnSeekBarChangeListener(this);
        mBrackSeekBar.setEnabled(false);
        
        // capture button
        mCaptureButton = (Button) mContentView.findViewById(R.id.button_capture);
        mCaptureButton.setOnClickListener(this);

        // histogram surface
        mHistogramView = (HistogramView) mContentView.findViewById(R.id.histogram_view);
        mHistogramView.setDataProvider(this);
    }

    /**
     * Initializes camera fragment view.
     */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        // TODO: fix bad app design: use setArguments(Bundle) to save/restore
        // instance state
        super.onCreate(savedInstanceState);
        initContentView();
    }

    /**
     * Start camera view UI refresh event.
     *
     * @return reference to camera fragment view
     */
    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        // start ui update timer
        mHandler.postDelayed(mUpdateUITask, 0);

        // set mono camera
        FCamInterface.GetInstance().selectCamera(mCurrentCamera);

        // setup default view params
        updateControls();

        return mContentView;
    }

    /**
     * Stops camera fragment UI refresh event.
     */
    @Override
    public void onDestroyView() {
        // stop ui update timer
        mHandler.removeCallbacks(mUpdateUITask);

        super.onDestroyView();
    }

    /**
     * Given current shooting mode and capture parameters, the method creates a
     * set of {@link FCamShot} that specify shots to be captured.
     *
     * @param shots
     *            contains array or pending shots
     * @param flashOn
     *            true if flash supposed to be enabled during the capture
     */
    private void pushShots(ArrayList<FCamShot> shots, boolean flashOn) {
        FCamInterface iface = FCamInterface.GetInstance();

        double exposure = iface.getPreviewParam(FCamInterface.PreviewParams.EXPOSURE);
        double gain = iface.getPreviewParam(FCamInterface.PreviewParams.GAIN);
        double wb = iface.getPreviewParam(FCamInterface.PreviewParams.WB);
        double focus = iface.getPreviewParam(FCamInterface.PreviewParams.FOCUS);

        FCamShot shot = new FCamShot();
        shot.exposure = exposure;
        shot.gain = gain;
        shot.wb = wb;
        shot.focus = focus;
        shot.flashOn = flashOn;

        // TODO: make selection based on object id not position (now its
        // constant dependent)
        switch (mShootingModeSpinner.getSelectedItemPosition()) {
        case 0:
            // single picture mode
            shots.add(shot);
            break;
        case 1:
            // 2 pictures burst
            for (int i = 0; i < 2; i++) {
                shots.add(shot.clone());
            }
            break;
        case 2:
            // 4 pictures burst
            for (int i = 0; i < 4; i++) {
                shots.add(shot.clone());
            }
            break;
        case 3:
            // 8 pictures burst
            for (int i = 0; i < 8; i++) {
                shots.add(shot.clone());
            }
            break;
        case 4: {
            // bracketing
        	int numImages = mBrackSeekBar.getProgress() + 3;
        	for(int i = 0; i < numImages; i++){
        		FCamShot bshot = shot.clone();
        		float ev = i * 4.0f / numImages;
        		ev -= 2;
        		bshot.exposure *= Math.pow(2, ev);
        		shots.add(bshot);
        	}
            break;
        }
//        case 5:
//            // bracketing
//            FCamShot bshot = shot.clone();
//            bshot.exposure *= 4;
//            shots.add(bshot);
//
//            shots.add(shot);
//
//            bshot = shot.clone();
//            bshot.exposure /= 4;
//            shots.add(bshot);
//            break;
        }
    }

    /**
     * Handles
     * {@link android.content.DialogInterface.OnClickListener#onClick(android.content.DialogInterface, int)}
     * events of camera fragment and its components.
     */
    public void onClick(View v) {
        FCamInterface iface = FCamInterface.GetInstance();
        // TODO: compact code

        if (v == mCaptureButton) {
            if (!iface.isCapturing()) {
                ArrayList<FCamShot> shots = new ArrayList<FCamShot>(16);

                switch (mFlashModeSpinner.getSelectedItemPosition()) {
                case 0: // flash off
                    pushShots(shots, false);
                    break;
                case 1: // flash on
                    pushShots(shots, true);
                    break;
                case 2: // flash off/on
                    pushShots(shots, false);
                    pushShots(shots, true);
                    break;
                }

                iface.capture(shots);
            }
        } else if (v == mAutoExposureCheckBox) {
            boolean autoEvaluate = mAutoExposureCheckBox.isChecked();
            iface.enablePreviewParamEvaluator(FCamInterface.PreviewParams.EXPOSURE, autoEvaluate);
            mExposureSeekBar.setEnabled(!autoEvaluate);
        } else if (v == mAutoFocusCheckBox) {
            boolean autoEvaluate = mAutoFocusCheckBox.isChecked();
            iface.enablePreviewParamEvaluator(FCamInterface.PreviewParams.FOCUS, autoEvaluate);
            mFocusSeekBar.setEnabled(!autoEvaluate);
        } else if (v == mAutoGainCheckBox) {
            boolean autoEvaluate = mAutoGainCheckBox.isChecked();
            iface.enablePreviewParamEvaluator(FCamInterface.PreviewParams.GAIN, autoEvaluate);
            mGainSeekBar.setEnabled(!autoEvaluate);
        } else if (v == mAutoWBCheckBox) {
            boolean autoEvaluate = mAutoWBCheckBox.isChecked();
            iface.enablePreviewParamEvaluator(FCamInterface.PreviewParams.WB, autoEvaluate);
            mWBSeekBar.setEnabled(!autoEvaluate);
        }
    }

    /**
     * Reads current capture settings from FCam native interface and configures
     * the UI accordingly
     */
    private void updateControls() {
        mHandler.postDelayed(new Runnable() {
            public void run() {
                FCamInterface iface = FCamInterface.GetInstance();

                boolean autoEvaluate = iface.isPreviewParamEvaluatorEnabled(FCamInterface.PreviewParams.EXPOSURE);
                mAutoExposureCheckBox.setChecked(autoEvaluate);
                iface.enablePreviewParamEvaluator(FCamInterface.PreviewParams.EXPOSURE, autoEvaluate);
                mExposureSeekBar.setEnabled(!autoEvaluate);
                mExposureSeekBar.setProgress(Utils.GetExposureForUI(iface.getPreviewParam(FCamInterface.PreviewParams.EXPOSURE)));
                onProgressChanged(mExposureSeekBar, mExposureSeekBar.getProgress(), true);

                autoEvaluate = iface.isPreviewParamEvaluatorEnabled(FCamInterface.PreviewParams.GAIN);
                mAutoGainCheckBox.setChecked(autoEvaluate);
                iface.enablePreviewParamEvaluator(FCamInterface.PreviewParams.GAIN, autoEvaluate);
                mGainSeekBar.setEnabled(!autoEvaluate);
                mGainSeekBar.setProgress(Utils.GetGainForUI(iface.getPreviewParam(FCamInterface.PreviewParams.GAIN)));
                onProgressChanged(mGainSeekBar, mGainSeekBar.getProgress(), true);

                autoEvaluate = iface.isPreviewParamEvaluatorEnabled(FCamInterface.PreviewParams.WB);
                mAutoWBCheckBox.setChecked(autoEvaluate);
                iface.enablePreviewParamEvaluator(FCamInterface.PreviewParams.WB, autoEvaluate);
                mWBSeekBar.setEnabled(!autoEvaluate);
                mWBSeekBar.setProgress(Utils.GetWBForUI(iface.getPreviewParam(FCamInterface.PreviewParams.WB)));
                onProgressChanged(mWBSeekBar, mWBSeekBar.getProgress(), true);

                autoEvaluate = iface.isPreviewParamEvaluatorEnabled(FCamInterface.PreviewParams.FOCUS);
                mAutoFocusCheckBox.setChecked(autoEvaluate);
                iface.enablePreviewParamEvaluator(FCamInterface.PreviewParams.FOCUS, autoEvaluate);
                mFocusSeekBar.setEnabled(!autoEvaluate);
                mFocusSeekBar.setProgress(Utils.GetFocusForUI(iface.getPreviewParam(FCamInterface.PreviewParams.FOCUS)));
                onProgressChanged(mFocusSeekBar, mFocusSeekBar.getProgress(), true);
            }
        }, 0);
    }

    /**
     * Reads the preview port estimated capture parameters from
     * {@link FCamInterface} and sets the seek bar position for capture
     * parameters that are evaluated automatically.
     */
    private void updateSeekBarValues() {
        FCamInterface iface = FCamInterface.GetInstance();

        if (!mExposureSeekBar.isEnabled()) {
            mExposureSeekBar.setProgress(Utils.GetExposureForUI(iface.getPreviewParam(FCamInterface.PreviewParams.EXPOSURE)));
        }
        if (!mGainSeekBar.isEnabled()) {
            mGainSeekBar.setProgress(Utils.GetGainForUI(iface.getPreviewParam(FCamInterface.PreviewParams.GAIN)));
        }
        if (!mWBSeekBar.isEnabled()) {
            mWBSeekBar.setProgress(Utils.GetWBForUI(iface.getPreviewParam(FCamInterface.PreviewParams.WB)));
        }
        if (!mFocusSeekBar.isEnabled()) {
            mFocusSeekBar.setProgress(Utils.GetFocusForUI(iface.getPreviewParam(FCamInterface.PreviewParams.FOCUS)));
        }
    }

    /**
     * Seek bar position changed event
     */
    public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
        FCamInterface iface = FCamInterface.GetInstance();

        // TODO: compact code
        if (seekBar == mExposureSeekBar) {
            double exposure = Utils.GetExposureFromUI(progress);
            mExposureTextView.setText(Utils.FormatExposure(exposure));
            if (seekBar.isEnabled()) {
                iface.setPreviewParam(FCamInterface.PreviewParams.EXPOSURE, exposure);
            }
        } else if (seekBar == mFocusSeekBar) {
            double focus = Utils.GetFocusFromUI(progress);
            mFocusTextView.setText(Utils.FormatFocus(focus));
            if (seekBar.isEnabled()) {
                iface.setPreviewParam(FCamInterface.PreviewParams.FOCUS, focus);
            }
        } else if (seekBar == mGainSeekBar) {
            double gain = Utils.GetGainFromUI(progress);
            mGainTextView.setText(Utils.FormatGain(gain));
            if (seekBar.isEnabled()) {
                iface.setPreviewParam(FCamInterface.PreviewParams.GAIN, gain);
            }
        } else if (seekBar == mWBSeekBar) {
            double wb = Utils.GetWBFromUI(progress);
            mWbTextView.setText(Utils.FormatWhiteBalance(wb));
            if (seekBar.isEnabled()) {
                iface.setPreviewParam(FCamInterface.PreviewParams.WB, wb);
            }
        } else if (seekBar == mBrackSeekBar) {
        	mBrackTextView.setText((progress + 3) + " Images");
        }
    }

    /**
     * Seek bar start tracking touch event
     */
    public void onStartTrackingTouch(SeekBar seekBar) {
    }

    /**
     * Seek bar stop tracking touch event
     */
    public void onStopTrackingTouch(SeekBar seekBar) {
    }

    /**
     * Returns the histogram data for histogram view. The data is fetched
     * directly from the {@link FCamInterface} and contains normalized 256 bins
     * histogram values.
     *
     * @param data
     *            reference to array holding 256 floats.
     */
    public void getHistogramData(float[] data) {
        FCamInterface.GetInstance().getHistogramData(data);
    }

    /**
     * Called when particular option menu is selected. Here we handle cases
     * specific only to viewer fragment.
     */
    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
        case R.id.mi_switch_cameras:
            mCurrentCamera = mCurrentCamera == Cameras.FRONT ? Cameras.BACK : Cameras.FRONT;
            FCamInterface.GetInstance().selectCamera(mCurrentCamera);
            updateControls();
            return true;

        default:
            return super.onOptionsItemSelected(item);
        }
    }

	@Override
	public void onItemSelected(AdapterView<?> arg0, View arg1, int arg2,
			long arg3) {
		int index = mShootingModeSpinner.getSelectedItemPosition();
		if(index == 4){
			mBrackSeekBar.setEnabled(true);
		} else {
			mBrackSeekBar.setEnabled(false);
		}
	}

	@Override
	public void onNothingSelected(AdapterView<?> arg0) {
		// NOOP
	}

}
