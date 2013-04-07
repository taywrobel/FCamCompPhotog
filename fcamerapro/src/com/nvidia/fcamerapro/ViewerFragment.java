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
import java.nio.Buffer;
import java.nio.ByteBuffer;
import java.util.ArrayList;

import Jama.Matrix;
import Jama.SingularValueDecomposition;
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
import android.util.Log;
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
		ImageStack istack = mImageStackManager.getStack(mSelectedStack);
		ArrayList<Buffer> imgBuffers = new ArrayList<Buffer>();
		int numPhotos = istack.getImageCount();
		int imW = 0, imH = 0;
		for(int i = 0; i < istack.getImageCount(); i++){

			String filename = new File(istack.getImage(i).getName()).getAbsolutePath();
			Log.d("algo", "Reading in " + filename);
//			Bitmap cur = BitmapFactory.decodeFile(filename);
			Bitmap cur = mImageStackManager.getStack(i).getImage(0).getThumbnail();
			cur = Bitmap.createScaledBitmap(cur, 32, 32, false);
			
			imW = cur.getWidth();
			imH = cur.getHeight();
			
			Log.d("size", ""+imW);
			Log.d("size", ""+imH);
			Log.d("size", ""+cur.getByteCount());
			Log.d("size", ""+(imW * imH * 4));
			
			ByteBuffer ib = ByteBuffer.allocate(cur.getByteCount());
			
			cur.copyPixelsToBuffer(ib);
			
			imgBuffers.add(ib);
		}
		
		Log.d("algo", "Read in " + imgBuffers.size() + " images.");
		Log.d("algo", "Image size is " + imW + " by " + imH);
		
		double zMin = Double.MAX_VALUE;
		double zMax = Double.MIN_VALUE;
		
		byte[][] imgDatas = new byte[imgBuffers.size()][];{
			int end = imgBuffers.size();
			int realIndex = 0;
			for(int i = 0; i < end; i++){
				imgDatas[realIndex] = (byte[])imgBuffers.get(0).array();
				realIndex++;
				imgBuffers.remove(i);
				end--;
				i--;
				System.gc();
			}
		}
		
		imgBuffers = null;
		System.gc();
		
		Log.d("algo", "Converted buffers into arrays");
		
		double[][][] zVals = new double[3][imgDatas[0].length/4][imgDatas.length];
		for(int channel = 0; channel < zVals.length; channel++){
			// Goes through each channel
			for(int imgNum = 0; imgNum < zVals[channel][0].length; imgNum++){
				// Goes through every image
				for(int pixelNum = 0; pixelNum < zVals[channel].length; pixelNum++){
					zVals[channel][pixelNum][imgNum] = imgDatas[imgNum][pixelNum * 4 + channel];
				}
			}
		}

		Log.d("algo", "Separated image color channels");
		
		imgDatas = null;
		System.gc();
			
		Matrix Z = new Matrix(zVals[0]);
		
		double[][] bVals = new double[numPhotos][1];
		for(int i = 0; i < bVals.length; i++){
			bVals[i][0] = (i * 4.0 / bVals.length) - 2.0;
		}
		
		Matrix B = new Matrix(bVals);
		bVals = null;
		System.gc();
		
		Log.d("algo", "Exposure info added to B array");
		
		double lambda = 1.0;
		
		double[][] wArr = new double[Z.getRowDimension()][1];
		for(int i = 0; i < wArr.length; i++){
			wArr[i][0] = 1.0;
		}
		Matrix w = new Matrix(wArr);
		wArr = null;
		System.gc();
		
		Log.d("algo", "Created weight matrix");
		
		// --------------------------------
		// - BEGIN INTERLACED MATLAB CODE -
		// --------------------------------
		
//		n = 256;
		int n = 256;
		
//		%Zeros returns a vector of all zeros
//		%Size in this case returns the size of the dimension specified by the second arg
//		%As in, size[(1 2 3), 2] would return 2
//		%in size(Z,1) we're returning the number of rows in Z
//		%then multiplying that by the num cols in Z
//		%This is using MATRIX multiplication rather than array multiplication
//		%then adding that to N+1
//		%for n+size(Z,1), we're getting the num rows in Z
//		%adding N to it
//		%AND THEEEEN we return a matrix of zeros based on those calculations


