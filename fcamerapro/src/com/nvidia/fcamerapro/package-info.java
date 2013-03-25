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
 * This package contains Java part of the implementation of FCameraPRO
 * application. {@link com.nvidia.fcamerapro.FCameraPROActivity} implements
 * Android activity and controls the application life-cycle. The main window
 * comprises a tab browser with tabs enclosing the following components:
 *
 * <h3>Top-level UI views</h3>
 * <ul>
 * <li> {@link com.nvidia.fcamerapro.CameraFragment} renders mono camera preview
 * together with components necessary for configuring capture settings.
 * <li> {@link com.nvidia.fcamerapro.StereoCameraFragment} renders stereo camera
 * preview together with components necessary for configuring capture settings.
 * <li> {@link com.nvidia.fcamerapro.ViewerFragment} allows to view and browse
 * through captured images. The {@code ViewerFragment} uses
 * {@link com.nvidia.fcamerapro.ImageStackManager} to manage and synchronize
 * with storage all captured images. Each image is abstracted by
 * {@link com.nvidia.fcamerapro.Image} class and in case of burst capture
 * grouped with others in {@link com.nvidia.fcamerapro.ImageStack}.
 * </ul>
 *
 * The communication with native FCam API is realized through
 * {@link com.nvidia.fcamerapro.FCamInterface}. Java components can receive
 * events related to camera and file system activity through
 * {@link com.nvidia.fcamerapro.FCamInterfaceEventListener}. The application
 * relies on a set of custom UI components:
 *
 * <h3>Classes related to UI components</h3>
 * <ul>
 * <li> {@link com.nvidia.fcamerapro.HistogramView} displays an image histogram.
 * Various objects can deliver histogram data to this view by implementing
 * {@link com.nvidia.fcamerapro.HistogramDataProvider}.
 * <li> {@link com.nvidia.fcamerapro.CameraView} displays camera preview using
 * OpenGL ES.
 * <li> {@link com.nvidia.fcamerapro.CameraViewRenderer} implements
 * {@link android.opengl.GLSurfaceView.Renderer} to process and render preview
 * frames with OpenGL.
 * <li> {@link com.nvidia.fcamerapro.Utils.ConfirmationDialogFragment} displays
 * confirmation (yes/no) message box.
 * <li> {@link com.nvidia.fcamerapro.Utils.InfoDialogFragment} displays
 * information (ok) message box.
 * </ul>
 */

package com.nvidia.fcamerapro;

