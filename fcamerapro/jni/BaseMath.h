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
#ifndef _BASEMATH_H
#define _BASEMATH_H

#define MATH_PI_D    (3.1415926535897932384626433832795)
#define MATH_PI      ((float)MATH_PI_D)
#define MATH_RAD     ((float)(180.0/MATH_PI_D))
#define MATH_IRAD    ((float)(MATH_PI_D/180.0))
#define MATH_IRAD2   ((float)(MATH_PI_D/360.0))
#define MATH_DELTA   (0.000001f)
#define MATH_FLT_MAX (1e37f)

namespace Math
{

class CVec2f;
class CVec3f;
class CVec4f;
class CQuat;

float Sinf( float angle );
float Cosf( float angle );
float Acosf( float cvalue );
float Tanf( float angle );
float Atan2f( float y, float x );
float Sqrtf( float v );
float InvSqrtf( float v );
float Floorf( float v );
float Ceilf( float v );
float Roundf( float v );
float Absf( float v );
float Modf( float n, float d );
float Powf( float x, float n );

float FastSqrtf( float v );
float FastInvSqrtf( float v );

class CMatrix2x2f
{
public:
    CMatrix2x2f( void ) { }

    CMatrix2x2f & setIdentity( void )
    {
        m_data[0] = 1.0f;
        m_data[1] = 0.0f;
        m_data[2] = 0.0f;
        m_data[3] = 1.0f;

        return *this;
    }

    float m_data[4];
};

class CMatrix3x2f
{
public:
    CMatrix3x2f( void ) { }

    CMatrix3x2f & setIdentity( void )
    {
        m_data[0] = 1.0f;
        m_data[1] = 0.0f;
        m_data[2] = 0.0f;
        m_data[3] = 1.0f;
        m_data[4] = 0.0f;
        m_data[5] = 0.0f;

        return *this;
    }

    void invert( const CMatrix3x2f & mat )
    {
        float invdet = 1.0f / ( mat.m_data[0] * mat.m_data[3] - mat.m_data[1] * mat.m_data[2] );
        m_data[0] =  mat.m_data[3] * invdet;
        m_data[1] = -mat.m_data[1] * invdet;
        m_data[2] = -mat.m_data[2] * invdet;
        m_data[3] =  mat.m_data[0] * invdet;
        m_data[4] = ( mat.m_data[2] * mat.m_data[5] - mat.m_data[4] * mat.m_data[3] ) * invdet;
        m_data[5] = -( mat.m_data[0] * mat.m_data[5] - mat.m_data[4] * mat.m_data[1] ) * invdet;
    }

    void operator *= ( const CMatrix3x2f & mat )
    {
        float m1, m2;

        m1 = m_data[0];
        m2 = m_data[1];
        m_data[0] = m1 * mat.m_data[0] + m2 * mat.m_data[2];
        m_data[1] = m1 * mat.m_data[1] + m2 * mat.m_data[3];

        m1 = m_data[2];
        m2 = m_data[3];
        m_data[2] = m1 * mat.m_data[0] + m2 * mat.m_data[2];
        m_data[3] = m1 * mat.m_data[1] + m2 * mat.m_data[3];

        m1 = m_data[4];
        m2 = m_data[5];
        m_data[4] = m1 * mat.m_data[0] + m2 * mat.m_data[2] + mat.m_data[4];
        m_data[5] = m1 * mat.m_data[1] + m2 * mat.m_data[3] + mat.m_data[5];
    }

    void setScale( const CVec2f & vec );
    void setRotate( float angle );
    void setTranslate( const CVec2f & vec );
    void setPretranslate( const CMatrix3x2f & mat, const CVec2f & vec );

    void setSRT( const CVec2f & scale, float rotation, const CVec2f & position );

    float m_data[6];
};

class CMatrix3x3f
{
public:
    CMatrix3x3f( void ) { }