//		A = zeros(size(Z,1)*size(Z,2)+n+1,n+size(Z,1));
		int pixelCount = Z.getRowDimension();
		int imgCount = Z.getColumnDimension();
		double[][] AArr = new double[pixelCount * imgCount + n + 1][n + pixelCount];
		Matrix A = new Matrix(AArr);
		AArr = null;
		System.gc();
		

		Log.d("algo", "Created A matrix");
				
//		%Getting the num rows in A
//		%And then performing zeros on that element and 1 (1 dimensional vector of zeros)

//		b = zeros(size(A,1),1);
		Matrix b = new Matrix(new double[A.getRowDimension()][1]);
		

		Log.d("algo", "Created b matrix");
		
//		%% Include the data−fitting equations
		

//		k = 1;
		int k = 1;

		Log.d("algo", "Looping through each pixel");
//		for i=1:size(Z,1)  %num rows in Z (for loop working on elements 1 through num rows in Z)
		for( int i = 0; i < Z.getRowDimension(); i++){

			Log.d("algo", "Looping through each image");
//			for j=1:size(Z,2) %num cols in Z  (for loop working on elements 1 through num cols in Z)
			for(int j = 0; j < Z.getRowDimension(); j++){
				 
//				 %A few things going on here
//				 %Z(i,j) gets the value at row i and col j in Z
//				 %then we add 1
//				 %then we find THAT value in Matrix w
				 
//				 wij = w(Z(i,j)+1); 
				 double wij = w.get((int)(Z.get(i,j)+0.5), 0);
				 
//				 %A(k, Z(i,j) is finding the value at row i and col j in Z
//				 %then finding the value at K and (Z(i,j) in matrix A
//				 %Then setting that equal to the value wij
				 
//				 A(k,Z(i,j)+1) = wij; 
				 A.set(k, (int)(Z.get(i,j)+0.5), wij);
				 
//				 %Finds the value located at row K and col n+1 in matrix A
//				 %Then sets it equal to the negative value of wij
				 
//				 A(k,n+i) = −wij; 
				 A.set(k, n+i, -1 * wij);
				 
				 
//				 %Matrix multiplication on wij and the value located at row i in B
//				 %Then sets ^^^^ that value to the value located at row k and col 1 in b
				 
//				 b(k,1) = wij * B(i);
				 b.set(k, 0, wij * B.get(i, 0));
				 
//				 k=k+1;
				 k++;
				 
				 
//			 end %ends inner loop
			}
//		end  %ends outer loop
		}


//		%% Fix the curve by setting its middle value to 0

//		%sets the value located at row k and col 129 in A to 1
//		A(k,129) = 1;
		A.set(k, 128, 1.0);
//		k=k+1;
		k++;
		
		Log.d("algo", "Setting curve mid value to 0");

//		%% Include the smoothness equations

//		for i=1:n−2 %for loops starting at 1 going to n-2
		for(int i = 0; i < n-2; i++){
	
//			%matrix multiplication on l (lambda constant) to the value located at row i+1 in W
//			%then set ^^^^ that value to the location in row k and col i in A
	
//			A(k,i)=l*w(i+1); 
			A.set(k, i, lambda * w.get(i+1, 0));
			
//			%matrix multiplication on row i+1 in W by -2 * l (also matrix multiplication)
//			%Then sets that value in location row k col i+1 in matrix A
			
//			A(k,i+1)=−2*l*w(i+1);
			A.set(k, i+1, -2 * lambda * w.get(i+1, 0));
			
//			%matrix multiplication of l with location row i+1 in matrix w
//			%sets that value to location row k col i+2 in A
//			A(k,i+2)=l*w(i+1);
			A.set(k, i+2, lambda * w.get(i+1, 0));
			
//			k=k+1;
			k++;
//		end
		}
		
//		%% Solve the system using SVD

		Log.d("algo", "Doing SVD, bless our hearts.");
		
//		x = A\b;
		Matrix x;
		SingularValueDecomposition svd = new SingularValueDecomposition(A);
		x = svd.getU().times(svd.getS().inverse()).times(svd.getV().transpose()).times(b);
		
//		g = x(1:n);
		Matrix g = new Matrix(n,1);
		for(int i = 0; i < n; i++){
			g.set(i, 0, x.get(i,0));
		}
//		lE = x(n+1:size(x,1));
		
		// ------------------------------
		// - END INTERLACED MATLAB CODE -
		// ------------------------------
	
	}
}
