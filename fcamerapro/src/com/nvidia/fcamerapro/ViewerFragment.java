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
import java.net.MalformedURLException;

import Jama.Matrix;
import android.app.DialogFragment;
import android.app.Fragment;
import android.app.FragmentTransaction;
import android.content.DialogInterface;
import android.content.res.TypedArray;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Handler;
import android.view.ActionMode;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.webkit.WebView;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.AdapterView.OnItemLongClickListener;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.Gallery;
import android.widget.ImageView;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import com.nvidia.fcamerapro.FCamInterface.PreviewParams;

/**
 * Image viewer component. It is a simple image gallery where top row shows
 * available image stacks, and bottom row shows images in currently selected
 * stack. Basic capture information about images and stacks is displayed on the
 * side.
 */
public final class ViewerFragment extends Fragment implements FCamInterfaceEventListener, OnClickListener {
    /**
     * Reference to fragment view component created by
     * {@link #initContentView()}
     */
    private View mContentView;

    /**
     * References to gallery UI components
     */
    private Gallery mStackGallery, mImageGallery;
    private HistogramView mHistogram;
    private ImageStackManager mImageStackManager;
    private WebView mLargePreview;
    private View mGalleryPreview;
    private TextView mGalleryInfoLabel, mGalleryInfoValue;
    private Toast mPreviewHint;
    private Button mRadianceButton;

    /**
     * UI thread handler. Needed for posting UI update messages directly from
     * the UI thread.
     */
    final private Handler mHandler = new Handler();

    /**
     * Selected image stack
     */
    private int mSelectedStack;

    /**
     * Action handler for image large preview. Executed when user long presses
     * the image in a stack.
     */
    private ActionMode.Callback mLargePreviewActionModeCallback = new ActionMode.Callback() {
        /**
         * Shows image preview panel on top of the viewer and hides the system
         * status bar
         */
        public boolean onCreateActionMode(ActionMode actionMode, Menu menu) {
            actionMode.setTitle(R.string.label_large_preview);

            mGalleryPreview.setVisibility(View.INVISIBLE);
            mLargePreview.setVisibility(View.VISIBLE);

            mContentView.setSystemUiVisibility(View.STATUS_BAR_HIDDEN);
            // activity.getActionBar().hide();

            return true;
        }

        /**
         * NOP
         */
        public boolean onPrepareActionMode(ActionMode actionMode, Menu menu) {
            return false;
        }

        /**
         * NOP
         */
        public boolean onActionItemClicked(ActionMode actionMode, MenuItem menuItem) {
            return false;
        }

        /**
         * Hides image preview panel and brings back the status bar
         */
        public void onDestroyActionMode(ActionMode actionMode) {
            mLargePreview.loadUrl("about:blank");
            mLargePreview.setVisibility(View.INVISIBLE);
            mGalleryPreview.setVisibility(View.VISIBLE);
            mContentView.setSystemUiVisibility(View.STATUS_BAR_VISIBLE);
        }
    };

    /**
     * Updates side information page. The page contains information about
     * selected image stack (number of images, file name, etc.) and current
     * image (capture parameters, file name, etc.)
     *
     * @param stackIndex
     *            is an index of selected {@link ImageStack}
     * @param imageIndex
     *            is an index of selected {@link Image}
     */
    private void updateInfoPage(int stackIndex, int imageIndex) {
        if (!isVisible()) {
            // do not update fragment ui if it is not attached
            return;
        }

        String label = getResources().getString(R.string.label_info_page);
        String value = Integer.toString(mImageStackManager.getStackCount());

        if (mImageStackManager.getStackCount() != 0) {
            ImageStack istack = mImageStackManager.getStack(stackIndex);
            label += getResources().getString(R.string.label_info_stack);
            value += "\n" + Utils.GetFileName(istack.getName());
            value += "\n" + istack.getImageCount();
            
            if(istack.getImageCount() == 1){
            	mRadianceButton.setEnabled(false);
            } else {
            	mRadianceButton.setEnabled(true);
            }

            Image image = null;
            if (imageIndex >= 0 && imageIndex < istack.getImageCount()) {
                image = istack.getImage(imageIndex);
                label += "\n" + getResources().getString(R.string.label_info_image);
                value += "\n\n" + image.getInfo();
            }

            mHistogram.setDataProvider(image);
        } else {
            mHistogram.setDataProvider(null);
        }

        mGalleryInfoLabel.setText(label);
        mGalleryInfoValue.setText(value);
    }