    CMatrix3x3f & setIdentity( void )
    {
        m_data[0] = 1.0f;
        m_data[1] = 0.0f;
        m_data[2] = 0.0f;
        m_data[3] = 1.0f;
        m_data[4] = 1.0f;
        m_data[5] = 0.0f;
        m_data[6] = 1.0f;
        m_data[7] = 0.0f;
        m_data[8] = 1.0f;

        return *this;
    }

    float m_data[9];
};

class CMatrix4x4f
{
public:
    enum EAxes
    {
        kAxisX,
        kAxisY,
        kAxisZ,
    };

    CMatrix4x4f( void ) { }

    void operator *= ( const CMatrix4x4f & mat );

    void setIdentity( void );
    void setOrtho( float left, float right, float bottom, float top, float near, float far );
    void setFrustum( float left, float right, float bottom, float top, float near, float far );
    void setPerspective( float fovy, float aspect, float znear, float zfar );

    void setRotateByAxis( EAxes axis, float angle );
    void setRotate( const CQuat & quat );
    void setScale( const CVec3f & vec );
    void applyScale( const CVec3f & vec );
    void setTranslate( const CVec3f & vec );
    void applyTranslate( const CVec3f & vec );
    void setTranspose( const CMatrix4x4f & mat );
    void applyTranspose( void );

    void setSRT( const CVec3f & scale, const CQuat & rotation, const CVec3f & position );

    float m_data[16];
};

class CVec2b
{
public:
    CVec2b( void ) : x( 0 ), y( 0 ) { }

    bool x, y;
};

class CVec3b
{
public:
    CVec3b( void ) : x( 0 ), y( 0 ), z( 0 ) { }

    bool x, y, z;
};

class CVec4b
{
public:
    CVec4b( void ) : x( 0 ), y( 0 ), z( 0 ), w( 0 ) { }

    bool x, y, z, w;
};

class CVec2i
{
public:
    CVec2i( void ) : x( 0 ), y( 0 ) { }
    CVec2i( int v ) : x( v ), y( v ) { }
    CVec2i( int x, int y ) : x( x ), y( y ) { }
    CVec2i( const CVec2i & v ) : x( v.x ), y( v.y ) { }

    CVec2i & operator = ( int v )
    {
        x = y = v;
        return *this;
    }

    CVec2i & operator = ( const CVec2i & v )
    {
        x = v.x;
        y = v.y;
        return *this;
    }

    void operator += ( int v )
    {
        x += v;
        y += v;
    }

    void operator += ( const CVec2i & v )
    {
        x += v.x;
        y += v.y;
    }

    void operator -= ( int v )
    {
        x -= v;
        y -= v;
    }

    void operator -= ( const CVec2i & v )
    {
        x -= v.x;
        y -= v.y;
    }

    void operator *= ( int v )
    {
        x *= v;
        y *= v;
    }

    void operator *= ( const CVec2i & v )
    {
        x *= v.x;
        y *= v.y;
    }

    void operator /= ( int v )
    {
        x /= v;
        y /= v;
    }

    void operator /= ( const CVec2i & v )
    {
        x /= v.x;
        y /= v.y;
    }

    CVec2i operator + ( int v ) const
    {
        return CVec2i( x + v, y + v );
    }

    CVec2i operator + ( const CVec2i & v ) const
    {
        return CVec2i( x + v.x, y + v.y );
    }

    CVec2i operator - ( void ) const
    {
        return CVec2i( -x, -y );
    }

    CVec2i operator - ( int v ) const
    {
        return CVec2i( x - v, y - v );
    }

    CVec2i operator - ( const CVec2i & v ) const
    {
        return CVec2i( x - v.x, y - v.y );
    }

    CVec2i operator *( int v ) const
    {
        return CVec2i( x * v, y * v );
    }

    CVec2i operator *( const CVec2i & v ) const
    {
        return CVec2i( x * v.x, y * v.y );
    }

    CVec2i operator / ( int v ) const
    {
        return CVec2i( x / v, y / v );
    }

