/* Copyright (c) 2019, NVIDIA CORPORATION. All rights reserved.
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

#ifndef NV_UTIL_NPP_IMAGE_IO_H
#define NV_UTIL_NPP_IMAGE_IO_H

#include "ImagesCPU.h"
#include "ImagesNPP.h"

#include "FreeImage.h"
#include "Exceptions.h"

#include <string>
#include "string.h"


// Error handler for FreeImage library.
//  In case this handler is invoked, it throws an NPP exception.
void
FreeImageErrorHandler(FREE_IMAGE_FORMAT oFif, const char *zMessage)
{
    throw npp::Exception(zMessage);
}

namespace npp
{
    // Load a rgb image from disk.
    void
    loadImage(const std::string &rFileName, ImageCPU_8u_C3 &rImage)
    {
        // set your own FreeImage error handler
        FreeImage_SetOutputMessage(FreeImageErrorHandler);

        FREE_IMAGE_FORMAT eFormat = FreeImage_GetFileType(rFileName.c_str());

        // no signature? try to guess the file format from the file extension
        if (eFormat == FIF_UNKNOWN)
        {
            eFormat = FreeImage_GetFIFFromFilename(rFileName.c_str());
        }

        NPP_ASSERT(eFormat != FIF_UNKNOWN);
        // check that the plugin has reading capabilities ...
        FIBITMAP *pBitmap;

        if (FreeImage_FIFSupportsReading(eFormat))
        {
            pBitmap = FreeImage_Load(eFormat, rFileName.c_str(), PNG_DEFAULT);
        }

        NPP_ASSERT(pBitmap != 0);
        // make sure this is an 8-bit single channel image
        // NPP_ASSERT(FreeImage_GetColorType(pBitmap) == FIC_MINISBLACK);
        // NPP_ASSERT(FreeImage_GetBPP(pBitmap) == 8);
        FIBITMAP *convertedBitmap = FreeImage_ConvertTo24Bits(pBitmap);
        FreeImage_Unload(pBitmap); // Unload the original bitmap

        // create an ImageCPU to receive the loaded image data
        int width = FreeImage_GetWidth(convertedBitmap);
        int height = FreeImage_GetHeight(convertedBitmap);
        ImageCPU_8u_C3 oImage(width, height);

        // Copy the FreeImage data into the new ImageCPU
        unsigned int nSrcPitch = FreeImage_GetPitch(convertedBitmap);
        const Npp8u *pSrcLine = FreeImage_GetBits(convertedBitmap) + nSrcPitch *(height - 1);
        Npp8u *pDstLine = oImage.data();
        unsigned int nDstPitch = oImage.pitch();

        for (int y = 0; y < height; ++y, pDstLine += oImage.pitch())
        {
            // sizeof(Npp8u) returns 1
            memcpy(pDstLine, pSrcLine - y * width * 3, width * 3);
        }

        // swap the user given image with our result image, effecively
        // moving our newly loaded image data into the user provided shell
        oImage.swap(rImage);
    }

    // Save a RGB image to disk.
    void
    saveImage(const std::string &rFileName, const ImageCPU_8u_C3 &rImage)
    {
        // create the result image storage using FreeImage so we can easily
        FIBITMAP *pResultBitmap = FreeImage_Allocate(rImage.width(), rImage.height(), 24);
        // FreeImage_Allocate(rImage.width(), rImage.height(), 24 /* bits per pixel */);
        NPP_ASSERT_NOT_NULL(pResultBitmap);
        unsigned int nDstPitch   = FreeImage_GetPitch(pResultBitmap);
        Npp8u *pDstLine = FreeImage_GetBits(pResultBitmap) + nDstPitch * (rImage.height()-1);
        unsigned int nSrcPitch = rImage.pitch();
        const Npp8u *pSrcLine = rImage.data() ;

        for (int y = 0; y < rImage.height(); ++y)
        {
            BYTE *pDstLine = FreeImage_GetScanLine(pResultBitmap, rImage.height() - 1 - y);
            memcpy(pDstLine, pSrcLine + y * nSrcPitch, rImage.width() * 3);
        }

        // now save the result image to PNG format
        bool bSuccess;
        bSuccess = FreeImage_Save(FIF_PNG, pResultBitmap, rFileName.c_str(), PNG_DEFAULT) == TRUE;
        NPP_ASSERT_MSG(bSuccess, "Failed to save result image.");
    }

    // Load a png RGB image from disk.
    void
    loadImage(const std::string &rFileName, ImageNPP_8u_C3 &rImage)
    {
        ImageCPU_8u_C3 oImage;
        loadImage(rFileName, oImage);
        ImageNPP_8u_C3 oResult(oImage);
        rImage.swap(oResult);
    }

    // Save a RGB image to disk.
    void
    saveImage(const std::string &rFileName, const ImageNPP_8u_C3 &rImage)
    {
        ImageCPU_8u_C3 oHostImage(rImage.size());
        // copy the device result data
        rImage.copyTo(oHostImage.data(), oHostImage.pitch());
        saveImage(rFileName, oHostImage);
    }
}


#endif // NV_UTIL_NPP_IMAGE_IO_H
