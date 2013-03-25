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
/**
 * @mainpage
 * The FCameraPRO application communicates with FCam API through FCamInterface class
 * which is a JNI bridge between java and native (FCam) code. {@link FCamInterface.cpp}
 * contains the implementation of java native methods. The preview and full-resolution
 * capture is realized in a separate thread created during FCamInterface construction
 * (see {@link Java_com_nvidia_fcamerapro_FCamInterface_init()}). To change capture
 * settings, java code calls native methods that send parameter set requests to the
 * work {@link FCAM_INTERFACE_DATA#requestQueue queue}. These requests are resolved
 * inside the worker thread before capture.
 *
 * Captured images are written out asynchronously (in a separate thread) by {@link AsyncImageWriter}.
 *
 * \file
 * FCamInterface contains native implementation (JNI) of
 * com.nvidia.fcamerapro.FCamInterface class methods.
 */

#include <jni.h>
#include <string>
#include <FCam/Tegra/hal/SharedBuffer.h>

//#define USE_GL_TEXTURE_UPLOAD

#define GL_GLEXT_PROTOTYPES
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "WorkQueue.h"
#include "TripleBuffer.h"
#include "ParamSetRequest.h"
#include "AsyncImageWriter.h"
#include "Camera.h"
#include "ParamStat.h"
#include "HPT.h"
#include "Utils.h"
#include "GLWrapper.h"

//#define MEASURE_JITTER

#define BACK_PREVIEW_IMAGE_WIDTH  960 /**< Back camera preview image width in pixels */
#define BACK_PREVIEW_IMAGE_HEIGHT 720 /**< Back camera preview image height in pixels */

#define FRONT_PREVIEW_IMAGE_WIDTH  960 /**< Front camera preview image width in pixels */
#define FRONT_PREVIEW_IMAGE_HEIGHT 720 /**< Front camera preview image height in pixels */

#define STEREO_PREVIEW_IMAGE_WIDTH  960 /**< Stereo camera preview image width in pixels */
#define STEREO_PREVIEW_IMAGE_HEIGHT 720 /**< Stereo camera preview image height in pixels */

// TODO: need to keep preview buffer resolution constant -> SharedBuffer dealloc bug
//#define FRONT_PREVIEW_IMAGE_WIDTH  640 /**< Front camera preview image width in pixels */
//#define FRONT_PREVIEW_IMAGE_HEIGHT 480 /**< Front camera preview image height in pixels */
//
//#define STEREO_PREVIEW_IMAGE_WIDTH  1280 /**< Stereo camera preview image width in pixels */
//#define STEREO_PREVIEW_IMAGE_HEIGHT 720 /**< Stereo camera preview image height in pixels */

#define TOUCH_PATCH_SIZE     15 /**< The size of the patch used in local white balancing */

#define FPS_UPDATE_PERIOD 500 /**< FPS estimation interval (in msec) */
#define FPS_JITTER_CAP    500 /**< FPS estimation outlayer threshold (in msec) */

// TODO: Remove when added to glext.h

/* GL_OES_egl_image_external */
#ifndef GL_OES_egl_image_external
#define GL_OES_egl_image_external 1
#define GL_TEXTURE_EXTERNAL_OES                         0x8D65
#define GL_SAMPLER_EXTERNAL_OES                         0x8D66
#define GL_TEXTURE_BINDING_EXTERNAL_OES                 0x8D67
#define GL_REQUIRED_TEXTURE_IMAGE_UNITS_OES             0x8D68
#endif

// ==========================================================================================
// NATIVE APP STATIC DATA
// ==========================================================================================

typedef struct S_FCAM_INTERFACE_DATA
{
    JavaVM * javaVM; /**< Pointer to Java VM instance */
    jobject fcamInstanceRef; /**< Reference to FCamInterface instance */
    jobject fcamClassRef; /**< Reference to Java FCamInterface class */
    jmethodID notifyCaptureStart; /**< Reference to FCamInterface.notifyCaptureStart() method */
    jmethodID notifyCaptureComplete; /**< Reference to FCamInterface.notifyCaptureComplete() method */
    jmethodID notifyFileSystemChange; /**< Reference to FCamInterface.notifyFileSystemChange() method */
    jmethodID notifyPreviewParamChange; /**< Reference to FCamInterface.notifyParamChange() method */

    pthread_t appThread; /**< FCamInterface thread handler */

    class Camera * currentCamera; /**< Encapsulation of FCam objects related to capture */
#ifdef USE_GL_TEXTURE_UPLOAD
    uchar * frameDataYUV;
    uint * frameDataRGBA;
    int frameWidth, frameHeight;
#else
    class PreviewBuffer * previewBuffer; /**< Encapsulation of preview buffers */
    pthread_mutex_t renderingThreadLock;
#endif
    int previewBufferTexId; /**< OpenGL texture id that is currently locked in the Java side */

    WorkQueue<ParamSetRequest> requestQueue; /**< Work queue with tasks from the Java side */

    Camera::CaptureState previousState; /**< Capture state for previous frame */
    pthread_mutex_t previousStateLock; /**< Mutex used in exclusive modification of #previousState */

    float captureFps; /**< Capture rate in frames per second */
    bool isCapturing; /**< Is capturing (0 - no, 1 - yes) */
    bool isViewerActive; /**< Is preview capture active (0 - off, 1 - on) */
    bool isGLInitDone; /**< Has OpenGL initialization been done? (0 - no, 1 - yes) */
} FCAM_INTERFACE_DATA;

static FCAM_INTERFACE_DATA * sAppData; /**< FCam worker thread data */

static void * FCamAppThread( void * tdata );

// ==========================================================================================
// PUBLIC JNI FUNCTIONS
// ==========================================================================================

/**
 * Provides triple buffer functionality for preview image
 */
class PreviewBuffer : public TripleBuffer<FCam::Tegra::Hal::SharedBuffer>
{
public:
    /**
     * Constructs hardware accelerated triple-buffered preview container (directly accessible from GPU).
     * @param width preview image width in pixels
     * @param height preview image height in pixels
     */
    PreviewBuffer( int width, int height )
    {
        // init triple buffering
        m_width = width;
        m_height = height;

        for ( int i = 0; i < 3; i++ )
        {
            m_viewBuffers[i] = new FCam::Tegra::Hal::SharedBuffer( width, height, FCam::Tegra::Hal::SharedBuffer::YUV420p );
        }

        m_backBuffer = m_viewBuffers[0];
        m_frontBuffer = m_viewBuffers[1];
        m_spareBuffer = m_viewBuffers[2];
    }


