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
 * Implementation of ImageSet and AsyncImageWriter.
 */

#include <stdio.h>
#include <string.h>
#include "AsyncImageWriter.h"
#include "FCam/processing/JPEG.h"
#include "FCam/processing/DNG.h"
#include "FCam/processing/TIFF.h"
#include "FCam/FCam.h"
#include "Common.h"
#include "HPT.h"

#define THUMBNAIL_WIDTH   384 /**< Image thumbnail width in pixels */
#define THUMBNAIL_HEIGHT  288 /**< Image thumbnail height in pixels */
#define THUMBNAIL_QUALITY 95  /**< Image thumbnail JPEG compression quality (0-100) */

#define THUMBNAIL_BLUR_RADIUS 5 /**< Downsampling filter width */
#define THUMBNAIL_BLUR_NORM   (0x10000/(THUMBNAIL_BLUR_RADIUS*THUMBNAIL_BLUR_RADIUS))

static const char sXmlName[] = "img_%04i.xml"; /**< Image stack descriptor file name pattern */
static const char sImageName[] = "img_%04i_%02i.%s"; /**< Image file name pattern */
static const char sThumbnailName[] = "thumb_%04i_%02i.jpg"; /**< Image thumbnail file name pattern */
static const char sJpegExt[] = "jpg"; /**< JPEG file extension */

ImageSet::ImageSet( int id, const char * outputDirPrefix ) :
    m_outputDirPrefix( outputDirPrefix ), m_fileId( id )
{
}

ImageSet::~ImageSet( void )
{
}

void ImageSet::add( const FileFormatDescriptor & ff, const FCam::Frame & frame )
{
    m_frames.push_back( frame );
    m_frameFormat.push_back( ff );
}

/**
 * Performs box-filter downsampling of single color channel data.
 * @param dest is a pointer to a downsampled buffer
 * @param dstWidth defines destination width in pixels
 * @param dstHeight defines destination height in pixels
 * @param src is a pointer to source buffer
 * @param srcWidth defines source width in pixels
 * @param srcHeight defines source height in pixels
 */
static void DownsampleChannel( unsigned char * dest, int dstWidth, int dstHeight,
                               unsigned char * src, int srcWidth, int srcHeight )
{
    int dstindex = 0;
    int ty = 0;
    int ax = (( srcWidth - ( THUMBNAIL_BLUR_RADIUS & ~1 ) ) << 16 ) / dstWidth;
    int ay = (( srcHeight - ( THUMBNAIL_BLUR_RADIUS & ~1 ) ) << 16 ) / dstHeight;

    for ( int i = 0; i < dstHeight; i++ )
    {
        int rowindex = ( ty >> 16 ) * srcWidth;
        int tx = 0;
        for ( int j = 0; j < dstWidth; j++ )
        {
            // blur filter
            int sum = 0;
            int srcindex = rowindex + ( tx >> 16 );
            for ( int y = 0; y < THUMBNAIL_BLUR_RADIUS; y++ )
            {
                for ( int x = 0; x < THUMBNAIL_BLUR_RADIUS; x++ )
                {
                    sum += src[srcindex + x];
                }
                srcindex += srcWidth;
            }

            dest[dstindex++] = sum * THUMBNAIL_BLUR_NORM >> 16;
            tx += ax;
        }

        ty += ay;
    }

}

/**
 * Creates a thumbnail image from source frame.
 * It works only for YUV420p input frame format. The downsampling
 * algorithm uses box-filter to minimize aliasing artifacts.
 * @param dest is a reference to a thumbnail image
 * @param source is a reference to the source image
 */
static void CreateThumbnail( FCam::Image & dest, const FCam::Image & source )
{
    // works only for YUV420P
    if ( source.type() != FCam::YUV420p )
    {
        return;
    }

    int swidth = source.width();
    int sheight = source.height();
    int dwidth = dest.width();
    int dheight = dest.height();
    unsigned char * srcdata = source( 0, 0 );
    unsigned char * dstdata = dest( 0, 0 );

    int csize = swidth * sheight;
    int dcsize = dwidth * dheight;

    // Y
    DownsampleChannel( dstdata, dwidth, dheight, srcdata, swidth, sheight );
    // U
    dstdata += dcsize;
    srcdata += csize;
    dwidth >>= 1;
    dheight >>= 1;
    swidth >>= 1;
    sheight >>= 1;
    DownsampleChannel( dstdata, dwidth, dheight, srcdata, swidth, sheight );
    // V
    dstdata += dcsize >> 2;
    srcdata += csize >> 2;
    DownsampleChannel( dstdata, dwidth, dheight, srcdata, swidth, sheight );
}

