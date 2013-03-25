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
#ifndef _TRIPLEBUFFER_H
#define _TRIPLEBUFFER_H

/**
 * @file
 *
 * Definition of TripleBuffer
 */

#include <pthread.h>

/**
 * Provides a thread-safe implementation of triple buffering mechanism.
 * We use triple buffering to minimize the synchronization between Java
 * UI and image capture code. It allows us to capture images and perform UI
 * refresh always at their full speeds (30hz camera, ~60hz UI).
 */

template<class T> class TSTripleBuffer
{
public:
    /**
     * Default constructor.
     */
    TSTripleBuffer( T * buffers[3] )
    {
        pthread_mutex_init( &m_lock, 0 );
        m_frontBuffer = buffers[0];
        m_backBuffer = buffers[1];
        m_spareBuffer = buffers[2];
        m_updateFrontBuffer = false;
    }

    /**
     * Default destructor.
     */
    ~TSTripleBuffer( void )
    {
        pthread_mutex_destroy( &m_lock );
    }

    /**
     * Gets a pointer to active front buffer. The pointer is valid
     * till next call to swapFrontBuffer().
     * @return pointer to the front buffer
     */
    T * getFrontBuffer( void )
    {
        T * rval;

        pthread_mutex_lock( &m_lock );
        rval = m_frontBuffer;
        pthread_mutex_unlock( &m_lock );

        return rval;
    }

    /**
     * Swaps front buffer with an empty buffer.
     * @return pointer to the active front buffer
     */
    T * swapFrontBuffer( void )
    {
        T * rval;

        pthread_mutex_lock( &m_lock );
        if ( m_updateFrontBuffer )
        {
            rval = m_spareBuffer;
            m_spareBuffer = m_frontBuffer;
            m_frontBuffer = rval;
            m_updateFrontBuffer = false;
        }
        else
        {
            rval = m_frontBuffer;
        }
        pthread_mutex_unlock( &m_lock );

        return rval;
    }


    /**
     * Gets a pointer to active back buffer. The pointer is valid
     * till next call to swapBackBuffer().
     * @return pointer to the back buffer
     */
    T * getBackBuffer( void )
    {
        T * rval;

        pthread_mutex_lock( &m_lock );
        rval = m_backBuffer;
        pthread_mutex_unlock( &m_lock );

        return rval;
    }

    /**
     * Swaps back buffer with an empty buffer.
     * @return pointer to the active back buffer
     */
    T * swapBackBuffer( void )
    {
        T * rval;

        pthread_mutex_lock( &m_lock );
        rval = m_spareBuffer;
        m_spareBuffer = m_backBuffer;
        m_backBuffer = rval;
        m_updateFrontBuffer = true;
        pthread_mutex_unlock( &m_lock );

        return rval;
    }

protected:
    TSTripleBuffer( void )
    {
        pthread_mutex_init( &m_lock, 0 );
        m_updateFrontBuffer = false;
    }

    T * m_frontBuffer, *m_backBuffer, *m_spareBuffer;
    bool m_updateFrontBuffer;
    pthread_mutex_t m_lock;
};

/**
 * Provides an implementation of triple buffering mechanism.
 * We use triple buffering to minimize the synchronization between Java
 * UI and image capture code. It allows us to capture images and perform UI
 * refresh always at their full speeds (30hz camera, ~60hz UI).
 */

template<class T> class TripleBuffer
{
public:
    /**
     * Default constructor.
     */
    TripleBuffer( T * buffers[3] )
    {
        m_frontBuffer = buffers[0];
        m_backBuffer = buffers[1];
        m_spareBuffer = buffers[2];
        m_updateFrontBuffer = false;
    }

    /**
     * Default destructor.
     */
    ~TripleBuffer( void )
    {
    }

    /**
     * Gets a pointer to active front buffer. The pointer is valid
     * till next call to swapFrontBuffer().
     * @return pointer to the front buffer
     */
    T * getFrontBuffer( void )
    {
        return m_frontBuffer;
    }

    /**
     * Swaps front buffer with an empty buffer.
     * @return pointer to the active front buffer
     */
    T * swapFrontBuffer( void )
    {
        T * rval;

        if ( m_updateFrontBuffer )
        {
            rval = m_spareBuffer;
            m_spareBuffer = m_frontBuffer;
            m_frontBuffer = rval;
            m_updateFrontBuffer = false;
        }
        else
        {
            rval = m_frontBuffer;
        }

        return rval;
    }


    /**
     * Gets a pointer to active back buffer. The pointer is valid
     * till next call to swapBackBuffer().
     * @return pointer to the back buffer
     */
    T * getBackBuffer( void )
    {
        return m_backBuffer;
    }

    /**
     * Swaps back buffer with an empty buffer.
     * @return pointer to the active back buffer
     */
    T * swapBackBuffer( void )
    {
        T * rval = m_spareBuffer;
        m_spareBuffer = m_backBuffer;
        m_backBuffer = rval;
        m_updateFrontBuffer = true;
        return rval;
    }

protected:
    TripleBuffer( void )
    {
        m_updateFrontBuffer = false;
    }

    T * m_frontBuffer, *m_backBuffer, *m_spareBuffer;
    bool m_updateFrontBuffer;
};

#endif

