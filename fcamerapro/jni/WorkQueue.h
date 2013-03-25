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
#ifndef _WORKQUEUE_H
#define _WORKQUEUE_H

/**
 * @file
 * Definition of WorkQueue.
 */

#include <pthread.h>
#include <semaphore.h>
#include <queue>

/**
 * This class provides a thread-safe implementation of a work queue (FIFO).
 */
template<class T> class WorkQueue
{
public:
    /**
     * Default constructor.
     */
    WorkQueue( void )
    {
        pthread_mutex_init( &m_lock, 0 );
        sem_init( &m_counterSem, 0, 0 );
    }

    /**
     * Default destructor.
     */
    ~WorkQueue( void )
    {
        sem_destroy( &m_counterSem );
        pthread_mutex_destroy( &m_lock );
    }

    /**
     * Adds a new element to the end of the queue.
     * @param elem value to be copied to the queue
     */
    void produce( const T & elem )
    {
        pthread_mutex_lock( &m_lock );
        m_queue.push( elem );
        sem_post( &m_counterSem );
        pthread_mutex_unlock( &m_lock );
    }

    /**
     * Removes an element from the queue. The consumed element is copied to the object passed
     * in the function call. If the queue is empty and blocking is set to true (default) then
     * the function blocks the execution till it can fill in the output. If blocking is set
     * to false, the function always returns immediately.
     * @param elem a location where the object from top of the queue should be put
     * @param blocking determines whether the consume call should block until it is able
     * to fill in the output object
     * @return false if blocking is set to false and the queue is empty. True otherwise.
     * (empty queue)
     */
    bool consume( T & elem, bool blocking = true )
    {
        if ( blocking )
        {
            sem_wait( &m_counterSem );
        }
        else
        {
            if ( sem_trywait( &m_counterSem ) != 0 )
            {
                return false;
            }
        }

        pthread_mutex_lock( &m_lock );
        elem = m_queue.front();
        m_queue.pop();
        pthread_mutex_unlock( &m_lock );

        return true;
    }

    /**
     * Copies the contents of this queue to an instance of std::queue. The function is not fully
     * thread-safe, i.e., will fail in case of concurrent call to consume().
     */
    void consumeAll( std::queue<T> &queue )
    {
        pthread_mutex_lock( &m_lock );
        queue = m_queue;
        if ( !m_queue.empty() )
        {
            m_queue = std::queue<T>();
            // XXX: assuming no concurrent call to consume will happen
            sem_destroy( &m_counterSem );
            sem_init( &m_counterSem, 0, 0 );
        }
        pthread_mutex_unlock( &m_lock );
    }

    /**
     * Returns the number of elements in this work queue.
     * @return number of elements in this work queue
     */
    int size( void )
    {
        return m_queue.size();
    }

private:
    std::queue<T> m_queue;
    pthread_mutex_t m_lock;
    sem_t m_counterSem;
};

#endif

