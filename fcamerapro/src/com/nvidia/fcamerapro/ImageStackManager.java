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

import java.io.FilenameFilter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Vector;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;
import java.util.regex.Pattern;

import android.os.Handler;
import android.widget.BaseAdapter;

/**
 * Abstract container for all image stacks. Its main purpose is to keep the
 * image stack structure synchronized with storage. It is responsible for
 * management, discovery, loading of new stacks, removing of non-existing
 * (deleted) stacks and scheduling asynchronous image thumbnail decompression.
 */
public final class ImageStackManager {
    /**
     * Absolute path to image storage directory
     */
    final private String mGalleryDir;

    /**
     * Thread pool used for thumbnail loading
     */
    final private ThreadPoolExecutor mThreadPool = new ThreadPoolExecutor(1, 4, 5, TimeUnit.SECONDS, new LinkedBlockingQueue<Runnable>());

    /**
     * To speed-up stack discovery we rely on a hash based path matching
     */
    final private HashMap<String, Object> mImageStackFilePaths = new HashMap<String, Object>();
    final private ArrayList<ImageStack> mImageStacks = new ArrayList<ImageStack>();

    /**
     * Content change event listeners
     */
    final private Vector<BaseAdapter> mContentChangeListenters = new Vector<BaseAdapter>();

    /**
     * UI thread handler. Needed for posting UI update messages directly from
     * the UI thread.
     */
    final private Handler mHandler = new Handler();

    /**
     * Default constructor.
     *
     * @param galleryDir
     *            absolute location of image storage directory
     */
    public ImageStackManager(String galleryDir) {
        mGalleryDir = galleryDir;
    }

    /**
     * Registers an event listener. When the state of a managed stack has
     * changed (e.g. new files discovered, stack removed, thumbnail loaded),
     * {@link ImageStackManager} emits
     * {@link BaseAdapter#notifyDataSetChanged()} event to all its listeners.
     *
     * @param listener
     *            contains reference to {@link BaseAdapter} interface
     */
    public void addContentChangeListener(BaseAdapter listener) {
        if (mContentChangeListenters.contains(listener)) {
            throw new IllegalArgumentException("listener already registered!");
        }

        mContentChangeListenters.add(listener);
    }

    /**
     * Removes an event listener previously registered with
     * {@link #addContentChangeListener(BaseAdapter)}.
     *
     * @param listener
     *            contains reference to {@link BaseAdapter} interface
     */
    public void removeContentChangeListener(FCamInterfaceEventListener listener) {
        if (!mContentChangeListenters.contains(listener)) {
            throw new IllegalArgumentException("listener not registered!");
        }

        mContentChangeListenters.remove(listener);
    }

    /**
     * Emits {@link BaseAdapter#notifyDataSetChanged()} event to all its
     * listeners. Normally this happens automatically when managed stack state
     * has changed (see {@link #addContentChangeListener(BaseAdapter)}).
     */
    public void notifyContentChange() {
        // notify data change from the UI thread
        mHandler.post(new Runnable() {
            public void run() {
                for (BaseAdapter listener : mContentChangeListenters) {
                    listener.notifyDataSetChanged();
                }
            }
        });
    }

    /**
     * Returns the image storage directory.
     *
     * @return image storage directory
     */
    public String getStorageDirectory() {
        return mGalleryDir;
    }

    /**
     * Searches the image storage directory for any changes, adds/removes image
     * stacks, schedules thumbnail loading. After this call is completed
     * {@link ImageStackManager} structure reflects the structure of image
     * storage directory.
     */
    public synchronized void refreshImageStacks() {
        // get xml list
        File dir = new File(mGalleryDir);
        final Pattern pattern = Pattern.compile(Settings.IMAGE_STACK_PATTERN);
        String[] stackFileNames = dir.list(new FilenameFilter() {
            public boolean accept(File dir, String filename) {
                if (pattern.matcher(filename).matches()) {
                    return true;
                }
                return false;
            }
        });

        if (stackFileNames == null) {
            // need zero sized array for stack remove notification
            stackFileNames = new String[0];
        }

        Arrays.sort(stackFileNames);

        try {
            // check out for deleted stacks
            HashMap<String, Object> stackFilePaths = new HashMap<String, Object>(stackFileNames.length);
            for (String fname : stackFileNames) {
                String filePath = mGalleryDir + File.separatorChar + fname;
                stackFilePaths.put(filePath, null);
            }

            boolean notifyContentChange = false;
            for (int i = 0; i < mImageStacks.size(); i++) {
                String imageStackFilePath = mImageStacks.get(i).getName();
                if (!stackFilePaths.containsKey(imageStackFilePath)) {
                    // remove the image stack from the ui (no file)
                    mImageStackFilePaths.remove(imageStackFilePath);
                    mImageStacks.remove(i);
                    notifyContentChange = true;
                    i--;
                }
            }

            // register new stacks
            for (int i = stackFileNames.length - 1; i >= 0; i--) {
                String filePath = mGalleryDir + File.separatorChar + stackFileNames[i];
                if (!mImageStackFilePaths.containsKey(filePath)) {
                    mImageStackFilePaths.put(filePath, null);
                    mImageStacks.add(stackFileNames.length - 1 - i, new ImageStack(filePath, this));
                }
            }

            // some stacks have been removed, notify listeners
            if (notifyContentChange) {
                notifyContentChange();
            }

            // queue stack load tasks
            for (ImageStack stack : mImageStacks) {
                if (!stack.isLoadComplete() && !mThreadPool.getQueue().contains(stack)) {
                    mThreadPool.execute(stack);
                }
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    /**
     * Gets the reference to a particular {@link ImageStack}.
     *
     * @param id
     *            denotes requested image stack index
     * @return reference to an image stack
     */
    public ImageStack getStack(int id) {
        return mImageStacks.get(id);
    }

    /**
     * Gets the total number of managed {@link ImageStack}.
     *
     * @return total number of managed image stacks
     */
    public int getStackCount() {
        return mImageStacks.size();
    }
}
