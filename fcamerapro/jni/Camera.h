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
#ifndef _CAMERA_H
#define _CAMERA_H

/**
 * @file
 *
 * Definition of Camera
 */

#include <FCam/Tegra.h>
#include <FCam/Tegra/AutoFocus.h>
#include "ParamSetRequest.h"

#define FCAM_MAX_PICTURES_PER_SHOT 16 /**< Defines maximum number of pictures per burst shot */

/**
 * Helper class encapsulating FCam sensor, lens, flash and capture functionality.
 */
class Camera
{
public:
    /**
     * Single image capture parameters.
     */
    class ShotParams
    {
    public:
        ShotParams( void );

        float exposure; /**< Exposure (microseconds) */
        float focus; /**< Focus (Dioptres) */
        float gain; /**< Gain (ISO) */
        float wb; /**< Color temparature (Kelwins) */
        int flashOn; /**< Flash state (0 - off, 1 - on) */
    };

    /**
     * Capture state for FCam work thread. This structure stores parameters
     * used for preview and full-resolution image capture.
     */
    class CaptureState
    {
    public:
        CaptureState( void );

        struct
        {
            ShotParams evaluated; /**< Auto-evaluated capture parameters */
            ShotParams user; /**< User defined capture parameters */
            bool autoExposure; /**< Auto-evaluation of exposure enabled? (0 - no, 1 - yes) */
            bool autoFocus; /**< Auto-evaluation of focus enabled? (0 - no, 1 - yes) */
            bool autoGain; /**< Auto-evaluation of gain enabled? (0 - no, 1 - yes) */
            bool autoWB; /**< Auto-evaluation of color temperature enabled? (0 - no, 1 - yes) */
            float histogramData[HISTOGRAM_SIZE]; /**< Normalized histogram data */
        } preview; /**< Capture preview settings */

        ShotParams pendingImages[FCAM_MAX_PICTURES_PER_SHOT]; /**< Image parameters for full-resolution capture */
        int pendingImagesCount; /**< Number of image to capture */
    };

    /**
     * Camera used for preview and capture
     */
    enum Mode { Front, Back, Stereo };

    /**
     * Constructs camera object. Creates FCam sensor and attaches default lens, flash
     * and auto-focuser.
     * @param width preview image width in pixels
     * @param height preview image height in pixels
     * @param mode camera id used for capture and preview
     */
    Camera( int width, int height, Mode mode );

    /**
     * Destroys camera object
     */
    ~Camera( void );

    int width( void )
    {
        return m_imageWidth;
    }
    int height( void )
    {
        return m_imageHeight;
    }

    /**
     * Performs full-resolution capture. The capture settings are stored in
     * m_currentState field which is modified in the worker thread body by
     * requests relayed from the Java UI.
     * @param writer pointer to {@link AsyncImageWriter} instance that writes captured images to storage.
     */
    void capture( class AsyncImageWriter * writer );

    CaptureState m_currentState; /**< Holds configuration of current preview and full-resolution frames */
    const Mode m_currentMode; /**< Current camera used for preview and capture */
    FCam::Tegra::Sensor * m_sensor; /**< pointer to FCam sensor */
    FCam::Tegra::Lens * m_lens; /**< pointer to FCam lens attached to the sensor */
    FCam::Tegra::Flash * m_flash; /**< pointer to FCam flash attached to the sensor */
    FCam::Tegra::AutoFocus * m_autoFocus; /**< pointer to auto-focuser implementation */
    FCam::Image * m_previewImage; /**< pointer to preview image */

private:
    const int m_imageWidth, m_imageHeight;
};

#endif

