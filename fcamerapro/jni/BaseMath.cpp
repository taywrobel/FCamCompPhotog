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
#include "BaseMath.h"
#include <math.h>

// ================================================================
// BASIC MATH FUNCTIONS IMPLEMENTATION
// ================================================================

float Math::Floorf( float v )
{
    return floorf( v );
}

float Math::Ceilf( float v )
{
    return ceilf( v );
}

float Math::Roundf( float v )
{
    return ( v > 0.0f ) ? floorf( v + 0.5f ) : ceilf( v - 0.5f );
}

float Math::Absf( float v )
{
    return fabsf( v );
}

float Math::Modf( float n, float d )
{
    return fmodf( n, d );
}

float Math::Powf( float x, float n )
{
    return powf( x, n );
}

float Math::Sinf( float angle )
{
    return sinf( angle );
}

float Math::Cosf( float angle )
{
    return cosf( angle );
}

float Math::Acosf( float cvalue )
{
    return acosf( cvalue );
}

float Math::Tanf( float angle )
{
    return tanf( angle );
}

float Math::Atan2f( float y, float x )
{
    return atan2f( y, x );
}

float Math::Sqrtf( float v )
{
    return sqrtf( v );
}

float Math::InvSqrtf( float v )
{
    float x2 = v * 0.5f;
    float y = v;
    // evil approximation hack
    int i = *( int * )&y;
    i = 0x5f3759df - ( i >> 1 );
    y = *( float * )&i;

    // 2 newton iteration
    y = y * ( 1.5f - ( x2 * y * y ) );
    y = y * ( 1.5f - ( x2 * y * y ) );

    return y;
}

float Math::FastSqrtf( float v )
{
    float x2 = v * 0.5f;
    float y = v;
    // evil approximation hack
    int i = *( int * )&y;
    i = 0x5f3759df - ( i >> 1 );
    y = *( float * )&i;

    // newton iteration
    y = y * ( 1.5f - ( x2 * y * y ) );

    return v * y;
}

float Math::FastInvSqrtf( float v )
{
    float x2 = v * 0.5f;
    float y = v;
    // evil approximation hack
    int i = *( int * )&y;
    i = 0x5f3759df - ( i >> 1 );
    y = *( float * )&i;

    // newton iteration
    y = y * ( 1.5f - ( x2 * y * y ) );

    return y;
}

// ================================================================
// 3x2 MATRIX IMPLEMENTATION
// ================================================================

using namespace Math;

void CMatrix3x2f::setSRT( const CVec2f & scale, float rotation, const CVec2f & position )
{
    float msin = sinf( rotation * MATH_IRAD );
    float mcos = cosf( rotation * MATH_IRAD );

    m_data[0] = mcos * scale.x;
    m_data[1] = msin * scale.x;
    m_data[2] = -msin * scale.y;
    m_data[3] = mcos * scale.y;
    m_data[4] = position.x;
    m_data[5] = position.y;
}

void CMatrix3x2f::setRotate( float angle )
{
    float msin = sinf( angle * MATH_IRAD );
    float mcos = cosf( angle * MATH_IRAD );

    m_data[0] = mcos;
    m_data[1] = msin;
    m_data[2] = -msin;
    m_data[3] = mcos;
    m_data[4] = 0.0f;
    m_data[5] = 0.0f;
}

void CMatrix3x2f::setTranslate( const CVec2f & vec )
{
    m_data[0] = 1.0f;
    m_data[1] = 0.0f;
    m_data[2] = 0.0f;
    m_data[3] = 1.0f;
    m_data[4] = vec.x;
    m_data[5] = vec.y;
}

void CMatrix3x2f::setPretranslate( const CMatrix3x2f & matrix, const CVec2f & vec )
{
    m_data[0] = matrix.m_data[0];
    m_data[1] = matrix.m_data[1];
    m_data[2] = matrix.m_data[2];
    m_data[3] = matrix.m_data[3];
    m_data[4] = vec.x * m_data[0] + vec.y * m_data[2] + matrix.m_data[4];
    m_data[5] = vec.x * m_data[1] + vec.y * m_data[3] + matrix.m_data[5];
}

