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
#ifndef _GLWRAPPER_H
#define _GLWRAPPER_H

#include <map>
#include <string>
#include "BaseMath.h"
#include "Common.h"

namespace GL
{

typedef enum
{
    SV_int,
    SV_ivec2,
    SV_ivec3,
    SV_ivec4,
    SV_float,
    SV_vec2,
    SV_vec3,
    SV_vec4,
    SV_bool,
    SV_bvec2,
    SV_bvec3,
    SV_bvec4,
    SV_mat2,
    SV_mat3,
    SV_mat4,
    SV_sampler2D,
    SV_samplerExternal,
} ShaderVarTypes;

template<ShaderVarTypes> struct SHADER_VAR_TYPES_ENUM;
template<> struct SHADER_VAR_TYPES_ENUM<SV_int>
{
    typedef int type;
};
template<> struct SHADER_VAR_TYPES_ENUM<SV_ivec2>
{
    typedef Math::CVec2i type;
};
template<> struct SHADER_VAR_TYPES_ENUM<SV_ivec3>
{
    typedef Math::CVec3i type;
};
template<> struct SHADER_VAR_TYPES_ENUM<SV_ivec4>
{
    typedef Math::CVec4i type;
};
template<> struct SHADER_VAR_TYPES_ENUM<SV_float>
{
    typedef float type;
};
template<> struct SHADER_VAR_TYPES_ENUM<SV_vec2>
{
    typedef Math::CVec2f type;
};
template<> struct SHADER_VAR_TYPES_ENUM<SV_vec3>
{
    typedef Math::CVec3f type;
};
template<> struct SHADER_VAR_TYPES_ENUM<SV_vec4>
{
    typedef Math::CVec4f type;
};
template<> struct SHADER_VAR_TYPES_ENUM<SV_bool>
{
    typedef bool type;
};
template<> struct SHADER_VAR_TYPES_ENUM<SV_bvec2>
{
    typedef Math::CVec2b type;
};
template<> struct SHADER_VAR_TYPES_ENUM<SV_bvec3>
{
    typedef Math::CVec3b type;
};
template<> struct SHADER_VAR_TYPES_ENUM<SV_bvec4>
{
    typedef Math::CVec4b type;
};
template<> struct SHADER_VAR_TYPES_ENUM<SV_mat2>
{
    typedef Math::CMatrix2x2f type;
};
template<> struct SHADER_VAR_TYPES_ENUM<SV_mat3>
{
    typedef Math::CMatrix3x3f type;
};
template<> struct SHADER_VAR_TYPES_ENUM<SV_mat4>
{
    typedef Math::CMatrix4x4f type;
};
template<> struct SHADER_VAR_TYPES_ENUM<SV_sampler2D>
{
    typedef class Texture type;
};

namespace Internal
{
class UniformDataGeneric : public ManagedObject
{
public:
    UniformDataGeneric( ShaderVarTypes type ) : m_location( -1 ), m_type( type ) { }

    int m_location;
    const ShaderVarTypes m_type;
};

template<ShaderVarTypes U> class UniformData : public UniformDataGeneric
{
public:
    UniformData( void ) : UniformDataGeneric( U ) { }

    typename SHADER_VAR_TYPES_ENUM<U>::type m_value;
};
};

template<ShaderVarTypes T> class ShaderVar
{
public:
    ShaderVar( void ) : m_currentValue( 0 ) { }
    ShaderVar( managed_ptr<Internal::UniformData<T> > const & svalue ) : m_currentValue( svalue ) { }
    ShaderVar( managed_ptr<Internal::UniformData<T> > const * svalue ) : m_currentValue( *svalue ) { }

    ShaderVar<T> &operator = ( const typename SHADER_VAR_TYPES_ENUM<T>::type & value )
    {
        m_currentValue->m_value = value;
        return *this;
    }

    const typename SHADER_VAR_TYPES_ENUM<T>::type & value( void ) const
    {
        return m_currentValue->m_value;
    }

    bool isValid( void )
    {
        return m_currentValue != 0 ? true : false;
    }

private:
    managed_ptr<Internal::UniformData<T> > m_currentValue;
};

class FragmentShader
{
private:
    typedef const std::string Key;
    typedef managed_ptr<Internal::UniformDataGeneric> Value;

    class FragmentShaderData : public ManagedObject
    {
    public:
        FragmentShaderData( const char * fileName );
        ~FragmentShaderData( void );

        std::map<Key, Value> m_map;
        const std::string m_fileName;

        class FragmentShaderImpl * m_impl;
    };

public:
    FragmentShader( const char * fileName );
    virtual ~FragmentShader( void );

    template<ShaderVarTypes T> ShaderVar<T> var( const char * name )
    {
        std::map<Key, Value>::const_iterator iter;

        iter = m_private->m_map.find( name );
        if ( iter != m_private->m_map.end() )
        {
            if ( iter->second.get()->m_type == T )
            {
                return ShaderVar<T>(( managed_ptr<Internal::UniformData<T> > * )&iter->second );
            }

            return ShaderVar<T>();
        }

        std::pair<Key, Value> entry( name, new Internal::UniformData<T>() );
        m_private->m_map.insert( entry );

        return ShaderVar<T>(( managed_ptr<Internal::UniformData<T> > * )&entry.second );
    }

