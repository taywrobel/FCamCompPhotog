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
 * @file
 *
 * Implementation of utility functions.
 */

#include <math.h>
#include "Common.h"
#include "Utils.h"

int GetColorTemparatureYCbCr( int temp, int y, int cb, int cr )
{
    // YCbCr to normalized sRGB
    cb -= 128;
    cr -= 128;

    const float iscale = 1.0f / 255.0f;

    float r = ( y + 1.402f * cr ) * iscale;
    if ( r < 0.0f )
    {
        r = 0.0f;
    }
    if ( r > 1.0f )
    {
        r = 1.0f;
    }
    float g = ( y - 0.34414f * cb - 0.71414f * cr ) * iscale;
    if ( g < 0.0f )
    {
        g = 0.0f;
    }
    if ( g > 1.0f )
    {
        g = 1.0f;
    }
    float b = ( y + 1.722f * cb ) * iscale;
    if ( b < 0.0f )
    {
        b = 0.0f;
    }
    if ( b > 1.0f )
    {
        b = 1.0f;
    }
    return GetColorTemparature( temp, r, g, b );
}

int GetColorTemparature( float temp, float r, float g, float b )
{
    // sRGB primaries matrix (coords in XYZ space)
    const float prim[9] = { 1.939394f, 0.500000f, 2.500000f, 1.000000f, 1.000000f, 1.000000f, 0.090909f, 0.166667f, 13.166667f };
    // inverted primaries matrix
    const float invprim[9] = { 0.689157f, -0.326908f, -0.106024f, -0.693173f, 1.341633f, 0.029719f, 0.004016f, -0.014726f, 0.076305f };

    // linearize
    if ( r <= 0.04045f )
    {
        r = r / 12.92f;
    }
    else
    {
        r = powf(( r + 0.055f ) / 1.055f, 2.4f );
    }

    if ( g <= 0.04045f )
    {
        g = g / 12.92f;
    }
    else
    {
        g = powf(( g + 0.055f ) / 1.055f, 2.4f );
    }

    if ( b <= 0.04045f )
    {
        b = b / 12.92f;
    }
    else
    {
        b = powf(( b + 0.055f ) / 1.055f, 2.4f );
    }

    // correlated color temperature of a CIE D-illuminant to the chromaticity of that D-illuminant (valid range: 4000-25000K)
    float wxc;
    if ( temp < 7000.0f )
    {
        wxc = -4.6070e9f / ( temp * temp * temp ) + 2.9678e6f / ( temp * temp ) + 0.09911e3f / temp + 0.244063f;
    }
    else
    {
        wxc = -2.0064e9f / ( temp * temp * temp ) + 1.9018e6f / ( temp * temp ) + 0.24748e3f / temp + 0.237040f;
    }
    float wyc = -3.0f * ( wxc * wxc ) + 2.870f * wxc - 0.275f;

    // sRGB color space white point in XYZ space
    float wx = wxc / wyc;
    float wy = 1.0f;
    float wz = ( 1 - wxc - wyc ) / wyc;

    // convert linear sRGB (with custom white point) to XYZ
    r *= invprim[0] * wx + invprim[1] * wy + invprim[2] * wz;
    g *= invprim[3] * wx + invprim[4] * wy + invprim[5] * wz;
    b *= invprim[6] * wx + invprim[7] * wy + invprim[8] * wz;

    float x = prim[0] * r + prim[1] * g + prim[2] * b;
    float y = prim[3] * r + prim[4] * g + prim[5] * b;
    float z = prim[6] * r + prim[7] * g + prim[8] * b;

    // get chromacity coords
    float cx = x / ( x + y + z );
    float cy = y / ( x + y + z );

    // compute color temp (approximation range: 3000-50000K)
    float n = ( cx - 0.3366f ) / ( cy - 0.1735f );
    float cct = -949.86315f + 6253.80338f * expf( -n / 0.92159f ) + 28.70599f * expf( -n / 0.20039f ) + 0.00004f * expf( -n / 0.07125f );

    return ( int ) cct;
}