void CMatrix3x2f::setScale( const CVec2f & vec )
{
    m_data[0] = vec.x;
    m_data[1] = 0.0f;
    m_data[2] = 0.0f;
    m_data[3] = vec.y;
    m_data[4] = 0.0f;
    m_data[5] = 0.0f;
}

// ================================================================
// 4x4 MATRIX IMPLEMENTATION
// XXX: Open GL style matrix format (column-wise)
// ================================================================

void CMatrix4x4f::setIdentity( void )
{
    m_data[0] = 1.0f;
    m_data[1] = 0.0f;
    m_data[2] = 0.0f;
    m_data[3] = 0.0f;
    m_data[4] = 0.0f;
    m_data[5] = 1.0f;
    m_data[6] = 0.0f;
    m_data[7] = 0.0f;
    m_data[8] = 0.0f;
    m_data[9] = 0.0f;
    m_data[10] = 1.0f;
    m_data[11] = 0.0f;
    m_data[12] = 0.0f;
    m_data[13] = 0.0f;
    m_data[14] = 0.0f;
    m_data[15] = 1.0f;
}

void CMatrix4x4f::setScale( const CVec3f & vec )
{
    m_data[0] = vec.x;
    m_data[1] = 0.0f;
    m_data[2] = 0.0f;
    m_data[3] = 0.0f;
    m_data[4] = 0.0f;
    m_data[5] = vec.y;
    m_data[6] = 0.0f;
    m_data[7] = 0.0f;
    m_data[8] = 0.0f;
    m_data[9] = 0.0f;
    m_data[10] = vec.z;
    m_data[11] = 0.0f;
    m_data[12] = 0.0f;
    m_data[13] = 0.0f;
    m_data[14] = 0.0f;
    m_data[15] = 1.0f;
}

void CMatrix4x4f::applyScale( const CVec3f & vec )
{
    m_data[0] *= vec.x;
    m_data[1] *= vec.y;
    m_data[2] *= vec.z;

    m_data[4] *= vec.x;
    m_data[5] *= vec.y;
    m_data[6] *= vec.z;

    m_data[8] *= vec.x;
    m_data[9] *= vec.y;
    m_data[10] *= vec.z;

    m_data[12] *= vec.x;
    m_data[13] *= vec.y;
    m_data[14] *= vec.z;
}

void CMatrix4x4f::setTranslate( const CVec3f & vec )
{
    m_data[0] = 1.0f;
    m_data[1] = 0.0f;
    m_data[2] = 0.0f;
    m_data[3] = 0.0f;
    m_data[4] = 0.0f;
    m_data[5] = 1.0f;
    m_data[6] = 0.0f;
    m_data[7] = 0.0f;
    m_data[8] = 0.0f;
    m_data[9] = 0.0f;
    m_data[10] = 1.0f;
    m_data[11] = 0.0f;
    m_data[12] = vec.x;
    m_data[13] = vec.y;
    m_data[14] = vec.z;
    m_data[15] = 1.0f;
}

void CMatrix4x4f::applyTranslate( const CVec3f & vec )
{
    m_data[0] += m_data[3] * vec.x;
    m_data[1] += m_data[3] * vec.y;
    m_data[2] += m_data[3] * vec.z;

    m_data[4] += m_data[7] * vec.x;
    m_data[5] += m_data[7] * vec.y;
    m_data[6] += m_data[7] * vec.z;

    m_data[8] += m_data[11] * vec.x;
    m_data[9] += m_data[11] * vec.y;
    m_data[10] += m_data[11] * vec.z;

    m_data[12] += m_data[15] * vec.x;
    m_data[13] += m_data[15] * vec.y;
    m_data[14] += m_data[15] * vec.z;
}

