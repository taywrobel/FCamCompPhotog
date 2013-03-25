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
#ifndef _COMMON_H
#define _COMMON_H

/**
 * \file
 * Application-wide definitions.
 */

#ifndef __WINDOWS_TARGET__

#include <android/log.h>

/**
 * Android JNI module name
 */
#define MODULE "fcam_iface"
/**
 * Wrapper around Android logging function (debug log)
 */
#define LOG(x,...) __android_log_print(ANDROID_LOG_DEBUG,MODULE,x,##__VA_ARGS__)
/**
 * Wrapper around Android logging function (error log)
 */
#define ERROR(x,...) __android_log_print(ANDROID_LOG_ERROR,MODULE,x,##__VA_ARGS__)

#else

#include <stdio.h>

#define LOG(x,...) fprintf(stdout,x,##__VA_ARGS__)
#define ERROR(x,...) fprintf(stderr,x,##__VA_ARGS__)

#endif

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;

/**
 * Base class for all objects managed with reference counting
 */

class ManagedObject
{
public:
    ManagedObject( void ) :
        m_refCount( 0 )
    {
        LOG( "object@0x%08x: created!\n", ( uint )this );
    }

    virtual ~ManagedObject( void )
    {
        if ( m_refCount != 0 )
        {
            ERROR( "object@0x%08x: non-zero reference count during removal!\n", ( uint )this );
        }
    }

    template<typename T> friend void managed_ptr_acquire( T * ptr )
    {
        ptr->m_refCount++;
    }

    template<typename T> friend void managed_ptr_release( T * ptr )
    {
        ptr->m_refCount--;
        if ( ptr->m_refCount <= 0 )
        {
            LOG( "object@0x%08x: dereferenced!\n", ( uint )ptr );
            delete ptr;
        }
    }

private:
    int m_refCount;
};

/**
 * Managed pointer template
 */

template<typename T> class managed_ptr
{
public:
    managed_ptr( void ) :
        m_ptr( 0 )
    {
    }

    managed_ptr( T * p, bool acquire = true ) :
        m_ptr( p )
    {
        if ( m_ptr != 0 && acquire )
        {
            managed_ptr_acquire( m_ptr );
        }
    }

    managed_ptr( managed_ptr<T> const & rhs ) :
        m_ptr( rhs.m_ptr )
    {
        if ( m_ptr != 0 )
        {
            managed_ptr_acquire( m_ptr );
        }
    }

    ~managed_ptr( void )
    {
        if ( m_ptr != 0 )
        {
            managed_ptr_release( m_ptr );
        }
    }

    T * get( void ) const
    {
        return m_ptr;
    }

    void reset( void )
    {
        if ( m_ptr != 0 )
        {
            managed_ptr_release( m_ptr );
            m_ptr = 0;
        }
    }

    void swap( managed_ptr & rhs )
    {
        T * tmp = m_ptr;
        m_ptr = rhs.m_ptr;
        rhs.m_ptr = tmp;
    }

    managed_ptr<T> &operator=( managed_ptr<T> const & rhs )
    {
        if ( rhs.m_ptr != 0 )
        {
            managed_ptr_acquire( rhs.m_ptr );
        }

        if ( m_ptr != 0 )
        {
            managed_ptr_release( m_ptr );
        }

        m_ptr = rhs.m_ptr;
        return *this;
    }

    managed_ptr & operator=( T * rhs )
    {
        if ( rhs != 0 )
        {
            managed_ptr_acquire( rhs );
        }

        if ( m_ptr != 0 )
        {
            managed_ptr_release( m_ptr );
        }

        m_ptr = rhs;
        return *this;
    }

    T & operator*( void ) const
    {
        // TODO: sanity check
        return *m_ptr;
    }

    T * operator->( void ) const
    {
        // TODO: sanity check
        return m_ptr;
    }

private:

    T * m_ptr;
};

template<typename T, typename U> inline bool operator==( managed_ptr<T> const & a, managed_ptr<U> const & b )
{
    return a.get() == b.get();
}

template<typename T, typename U> inline bool operator!=( managed_ptr<T> const & a, managed_ptr<U> const & b )
{
    return a.get() != b.get();
}

template<typename T, typename U> inline bool operator==( managed_ptr<T> const & a, U * b )
{
    return a.get() == b;
}

template<typename T, typename U> inline bool operator!=( managed_ptr<T> const & a, U * b )
{
    return a.get() != b;
}

template<typename T, typename U> inline bool operator==( T * a, managed_ptr<U> const & b )
{
    return a == b.get();
}

template<typename T, typename U> inline bool operator!=( T * a, managed_ptr<U> const & b )
{
    return a != b.get();
}

template<typename T> inline bool operator<( managed_ptr<T> const & a, managed_ptr<T> const & b )
{
    return a.get() < b.get();
}

#endif