    /**
     * Default destructor
     */
    ~PreviewBuffer( void )
    {
        for ( int i = 0; i < 3; i++ )
        {
            delete m_viewBuffers[i];
        }
    }

    int width( void )
    {
        return m_width;
    }
    int height( void )
    {
        return m_height;
    }

private:
    FCam::Tegra::Hal::SharedBuffer * m_viewBuffers[3]; /**< FCam hardware surfaces (CPU/GPU sharing) */
    int m_width, m_height;
};


extern "C" {

    /**
     * Acquires preview frame texture id. The texture binding target can be
     * determined with {@link #Java_com_nvidia_fcamerapro_FCamInterface_getViewerTextureTarget()}.
     * After drawing finishes the texture ownership should be transferred back to the native code by
     * calling {@link #Java_com_nvidia_fcamerapro_FCamInterface_unlockViewerTexture()}.
     *
     * @param env pointer to Java VM
     * @param thiz reference to FCamInterface class instance
     * @return OpenGL texture handler or -1 if failed
     */
    JNIEXPORT int JNICALL Java_com_nvidia_fcamerapro_FCamInterface_lockViewerTexture( JNIEnv * env, jobject thiz )
    {
        if ( sAppData->previewBufferTexId >= 0 )
        {
            return sAppData->previewBufferTexId;
        }

        pthread_mutex_lock( &sAppData->renderingThreadLock );

        if ( sAppData->currentCamera == 0 )
        {
            sAppData->previewBufferTexId = -1;
        }
        else
        {
#ifdef USE_GL_TEXTURE_UPLOAD
            const int width = sAppData->currentCamera->width();
            const int height = sAppData->currentCamera->height();
            const int isize = width * height;

            if ( sAppData->frameDataYUV == 0 )
            {
                sAppData->frameDataYUV = new uchar[isize * 3 / 2];
                sAppData->frameDataRGBA = new uint[isize];
                sAppData->frameWidth = width;
                sAppData->frameHeight = height;

                memset( sAppData->frameDataYUV, 0, isize * 3 / 2 );
            }

            if ( sAppData->frameWidth != width || sAppData->frameHeight != height )
            {
                delete sAppData->frameDataYUV;
                delete sAppData->frameDataRGBA;

                sAppData->frameDataYUV = new uchar[isize * 3 / 2];
                sAppData->frameDataRGBA = new uint[isize];
                sAppData->frameWidth = width;
                sAppData->frameHeight = height;

                memset( sAppData->frameDataYUV, 0, isize * 3 / 2 );
            }


            // convert YUV420p to RGBA8888
            uchar * src = sAppData->frameDataYUV;
            uint * dst = sAppData->frameDataRGBA;

            int uvindex = isize;
            int yindex = 0;
            for ( int y = 0; y < height; y++ )
            {
                for ( int x = 0; x < width; x++ )
                {
                    int y = src[yindex];
                    int u = src[uvindex + ( x >> 1 )] - 128;
                    int v = src[uvindex + ( isize >> 2 ) + ( x >> 1 )] - 128;

                    int r = y + (( v * 91881 ) >> 16 );
                    int g = y - (( u * 22554 + v * 46802 ) >> 16 );
                    int b = y + (( u * 112853 ) >> 16 );

                    if ( r < 0 )
                    {
                        r = 0;
                    }
                    else if ( r > 255 )
                    {
                        r = 255;
                    }

                    if ( g < 0 )
                    {
                        g = 0;
                    }
                    else if ( g > 255 )
                    {
                        g = 255;
                    }

                    if ( b < 0 )
                    {
                        b = 0;
                    }
                    else if ( b > 255 )
                    {
                        b = 255;
                    }

                    dst[yindex++] = ( r << 16 ) | ( g << 8 ) | b | 0xff000000;
                }

                if (( y & 0x1 ) != 0 )
                {
                    uvindex += width >> 1;
                }
            }


            GLuint tid;
            glGenTextures( 1, &tid );
            glBindTexture( GL_TEXTURE_2D, tid );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
            glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, sAppData->frameDataRGBA );
#else

            if ( sAppData->previewBuffer == 0 )
            {
                sAppData->previewBuffer = new PreviewBuffer( sAppData->currentCamera->width(), sAppData->currentCamera->height() );
            }

            // reallocate if capture resolution changed
            if ( sAppData->previewBuffer->width() != sAppData->currentCamera->width() ||
                 sAppData->previewBuffer->height() != sAppData->currentCamera->height() )
            {
                delete sAppData->previewBuffer;
                sAppData->previewBuffer = new PreviewBuffer( sAppData->currentCamera->width(), sAppData->currentCamera->height() );
            }

            FCam::Tegra::Hal::SharedBuffer * buffer = sAppData->previewBuffer->swapFrontBuffer();

            GLuint tid;
            glGenTextures( 1, &tid );
            glBindTexture( GL_TEXTURE_EXTERNAL_OES, tid );
            glEGLImageTargetTexture2DOES( GL_TEXTURE_EXTERNAL_OES, ( GLeglImageOES ) buffer->getEGLImage() );
            glTexParameteri( GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
            glTexParameteri( GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
            glTexParameteri( GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
            glTexParameteri( GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
#endif

            sAppData->previewBufferTexId = tid;
        }

        pthread_mutex_unlock( &sAppData->renderingThreadLock );

        return sAppData->previewBufferTexId;
    }

    /**
     * Gets preview frame texture target type.
     *
     * @param env pointer to Java VM
     * @param thiz reference to FCamInterface class instance
     */
    JNIEXPORT int JNICALL Java_com_nvidia_fcamerapro_FCamInterface_getViewerTextureTarget( JNIEnv * env, jobject thiz )
    {
#ifdef USE_GL_TEXTURE_UPLOAD
        return GL_TEXTURE_2D;
#else
        return GL_TEXTURE_EXTERNAL_OES;
#endif
    }

    /**
     * Releases preview texture id. The application should not use the texture
     * after this function has returned.
     *
     * @param env pointer to Java VM
     * @param thiz reference to FCamInterface class instance
     */
    JNIEXPORT void JNICALL Java_com_nvidia_fcamerapro_FCamInterface_unlockViewerTexture( JNIEnv * env, jobject thiz )
    {
        if ( sAppData->previewBufferTexId >= 0 )
        {
            glDeleteTextures( 1, ( GLuint * )&sAppData->previewBufferTexId );
            sAppData->previewBufferTexId = -1;
        }
    }

    /**
     * Sends a parameter set (id, value) command to the message queue. The commands in the queue
     * are resolved and executed in submission order by {@link #FCamAppThread()}.
     *
     * @param env pointer to Java VM
     * @param thiz reference to FCamInterface class instance
     * @param param holds the parameter id (see {@link param_set parameter identifiers})
     * @param value contains parameter value
     */
    JNIEXPORT void JNICALL Java_com_nvidia_fcamerapro_FCamInterface_setParamInt( JNIEnv * env, jobject thiz, jint param, jint value )
    {
        sAppData->requestQueue.produce( ParamSetRequest( param, &value, sizeof( int ) ) );
    }

    /**
     * Sends a parameter set (id, value) command to the message queue. The commands in the queue
     * are resolved and executed in submission order by {@link #FCamAppThread()}.
     *
     * @param env pointer to Java VM
     * @param thiz reference to FCamInterface class instance
     * @param param holds the parameter id (see {@link param_set parameter identifiers})
     * @param value contains parameter value
     */
    JNIEXPORT void JNICALL Java_com_nvidia_fcamerapro_FCamInterface_setParamIntArray( JNIEnv * env, jobject thiz, jint param, jintArray value )
    {
        int arraySize;
        int * arrayData;

        int paramId = param & 0xffff;
        int pictureId = param >> 16;

        switch ( paramId )
        {
            default:
                ERROR( "setParamIntArray(%i): received unsupported param id!", paramId );
        }
    }

    JNIEXPORT int JNICALL Java_com_nvidia_fcamerapro_FCamInterface_getParamInt( JNIEnv * env, jobject thiz, jint param )
    {
        pthread_mutex_lock( &sAppData->previousStateLock );

        Camera::CaptureState * previousShot = &sAppData->previousState;
        int rval = -1;

        int paramId = param & 0xffff;
        int pictureId = param >> 16;

        switch ( paramId )
        {
            case PARAM_PREVIEW_AUTO_EXPOSURE_ON:
                rval = previousShot->preview.autoExposure ? 1 : 0;
                break;
            case PARAM_PREVIEW_AUTO_GAIN_ON:
                rval = previousShot->preview.autoGain ? 1 : 0;
                break;
            case PARAM_PREVIEW_AUTO_WB_ON:
                rval = previousShot->preview.autoWB ? 1 : 0;
                break;
            case PARAM_PREVIEW_AUTO_FOCUS_ON:
                rval = previousShot->preview.autoFocus ? 1 : 0;
                break;
            case PARAM_BURST_SIZE:
                rval = previousShot->pendingImagesCount;
                break;
            case PARAM_VIEWER_ACTIVE:
                rval = sAppData->isViewerActive;
                break;
            case PARAM_TAKE_PICTURE:
                rval = sAppData->isCapturing;
                break;
            case PARAM_SELECT_CAMERA:
                if ( sAppData->currentCamera != 0 )
                {
                    switch ( sAppData->currentCamera->m_currentMode )
                    {
                        case Camera::Front:
                            rval = SELECT_FRONT_CAMERA;
                            break;
                        case Camera::Back:
                            rval = SELECT_BACK_CAMERA;
                            break;
                        case Camera::Stereo:
                            rval = SELECT_STEREO_CAMERA;
                            break;
                    }
                }
                break;
            default:
                ERROR( "received unsupported param id (%i)!", paramId );
        }

        pthread_mutex_unlock( &sAppData->previousStateLock );

        return rval;
    }

    JNIEXPORT void JNICALL Java_com_nvidia_fcamerapro_FCamInterface_getParamIntArray( JNIEnv * env, jobject thiz, jint param, jintArray value )
    {
        int arraySize;
        int * arrayData;

        pthread_mutex_lock( &sAppData->previousStateLock );

        int paramId = param & 0xffff;
        int pictureId = param >> 16;

        switch ( paramId )
        {
            default:
                ERROR( "getParamIntArray(%i): received unsupported param id!", paramId );
        }

        pthread_mutex_unlock( &sAppData->previousStateLock );
    }

    /**
     * Sends a parameter set (id, value) command to the message queue. The commands in the queue
     * are resolved and executed in submission order by {@link #FCamAppThread()}.
     *
     * @param env pointer to Java VM
     * @param thiz reference to FCamInterface class instance
     * @param param holds the parameter id (see {@link param_set parameter identifiers})
     * @param value contains parameter value
     */
    JNIEXPORT void JNICALL Java_com_nvidia_fcamerapro_FCamInterface_setParamFloat( JNIEnv * env, jobject thiz, jint param, jfloat value )
    {
        sAppData->requestQueue.produce( ParamSetRequest( param, &value, sizeof( float ) ) );
    }

    /**
     * Sends a parameter set (id, value) command to the message queue. The commands in the queue
     * are resolved and executed in submission order by {@link #FCamAppThread()}.
     *
     * @param env pointer to Java VM
     * @param thiz reference to FCamInterface class instance
     * @param param holds the parameter id (see {@link param_set parameter identifiers})
     * @param value contains parameter value
     */
    JNIEXPORT void JNICALL Java_com_nvidia_fcamerapro_FCamInterface_setParamFloatArray( JNIEnv * env, jobject thiz, jint param, jfloatArray value )
    {
        int arraySize;
        float * arrayData;

        int paramId = param & 0xffff;
        int pictureId = param >> 16;

        switch ( paramId )
        {
            case PARAM_SHOT:
                arraySize = env->GetArrayLength( value );
                if ( arraySize != 5 )
                {
                    ERROR( "setParamFloatArray(PARAM_SHOT): incorrect array size!" );
                    return;
                }

                arrayData = env->GetFloatArrayElements( value, 0 );
                sAppData->requestQueue.produce( ParamSetRequest( param, arrayData, arraySize * sizeof( float ) ) );
                env->ReleaseFloatArrayElements( value, arrayData, 0 );
                break;

            case PARAM_FOCUS_ON_TOUCH:
            case PARAM_WB_ON_TOUCH:
                arraySize = env->GetArrayLength( value );
                if ( arraySize != 2 )
                {
                    ERROR( "setParamFloatArray(%s): incorrect array size!", paramId == PARAM_FOCUS_ON_TOUCH ? "PARAM_FOCUS_ON_TOUCH" : "PARAM_WB_ON_TOUCH" );
                    return;
                }

                arrayData = env->GetFloatArrayElements( value, 0 );
                sAppData->requestQueue.produce( ParamSetRequest( param, arrayData, arraySize * sizeof( float ) ) );
                env->ReleaseFloatArrayElements( value, arrayData, 0 );
                break;

            default:
                ERROR( "setParamFloatArray(%i): received unsupported param id!", paramId );
        }

    }

    /**
     * Gets the current value of a parameter. If the parameter is related to
     * capture settings the actual value reflects previous frame state. Consequently,
     * querying capture parameter with this function might not return the value
     * which has been set before with a parameter set function.
     *
     * @param env pointer to Java VM
     * @param thiz reference to FCamInterface class instance
     * @param param holds the parameter id (see {@link param_set parameter identifiers})
     * @return parameter value
     */
    JNIEXPORT float JNICALL Java_com_nvidia_fcamerapro_FCamInterface_getParamFloat( JNIEnv * env, jobject thiz, jint param )
    {
        Camera::CaptureState * previousShot = &sAppData->previousState;
        float rval = -1.0f;

        int paramId = param & 0xffff;
        int pictureId = param >> 16;

        switch ( paramId )
        {
            case PARAM_CAPTURE_FPS:
                rval = sAppData->captureFps;
                break;
            case PARAM_PREVIEW_EXPOSURE:
                rval = previousShot->preview.autoExposure ? previousShot->preview.evaluated.exposure : previousShot->preview.user.exposure;
                break;
            case PARAM_PREVIEW_FOCUS:
                rval = previousShot->preview.autoFocus ? previousShot->preview.evaluated.focus : previousShot->preview.user.focus;
                break;
            case PARAM_PREVIEW_GAIN:
                rval = previousShot->preview.autoGain ? previousShot->preview.evaluated.gain : previousShot->preview.user.gain;
                break;
            case PARAM_PREVIEW_WB:
                rval = previousShot->preview.autoWB ? previousShot->preview.evaluated.wb : previousShot->preview.user.wb;
                break;
            default:
                ERROR( "getParamFloat(%i): received unsupported param id!", paramId );
        }

        return rval;
    }

    /**
     * Gets the current value of a parameter. If the parameter is related to
     * capture settings the actual value reflects previous frame state. Consequently,
     * querying capture parameter with this function might not return the value
     * which has been set before with a parameter set function.
     *
     * @param env pointer to Java VM
     * @param thiz reference to FCamInterface class instance
     * @param param holds the parameter id (see {@link param_set parameter identifiers})
     * @param value contains reference to parameter value
     */
    JNIEXPORT void JNICALL Java_com_nvidia_fcamerapro_FCamInterface_getParamFloatArray( JNIEnv * env, jobject thiz, jint param, jfloatArray value )
    {
        int arraySize;
        float * arrayData;

        pthread_mutex_lock( &sAppData->previousStateLock );

        int paramId = param & 0xffff;
        int pictureId = param >> 16;

        switch ( paramId )
        {
            case PARAM_SHOT:
                arraySize = env->GetArrayLength( value );
                if ( arraySize != 5 )
                {
                    ERROR( "getParamFloatArray(PARAM_SHOT): incorrect shot array size!" );
                    return;
                }

                arrayData = env->GetFloatArrayElements( value, 0 );

                arrayData[SHOT_PARAM_EXPOSURE] = sAppData->previousState.pendingImages[pictureId].exposure;
                arrayData[SHOT_PARAM_FOCUS] = sAppData->previousState.pendingImages[pictureId].focus;
                arrayData[SHOT_PARAM_GAIN] = sAppData->previousState.pendingImages[pictureId].gain;
                arrayData[SHOT_PARAM_WB] = sAppData->previousState.pendingImages[pictureId].wb;
                arrayData[SHOT_PARAM_FLASH] = sAppData->previousState.pendingImages[pictureId].flashOn;

                env->ReleaseFloatArrayElements( value, arrayData, 0 );
                break;

            case PARAM_LUMINANCE_HISTOGRAM:
                arraySize = env->GetArrayLength( value );
                if ( arraySize != HISTOGRAM_SIZE )
                {
                    ERROR( "getParamFloatArray(PARAM_LUMINANCE_HISTOGRAM): incorrect array size!" );
                    return;
                }

                arrayData = env->GetFloatArrayElements( value, 0 );

                for ( int i = 0; i < HISTOGRAM_SIZE; i++ )
                {
                    arrayData[i] = sAppData->previousState.preview.histogramData[i];
                }

                env->ReleaseFloatArrayElements( value, arrayData, 0 );
                break;

            default:
                ERROR( "getParamFloatArray(%i): received unsupported param id!", paramId );
        }

        pthread_mutex_unlock( &sAppData->previousStateLock );
    }

    /**
     * Sends a parameter set (id, value) command to the message queue. The commands in the queue
     * are resolved and executed in submission order by {@link #FCamAppThread()}.
     *
     * @param env pointer to Java VM
     * @param thiz reference to FCamInterface class instance
     * @param param holds the parameter id (see {@link param_set parameter identifiers})
     * @param value contains parameter value
     */
    JNIEXPORT void JNICALL Java_com_nvidia_fcamerapro_FCamInterface_setParamString( JNIEnv * env, jobject thiz, jint param, jstring value )
    {
        const char * str = ( const char * ) env->GetStringUTFChars( value, 0 );
        sAppData->requestQueue.produce( ParamSetRequest( param, str, strlen( str ) + 1 ) );
        env->ReleaseStringUTFChars( value, str );
    }

    /**
     * Gets the current value of a parameter. If the parameter is related to
     * capture settings the actual value reflects previous frame state. Consequently,
     * querying capture parameter with this function might not return the value
     * which has been set before with a parameter set function.
     *
     * @param env pointer to Java VM
     * @param thiz reference to FCamInterface class instance
     * @param param holds the parameter id (see {@link param_set parameter identifiers})
     * @return parameter value
     */
    JNIEXPORT jstring JNICALL FCamInterface_getParamString( JNIEnv * env, jobject thiz, jint param )
    {
        return 0;
    }

    /**
     * Initialization of the capture process. Creates a worker thread which uses FCam API
     * to capture preview or full-resolution images in an infinite loop. Each capture
     * action is followed by queue resolve stage where capture parameter change requests
     * coming from other (Java UI) threads are executed.
     * @param env pointer to Java VM
     * @param thiz reference to FCamInterface class instance
     */
    JNIEXPORT void JNICALL Java_com_nvidia_fcamerapro_FCamInterface_init( JNIEnv * env, jobject thiz )
    {
        sAppData->fcamInstanceRef = env->NewGlobalRef( thiz );

        // launch the work thread
        pthread_create( &sAppData->appThread, 0, FCamAppThread, sAppData );
    }

#include "GLWrapper.h"

    using namespace GL;

    class EdgeDetectShader : public GL::FragmentShader
    {
    public:
        ShaderVar<SV_float> threshold;
        ShaderVar<SV_sampler2D> input;

        EdgeDetectShader( void ) : FragmentShader( "edge_detect.fs" )
        {
            threshold = var<SV_float>( "edge_threshold" );
            input = var<SV_sampler2D>( "input_texture" );
        }
    };

    void detectEdges( Texture & output, Texture & input )
    {
        EdgeDetectShader edgeDetectShader;

        edgeDetectShader.input = input;
        edgeDetectShader.threshold = 3.0f;
        //    LOG("%f\n", edgeDetectShader.threshold.value());
        //
        //    edgeDetectShader.var<SV_float>("edge_threshold") = 1.0f;
        //    LOG("%f\n", edgeDetectShader.threshold.value());
        //
        //    edgeDetectShader.threshold = 5.0f;
        //    LOG("%f\n", edgeDetectShader.var<SV_float>("edge_threshold").value());

        FrameBuffer fbo;
        fbo.color( 0 ) = output;
        fbo.render( edgeDetectShader );
    }

    //void detectEdges(Texture &output, Texture &input) {
    //  Shader edgeDetectShader("edge_detect.fs");
    //  edgeDetectShader.var<SV_float>("edge_threshold") = 3.0f;
    //  edgeDetectShader.var<SV_sampler2D>("input_texture") = input;
    //
    //  FrameBuffer fbo;
    //  fbo.color[0] = output;
    //  fbo.render(edgeDetectShader);
    //}

    /**
     * Called when JNI library is loaded. Performs initialization of worker thread data.
     * @param vm pointer to Java VM
     * @param reserved opaque data pointer
     */
    JNIEXPORT jint JNICALL JNI_OnLoad( JavaVM * vm, void * reserved )
    {
        JNIEnv * env;

        LOG( "JNI_OnLoad called" );
        if ( vm->GetEnv(( void ** ) &env, JNI_VERSION_1_4 ) != JNI_OK )
        {
            LOG( "Failed to get the environment using GetEnv()" );
            return -1;
        }

        jclass fcamClassRef = env->FindClass( "com/nvidia/fcamerapro/FCamInterface" );

        // init app data
        sAppData = new FCAM_INTERFACE_DATA;
        sAppData->javaVM = vm;
        sAppData->notifyCaptureStart = env->GetMethodID( fcamClassRef, "notifyCaptureStart", "()V" );
        sAppData->notifyCaptureComplete = env->GetMethodID( fcamClassRef, "notifyCaptureComplete", "()V" );
        sAppData->notifyFileSystemChange = env->GetMethodID( fcamClassRef, "notifyFileSystemChange", "()V" );
        sAppData->notifyPreviewParamChange = env->GetMethodID( fcamClassRef, "notifyPreviewParamChange", "(I)V" );
        sAppData->fcamClassRef = env->NewGlobalRef( fcamClassRef );
        env->DeleteLocalRef( fcamClassRef );

        pthread_mutex_init( &sAppData->previousStateLock, 0 );

        // flags
        sAppData->isCapturing = false;
        sAppData->isViewerActive = false;
        sAppData->isGLInitDone = false;

        sAppData->currentCamera = 0;
#ifdef USE_GL_TEXTURE_UPLOAD
        sAppData->frameDataYUV = 0;
        sAppData->frameDataRGBA = 0;
#else
        sAppData->previewBuffer = 0;
        sAppData->previewBufferTexId = -1;
        pthread_mutex_init( &sAppData->renderingThreadLock, 0 );
#endif

        return JNI_VERSION_1_4;
    }

    /**
     * Called when JNI library in unloaded.
     * @param vm pointer to Java VM
     * @param reserved opaque data pointer
     */
    JNIEXPORT void JNICALL JNI_OnUnload( JavaVM * vm, void * reserved )
    {
        JNIEnv * env;

        LOG( "JNI_OnLoad called" );
        if ( vm->GetEnv(( void ** ) &env, JNI_VERSION_1_4 ) != JNI_OK )
        {
            LOG( "Failed to get the environment using GetEnv()" );
            return;
        }

        env->DeleteGlobalRef( sAppData->fcamClassRef );
    }

}

// ==========================================================================================
// FCAM NATIVE THREAD
// ==========================================================================================

/**
 * Called by an instance AsyncImageWriter to notify the worker thread about file system
 * modification, specifically when new images have been written out. The worker thread
 * calls corresponding FCamInterface java method to notify UI components about the change.
 */
static void OnFileSystemChanged( void )
{
    // called from async image writer thread -> queue request to resolve in the main app thread
    const int value = 1;
    sAppData->requestQueue.produce( ParamSetRequest( PARAM_PRIV_FS_CHANGED, &value, sizeof( int ) ) );
}

/**
 * Gets the average value of a color patch
 * @param idata pointer to color channel patch
 * @param rowStride patch row size in bytes
 * @param patchSize patch size in pixels
 * @return average value inside the patch
 */
static int GetChannelPatchAverage( unsigned char * idata, const int rowStride, const int patchSize )
{
    int average = 0;
    for ( int y = 0; y < patchSize; y++ )
    {
        for ( int x = 0; x < patchSize; x++ )
        {
            average += idata[x];
        }
        idata += rowStride;
    }

    return average / ( patchSize * patchSize );
}

/**
 * Gets the color temperature of a patch at specific location inside YUV420p frame.
 * @param currentTemp current white point temparature (sRGB color space)
 * @param idata pointer to YUV420p image data
 * @param tx x position of a patch center inside the image
 * @param ty y position of a patch center inside the image
 * @param width frame width in pixels
 * @param height frame height in pixels
 */
static int GetLocalColorTemparature( int currentTemp, unsigned char * idata, int tx, int ty, int width, int height )
{
    if ( tx < ( TOUCH_PATCH_SIZE >> 1 ) )
    {
        tx = TOUCH_PATCH_SIZE >> 1;
    }

    if ( ty < ( TOUCH_PATCH_SIZE >> 1 ) )
    {
        ty = TOUCH_PATCH_SIZE >> 1;
    }

    if ( tx >= ( width - ( TOUCH_PATCH_SIZE >> 1 ) ) )
    {
        tx = width - ( TOUCH_PATCH_SIZE >> 1 ) - 1;
    }

    if ( ty >= ( height - ( TOUCH_PATCH_SIZE >> 1 ) ) )
    {
        ty = height - ( TOUCH_PATCH_SIZE >> 1 ) - 1;
    }

    int offset = width * ( ty - ( TOUCH_PATCH_SIZE >> 1 ) ) + tx - ( TOUCH_PATCH_SIZE >> 1 );
    int y = GetChannelPatchAverage( idata + offset, width, TOUCH_PATCH_SIZE );

    offset = ( width >> 1 ) * (( ty >> 1 ) - ( TOUCH_PATCH_SIZE >> 2 ) ) + ( tx >> 1 ) - ( TOUCH_PATCH_SIZE >> 2 );
    int isize = width * height;
    int cb = GetChannelPatchAverage( idata + isize + offset, width >> 1, TOUCH_PATCH_SIZE >> 1 );
    int cr = GetChannelPatchAverage( idata + isize + ( isize >> 2 ) + offset, width >> 1, TOUCH_PATCH_SIZE >> 1 );

    // XXX: bug: need to convert averaged pixel color to d65 srgb space before calling the function below
    int temp = GetColorTemparatureYCbCr( currentTemp, y, cb, cr );
    LOG( "GetLocalColorTemparature(): y: %i cb: %i cr: %i temp: %iK", y, cb, cr, temp );
    return temp;
}

/**
 * FCam worker thread body. This thread is created in {@link Java_com_nvidia_fcamerapro_FCamInterface_init()}
 * which is called by FCamInterface constructor and is responsible for image and preview capture with FCam API.
 *
 * @param ptr pointer to thread local data
 */

static void * FCamAppThread( void * ptr )
{
    FCAM_INTERFACE_DATA * tdata = ( FCAM_INTERFACE_DATA * )ptr;
    Timer timer;

    JNIEnv * env;
    tdata->javaVM->AttachCurrentThread( &env, 0 );

    //    volatile bool __debug_flag = true;
    //    while (__debug_flag) {
    //
    //    }

    {
        GL::Texture a, b;
        detectEdges( a, b );
    }

    // async writer is initialized on the first PARAM_OUTPUT_DIRECTORY set request
    AsyncImageWriter * writer = 0;

    // init fcam
    Camera * camera = new Camera( BACK_PREVIEW_IMAGE_WIDTH, BACK_PREVIEW_IMAGE_HEIGHT, Camera::Back );
    sAppData->currentCamera = camera;
    memcpy( &sAppData->previousState, &camera->m_currentState, sizeof( Camera::CaptureState ) );

    FCam::Tegra::Shot shot;

    // fps stat init
    tdata->captureFps = 30; // assuming 30hz
    double fpsUpdateTime = timer.get();
    int frameCount = 0;

    // local task queue
    std::queue<ParamSetRequest> taskQueue;
    ParamSetRequest task;

#ifdef MEASURE_JITTER
    ParamStat stat;
    double nextFrameTime = fpsUpdateTime + ( 1000.0f / tdata->captureFps );
#endif

    for ( ;; )
    {
        // TODO: think about removing the queue, double buffered param struct would be better/faster
        int touchX = 0, touchY = 0;
        enum { TOUCH_ACTION_NONE, TOUCH_ACTION_WHITE_BALANCE, TOUCH_ACTION_FOCUS } touchAction = TOUCH_ACTION_NONE;

        // copy tasks to local queue
        sAppData->requestQueue.consumeAll( taskQueue );

        while ( !taskQueue.empty() )
        {
            task = taskQueue.front();
            taskQueue.pop();

            bool prevValue;
            int taskId = task.getId() & 0xffff;
            int * taskDataInt = ( int * )task.getData();
            float * taskDataFloat = ( float * )taskDataInt;
            int pictureId = task.getId() >> 16;

            switch ( taskId )
            {
                case PARAM_SHOT:
                    // XXX: fcam bug, exposure bottom cap at 1/1000
                    camera->m_currentState.pendingImages[pictureId].exposure = taskDataFloat[SHOT_PARAM_EXPOSURE];
                    camera->m_currentState.pendingImages[pictureId].focus = taskDataFloat[SHOT_PARAM_FOCUS];
                    camera->m_currentState.pendingImages[pictureId].gain = taskDataFloat[SHOT_PARAM_GAIN];
                    camera->m_currentState.pendingImages[pictureId].wb = taskDataFloat[SHOT_PARAM_WB];
                    camera->m_currentState.pendingImages[pictureId].flashOn = taskDataFloat[SHOT_PARAM_FLASH] > 0.0f;
                    break;
                case PARAM_PREVIEW_EXPOSURE:
                    camera->m_currentState.preview.user.exposure = taskDataFloat[0];
                    break;
                case PARAM_PREVIEW_FOCUS:
                    camera->m_currentState.preview.user.focus = taskDataFloat[0];
                    break;
                case PARAM_PREVIEW_GAIN:
                    camera->m_currentState.preview.user.gain = taskDataFloat[0];
                    break;
                case PARAM_PREVIEW_WB:
                    camera->m_currentState.preview.user.wb = taskDataFloat[0];
                    break;
                case PARAM_PREVIEW_AUTO_EXPOSURE_ON:
                    prevValue = camera->m_currentState.preview.autoExposure;
                    camera->m_currentState.preview.autoExposure = taskDataInt[0] != 0;
                    if ( !prevValue && prevValue ^ camera->m_currentState.preview.autoExposure != 0 )
                    {
                        tdata->previousState.preview.evaluated.exposure = camera->m_currentState.preview.user.exposure;
                    }
                    else
                    {
                        camera->m_currentState.preview.user.exposure = tdata->previousState.preview.evaluated.exposure;
                    }
                    break;
                case PARAM_PREVIEW_AUTO_FOCUS_ON:
                    prevValue = camera->m_currentState.preview.autoFocus;
                    camera->m_currentState.preview.autoFocus = taskDataInt[0] != 0;
                    if ( !prevValue && prevValue ^ camera->m_currentState.preview.autoFocus != 0 )
                    {
                        tdata->previousState.preview.evaluated.focus = camera->m_currentState.preview.user.focus;
                    }
                    else
                    {
                        camera->m_currentState.preview.user.focus = tdata->previousState.preview.evaluated.focus;
                    }
                    break;
                case PARAM_PREVIEW_AUTO_GAIN_ON:
                    prevValue = camera->m_currentState.preview.autoGain;
                    camera->m_currentState.preview.autoGain = taskDataInt[0] != 0;
                    if ( !prevValue && prevValue ^ camera->m_currentState.preview.autoGain != 0 )
                    {
                        tdata->previousState.preview.evaluated.gain = camera->m_currentState.preview.user.gain;
                    }
                    else
                    {
                        camera->m_currentState.preview.user.gain = tdata->previousState.preview.evaluated.gain;
                    }
                    break;
                case PARAM_PREVIEW_AUTO_WB_ON:
                    prevValue = camera->m_currentState.preview.autoWB;
                    camera->m_currentState.preview.autoWB = taskDataInt[0] != 0;
                    if ( !prevValue && prevValue ^ camera->m_currentState.preview.autoWB != 0 )
                    {
                        tdata->previousState.preview.evaluated.wb = camera->m_currentState.preview.user.wb;
                    }
                    else
                    {
                        camera->m_currentState.preview.user.wb = tdata->previousState.preview.evaluated.wb;
                    }
                    break;
                case PARAM_RESOLUTION:
                    break;
                case PARAM_BURST_SIZE:
                    camera->m_currentState.pendingImagesCount = taskDataInt[0];
                    break;
                case PARAM_OUTPUT_FORMAT:
                    break;
                case PARAM_VIEWER_ACTIVE:
                    tdata->isViewerActive = taskDataInt[0] != 0;
                    if ( !tdata->isViewerActive )
                    {
                        camera->m_sensor->stopStreaming();
                    }
                    break;
                case PARAM_OUTPUT_DIRECTORY:
                    // one-time async writer initialization
                    if ( writer == 0 )
                    {
                        writer = new AsyncImageWriter(( char * )task.getData() );
                        writer->setOnFileSystemChangedCallback( OnFileSystemChanged );
                    }
                    break;
                case PARAM_OUTPUT_FILE_ID:
                    AsyncImageWriter::SetFreeFileId( taskDataInt[0] );
                    break;
                case PARAM_TAKE_PICTURE:
                    // take pictures only if we can write them out
                    if ( writer != 0 )
                    {
                        if ( task.getDataAsInt() != 0 )
                        {
                            // capture begin
                            tdata->isCapturing = true;

                            // notify capture start
                            env->CallVoidMethod( tdata->fcamInstanceRef, tdata->notifyCaptureStart );

                            camera->capture( writer );

                            // capture done
                            tdata->isCapturing = false;

                            // notify capture completion
                            env->CallVoidMethod( tdata->fcamInstanceRef, tdata->notifyCaptureComplete );
                        }
                    }
                    break;
                case PARAM_PRIV_FS_CHANGED:
                    if ( taskDataInt[0] != 0 )
                    {
                        // notify fs change
                        env->CallVoidMethod( tdata->fcamInstanceRef, tdata->notifyFileSystemChange );
                    }
                    break;

                case PARAM_SELECT_CAMERA:
                    pthread_mutex_lock( &tdata->renderingThreadLock );
                    switch ( taskDataInt[0] )
                    {
                        case SELECT_FRONT_CAMERA:
                            if ( camera->m_currentMode != Camera::Front )
                            {
                                delete camera;
                                camera = new Camera( FRONT_PREVIEW_IMAGE_WIDTH, FRONT_PREVIEW_IMAGE_HEIGHT, Camera::Front );
                            }
                            break;
                        case SELECT_BACK_CAMERA:
                            if ( camera->m_currentMode != Camera::Back )
                            {
                                delete camera;
                                camera = new Camera( BACK_PREVIEW_IMAGE_WIDTH, BACK_PREVIEW_IMAGE_HEIGHT, Camera::Back );
                            }
                            break;
                        case SELECT_STEREO_CAMERA:
                            if ( camera->m_currentMode != Camera::Stereo )
                            {
                                delete camera;
                                camera = new Camera( STEREO_PREVIEW_IMAGE_WIDTH, STEREO_PREVIEW_IMAGE_HEIGHT, Camera::Stereo );
                            }
                            break;
                    }

                    // update external camera pointer
                    tdata->currentCamera = camera;
                    pthread_mutex_unlock( &tdata->renderingThreadLock );
                    break;

                case PARAM_FOCUS_ON_TOUCH:
                case PARAM_WB_ON_TOUCH:
                    touchX = taskDataFloat[0] * camera->m_previewImage->width();
                    touchY = taskDataFloat[1] * camera->m_previewImage->height();

                    if ( task.getId() == PARAM_FOCUS_ON_TOUCH )
                    {
                        touchAction = TOUCH_ACTION_FOCUS;
                    }
                    else
                    {
                        touchAction = TOUCH_ACTION_WHITE_BALANCE;
                    }
                    break;
                default:
                    ERROR( "TaskDispatch(): received unsupported task id (%i)!", taskId );
            }
        }

        if ( !tdata->isViewerActive )
        {
            // viewer inactive, skip capture
            continue;
        }

        // setup preview shot params
        shot.exposure = camera->m_currentState.preview.autoExposure ? tdata->previousState.preview.evaluated.exposure : camera->m_currentState.preview.user.exposure;
        shot.gain = camera->m_currentState.preview.autoGain ? tdata->previousState.preview.evaluated.gain : camera->m_currentState.preview.user.gain;
        shot.whiteBalance = camera->m_currentState.preview.autoWB ? tdata->previousState.preview.evaluated.wb : camera->m_currentState.preview.user.wb;
        shot.image = *( camera->m_previewImage );
        shot.histogram.enabled = true;
        shot.histogram.region = FCam::Rect( 0, 0, camera->width(), camera->height() );
        shot.fastMode = true;

        if ( !camera->m_autoFocus->idle() )
        {
            shot.sharpness.enabled = true;
        }

        if ( !camera->m_currentState.preview.autoFocus && tdata->previousState.preview.user.focus != camera->m_currentState.preview.user.focus )
        {
            shot.clearActions();
            FCam::Lens::FocusAction focusAction( camera->m_lens );
            focusAction.time = 0;
            focusAction.focus = camera->m_currentState.preview.user.focus;
            shot.addAction( focusAction );
        }

        camera->m_sensor->stream( shot );

        // update param estimates
        FCam::Frame frame = camera->m_sensor->getFrame();

        // clear any actions we have previously defined.
        shot.clearActions();

        switch ( touchAction )
        {
            case TOUCH_ACTION_FOCUS:
                if ( camera->m_currentState.preview.autoFocus && camera->m_autoFocus->idle() )
                {
                    camera->m_autoFocus->startSweep();
                }
                break;
            case TOUCH_ACTION_WHITE_BALANCE:
                GetLocalColorTemparature( shot.whiteBalance, frame.image()( 0, 0 ), touchX, touchY,
                                          frame.image().width(), frame.image().height() );
                break;
        }

        if ( camera->m_currentState.preview.autoExposure || camera->m_currentState.preview.autoGain )
        {
            FCam::autoExpose( &shot, frame, camera->m_sensor->maxGain(), camera->m_sensor->maxExposure(),
                              camera->m_sensor->minExposure(), 0.3 );
            camera->m_currentState.preview.evaluated.exposure = shot.exposure;
            camera->m_currentState.preview.evaluated.gain = shot.gain;
        }

        if ( camera->m_currentState.preview.autoWB )
        {
            FCam::autoWhiteBalance( &shot, frame );
            camera->m_currentState.preview.evaluated.wb = shot.whiteBalance;
        }

        if ( !camera->m_autoFocus->idle() )
        {
            camera->m_autoFocus->update( frame, &shot );
            camera->m_currentState.preview.evaluated.focus = frame["lens.focus"];
        }

        // update histogram data
        const FCam::Histogram & histogram = frame.histogram();

        int maxBinValue = 1;
        for ( int i = 0; i < 64; i++ )
        {
            int currBinValue = histogram( i );
            if ( currBinValue > maxBinValue )
            {
                maxBinValue = currBinValue;
            }
            camera->m_currentState.preview.histogramData[i * 4] = currBinValue;
        }

        float norm = 1.0f / maxBinValue;
        for ( int i = 0; i < 64; i++ )
        {
            camera->m_currentState.preview.histogramData[i * 4] *= norm;
            camera->m_currentState.preview.histogramData[i * 4 + 1] = 0.0f;
            camera->m_currentState.preview.histogramData[i * 4 + 2] = 0.0f;
            camera->m_currentState.preview.histogramData[i * 4 + 3] = 0.0f;
        }

        // update framebuffer
#ifdef USE_GL_TEXTURE_UPLOAD
        if ( tdata->frameDataYUV != 0 )
        {
            memcpy( tdata->frameDataYUV, frame.image()( 0, 0 ), camera->width() * camera->height() * 3 / 2 );
        }
#else
        pthread_mutex_lock( &tdata->renderingThreadLock );
        if ( tdata->previewBuffer != 0 )
        {
            FCam::Image image = frame.image();
            if ( tdata->previewBuffer->width() == image.width() && tdata->previewBuffer->height() == image.height() )
            {
                uchar * src = ( uchar * )image( 0, 0 );
                FCam::Tegra::Hal::SharedBuffer * captureBuffer = tdata->previewBuffer->getBackBuffer();
                uchar * dest = ( uchar * )captureBuffer->lock();

                // TODO: why do we need to shuffle U and V channels?
                int isize = camera->width() * camera->height();
                memcpy( dest, src, isize );
                memcpy( dest + isize, src + isize + ( isize >> 2 ), isize >> 2 );
                memcpy( dest + isize + ( isize >> 2 ), src + isize, isize >> 2 );

                captureBuffer->unlock();
                tdata->previewBuffer->swapBackBuffer();
            }
        }
        pthread_mutex_unlock( &tdata->renderingThreadLock );
#endif

        // frame capture complete, copy current shot data to previous one
        pthread_mutex_lock( &tdata->previousStateLock );
        memcpy( &tdata->previousState, &camera->m_currentState, sizeof( Camera::CaptureState ) );
        pthread_mutex_unlock( &tdata->previousStateLock );
        //    env->CallVoidMethod(sAppData.callbackJavaObject, sAppData.onPictureTaken);

        // frame counter
        frameCount++;

        // update fps
        double time = timer.get();
        double dt = time - fpsUpdateTime;
        if ( dt > FPS_UPDATE_PERIOD )
        {
            float fps = frameCount * ( 1000.0 / dt );
            fpsUpdateTime = time;
            frameCount = 0;
            tdata->captureFps = fps;
#ifdef MEASURE_JITTER
            LOG( "fps: %.3f jitter mean: %.3f jitter std: %.3f", fps, stat.getMean(), stat.getStdDev() );
#endif
        }

#ifdef MEASURE_JITTER
        // compute jitter stats
        dt = time - nextFrameTime;
        nextFrameTime = time + ( 1000.0f / tdata->captureFps );
        // exclude outlayers
        if ( abs( dt ) < FPS_JITTER_CAP )
        {
            stat.update( dt );
        }
#endif
    }

    tdata->javaVM->DetachCurrentThread();

    // delete instance ref
    env->DeleteGlobalRef( tdata->fcamInstanceRef );

    return 0;
}

