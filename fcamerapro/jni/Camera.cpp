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
#include "Camera.h"
#include "AsyncImageWriter.h"

Camera::ShotParams::ShotParams( void )
{
    exposure = 30000; // 30ms
    gain = 1.0f;
    wb = 6500.0f;
    focus = 10.0f;
    flashOn = 0;
}

Camera::CaptureState::CaptureState( void )
{
    preview.autoExposure = true;
    preview.autoGain = true;
    preview.autoWB = true;
    preview.autoFocus = false;

    memset( preview.histogramData, 0, sizeof( float ) * HISTOGRAM_SIZE );
    pendingImagesCount = 0;
}

Camera::Camera( int width, int height, Mode mode ) : m_currentMode( mode ),
    m_imageWidth( width ), m_imageHeight( height )
{
    switch ( mode )
    {
        case Front:
            m_sensor = new FCam::Tegra::Sensor( FCam::Tegra::Sensor::FRONT );
            break;
        case Back:
            m_sensor = new FCam::Tegra::Sensor( FCam::Tegra::Sensor::REAR );
            break;
        case Stereo:
            m_sensor = new FCam::Tegra::Sensor( FCam::Tegra::Sensor::REAR_STEREO );
            break;
    }

    m_lens = new FCam::Tegra::Lens();
    m_flash = new FCam::Tegra::Flash();
    m_sensor->attach( m_lens );
    m_sensor->attach( m_flash );
    m_autoFocus = new FCam::Tegra::AutoFocus( m_lens );

    m_previewImage = new FCam::Image( width, height, FCam::YUV420p );

    // init default shot params
    memset( &m_currentState, 0, sizeof( CaptureState ) );
}

Camera::~Camera( void )
{
    delete m_previewImage;

    delete m_autoFocus;
    delete m_flash;
    delete m_lens;
    delete m_sensor;
}

void Camera::capture( AsyncImageWriter * writer )
{
    // stop streaming
    m_sensor->stopStreaming();

    // TODO: safe to remove this?
    while ( m_sensor->shotsPending() > 0 )
    {
        m_sensor->getFrame();
    }

    // prepare new image set
    ImageSet * is = writer->newImageSet();

    // register capture

    FCam::Flash::FireAction flashAction( m_flash );
    // synchronize the flash with the shutter
    flashAction.time = 0;
    flashAction.brightness = m_flash->maxBrightness();

    FCam::Size isize = m_sensor->maxImageSize();

    for ( int i = 0; i < m_currentState.pendingImagesCount; i++ )
    {
        // TODO: add support for per-shot focus value
        FCam::Tegra::Shot shot;
        shot.exposure = m_currentState.pendingImages[i].exposure;
        shot.gain = m_currentState.pendingImages[i].gain;
        shot.whiteBalance = m_currentState.pendingImages[i].wb;
        shot.image = FCam::Image( isize.width, isize.height, FCam::YUV420p );
        shot.histogram.enabled = false;
        shot.sharpness.enabled = false;

        if ( m_currentState.pendingImages[i].flashOn != 0 )
        {
            shot.addAction( flashAction );
        }

        m_sensor->capture( shot );
    }

    // capture
    FileFormatDescriptor fmt( FileFormatDescriptor::EFormatJPEG, 95 );

    // TODO: much faster would be to consider simultaneous writing and capture (without prebuffering in mem).
    while ( m_sensor->shotsPending() > 0 )
    {
        is->add( fmt, m_sensor->getFrame() );
    }

    // write out images
    writer->push( is );
}