    CVec2i operator / ( const CVec2i & v ) const
    {
        return CVec2i( x / v.x, y / v.y );
    }

    int x, y;
};

class CVec3i
{
public:
    CVec3i( void ) : x( 0 ), y( 0 ), z( 0 ) { }

    int x, y, z;
};

class CVec4i
{
public:
    CVec4i( void ) : x( 0 ), y( 0 ), z( 0 ), w( 0 ) { }

    int x, y, z, w;
};

class CVec2f
{
public:
    CVec2f( void ) : x( 0.0f ), y( 0.0f ) { }
    CVec2f( float v ) : x( v ), y( v ) { }
    CVec2f( float x, float y ) : x( x ), y( y ) { }
    CVec2f( const CVec2f & v ) : x( v.x ), y( v.y ) { }
    CVec2f( const CVec2i & v ) : x(( float )v.x ), y(( float )v.y ) { }

    void set( float vx, float vy )
    {
        x = vx;
        y = vy;
    }

    CVec2f & operator = ( float v )
    {
        x = y = v;
        return *this;
    }

    CVec2f & operator = ( const CVec2f & v )
    {
        x = v.x;
        y = v.y;
        return *this;
    }

    void operator += ( float v )
    {
        x += v;
        y += v;
    }

    void operator += ( const CVec2f & v )
    {
        x += v.x;
        y += v.y;
    }

    void operator -= ( float v )
    {
        x -= v;
        y -= v;
    }

    void operator -= ( const CVec2f & v )
    {
        x -= v.x;
        y -= v.y;
    }

    void operator *= ( float v )
    {
        x *= v;
        y *= v;
    }

    void operator *= ( const CVec2f & v )
    {
        x *= v.x;
        y *= v.y;
    }

    void operator /= ( float v )
    {
        x /= v;
        y /= v;
    }

    void operator /= ( int v )
    {
        x /= v;
        y /= v;
    }

    void operator /= ( const CVec2f & v )
    {
        x /= v.x;
        y /= v.y;
    }

    CVec2f operator + ( float v ) const
    {
        return CVec2f( x + v, y + v );
    }

    CVec2f operator + ( const CVec2f & v ) const
    {
        return CVec2f( x + v.x, y + v.y );
    }

    CVec2f operator - ( void ) const
    {
        return CVec2f( -x, -y );
    }

    CVec2f operator - ( float v ) const
    {
        return CVec2f( x - v, y - v );
    }

    CVec2f operator - ( const CVec2f & v ) const
    {
        return CVec2f( x - v.x, y - v.y );
    }

    CVec2f operator *( float v ) const
    {
        return CVec2f( x * v, y * v );
    }

    CVec2f operator *( const CVec2f & v ) const
    {
        return CVec2f( x * v.x, y * v.y );
    }

    CVec2f operator *( const CMatrix3x2f & mat ) const
    {
        return CVec2f( x * mat.m_data[0] + y * mat.m_data[2] + mat.m_data[4],
                       x * mat.m_data[1] + y * mat.m_data[3] + mat.m_data[5] );
    }

    CVec2f operator / ( float v ) const
    {
        return CVec2f( x / v, y / v );
    }

    CVec2f operator / ( int v ) const
    {
        return CVec2f( x / v, y / v );
    }

    CVec2f operator / ( const CVec2f & v ) const
    {
        return CVec2f( x / v.x, y / v.y );
    }

    float dot( CVec2f & v ) const
    {
        return x * v.x + y * v.y;
    }

    CVec2f & normalize( void )
    {
        float scale = x * x + y * y;
        if ( scale > 0.0f )
        {
            scale = FastInvSqrtf( scale );
            x *= scale;
            y *= scale;
        }
        else
        {
            x = y = 0.0f;
        }
        return *this;
    }

    float length( void ) const
    {
        return FastSqrtf( x * x + y * y );
    }

    float length2( void ) const
    {
        return x * x + y * y;
    }