    /**
     * Creates the viewer fragment and initializes UI components. Because the
     * android API tries to to destroy fragments between tab switches, we create
     * the view once and reuse it in later calls to
     * {@link #onCreateView(LayoutInflater, ViewGroup, Bundle)} or
     * {@link #onCreate(Bundle)}. This way we preserve the UI state between view
     * changes.
     */
    private void initContentView() {
        if (mContentView != null) {
            return;
        }

        // this fragment can be only used in fcamera app
        final FCameraPROActivity activity = ((FCameraPROActivity) getActivity());
        // load layout
        mContentView = activity.getLayoutInflater().inflate(R.layout.viewer, null);
        setHasOptionsMenu(true);

        TypedArray attr = activity.obtainStyledAttributes(R.styleable.Gallery);
        final int itemBackgroundStyleId = attr.getResourceId(R.styleable.Gallery_android_galleryItemBackground, 0);

        mGalleryInfoLabel = (TextView) mContentView.findViewById(R.id.tv_gallery_info_label);
        mGalleryInfoValue = (TextView) mContentView.findViewById(R.id.tv_gallery_info_value);
        mRadianceButton = (Button) mContentView.findViewById(R.id.radiance_gen);
        mRadianceButton.setOnClickListener(this);
        mHistogram = (HistogramView) mContentView.findViewById(R.id.gallery_histogram);

        mPreviewHint = Toast.makeText(activity, R.string.label_preview_hint, Toast.LENGTH_SHORT);

        mGalleryPreview = (View) mContentView.findViewById(R.id.gallery_preview);
        // webview for large picture preview
        mLargePreview = (WebView) mContentView.findViewById(R.id.gallery_large_preview);
        // mLargePreview.setLayerType(View.LAYER_TYPE_SOFTWARE, null); //
        // disable hw acceleration (android bug: white background if enabled)
        mLargePreview.setBackgroundColor(Color.BLACK);
        mLargePreview.getSettings().setBuiltInZoomControls(true);
        mLargePreview.getSettings().setUseWideViewPort(true);
        mLargePreview.getSettings().setLoadWithOverviewMode(true);

        // async loader
        mImageStackManager = new ImageStackManager(activity.getStorageDirectory());

        // stack gallery
        mStackGallery = (Gallery) mContentView.findViewById(R.id.stack_gallery);
        mStackGallery.setHorizontalFadingEdgeEnabled(true);
        mStackGallery.setAdapter(new BaseAdapter() {
            public int getCount() {
                return mImageStackManager.getStackCount();
            }

            public Object getItem(int position) {
                return position;
            }

            public long getItemId(int position) {
                return position;
            }

            public View getView(int position, View convertView, ViewGroup parent) {
                View thumbnail;

                if (convertView != null) {
                    thumbnail = convertView;
                } else {
                    thumbnail = activity.getLayoutInflater().inflate(R.layout.thumbnail_stack, null);
                    thumbnail.setBackgroundResource(itemBackgroundStyleId);
                }

                ImageView imageView = (ImageView) thumbnail.findViewById(R.id.thumbnail_image);
                ProgressBar busyBar = (ProgressBar) thumbnail.findViewById(R.id.thumbnail_busy);

                Bitmap bitmap = mImageStackManager.getStack(position).getImage(0).getThumbnail();
                if (bitmap != null) {
                    imageView.setImageBitmap(bitmap);
                }

                if (!mImageStackManager.getStack(position).isLoadComplete()) {
                    busyBar.setVisibility(View.VISIBLE);
                } else {
                    busyBar.setVisibility(View.INVISIBLE);
                }

                return thumbnail;
            }
        });
        // listen to content changes
        mImageStackManager.addContentChangeListener((BaseAdapter) mStackGallery.getAdapter());

        mStackGallery.setOnItemClickListener(new OnItemClickListener() {
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                mSelectedStack = position;
                ((BaseAdapter) mImageGallery.getAdapter()).notifyDataSetChanged();
            }
        });