void CMatrix4x4f::setTranspose( const CMatrix4x4f & mat )
{
    int * srcdata = ( int * )mat.m_data;
    int * dstdata = ( int * )m_data;
    dstdata[0] = srcdata[0];
    dstdata[1] = srcdata[4];
    dstdata[2] = srcdata[8];
    dstdata[3] = srcdata[12];
    dstdata[4] = srcdata[1];
    dstdata[5] = srcdata[5];
    dstdata[6] = srcdata[9];
    dstdata[7] = srcdata[13];
    dstdata[8] = srcdata[2];
    dstdata[9] = srcdata[6];
    dstdata[10] = srcdata[10];
    dstdata[11] = srcdata[14];
    dstdata[12] = srcdata[3];
    dstdata[13] = srcdata[7];
    dstdata[14] = srcdata[11];
    dstdata[15] = srcdata[15];
}

void CMatrix4x4f::applyTranspose( void )
{
    int * idata = ( int * )m_data;
    int t = idata[1];
    idata[1] = idata[4];
    idata[4] = t;

    t = idata[2];
    idata[2] = idata[8];
    idata[8] = t;

    t = idata[3];
    idata[3] = idata[12];
    idata[12] = t;
    t = idata[6];
    idata[6] = idata[9];
    idata[9] = t;

    t = idata[7];
    idata[7] = idata[13];
    idata[13] = t;

    t = idata[11];
    idata[11] = idata[14];
    idata[14] = t;
}

void CMatrix4x4f::setRotateByAxis( EAxes axis, float angle )
{
    setIdentity();

    angle = angle * MATH_IRAD;
    float msin = Sinf( angle );
    float mcos = Cosf( angle );

    switch ( axis )
    {
        case kAxisX:
            m_data[5] = mcos;
            m_data[6] = msin;
            m_data[9] = -msin;
            m_data[10] = mcos;
            break;

        case kAxisY:
            m_data[0] = mcos;
            m_data[2] = -msin;
            m_data[8] = msin;
            m_data[10] = mcos;
            break;

        case kAxisZ:
            m_data[0] = mcos;
            m_data[1] = msin;
            m_data[4] = -msin;
            m_data[5] = mcos;
            break;
    }
}

void CMatrix4x4f::setRotate( const CQuat & quat )
{
    float mx, my, mz;
    float xx, xy, xz;
    float yy, yz, zz;
    float wx, wy, wz;

    mx = quat.x + quat.x;
    my = quat.y + quat.y;
    mz = quat.z + quat.z;

    xx = quat.x * mx;
    xy = quat.x * my;
    xz = quat.x * mz;

    yy = quat.y * my;
    yz = quat.y * mz;
    zz = quat.z * mz;

    wx = quat.w * mx;
    wy = quat.w * my;
    wz = quat.w * mz;

    m_data[0] = 1.0f - ( yy + zz );
    m_data[1] = xy + wz;
    m_data[2] = xz - wy;
    m_data[3] = 0.0f;

    m_data[4] = xy - wz;
    m_data[5] = 1.0f - ( xx + zz );
    m_data[6] = yz + wx;
    m_data[7] = 0.0f;

    m_data[8] = xz + wy;
    m_data[9] = yz - wx;
    m_data[10] = 1.0f - ( xx + yy );
    m_data[11] = 0.0f;

    m_data[12] = 0.0f;
    m_data[13] = 0.0f;
    m_data[14] = 0.0f;
    m_data[15] = 1.0f;
}

void CMatrix4x4f::setOrtho( float l, float r, float b, float t, float n, float f )
{
    float rl = 1.0f / ( r - l );
    float tb = 1.0f / ( t - b );
    float fn = 1.0f / ( f - n );

    m_data[0] = 2.0f * rl;
    m_data[1] = 0.0f;
    m_data[2] = 0.0f;
    m_data[3] = 0.0f;

    m_data[4] = 0.0f;
    m_data[5] = 2.0f * tb;
    m_data[6] = 0.0f;
    m_data[7] = 0.0f;

    m_data[8] = 0.0f;
    m_data[9] = 0.0f;
    m_data[10] = -2.0f * fn;
    m_data[11] = 0.0f;

    m_data[12] = -( r + l ) * rl;
    m_data[13] = -( t + b ) * tb;
    m_data[14] = -( f + n ) * fn;
    m_data[15] = 1.0f;
}