    float distance( const CVec2f & v ) const
    {
        return FastSqrtf(( x - v.x ) * ( x - v.x ) + ( y - v.y ) * ( y - v.y ) );
    }

    float distance2( const CVec2f & v ) const
    {
        return ( x - v.x ) * ( x - v.x ) + ( y - v.y ) * ( y - v.y );
    }

    float x, y;
};

class CVec3f
{
public:
    CVec3f( void ) : x( 0.0f ), y( 0.0f ), z( 0.0f ) { }
    CVec3f( float v ) : x( v ), y( v ), z( v ) { }
    CVec3f( const CVec2f & v ) : x( v.x ), y( v.y ), z( 0.0f ) { }
    CVec3f( const CVec2f & v, float z ) : x( v.x ), y( v.y ), z( z ) { }
    CVec3f( float x, float y, float z ) : x( x ), y( y ), z( z ) { }
    CVec3f( const CVec3f & v ) : x( v.x ), y( v.y ), z( v.z ) { }

    CVec3f & operator = ( float v )
    {
        x = y = z = v;
        return *this;
    }

    CVec3f & operator = ( const CVec3f & v )
    {
        x = v.x;
        y = v.y;
        z = v.z;
        return *this;
    }

    void set( float vx, float vy, float vz )
    {
        x = vx;
        y = vy;
        z = vz;
    }

    void operator += ( float v )
    {
        x += v;
        y += v;
        z += v;
    }

    void operator += ( const CVec3f & v )
    {
        x += v.x;
        y += v.y;
        z += v.z;
    }

    void operator -= ( float v )
    {
        x -= v;
        y -= v;
        z -= v;
    }

    void operator -= ( const CVec3f & v )
    {
        x -= v.x;
        y -= v.y;
        z -= v.z;
    }

    void operator *= ( float v )
    {
        x *= v;
        y *= v;
        z *= v;
    }

    void operator *= ( const CVec3f & v )
    {
        x *= v.x;
        y *= v.y;
        z *= v.z;
    }

    void operator /= ( float v )
    {
        float iv = 1.0f / v;
        x *= iv;
        y *= iv;
        z *= iv;
    }

    void operator /= ( const CVec3f & v )
    {
        x /= v.x;
        y /= v.y;
        z /= v.z;
    }

    CVec3f operator + ( float v ) const
    {
        return CVec3f( x + v, y + v, z + v );
    }

    CVec3f operator + ( const CVec3f & v ) const
    {
        return CVec3f( x + v.x, y + v.y, z + v.z );
    }

    CVec3f operator - ( void ) const
    {
        return CVec3f( -x, -y, -z );
    }

    CVec3f operator - ( float v ) const
    {
        return CVec3f( x - v, y - v, z - v );
    }

    CVec3f operator - ( const CVec3f & v ) const
    {
        return CVec3f( x - v.x, y - v.y, z - v.z );
    }

    CVec3f operator *( float v ) const
    {
        return CVec3f( x * v, y * v, z * v );
    }

    CVec3f operator *( const CVec3f & v ) const
    {
        return CVec3f( x * v.x, y * v.y, z * v.z );
    }

    CVec3f operator *( const CMatrix4x4f & mat ) const
    {
        return CVec3f( x * mat.m_data[0] + y * mat.m_data[4] + z * mat.m_data[8] + mat.m_data[12],
                       x * mat.m_data[1] + y * mat.m_data[5] + z * mat.m_data[9] + mat.m_data[13],
                       x * mat.m_data[2] + y * mat.m_data[6] + z * mat.m_data[10] + mat.m_data[14] );
    }

    CVec3f operator / ( float v ) const
    {
        float iv = 1.0f / v;
        return CVec3f( x * iv, y * iv, z * iv );
    }

    CVec3f operator / ( const CVec3f & v ) const
    {
        return CVec3f( x / v.x, y / v.y, z / v.z );
    }

    float dot( const CVec3f & v ) const
    {
        return x * v.x + y * v.y + z * v.z;
    }

