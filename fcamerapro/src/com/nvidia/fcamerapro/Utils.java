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
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.os.Bundle;

/**
 * Various utility functions used across the application.
 */
public final class Utils {

    /**
     * Dialog fragment displaying top-layer confirmation message box.
     */
    static class ConfirmationDialogFragment extends DialogFragment {
        private DialogInterface.OnClickListener mOnConfirmListener;

        /**
         * Factory-type constructor of {@code ConfirmationDialogFragment}.
         *
         * @param title
         *            defines the message box tile
         * @param text
         *            stores the message box text
         * @param onConfirmListener
         *            is an {@link OnClickListener} executed when user clicks on
         *            positive button
         * @return {@code ConfirmationDialogFragment} instance
         */
        public static ConfirmationDialogFragment NewInstance(String title, String text, DialogInterface.OnClickListener onConfirmListener) {
            ConfirmationDialogFragment frag = new ConfirmationDialogFragment();
            Bundle args = new Bundle();
            args.putString("text", text);
            args.putString("title", title);
            frag.setArguments(args);
            frag.mOnConfirmListener = onConfirmListener;
            return frag;
        }

        /**
         * Called when {@code ConfirmationDialogFragment} is about to be
         * displayed.
         */
        @Override
        public Dialog onCreateDialog(Bundle savedInstanceState) {
            String text = getArguments().getString("text");
            String title = getArguments().getString("title");

            return new AlertDialog.Builder(getActivity()).setTitle(title).setMessage(text)
                   .setPositiveButton(android.R.string.yes, mOnConfirmListener).setNegativeButton(android.R.string.no, null).create();
        }
    }

    /**
     * Dialog fragment displaying top-layer information message box.
     */
    static class InfoDialogFragment extends DialogFragment {

        /**
         * Factory-type constructor of {@code InfoDialogFragment}.
         *
         * @param title
         *            defines the message box tile
         * @param text
         *            stores the message box text
         * @return {@code InfoDialogFragment} instance
         */
        public static InfoDialogFragment NewInstance(String title, String text) {
            InfoDialogFragment frag = new InfoDialogFragment();
            Bundle args = new Bundle();
            args.putString("text", text);
            args.putString("title", title);
            frag.setArguments(args);
            return frag;
        }

