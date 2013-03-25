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

/**
 * Interface definition storing application wide constants. Constants like
 * {@link #IMAGE_STACK_PATTERN} should be modified with particular attention,
 * since they rely on definitions in the native code.
 */
public interface Settings {
    /**
     * Default location of fcam image gallery in SD card file system
     */
    final static public String STORAGE_DIRECTORY = "DCIM/fcam";

    /**
     * Default matching pattern for xml image stack descriptors.
     */
    final static public String IMAGE_STACK_PATTERN = "img_\\d{4}.xml";

    /**
     * UI maximum refresh rate (histogram data, seek bars).
     */
    final static public int UI_MAX_FPS = 15;

    /**
     * Seek bar step count.
     */
    final static public int SEEK_BAR_PRECISION = 1000;

    /**
     * Non-linear mapping between seek bar position and actual value (improves
     * UI experience).
     */
    final static public double SEEK_BAR_EXPOSURE_GAMMA = 5.0;

    /**
     * Non-linear mapping between seek bar position and actual value (improves
     * UI experience).
     */
    final static public double SEEK_BAR_GAIN_GAMMA = 2.0;

    /**
     * Non-linear mapping between seek bar position and actual value (improves
     * UI experience).
     */
    final static public double SEEK_BAR_FOCUS_GAMMA = 0.5;

    /**
     * Non-linear mapping between seek bar position and actual value (improves
     * UI experience).
     */
    final static public double SEEK_BAR_WB_GAMMA = 1.0;

    /**
     * Busy icon display scale (see {@link CameraView}).
     */
    final static public float BUSY_ICON_SCALE = 0.1f;

    /**
     * Busy icon animation speed (see {@link CameraView}).
     */
    final static public float BUSY_ICON_SPEED = 0.2f;


    // TODO: implement normalized units using doubles (sec/iso/kelwins/cm)
    final static public double MIN_EXPOSURE = 125;
    final static public double MAX_EXPOSURE = 1000000;
    final static public double MIN_GAIN = 1.0;
    final static public double MAX_GAIN = 32.0;
    final static public double MIN_WB = 3200;
    final static public double MAX_WB = 9500;
    final static public double MIN_FOCUS = 10.0;
    final static public double MAX_FOCUS = 0.0;
}