        mStackGallery.setOnItemSelectedListener(new OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                updateInfoPage(mSelectedStack, mImageGallery.getSelectedItemPosition());
            }

            public void onNothingSelected(AdapterView<?> parent) {
                updateInfoPage(mSelectedStack, -1);
            }
        });

        mImageGallery = (Gallery) mContentView.findViewById(R.id.image_gallery);
        mImageGallery.setHorizontalFadingEdgeEnabled(true);
        mImageGallery.setAdapter(new BaseAdapter() {
            public int getCount() {
                if (mImageStackManager.getStackCount() != 0) {
                    return mImageStackManager.getStack(mSelectedStack).getImageCount();
                }

                return 0;
            }

            public void notifyDataSetChanged() {
                super.notifyDataSetChanged();
                updateInfoPage(mSelectedStack, mImageGallery.getSelectedItemPosition());
            }

            public Object getItem(int position) {
                return position;
            }

            public long getItemId(int position) {
                return position;
            }

            public View getView(int position, View convertView, ViewGroup parent) {
                View thumbnail;

                if (convertView != null) {
                    thumbnail = convertView;
                } else {
                    thumbnail = activity.getLayoutInflater().inflate(R.layout.thumbnail_image, null);
                    thumbnail.setBackgroundResource(itemBackgroundStyleId);
                }

                ImageView imageView = (ImageView) thumbnail.findViewById(R.id.thumbnail_image);
                ProgressBar busyBar = (ProgressBar) thumbnail.findViewById(R.id.thumbnail_busy);

                Bitmap bitmap = mImageStackManager.getStack(mSelectedStack).getImage(position).getThumbnail();
                if (bitmap != null) {
                    busyBar.setVisibility(View.INVISIBLE);
                    imageView.setImageBitmap(bitmap);
                } else {
                    busyBar.setVisibility(View.VISIBLE);
                }

                return thumbnail;
            }
        });

        mImageGallery.setOnItemLongClickListener(new OnItemLongClickListener() {
            public boolean onItemLongClick(AdapterView<?> parent, View view, int position, long id) {
                Image image = mImageStackManager.getStack(mSelectedStack).getImage(position);
                if (image.getThumbnail() != null) {
                    activity.startActionMode(mLargePreviewActionModeCallback);
                    try {
                        mLargePreview.loadUrl(new File(image.getName()).toURL().toString());
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                }

                return true;
            }
        });

        // mImageGallery.setOnItemClickListener(new OnItemClickListener() {
        // public void onItemClick(AdapterView<?> parent, View view, int
        // position, long id) {
        // if (position == 0) {
        // mPreviewHint.show();
        // }
        // }
        // });

        mImageGallery.setOnItemSelectedListener(new OnItemSelectedListener() {
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                updateInfoPage(mSelectedStack, position);
            }

            public void onNothingSelected(AdapterView<?> parent) {
                updateInfoPage(mSelectedStack, -1);
            }
        });

        // listen to content changes
        mImageStackManager.addContentChangeListener((BaseAdapter) mImageGallery.getAdapter());
    }

    /**
     * Shows confirmation dialog.
     *
     * @param title
     *            a dialog title text
     * @param text
     *            stores dialog message
     * @param onConfirmListener
     *            a listener called upon a positive button press event (yes)
     */
    private void showConfirmationDialog(String title, String text, DialogInterface.OnClickListener onConfirmListener) {
        FragmentTransaction ft = getFragmentManager().beginTransaction();
        DialogFragment newFragment = Utils.ConfirmationDialogFragment.NewInstance(title, text, onConfirmListener);
        newFragment.show(ft, "dialog");
    }

    /**
     * Called when particular option menu is selected. Here we handle cases
     * specific only to viewer fragment.
     */
    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
        case R.id.mi_delete:
            if (mImageStackManager.getStackCount() != 0 && mImageStackManager.getStack(mSelectedStack).isLoadComplete()) {
                showConfirmationDialog(
                    getResources().getString(R.string.menu_item_delete_stack),
                    String.format(getResources().getString(R.string.confirmation_delete_stack),
                                  Utils.GetFileName(mImageStackManager.getStack(mSelectedStack).getName())),
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        mImageStackManager.getStack(mSelectedStack).removeFromFileSystem();
                        if (mSelectedStack != 0 && mSelectedStack == mImageStackManager.getStackCount() - 1) {
                            mSelectedStack--;
                        }
                        mImageStackManager.refreshImageStacks();
                    }
                });
            }
            return true;

        default:
            return super.onOptionsItemSelected(item);
        }
    }

    /**
     * Initializes viewer fragment.
     */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        initContentView();
    }

    /**
     * Registers viewer fragment as {@link FCamInterface} event listener and
     * refreshes the image stack view.
     *
     * @return reference to viewer fragment
     */
    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        FCamInterface.GetInstance().addEventListener(this);

        // refresh image stack on view creation
        mImageStackManager.refreshImageStacks();

        // move to the begining of the stack
        if (mImageStackManager.getStackCount() != 0) {
            mHandler.post(new Runnable() {
                public void run() {
                    mStackGallery.setSelection(0);
                }
            });
        } else {
            // empty gallery?
            updateInfoPage(0, 0);
        }

        mPreviewHint.show();

        return mContentView;
    }

    /**
     * Removes viewer fragment instance from {@link FCamInterface} event
     * listeners.
     */
    @Override
    public void onDestroyView() {
        super.onDestroyView();

        mPreviewHint.cancel();

        FCamInterface.GetInstance().removeEventListener(this);
    }

    /**
     * NOP
     */
    public void onCaptureStart() {
    }

    /**
     * NOP
     */
    public void onCaptureComplete() {
    }

    /**
     * Updates the image stack view after file system has been changed (new
     * images arrived, image stack has been added/removed)
     */
    public void onFileSystemChange() {
        mImageStackManager.refreshImageStacks();
    }

    /**
     * NOP
     */
    public void onPreviewParamChange(PreviewParams paramId) {
    }

	@Override
	public void onClick(View arg0) {
		// TODO: Write code for radiance generation here
		ImageStack istack = mImageStackManager.getStack(mSelectedStack);
		Bitmap[] images = new Bitmap[istack.getImageCount()];
		int[][] pixels = new int[images.length][];
		for(int i = 0; i < images.length; i++){
			images[i] = BitmapFactory.decodeFile(new File(istack.getImage(i).getName()).toURI().toString());
			images[i].getPixels(pixels[i], 0, 0, 0, 0, images[i].getWidth(), images[i].getHeight());
		}
		
		double[][] zVals = new double[pixels[0].length][images.length];
		for(int i = 0; i < zVals.length; i++){
			for(int j = 0; j < zVals[i].length; i++){
				zVals[i][j] = pixels[j][i];
			}
		}
	
		Matrix Z = new Matrix(zVals);
		
		double[][] bVals = new double[images.length][1];
		for(int i = 0; i < bVals.length; i++){
			bVals[i][0] = (i * 4.0 / bVals.length) - 2.0;
		}
		
		Matrix B = new Matrix(bVals);
		
		double lambda = 1.0;
		
	}
}
