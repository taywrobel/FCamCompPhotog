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
#ifndef _HPT_H
#define _HPT_H

/**
 * @file
 * Definition of Timer.
 */

#include <sys/time.h>
#include <queue>

/**
 * High-precision timer implementation. The time between function calls
 * can be measured in two ways: by measuring absolute time increments
 * with get() function or by calling matlab style tic()/toc() functions.
 */
class Timer
{
public:
    /**
     * Default constructor.
     */
    Timer( void )
    {
        gettimeofday( &m_startup, 0 );
    }

    /**
     * Default destructor.
     */
    ~Timer( void )
    {
    }

    /**
     * Gets current time value and pushes it on the stack.
     */
    void tic( void )
    {
        timeval tv;
        gettimeofday( &tv, 0 );
        m_timeStampStack.push( tv );
    }

    /**
     * Measures time between toc() and last call to tic(). Technically the function
     * pops the tic() time from the stack and returns a different between current time
     * and tic() value.
     * @return time difference between toc() and last call to tic()
     */
    double toc( void )
    {
        timeval end;
        gettimeofday( &end, 0 );
        const timeval & start = m_timeStampStack.front();
        m_timeStampStack.pop();

        return ( end.tv_sec - start.tv_sec ) * 1000.0 + ( end.tv_usec - start.tv_usec ) * 0.001;
    }

    /**
     * Gets the absolute time that has passed since the
     * construction of this Timer object (in milliseconds).
     * @return time in milliseconds
     */
    double get( void )
    {
        timeval tv;
        gettimeofday( &tv, 0 );
        return ( tv.tv_sec - m_startup.tv_sec ) * 1000.0 + ( tv.tv_usec - m_startup.tv_usec ) * 0.001;
    }

private:
    timeval m_startup; /**< Start-up time */
    std::queue<timeval> m_timeStampStack; /**< Stack storing time stamps of tic() calls */
};

#endif

