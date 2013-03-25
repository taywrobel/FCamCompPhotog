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
#ifndef _PARAMSTAT_H
#define _PARAMSTAT_H

/**
 * @file
 * Definition of ParamStat.
 */

#include <math.h>

/**
 * ParamStat computes mean and standard deviation metrics for a series of numbers.
 */
class ParamStat
{
public:
    /**
     * Default constructor.
     */
    ParamStat( void )
    {
        reset();
    }

    /**
     * Default destructor.
     */
    ~ParamStat( void ) { }

    /**
     * Gets the mean value of the population.
     * @return mean value
     */
    double getMean( void )
    {
        if ( m_counter == 0 )
        {
            return 0.0;
        }

        return m_accum / m_counter;
    }

    /**
     * Gets the standard deviation of the population.
     * @return standard deviation value
     */
    double getStdDev( void )
    {
        if ( m_counter == 0 )
        {
            return 0.0;
        }

        double mean = m_accum / m_counter;
        return sqrt( m_squareAccum / m_counter - mean * mean );
    }

    /**
     * Resets the statistics.
     */
    void reset( void )
    {
        m_accum = m_squareAccum = 0.0;
        m_counter = 0;
    }

    /**
     * Adds a number to the population.
     * @param value a number
     */
    void update( double value )
    {
        m_accum += value;
        m_squareAccum += value * value;
        m_counter++;
    }

private:
    double m_accum;
    double m_squareAccum;
    int m_counter;
};

#endif