    CVec3f & normalize( void )
    {
        float scale = x * x + y * y + z * z;
        if ( scale > 0.0f )
        {
            scale = FastInvSqrtf( scale );
            x *= scale;
            y *= scale;
            z *= scale;
        }
        else
        {
            x = y = z = 0.0f;
        }
        return *this;
    }

    float length( void ) const
    {
        return FastSqrtf( x * x + y * y + z * z );
    }

    float length2( void ) const
    {
        return x * x + y * y + z * z;
    }

    float distance( const CVec3f & v ) const
    {
        return FastSqrtf(( x - v.x ) * ( x - v.x ) + ( y - v.y ) * ( y - v.y ) + ( z - v.z ) * ( z - v.z ) );
    }

    float distance2( const CVec3f & v ) const
    {
        return ( x - v.x ) * ( x - v.x ) + ( y - v.y ) * ( y - v.y ) + ( z - v.z ) * ( z - v.z );
    }

    float x, y, z;
};


class CVec4f
{
public:
    CVec4f( void ) : x( 0.0f ), y( 0.0f ), z( 0.0f ), w( 0.0f ) { }
    CVec4f( float x, float y, float z, float w ) : x( x ), y( y ), z( z ), w( w ) { }
    CVec4f( const CVec4f & v ) : x( v.x ), y( v.y ), z( v.z ), w( v.w ) { }

    CVec4f & operator = ( const CVec4f & v )
    {
        x = v.x;
        y = v.y;
        z = v.z;
        w = v.w;
        return *this;
    }

    float x, y, z, w;
};


class CQuat : public CVec4f
{
public:
    CQuat( void ) : CVec4f( 0.0f, 0.0f, 0.0f, 1.0f ) { }
    CQuat( const CVec3f & v, float w ) : CVec4f( v.x, v.y, v.z, w )
    {
        toQuaternion();
    }
    CQuat( float x, float y, float z, float w ) : CVec4f( x, y, z, w ) { }
    CQuat( const CQuat & v ) : CVec4f( v.x, v.y, v.z, v.w ) { }

    void toQuaternion( void )
    {
        float angle = w * MATH_IRAD2;
        float d = Sinf( angle );
        x *= d;
        y *= d;
        z *= d;
        w = Cosf( angle );
    }

    void toAngleVector( void )
    {
        float ha = Acosf( w );
        float sinval2 = 1.0f - w * w;
        float isinval = FastInvSqrtf( sinval2 );
        w = ( 2.0f * MATH_RAD ) * ha;
        if ( sinval2 * isinval < MATH_DELTA )
        {
            x = 1.0f;
            y = 0.0f;
            z = 0.0f;
        }
        else
        {
            x *= isinval;
            y *= isinval;
            z *= isinval;
        }
    }

    void inverse( void )
    {
        float d = x * x + y * y + z * z + w * w;
        if ( d == 0.0f )
        {
            d = 1.0f;
        }
        else
        {
            d = 1.0f / d;
        }
        x *= -d;
        y *= -d;
        z *= -d;
        w *= d;
    }

    void operator *= ( const CQuat & q )
    {
        float nx = w * q.x + x * q.w + y * q.z - z * q.y;
        float ny = w * q.y + y * q.w + z * q.x - x * q.z;
        float nz = w * q.z + z * q.w + x * q.y - y * q.x;
        float nw = w * q.w - x * q.x - y * q.y - z * q.z;
        x = nx;
        y = ny;
        z = nz;
        w = nw;
    }

    CQuat operator *( const CQuat & q )
    {
        return CQuat( w * q.x + x * q.w + y * q.z - z * q.y,
                      w * q.y + y * q.w + z * q.x - x * q.z,
                      w * q.z + z * q.w + x * q.y - y * q.x,
                      w * q.w - x * q.x - y * q.y - z * q.z );
    }

    void slerp( const CQuat & q1 , const CQuat & q2 , float t );

    float x, y, z, w;
};

};

#endif

