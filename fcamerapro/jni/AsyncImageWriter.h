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
#ifndef _ASYNCIMAGEWRITER_H
#define _ASYNCIMAGEWRITER_H

/**
 * @file
 * Definition of FileFormatDescriptor, ImageSet and AsyncImageWriter.
 */

#include <FCam/Tegra.h>
#include <vector>
#include "WorkQueue.h"


/**
 * Defines output image settings such as file type and compression settings.
 */
class FileFormatDescriptor
{
    // TODO: add compression and save settings
public:
    /**
     * Enumeration of available output file format
     */
    enum EFormats
    {
        EFormatJPEG, EFormatTIFF, EFormatDNG, EFormatRAW
    };

    /**
     * Default constructor.
     * @param format specifies output format
     * @param quality defines the output compression quality
     */
    FileFormatDescriptor( EFormats format, int quality = 80 ) : m_format( format ), m_quality( quality ) { }

    /**
     * Default destructor.
     */
    ~FileFormatDescriptor( void ) { }

    /**
     * Gets the output file format.
     * @return output file format
     */
    EFormats getFormat( void )
    {
        return m_format;
    }

    /**
     * Gets output file compression quality.
     * @return output file compression quality
     */
    int getQuality( void )
    {
        return m_quality;
    }

private:

    EFormats m_format; /**< Output file format */
    int m_quality; /**< Output compression quality */
};

/**
 * Callback function type for AsyncImageWriter file system changed notification.
 */
typedef void ( *ASYNC_IMAGE_WRITER_CALLBACK )( void );

/**
 * Image set container. Stores a vector of references to FCam and corresponding
 * frame compression settings. ImageSet is a helper class and can be created
 * only through AsyncImageWriter::newImageSet(). Upon the construction, each
 * ImageSet is assigned an integer id which directly corresponds to the xml
 * descriptor file id.
 */
class ImageSet
{
    friend class AsyncImageWriter;
public:

    /**
     * Adds FCam frame to the image set with specific compression settings.
     * @param ff defines compression settings (file format, quality, etc.) for the frame
     * @param frame is a reference to FCam frame container
     */
    void add( const FileFormatDescriptor & ff, const FCam::Frame & frame );

private:
    /**
     * Default constructor.
     * @param id image set file id
     * @param outputDirPrefix contains absolute location of output directory
     */
    ImageSet( int id, const char * outputDirPrefix );
    /**
     * Default destructor.
     */
    ~ImageSet( void );

    /**
     * Writes the content of this ImageSet to the storage and invokes the callback
     * function to notify the output is ready.
     * @param proc pointer to a file system changed notification function
     */
    void dumpToFileSystem( ASYNC_IMAGE_WRITER_CALLBACK proc );

    std::vector<FCam::Frame> m_frames; /**< FCam frame vector */
    std::vector<FileFormatDescriptor> m_frameFormat; /**< Per frame output settings vector */
    const char * m_outputDirPrefix; /**< Output directory location */
    const int m_fileId; /**< ImageSet instance file id */
};

/**
 * Asynchronous image writer. Creates a separate work thread
 * upon construction and waits for the user to push ImageSet instances
 * to the queue.
 */

class AsyncImageWriter
{
public:
    /**
     * Default constructor.
     * @param outputDirPrefix contains absolute location of image
     * output directory
     */
    AsyncImageWriter( const char * outputDirPrefix );
    /**
     * Default destructor.
     */
    ~AsyncImageWriter( void );

    /**
     * Creates a new instance of ImageSet. Each instance has assigned a
     * unique integer (file id), which determines the xml descriptor
     * file name.
     * @return instance of ImageSet
     */
    ImageSet * newImageSet( void );

    /**
     * Adds an image set to output queue. The ownership of the pointer
     * is passed to AsyncImageWriter, i.e., after the ImageSet has been
     * written out, the worker thread will free the ImageSet resources.
     * @param is pointer to ImageSet storing frames
     */
    void push( ImageSet * is );

    /**
     * Sets a file system changed event callback. The function is
     * called by the worker thread after contents of ImageSet
     * instance have been dumped to the storage.
     * @param cb pointer to callback function
     */
    void setOnFileSystemChangedCallback( ASYNC_IMAGE_WRITER_CALLBACK cb );

    /**
     * Sets image set descriptor file id. This id will be assigned to next instance
     * of ImageSet produced with newImageSet().
     * @param id descriptor file id (should be a positive integer)
     */
    static void SetFreeFileId( int id );

private:
    char * m_outputDirPrefix; /**< Output directory location */
    WorkQueue<ImageSet *> m_queue; /**< Queue with ImageSet instances to be written */
    ASYNC_IMAGE_WRITER_CALLBACK m_onChangedCallback; /**< Callback function called when file system has been changed */

    pthread_t m_thread; /**< Worker thread handler */

    /**
     * Worker thread implementation. The thread sleeps until there is a new ImageSet
     * instance in the work queue. Then, for every instance of ImageSet in the queue,
     * ImageSet::dumpToFileSystem() function is invoked to write the ImageSet data to storage.
     */
    static void * ThreadProc( void * );


    static int sFreeId; /**< Integer storing next image set instance file id @see newImageSet()*/
};

#endif
