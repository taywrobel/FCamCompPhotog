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
package com.nvidia.fcamerapro;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;

import javax.xml.parsers.ParserConfigurationException;
import javax.xml.parsers.SAXParser;
import javax.xml.parsers.SAXParserFactory;

import org.xml.sax.Attributes;
import org.xml.sax.SAXException;
import org.xml.sax.helpers.DefaultHandler;

/**
 * Container for an image stack, a set of images captured with (optionally)
 * different parameters. The native code outputs xml file which describes
 * capture settings, file names, thumbnails of each image in the stack. This xml
 * can be read and interpreted directly by {@code ImageStack} constructor.
 */
public final class ImageStack implements Runnable {

    /**
     * {@link ImageStackManager} owner class. {@code ImageStack} uses the
     * storage location from its owner and sends notifications about state
     * change ({@code ImageStack} content is loaded asynchronously).
     */
    final private ImageStackManager mOwner;

    /**
     * Xml descriptor file name
     */
    final private String mFileName;

    /**
     * Image array
     */
    final private ArrayList<Image> mImages = new ArrayList<Image>();

    /**
     * Xml and thumbnail data loaded?
     */
    private boolean mLoadComplete;

    /**
     * Default image stack constructor. Parses xml descriptor file and creates
     * the image set. Afterwards, the thumbnail data can be loaded with
     * {@link #run()} from any thread.
     *
     * @param fileName
     *            contains absolute location of xml file
     * @param owner
     *            reference to {@link ImageStackManager}
     * @throws IOException
     */
    public ImageStack(String fileName, ImageStackManager owner) throws IOException {
        mOwner = owner;
        mFileName = fileName;

        // parse xml
        try {
            SAXParserFactory factory = SAXParserFactory.newInstance();
            SAXParser saxParser = factory.newSAXParser();

            DefaultHandler handler = new DefaultHandler() {
                public void startElement(String uri, String localName, String qname, Attributes attributes) throws SAXException {
                    if (qname.equals("image")) {
                        mImages.add(new Image(mOwner.getStorageDirectory(), attributes));
                    }
                }

                public void endElement(String uri, String localName, String qName) throws SAXException {
                }

                public void characters(char ch[], int start, int length) throws SAXException {
                }
            };

            saxParser.parse(new File(fileName), handler);
        } catch (SAXException e) {
            e.printStackTrace();
        } catch (ParserConfigurationException e) {
            e.printStackTrace();
        }
    }

    /**
     * Removes all the files associated with this image stack: images, image
     * thumbnails and xml descriptor file.
     */
    public void removeFromFileSystem() {
        for (Image image : mImages) {
            new File(image.getThumbnailName()).delete();
            new File(image.getName()).delete();
        }

        new File(mFileName).delete();
    }

    /**
     * Gets the image stack descriptor file name.
     *
     * @return descriptor file name
     */
    public String getName() {
        return mFileName;
    }

    /**
     * Returns true if thumbnail load has been completed.
     *
     * @return true if thumbnail load is complete
     */
    public boolean isLoadComplete() {
        return mLoadComplete;
    }

    /**
     * Gets the number of images in this stack.
     *
     * @return number of images in this stack
     */
    public int getImageCount() {
        return mImages.size();
    }

    /**
     * Gets the reference to a particular image.
     *
     * @param index
     *            denotes requested image index
     * @return reference to image at {@code index} position
     */
    public Image getImage(int index) {
        return mImages.get(index);
    }

    /**
     * Loads thumbnail data. Since the function is realized as an implementation
     * of {@link Runnable#run()}, we can perform loading in a separate thread.
     * After loading is completed successfully, the code sends an event to its
     * owner (see {@link ImageStackManager#notifyContentChange()})
     */
    public void run() {
        boolean loadComplete = true;
        for (Image image : mImages) {
            if (image.getThumbnail() == null) {
                if (!image.loadThumbnail()) {
                    loadComplete = false;
                    break;
                }
            }
        }

        mLoadComplete = loadComplete;
        mOwner.notifyContentChange();
    }
}