    bool reload( void );
    virtual void activate( void ) const;

private:
    managed_ptr<FragmentShaderData> m_private;
};

typedef enum
{
    VA_byte1,
    VA_byte2,
    VA_byte3,
    VA_byte4,
    VA_ubyte1,
    VA_ubyte2,
    VA_ubyte3,
    VA_ubyte4,
    VA_short1,
    VA_short2,
    VA_short3,
    VA_short4,
    VA_ushort1,
    VA_ushort2,
    VA_ushort3,
    VA_ushort4,
    VA_fixed1,
    VA_fixed2,
    VA_fixed3,
    VA_fixed4,
    VA_float1,
    VA_float2,
    VA_float3,
    VA_float4,
} VertexAttrTypes;

class VertexAttr
{
public:
    VertexAttr( VertexAttrTypes type, int offset ) : m_type( type ), m_offset( offset ) { }

private:
    const VertexAttrTypes m_type;
    const int m_offset;
};

typedef enum
{
    VB_static,
    VB_dynamic,
    VB_stream,
} VertexBufferTypes;

typedef enum
{
    IB_ubyte,
    IB_ushort,
} IndexBufferTypes;

namespace Internal
{
template<typename T> class Buffer
{
    void upload( void * data );
    void upload( int offset, int size, void * data );

    void * map( void );
    void unmap( void );

protected:
    managed_ptr<T> m_private;
};

class VertexBufferData
{
public:
    const VertexBufferTypes m_type;
    std::map<std::string, VertexAttr> m_attributeMap;

    class VertexBufferImpl * m_impl;
};

class IndexBufferData
{
public:
    const IndexBufferTypes m_type;
    class IndexBufferImpl * m_impl;
};
}

class VertexBuffer : public Internal::Buffer<Internal::VertexBufferData>
{
public:
    VertexBuffer( int vertexSize, int vertexCount, VertexBufferTypes type = VB_static );
    ~VertexBuffer( void );

    void add( const VertexAttr & attr );
};

class IndexBuffer : public Internal::Buffer<Internal::IndexBufferData>
{
public:
    IndexBuffer( int size, IndexBufferTypes type );
    ~IndexBuffer( void );
};

class VertexShader
{
private:
    typedef const std::string Key;
    typedef managed_ptr<Internal::UniformDataGeneric> Value;

    class VertexShaderData : public ManagedObject
    {
    public:
        VertexShaderData( const char * fileName );
        ~VertexShaderData( void );

        std::map<Key, Value> m_variableMap;
        std::map<Key, VertexAttr> m_attributeMap;
        const std::string m_fileName;

        class VertexShaderImpl * m_impl;
    };

public:
    VertexShader( const char * fileName );
    virtual ~VertexShader( void );

    template<ShaderVarTypes T> ShaderVar<T> var( const char * name )
    {
        std::map<Key, Value>::const_iterator iter;

        iter = m_private->m_variableMap.find( name );
        if ( iter != m_private->m_variableMap.end() )
        {
            if ( iter->second.get()->m_type == T )
            {
                return ShaderVar<T>(( managed_ptr<Internal::UniformData<T> > * )&iter->second );
            }

            return ShaderVar<T>();
        }

        std::pair<Key, Value> entry( name, new Internal::UniformData<T>() );
        m_private->m_variableMap.insert( entry );

        return ShaderVar<T>(( managed_ptr<Internal::UniformData<T> > * )&entry.second );
    }

    bool reload( void );
    virtual void activate( void );

private:
    managed_ptr<VertexShaderData> m_private;
};

class Texture
{
    friend class FragmentShader;
public:
    Texture( void );
    ~Texture( void );

    void generateMipmaps( void );

    void upload( void * data );
    void upload( int level, int x, int y, int width, int height, void * data );

    void * map( int level = 0 );
    void unmap( void );

private:
    managed_ptr<ManagedObject> m_private;
};

class FrameBuffer
{
private:
    class FrameBufferData : public ManagedObject
    {
    public:
        FrameBufferData( void );
        ~FrameBufferData( void );

        Texture m_color[4];
        Texture m_depth;

        class FrameBufferImpl * m_impl;
    };

public:
    FrameBuffer( void );
    ~FrameBuffer( void );

    void render( const FragmentShader & shader );
    bool isValid( void );

    Texture & color( int outputNum )
    {
        return m_private->m_color[outputNum];
    }
    Texture & depth( void )
    {
        return m_private->m_depth;
    }

private:
    managed_ptr<FrameBufferData> m_private;
};

class Wrapper
{
public:
    static Wrapper * GetInstance( void );

    void render( const VertexBuffer & vertexData );
    void render( const VertexBuffer & vertexData, const IndexBuffer & indexData );

private:
    Wrapper( void );
    ~Wrapper( void );

};

}

#endif
