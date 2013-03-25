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
import java.util.Arrays;
import java.util.regex.Pattern;

import android.app.ActionBar;
import android.app.ActionBar.Tab;
import android.app.Activity;
import android.app.DialogFragment;
import android.app.FragmentTransaction;
import android.os.Bundle;
import android.os.Environment;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;

/**
 * Main android activity. Apart from handling the application life-cycle, the
 * code creates and controls switching between different fragments (viewer, mono
 * camera, stereo camera).
 */
public final class FCameraPROActivity extends Activity implements ActionBar.TabListener {
	
    /**
     * Fragment tag enumeration
     */
    enum Fragments {
        /**
         * Camera fragment (mono image capture)
         */
        FRAGMENT_CAPTURE_MONO,
        /**
         * Camera fragment (stereo image capture)
         */
        FRAGMENT_CAPTURE_STEREO,
        /**
         * Image viewer fragment
         */
        FRAGMENT_VIEWER
    };

    /**
     * Reference to mono camera view fragment
     */
    private CameraFragment mCameraFragment;
    /**
     * Reference to stereo camera view fragment
     */
    private StereoCameraFragment mStereoCameraFragment;
    /**
     * Reference to the image viewer
     */
    private ViewerFragment mViewerFragment;

    /**
     * Absolute path to image storage directory
     */
    private String mStorageDirectory;

    /**
     * Returns the absolute path to image storage directory
     *
     * @return absolute path to image storage directory
     */
    public String getStorageDirectory() {
        return mStorageDirectory;
    }

    /**
     * Called when activity stops execution (goes background). Currently the
     * native FCam implementation does not support running background so we need
     * to kill the application.
     */
    @Override
    public void onStop() {
        // XXX: hack to kill background app instance
        System.exit(0);
    }

    /**
     * Called when activity starts. Gets (or creates if it does not exist) the
     * storage directory, enumerates next free file image id, creates camera and
     * viewer fragments and puts them into tabs.
     */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // create storage dir
        try {
            mStorageDirectory = Environment.getExternalStorageDirectory().getCanonicalPath() + File.separatorChar
                                + Settings.STORAGE_DIRECTORY;

            File dir = new File(mStorageDirectory);
            if (!dir.exists()) {
                if (!dir.mkdirs()) {
                    throw new IOException("Unable to create gallery storage!");
                }
            }
        } catch (IOException e) {
            e.printStackTrace();
        }

        // pass the storage location to the native code
        FCamInterface.GetInstance().setStorageDirectory(mStorageDirectory);

        // figure out first available stack id
        File dir = new File(mStorageDirectory);
        final Pattern pattern = Pattern.compile(Settings.IMAGE_STACK_PATTERN);
        String[] stackFileNames = dir.list(new FilenameFilter() {
            public boolean accept(File dir, String filename) {
                if (pattern.matcher(filename).matches()) {
                    return true;
                }
                return false;
            }
        });

        if (stackFileNames != null && stackFileNames.length > 0) {
            Arrays.sort(stackFileNames);
            // search
            int startIndex = Settings.IMAGE_STACK_PATTERN.indexOf("\\d");
            // XXX: assuming stack id will *always* have 4 digits
            int lastId = Integer.parseInt(stackFileNames[stackFileNames.length - 1].substring(startIndex, startIndex + 4));
            FCamInterface.GetInstance().setOutputFileId(lastId + 1);
        }

        // we keep a single instance of each fragment despite Android
        // Fragment API seems to destroy the fragment after each tab switch
        mCameraFragment = new CameraFragment();
        mViewerFragment = new ViewerFragment();
        mStereoCameraFragment = new StereoCameraFragment();

        setContentView(R.layout.main);

        // full-screen
        // requestWindowFeature(Window.FEATURE_NO_TITLE);
        // getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
        // WindowManager.LayoutParams.FLAG_FULLSCREEN);

        // add tabs
        ActionBar bar = getActionBar();
        bar.addTab(bar.newTab().setText(getResources().getString(R.string.menu_mono_capture)).setTabListener(this)
                   .setTag(Fragments.FRAGMENT_CAPTURE_MONO));
        bar.addTab(bar.newTab().setText(getResources().getString(R.string.menu_stereo_capture)).setTabListener(this)
                   .setTag(Fragments.FRAGMENT_CAPTURE_STEREO));
        bar.addTab(bar.newTab().setText(getResources().getString(R.string.menu_viewer)).setTabListener(this)
                   .setTag(Fragments.FRAGMENT_VIEWER));

        bar.setDisplayOptions(ActionBar.DISPLAY_SHOW_CUSTOM | ActionBar.DISPLAY_USE_LOGO);
        bar.setNavigationMode(ActionBar.NAVIGATION_MODE_TABS);
        bar.setDisplayShowHomeEnabled(true);
    }

    /**
     * Called when tab is reselected.
     */
    public void onTabReselected(Tab tab, FragmentTransaction ft) {
    }

    /**
     * Called when a new tab is selected.
     */
    public void onTabSelected(Tab tab, FragmentTransaction ft) {
        switch ((Fragments) tab.getTag()) {
        case FRAGMENT_CAPTURE_MONO:
            ft.replace(R.id.main_view, mCameraFragment);
            // TODO: check how to add support for `back' button
            if (ft.isAddToBackStackAllowed()) {
                ft.addToBackStack(null);
            }
            break;
        case FRAGMENT_CAPTURE_STEREO:
            ft.replace(R.id.main_view, mStereoCameraFragment);
            if (ft.isAddToBackStackAllowed()) {
                ft.addToBackStack(null);
            }
            break;
        case FRAGMENT_VIEWER:
            ft.replace(R.id.main_view, mViewerFragment);
            if (ft.isAddToBackStackAllowed()) {
                ft.addToBackStack(null);
            }
            break;
        }

        // switch menu
        invalidateOptionsMenu();
    }

    /**
     * Called when tab in unselected.
     */
    public void onTabUnselected(Tab tab, FragmentTransaction ft) {
    }

    /**
     * Shows top-layer message box
     *
     * @param title
     *            defines the message box tile
     * @param text
     *            stores the message box text
     */
    public void showDialog(String title, String text) {
        FragmentTransaction ft = getFragmentManager().beginTransaction();
        DialogFragment newFragment = Utils.InfoDialogFragment.NewInstance(title, text);
        newFragment.show(ft, "dialog");
    }

    /**
     * Loads fragment specific options menu (pull-down, top-right corner).
     */
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        switch ((Fragments) getActionBar().getSelectedTab().getTag()) {
        case FRAGMENT_CAPTURE_MONO:
            inflater.inflate(R.menu.camera_menu, menu);
            break;
        case FRAGMENT_CAPTURE_STEREO:
            inflater.inflate(R.menu.stereo_camera_menu, menu);
            break;
        case FRAGMENT_VIEWER:
            inflater.inflate(R.menu.viewer_menu, menu);
            break;
        }
        return true;
    }

    /**
     * Called when particular option menu is selected. Here we handle cases
     * common to option menus in all fragments, like "about".
     */
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
        case R.id.mi_about:
            showDialog(getResources().getString(R.string.menu_item_about), getResources().getString(R.string.copyright));
            return true;

        default:
            return super.onOptionsItemSelected(item);
        }
    }

}