void ImageSet::dumpToFileSystem(
    ASYNC_IMAGE_WRITER_CALLBACK onFileSystemChange )
{
    char fname[128];
    char buf[128];

    // TODO: add i/o error support
    int icount = 0;
    for ( int i = 0; i < m_frames.size(); i++ )
    {
        if ( m_frames[i].valid() )
        {
            icount++;
        }
    }

    if ( icount == 0 )
    {
        return;
    }

    // output xml file
    sprintf( fname, sXmlName, m_fileId );
    sprintf( buf, "%s%s", m_outputDirPrefix, fname );

    FILE * xml = fopen( buf, "wb" );

    fprintf( xml, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n" );
    fprintf( xml, "<imagestack imagecount=\"%i\">\n", icount );

    for ( int i = 0; i < m_frames.size(); i++ )
    {
        const FCam::Frame & frame = m_frames[i];

        if ( frame.valid() )
        {
            fprintf( xml, "<image " );
            // image name
            sprintf( fname, sImageName, m_fileId, i, sJpegExt );
            fprintf( xml, "name=\"%s\" ", fname );
            // thumbnail name
            sprintf( fname, sThumbnailName, m_fileId, i );
            fprintf( xml, "thumbnail=\"%s\" ", fname );
            // flash on/off
            FCam::Flash::Tags flashTags( frame );
            fprintf( xml, "flash=\"%i\" ", flashTags.brightness > 0.0f ? 1 : 0 );
            // gain
            fprintf( xml, "gain=\"%i\" ", ( int )( frame.gain() * 100 ) );
            // exposure
            fprintf( xml, "exposure=\"%i\" ", frame.exposure() );
            // whitebalance
            fprintf( xml, "wb=\"%i\" ", frame.whiteBalance() );
            // focus
            FCam::Lens::Tags lensTags( frame );
            fprintf( xml, "focus=\"%.2f\" ", lensTags.focus );


            fprintf( xml, "/>\n" );
        }
    }

    fprintf( xml, "</imagestack>\n" );
    fclose( xml );

    // notify fs change
    if ( onFileSystemChange != 0 )
    {
        onFileSystemChange();
    }

    FCam::Image thumbnail( THUMBNAIL_WIDTH, THUMBNAIL_HEIGHT, FCam::YUV420p );

    Timer timer;

    // output data files
    for ( int i = 0; i < m_frames.size(); i++ )
    {
        const FCam::Frame & frame = m_frames[i];
        if ( frame.valid() )
        {
            // write image
            switch ( m_frameFormat[i].getFormat() )
            {
                case FileFormatDescriptor::EFormatJPEG:
                    sprintf( fname, sImageName, m_fileId, i, sJpegExt );
                    sprintf( buf, "%s%s", m_outputDirPrefix, fname );
                    FCam::saveJPEG( frame, buf, m_frameFormat[i].getQuality() );
                    break;
            }

            // write thumbnail
            sprintf( fname, sThumbnailName, m_fileId, i );
            sprintf( buf, "%s%s", m_outputDirPrefix, fname );
            timer.tic();
            CreateThumbnail( thumbnail, frame.image() );
            LOG( "create thumbnail time: %.3f\n", timer.toc() );
            FCam::saveJPEG( thumbnail, buf, THUMBNAIL_QUALITY );

            // notify fs change
            if ( onFileSystemChange != 0 )
            {
                onFileSystemChange();
            }
        }
    }
}

// ==============================================================================

int AsyncImageWriter::sFreeId = 0;

void AsyncImageWriter::SetFreeFileId( int id )
{
    sFreeId = id;
}

AsyncImageWriter::AsyncImageWriter( const char * outputDirPrefix )
{
    // copy dir prefix

    // add '/' to the path if its not there already
    int slen = strlen( outputDirPrefix );
    if ( outputDirPrefix[slen - 1] != '/' )
    {
        m_outputDirPrefix = new char[slen + 2];
        strcpy( m_outputDirPrefix, outputDirPrefix );
        m_outputDirPrefix[slen] = '/';
        m_outputDirPrefix[slen + 1] = 0;
    }
    else
    {
        m_outputDirPrefix = new char[slen + 1];
        strcpy( m_outputDirPrefix, outputDirPrefix );
    }

    m_onChangedCallback = 0;
    // launch the work thread
    pthread_create( &m_thread, 0, AsyncImageWriter::ThreadProc, this );
}

AsyncImageWriter::~AsyncImageWriter( void )
{
    // null ImageSet terminates the work thread
    m_queue.produce( 0 );
    pthread_join( m_thread, 0 );

    delete[] m_outputDirPrefix;
}

ImageSet * AsyncImageWriter::newImageSet( void )
{
    char fname[128];
    char buf[128];
    FILE * f;

    for ( ;; )
    {
        sprintf( fname, sXmlName, sFreeId );
        sprintf( buf, "%s%s", m_outputDirPrefix, fname );

        f = fopen( buf, "rb" );
        if ( f == 0 )
        {
            break;
        }

        fclose( f );
        sFreeId++;
    }

    return new ImageSet( sFreeId++, m_outputDirPrefix );
}

void AsyncImageWriter::push( ImageSet * is )
{
    if ( is != 0 )
    {
        m_queue.produce( is );
    }
}

void AsyncImageWriter::setOnFileSystemChangedCallback(
    ASYNC_IMAGE_WRITER_CALLBACK cb )
{
    m_onChangedCallback = cb;
}

void * AsyncImageWriter::ThreadProc( void * opaque )
{
    AsyncImageWriter * instance = ( AsyncImageWriter * ) opaque;
    ImageSet * imageset;

    while ( instance->m_queue.consume( imageset, true ) )
    {
        if ( imageset == 0 )
        {
            // end of work, leave
            break;
        }

        imageset->dumpToFileSystem( instance->m_onChangedCallback );
        delete imageset;
    }

    return 0;
}