void CMatrix4x4f::setFrustum( float l, float r, float b, float t, float n, float f )
{
    float rl = 1.0f / ( r - l );
    float tb = 1.0f / ( t - b );
    float fn = 1.0f / ( f - n );

    m_data[0] = 2.0f * n * rl;
    m_data[1] = 0.0f;
    m_data[2] = 0.0f;
    m_data[3] = 0.0f;

    m_data[4] = 0.0f;
    m_data[5] = 2.0f * n * tb;
    m_data[6] = 0.0f;
    m_data[7] = 0.0f;

    m_data[8] = ( r + l ) * rl;
    m_data[9] = ( t + b ) * tb;
    m_data[10] = -( f + n ) * fn;
    m_data[11] = -1.0f;

    m_data[12] = 0.0f;
    m_data[13] = 0.0f;
    m_data[14] = -2.0f * f * n * fn;
    m_data[15] = 0.0f;
}

void CMatrix4x4f::setPerspective( float fovy, float aspect, float znear, float zfar )
{
    float ymax = znear * Tanf( fovy * MATH_IRAD2 );
    float ymin = -ymax;
    float xmin = ymin * aspect;
    float xmax = ymax * aspect;
    setFrustum( xmin, xmax, ymin, ymax, znear, zfar );
}

void CMatrix4x4f::operator *= ( const CMatrix4x4f & mat )
{
    for ( int y = 0; y < 4; y++ )
    {
        int index = y << 2;
        float m1 = m_data[index];
        float m2 = m_data[index + 1];
        float m3 = m_data[index + 2];
        float m4 = m_data[index + 3];
        m_data[index    ] = m1 * mat.m_data[0] + m2 * mat.m_data[4] + m3 * mat.m_data[8]  + m4 * mat.m_data[12];
        m_data[index + 1] = m1 * mat.m_data[1] + m2 * mat.m_data[5] + m3 * mat.m_data[9]  + m4 * mat.m_data[13];
        m_data[index + 2] = m1 * mat.m_data[2] + m2 * mat.m_data[6] + m3 * mat.m_data[10] + m4 * mat.m_data[14];
        m_data[index + 3] = m1 * mat.m_data[3] + m2 * mat.m_data[7] + m3 * mat.m_data[11] + m4 * mat.m_data[15];
    }
}

void CMatrix4x4f::setSRT( const CVec3f & scale, const CQuat & rotation, const CVec3f & position )
{
    setRotate( rotation );

    m_data[0] *= scale.x;
    m_data[1] *= scale.x;
    m_data[2] *= scale.x;

    m_data[4] *= scale.y;
    m_data[5] *= scale.y;
    m_data[6] *= scale.y;

    m_data[8] *= scale.z;
    m_data[9] *= scale.z;
    m_data[10] *= scale.z;

    m_data[12] = position.x;
    m_data[13] = position.y;
    m_data[14] = position.z;
}

// ================================================================
// QUATERNION CLASS IMPLEMENTATION
// ================================================================

void CQuat::slerp( const CQuat & q1, const CQuat & q2, float t )
{
    CQuat to( q2 );

    float mcos = q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;
    if ( mcos < 0.0f )
    {
        mcos = -mcos;
        to.x = -to.x;
        to.y = -to.y;
        to.z = -to.z;
        to.w = -to.w;
    }

    float s0, s1;
    if (( 1.0f - mcos ) > MATH_DELTA )
    {
        float omega = Acosf( mcos );
        float imsin = 1.0f / Sinf( omega );
        s0 = Sinf(( 1.0f - t ) * omega ) * imsin;
        s1 = Sinf( t * omega ) * imsin;
    }
    else
    {
        s0 = 1.0f - t;
        s1 = t;
    }

    x = q1.x * s0 + to.x * s1;
    y = q1.y * s0 + to.y * s1;
    z = q1.z * s0 + to.z * s1;
    w = q1.w * s0 + to.w * s1;
}
