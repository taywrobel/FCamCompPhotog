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
#ifndef _PARAMSETREQUEST_H
#define _PARAMSETREQUEST_H

/**
 * @file
 * Definition of ParamSetRequest.
 */

#include "Common.h"

#define HISTOGRAM_SIZE 256 /**< Histogram bin count (needs to match Java counterpart!) */

/**
 * @defgroup param_set Parameter identifiers
 * @{
 */
#define PARAM_SHOT                     0 /**< Burst shot capture parameters (float array, read/write) */
#define PARAM_RESOLUTION               1 /**< Target capture resolution (int, read/write) */
#define PARAM_BURST_SIZE               2 /**< The number of images in a burst shot (int, read/write) */
#define PARAM_OUTPUT_FORMAT            3 /**< Image output format (int, read/write) */
#define PARAM_VIEWER_ACTIVE            4 /**< Preview stream state (int, read/write) */
#define PARAM_OUTPUT_DIRECTORY         5 /**< Image output directory location (string, read/write) */
#define PARAM_OUTPUT_FILE_ID           6 /**< File index of next image stack (int, read/write) */
#define PARAM_LUMINANCE_HISTOGRAM      7 /**< Preview stream histogram data (float array, read) */
#define PARAM_PREVIEW_EXPOSURE         8 /**< Preview stream exposure value (float, read/write) */
#define PARAM_PREVIEW_FOCUS            9 /**< Preview stream focus value (float, read/write) */
#define PARAM_PREVIEW_GAIN             10 /**< Preview stream gain value (float, read/write) */
#define PARAM_PREVIEW_WB               11 /**< Preview stream color temparature value (float, read/write) */
#define PARAM_PREVIEW_AUTO_EXPOSURE_ON 12 /**< Preview stream exposure auto-evaluation state (int, read/write) */
#define PARAM_PREVIEW_AUTO_FOCUS_ON    13 /**< Preview stream focus auto-evaluation state (int, read/write) */
#define PARAM_PREVIEW_AUTO_GAIN_ON     14 /**< Preview stream gain auto-evaluation state (int, read/write) */
#define PARAM_PREVIEW_AUTO_WB_ON       15 /**< Preview stream color temparature auto-evaluation state (int, read/write) */
#define PARAM_CAPTURE_FPS              16 /**< Capture speed in frames per second (float, read) */
#define PARAM_TAKE_PICTURE             17 /**< Capture request (int, read/write) */
#define PARAM_FOCUS_ON_TOUCH           18 /**< Touch to focus event (float array, write) */
#define PARAM_WB_ON_TOUCH              19 /**< Touch to white balance event (float array, write) */
#define PARAM_SELECT_CAMERA            20 /**< Select capture camera front/back/stereo (int, read/write) */

#define PARAM_PRIV_FS_CHANGED     100 /**< File system changed notification */

/**< @} */

/**
 * @defgroup param_values Parameter values
 * @{
 */
#define SHOT_PARAM_EXPOSURE 0 /**< #PARAM_SHOT exposure value */
#define SHOT_PARAM_FOCUS    1 /**< #PARAM_SHOT focus value */
#define SHOT_PARAM_GAIN     2 /**< #PARAM_SHOT gain value */
#define SHOT_PARAM_WB       3 /**< #PARAM_SHOT color temperature value */
#define SHOT_PARAM_FLASH    4 /**< #PARAM_SHOT flash state value */

#define SELECT_FRONT_CAMERA  0 /**< #PARAM_SELECT_CAMERA value */
#define SELECT_BACK_CAMERA   1 /**< #PARAM_SELECT_CAMERA value */
#define SELECT_STEREO_CAMERA 2 /**< #PARAM_SELECT_CAMERA value */

/**< @} */

/**
 * Container for parameter set requests send from Java code.
 */
class ParamSetRequest
{
public:
    /**
     * Default constructor. Creates parameter with invalid id and no data.
     */
    ParamSetRequest( void )
    {
        m_id = -1;
        m_dataSize = 0;
        m_data = 0;
    }

    /**
     * Creates a new parameter set request with specific id and value.
     * @see param_set
     * @param param parameter id
     * @param data pointer to serialized parameter value
     * @param dataSize the size in bytes of the serialized parameter value
     */
    ParamSetRequest( int param, const void * data, int dataSize )
    {
        m_id = param;
        m_dataSize = dataSize;
        m_data = new uchar[m_dataSize];
        memcpy( m_data, data, m_dataSize );
    }

    /**
     * Default copy constructor.
     * @param instance ParamSetRequest instance to copy from
     */
    ParamSetRequest( const ParamSetRequest & instance )
    {
        // copy constructor
        m_id = instance.m_id;
        m_dataSize = instance.m_dataSize;
        m_data = new uchar[m_dataSize];
        memcpy( m_data, instance.m_data, m_dataSize );
    }

    /**
     * Assignment operator. Copies parameter id and value
     * to another ParamSetRequest instance.
     */
    ParamSetRequest & operator=( const ParamSetRequest & instance )
    {
        m_id = instance.m_id;
        m_dataSize = instance.m_dataSize;
        if ( m_data != 0 )
        {
            delete[] m_data;
        }

        m_data = new uchar[m_dataSize];
        memcpy( m_data, instance.m_data, m_dataSize );

        return *this;
    }

    /**
     * Default destructor.
     */
    ~ParamSetRequest( void )
    {
        if ( m_data != 0 )
        {
            delete[] m_data;
        }
    }

    /**
     * Gets the parameter id. @see param_set
     * @return parameter id
     */
    int getId( void )
    {
        return m_id;
    }

    /**
     * Gets the pointer to parameter raw data.
     * @return pointer to parameter data
     */
    uchar * getData( void )
    {
        return m_data;
    }

    /**
     * Casts parameter data to an integer and returns its value. The function
     * does not perform any sanity checks, so its up to the caller to make sure
     * the data is big enough to be interpreted as an integer.
     * @return integer value of data
     */
    int getDataAsInt( void )
    {
        return (( int * ) m_data )[0];
    }

    /**
     * Gets the parameter data size in bytes.
     * @return parameter data size in bytes
     */
    int getDataSize( void )
    {
        return m_dataSize;
    }

private:
    int m_id;
    uchar * m_data; // serialized param value
    int m_dataSize;
};

#endif