        /**
         * Called when {@code InfoDialogFragment} is about to be displayed.
         */
        @Override
        public Dialog onCreateDialog(Bundle savedInstanceState) {
            String text = getArguments().getString("text");
            String title = getArguments().getString("title");

            return new AlertDialog.Builder(getActivity()).setTitle(title).setMessage(text)
            .setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int whichButton) {
                }
            }).create();
        }
    }

    /**
     * Converts exposure value to a common notation.
     *
     * @param exposure
     *            in microseconds
     * @return string label describing the exposure value
     */
    public static String FormatExposure(double exposure) {
        // Convert the exposure to seconds
        double val = exposure / 1000000;

        if (val < 0.001) {
            return String.format("1/%d000s", (int) (0.001 / val + 0.5));
        } else if (val < 0.01) {
            return String.format("1/%d00s", (int) (0.01 / val + 0.5));
        } else if (val < 0.1) {
            return String.format("1/%d0s", (int) (0.1 / val + 0.5));
        } else if (val < 0.2) {
            return String.format("1/%ds", (int) (1.0 / val + 0.5));
        } else if (val < 0.95) {
            return String.format("0.%ds", (int) (10 * val + 0.5));
        } else {
            return String.format("%ds", (int) (val + 0.5));
        }
    }

    /**
     * Converts gain value to a common notation.
     *
     * @param gain
     *            in Kelwins
     * @return string label describing the gain value
     */
    public static String FormatGain(double gain) {
        return String.format("ISO%d00", (int) gain);
    }

    /**
     * Converts focus value to a common notation.
     *
     * @param focus
     *            in Dioptres
     * @return string label describing the focus value
     */
    public static String FormatFocus(double focus) {
        if (focus > 1.0) { // up to 1m
            return String.format("%dcm", (int) (100.0 / focus + 0.5));
        } else if (focus > 0.2) { // up to 5m
            return String.format("%.1fm", 1.0 / focus);
        } else {
            return String.format(">5m");
        }
    }

    /**
     * Converts color temperature value to a common notation.
     *
     * @param wb
     *            in Kelwins
     * @return string label describing color temperature value
     */

    public static String FormatWhiteBalance(double wb) {
        return String.format("%d00K", (int) (wb / 100 + 0.5));
    }

    /**
     * Extracts file name from full path representation.
     *
     * @param name
     *            stores full file path
     * @return the name of the file
     */
    public static String GetFileName(String name) {
        return name.substring(name.lastIndexOf(File.separatorChar) + 1);
    }


    /**
     * Converts the exposure seek bar position to {@link FCamInterface}
     * compatible exposure value. To improve the UI experience we apply a
     * non-linear transformation between seek bar position and actual exposure
     * value.
     *
     * @param progress
     *            seek bar position between 0 and
     *            {@link Settings#SEEK_BAR_PRECISION}-1</code>
     * @return exposure value
     */
    public static double GetExposureFromUI(int progress) {
        double nvalue = (double) progress / Settings.SEEK_BAR_PRECISION;
        return Math.pow(nvalue, Settings.SEEK_BAR_EXPOSURE_GAMMA) * (Settings.MAX_EXPOSURE - Settings.MIN_EXPOSURE)
               + Settings.MIN_EXPOSURE;
    }

    /**
     * Converts the exposure value to integer seek bar position shift.
     *
     * @param value
     *            exposure value
     * @return integer seek bar position
     */
    public static int GetExposureForUI(double value) {
        return (int) (Math.pow((value - Settings.MIN_EXPOSURE) / (Settings.MAX_EXPOSURE - Settings.MIN_EXPOSURE),
                               1.0 / Settings.SEEK_BAR_EXPOSURE_GAMMA) * Settings.SEEK_BAR_PRECISION);
    }

    /**
     * Converts the gain seek bar position to {@link FCamInterface} compatible
     * gain value. To improve the UI experience we apply a non-linear
     * transformation between seek bar position and actual gain value.
     *
     * @param progress
     *            seek bar position between 0 and
     *            {@link Settings#SEEK_BAR_PRECISION}-1</code>
     * @return gain value
     */
    public static double GetGainFromUI(int progress) {
        double nvalue = (double) progress / Settings.SEEK_BAR_PRECISION;
        return Math.pow(nvalue, Settings.SEEK_BAR_GAIN_GAMMA) * (Settings.MAX_GAIN - Settings.MIN_GAIN)
               + Settings.MIN_GAIN;
    }

    /**
     * Converts the gain value to integer seek bar position shift.
     *
     * @param value
     *            gain value
     * @return integer seek bar position
     */
    public static int GetGainForUI(double value) {
        return (int) (Math.pow((value - Settings.MIN_GAIN) / (Settings.MAX_GAIN - Settings.MIN_GAIN),
                               1.0 / Settings.SEEK_BAR_GAIN_GAMMA) * Settings.SEEK_BAR_PRECISION);
    }

    /**
     * Converts the focus seek bar position to {@link FCamInterface} compatible
     * focus value. To improve the UI experience we apply a non-linear
     * transformation between seek bar position and actual focus value.
     *
     * @param progress
     *            seek bar position between 0 and
     *            {@link Settings#SEEK_BAR_PRECISION}-1</code>
     * @return focus value
     */
    public static double GetFocusFromUI(int progress) {
        double nvalue = (double) progress / Settings.SEEK_BAR_PRECISION;
        return Math.pow(nvalue, Settings.SEEK_BAR_FOCUS_GAMMA) * (Settings.MAX_FOCUS - Settings.MIN_FOCUS)
               + Settings.MIN_FOCUS;
    }

    /**
     * Converts the focus value to integer seek bar position shift.
     *
     * @param value
     *            focus value
     * @return integer seek bar position
     */
    public static int GetFocusForUI(double value) {
        return (int) (Math.pow((value - Settings.MIN_FOCUS) / (Settings.MAX_FOCUS - Settings.MIN_FOCUS),
                               1.0 / Settings.SEEK_BAR_FOCUS_GAMMA) * Settings.SEEK_BAR_PRECISION);
    }

    /**
     * Converts the white balance seek bar position to {@link FCamInterface}
     * compatible white balance value. To improve the UI experience we apply a
     * non-linear transformation between seek bar position and actual white
     * balance value.
     *
     * @param progress
     *            seek bar position between 0 and
     *            {@link Settings#SEEK_BAR_PRECISION}-1</code>
     * @return exposure value
     */
    public static double GetWBFromUI(int progress) {
        double nvalue = (double) progress / Settings.SEEK_BAR_PRECISION;
        return Math.pow(nvalue, Settings.SEEK_BAR_WB_GAMMA) * (Settings.MAX_WB - Settings.MIN_WB) + Settings.MIN_WB;
    }

    /**
     * Converts the white balance value to integer seek bar position shift.
     *
     * @param value
     *            white balance value
     * @return integer seek bar position
     */
    public static int GetWBForUI(double value) {
        return (int) (Math.pow((value - Settings.MIN_WB) / (Settings.MAX_WB - Settings.MIN_WB),
                               1.0 / Settings.SEEK_BAR_WB_GAMMA) * Settings.SEEK_BAR_PRECISION);
    }



}
