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
#ifndef _UTILS_H
#define _UTILS_H

/**
 * @file
 *
 * Definition of utility functions.
 */

/**
 * Computes correlated color temperature of YCbCr pixel. The function assumes
 * the pixel color is denoted in sRGB color space with varying white point.
 * @param srcTemp source sRGB color space color temparature (6500K for a standard sRGB)
 * @param y pixel luma component
 * @param cb pixel chroma component
 * @param cr pixel chroma component
 * @return pixel color temparature in Kelwins
 */
int GetColorTemparatureYCbCr( int srcTemp, int y, int cb, int cr );

/**
 * Computes correlated color temperature of normalized sRGB pixel. The function assumes
 * the pixel color is denoted in sRGB color space with varying white point.
 * @param srcTemp source sRGB color space color temparature (6500K for a standard sRGB)
 * @param r pixel R color component
 * @param g pixel G color component
 * @param b pixel B color component
 * @return pixel color temparature in Kelwins
 */
int GetColorTemparature( float srcTemp, float r, float g, float b );

#endif
