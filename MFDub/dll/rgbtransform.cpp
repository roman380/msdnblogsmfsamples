// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE
//
// Copyright (c) Microsoft Corporation.  All rights reserved.

#include "stdafx.h"

#include "rgbtransform.h"
#include "rgbutils.h"
#include "intsafe.h"

/////////////////////////////////////////
// CLocalAverageTransform
// Performs a local average for each pixel in the image.
CLocalAverageTransform::CLocalAverageTransform(UINT32 unLocalSize)
    : m_unLocalSize(unLocalSize)
{
}

void CLocalAverageTransform::Transform(UINT32 unWidth, UINT32 unHeight, RGBQUAD* pImageIn, RGBQUAD* pImageOut)
{
    for(UINT32 i = m_unLocalSize; i < unHeight - m_unLocalSize; i++)
    {
        for(UINT32 j = m_unLocalSize; j < unWidth - m_unLocalSize; j++)
        {
            UINT32 unRedTotal = 0, unGreenTotal = 0, unBlueTotal = 0;
            for(UINT32 k = 0; k < m_unLocalSize; k++)
            {
                for(UINT32 l = 0; l < m_unLocalSize; l++)
                {
                    RGBQUAD* pPixel = pImageIn + (i - k) * unWidth + j - l;
                    unRedTotal += pPixel->rgbRed;
                    unGreenTotal += pPixel->rgbGreen;
                    unBlueTotal += pPixel->rgbBlue;
                    
                    pPixel = pImageIn + (i - k) * unWidth + j + l;
                    unRedTotal += pPixel->rgbRed;
                    unGreenTotal += pPixel->rgbGreen;
                    unBlueTotal += pPixel->rgbBlue;
                    
                    pPixel = pImageIn + (i + k) * unWidth + j - l;
                    unRedTotal += pPixel->rgbRed;
                    unGreenTotal += pPixel->rgbGreen;
                    unBlueTotal += pPixel->rgbBlue;
                    
                    pPixel = pImageIn + (i + k) * unWidth + j + l;
                    unRedTotal += pPixel->rgbRed;
                    unGreenTotal += pPixel->rgbGreen;
                    unBlueTotal += pPixel->rgbBlue;
                }
            }

            unRedTotal /= (2 * m_unLocalSize);
            unGreenTotal /= (2 * m_unLocalSize);
            unBlueTotal /= (2 * m_unLocalSize);

            pImageOut[i * unWidth + j].rgbRed = (BYTE) unRedTotal;
            pImageOut[i * unWidth + j].rgbGreen = (BYTE) unGreenTotal;
            pImageOut[i * unWidth + j].rgbBlue = (BYTE) unBlueTotal;
        }
    }
}

/////////////////////////////////////////
// CLocalMedianTransform
// Performs a local median for each pixel in the image.
CLocalMedianTransform::CLocalMedianTransform(UINT32 unLocalSize)
    : m_unLocalSize(unLocalSize)
{
}

void CLocalMedianTransform::Transform(UINT32 unWidth, UINT32 unHeight, RGBQUAD* pImageIn, RGBQUAD* pImageOut)
{
    UINT32 unRowSize = m_unLocalSize * 2 + 1;
    UINT32 unNhoodSize = unRowSize * unRowSize - 1;
    
    CAtlArray<INT32> arrRedValues;
    CAtlArray<INT32> arrGreenValues;
    CAtlArray<INT32> arrBlueValues;
    
    pImageOut += m_unLocalSize * unWidth;
    for(UINT32 i = m_unLocalSize; i < unHeight - m_unLocalSize; i++)
    {
        pImageOut += m_unLocalSize;
        for(UINT32 j = m_unLocalSize; j < unWidth - m_unLocalSize; j++)
        {
            RGBQUAD* pCurrentNhood = pImageIn;
            
            arrRedValues.RemoveAll();
            arrGreenValues.RemoveAll();
            arrBlueValues.RemoveAll();
            
            arrRedValues.Add(0x7FFFFFFF);
            arrGreenValues.Add(0x7FFFFFFF);
            arrBlueValues.Add(0x7FFFFFFF);
            
            DWORD cCount = 0;
            for(UINT32 k = 0; k <= unNhoodSize; k++)
            {
                // The middle of the neighborhood is the current pixel, don't want to include this in our median
                if(k == unNhoodSize >> 1)
                {
                    continue;
                }
                
                for(size_t l = 0; l < arrRedValues.GetCount(); l++)
                {
                    if(pCurrentNhood->rgbRed < arrRedValues.GetAt(l))
                    {
                        arrRedValues.InsertAt(l, pCurrentNhood->rgbRed);
                        break;
                    }
                }
                
                for(size_t l = 0; l < arrGreenValues.GetCount(); l++)
                {
                    if(pCurrentNhood->rgbGreen < arrGreenValues.GetAt(l))
                    {
                        arrGreenValues.InsertAt(l, pCurrentNhood->rgbGreen);
                        break;
                    }
                }
                
                for(size_t l = 0; l < arrBlueValues.GetCount(); l++)
                {
                    if(pCurrentNhood->rgbBlue < arrBlueValues.GetAt(l))
                    {
                        arrBlueValues.InsertAt(l, pCurrentNhood->rgbBlue);
                        break;
                    }
                }
                
                pCurrentNhood++;
                cCount++;
                if(cCount >= unRowSize - 1)
                {
                    cCount = 0;
                    pCurrentNhood += unWidth - unRowSize + 1;
                }
            }

            INT32 iRedMedian = arrRedValues.GetAt(unNhoodSize >> 1);
            INT32 iGreenMedian = arrGreenValues.GetAt(unNhoodSize >> 1);
            INT32 iBlueMedian = arrBlueValues.GetAt(unNhoodSize >> 1);

            pImageOut->rgbRed = (BYTE) iRedMedian;
            pImageOut->rgbGreen = (BYTE) iGreenMedian;
            pImageOut->rgbBlue = (BYTE) iBlueMedian;
            
            pImageOut++;
            pImageIn++;
        }
        
        pImageOut += m_unLocalSize;
        pImageIn += unRowSize - 1;
    }
}

/////////////////////////////////////////
// CUnsharpTransform
// Apply an unsharp mask to the image.  For gamma > 1.0, this sharpens the image.
// For gamma < 1.0, this blurs the image.
CUnsharpTransform::CUnsharpTransform(UINT32 unLocalSize, float gamma)
    : m_unLocalSize(unLocalSize), m_gamma(gamma)
{
}


void CUnsharpTransform::Transform(UINT32 unWidth, UINT32 unHeight, RGBQUAD* pImageIn, RGBQUAD* pImageOut)
{
    UINT32 unRowSize = m_unLocalSize * 2 + 1;
    const UINT32 unNhoodSize = unRowSize * unRowSize - 1;
    float invGamma = 1.0f - m_gamma;
    
    // Copy border pixels -- since they do not have neighbors on at least one
    // side, the unsharp mask will not be applied to them.
    for(UINT32 i = 0; i < unHeight; i++)
    {
        pImageOut[i * unWidth] = pImageIn[i * unWidth];
        pImageOut[i * unWidth + unWidth - 1] = pImageIn[i * unWidth + unWidth - 1];
    }
    
    for(UINT32 j = 0; j < unWidth; j++)
    {
        pImageOut[j] = pImageIn[j];
        pImageOut[(unHeight - 1) * unWidth + j] = pImageIn[(unHeight - 1) * unWidth + j];
    }
    
    // Precalculate applying gamma to original colors.  The result of this calculation
    // could be made static and used for future transforms if more CPU efficiency
    // is necessary.
    UINT32 aunOrigToGamma[256];
    for(UINT32 i = 0; i < 256; i++)
    {
        aunOrigToGamma[i] = static_cast<UINT32>(float(i) * m_gamma);
    }
    
    // Precalculate applying gamma to sum of the pixel's neighborhood.  Possible values 
    // of 0 ... 255 * neighborhood size.  This could also be calculated once and stored.
    UINT32* aunAvgToGamma = new UINT32[unNhoodSize * 255 + 1];
    for(UINT32 i = 0; i <= unNhoodSize * 255; i++)
    {
        aunAvgToGamma[i] = static_cast<UINT32>(float(i) / float(unNhoodSize) * invGamma);
    }
    
    pImageOut += m_unLocalSize * unWidth;
    for(UINT32 i = m_unLocalSize; i < unHeight - m_unLocalSize; i++)
    {
        pImageOut += m_unLocalSize;
        for(UINT32 j = m_unLocalSize; j < unWidth - m_unLocalSize; j++)
        {
            // Sum the values of all pixels in the neighborhood, and then apply gamma.
            INT32 unRedTotal = 0, unGreenTotal = 0, unBlueTotal = 0;
            RGBQUAD* pOrigPixel = NULL;
            RGBQUAD* pCurrentNhood = pImageIn;
            DWORD cCount = 0;
            for(UINT32 k = 0; k <= unNhoodSize; k++)
            {
                // The middle of the neighborhood is the current pixel; don't want to include 
                // this in our total but save the values for later
                if(k == unNhoodSize >> 1)
                {
                    pOrigPixel = pCurrentNhood;
                    continue;
                }
                
                unRedTotal += pCurrentNhood->rgbRed;
                unGreenTotal += pCurrentNhood->rgbGreen;
                unBlueTotal += pCurrentNhood->rgbBlue;
                
                pCurrentNhood++;
                cCount++;
                if(cCount >= unRowSize)
                {
                    cCount = 0;
                    pCurrentNhood += unWidth - unRowSize;
                }
            }
         
            unRedTotal = aunOrigToGamma[pOrigPixel->rgbRed] + aunAvgToGamma[unRedTotal];
            unGreenTotal = aunOrigToGamma[pOrigPixel->rgbGreen] + aunAvgToGamma[unGreenTotal];
            unBlueTotal = aunOrigToGamma[pOrigPixel->rgbBlue] + aunAvgToGamma[unBlueTotal];
            
            // Fix up overflow or underflow.
            if(unRedTotal < 0) unRedTotal = 0;
            if(unGreenTotal < 0) unGreenTotal = 0;
            if(unBlueTotal < 0) unBlueTotal = 0;
            if(unRedTotal > 255) unRedTotal = 255;
            if(unGreenTotal > 255) unGreenTotal = 255;
            if(unBlueTotal > 255) unBlueTotal = 255;

            pImageOut->rgbRed = (BYTE) unRedTotal;
            pImageOut->rgbGreen = (BYTE) unGreenTotal;
            pImageOut->rgbBlue = (BYTE) unBlueTotal;
            
            pImageOut++;
            pImageIn++;
        }
        pImageOut += m_unLocalSize;
        pImageIn += unRowSize - 1;
    }
    
    delete[] aunAvgToGamma;
}

//////////////////////////////////////////////////////
// CMinTransform
// Replaces pixel values with the minimum pixel value from
// a non-diagonal neighborhood (neighborhood looks like a cross).
CMinTransform::CMinTransform()
{
}

void CMinTransform::Transform(UINT32 unWidth, UINT32 unHeight, RGBQUAD* pImageIn, RGBQUAD* pImageOut)
{
    RGBQUAD* pRowAbove = pImageIn;
    RGBQUAD* pRowCurrent = pImageIn + unWidth;
    RGBQUAD* pRowBelow = pImageIn + unWidth + unWidth;
    RGBQUAD* pRowOut = pImageOut + unWidth;
    for(UINT32 i = 1; i < unHeight - 1; i++)
    {
        for(UINT32 j = 1; j < unWidth - 1; j++)
        {
            RGBQUAD MinValues;

            MinValues = pRowAbove[j];
            
            MinValues.rgbRed = (pRowCurrent[j - 1].rgbRed < MinValues.rgbRed) ? pRowCurrent[j - 1].rgbRed : MinValues.rgbRed;
            MinValues.rgbGreen = (pRowCurrent[j - 1].rgbGreen  < MinValues.rgbGreen) ? pRowCurrent[j - 1].rgbGreen : MinValues.rgbGreen;
            MinValues.rgbBlue = (pRowCurrent[j - 1].rgbBlue < MinValues.rgbBlue) ? pRowCurrent[j - 1].rgbBlue : MinValues.rgbBlue;
            
            MinValues.rgbRed = (pRowCurrent[j + 1].rgbRed < MinValues.rgbRed) ? pRowCurrent[j + 1].rgbRed : MinValues.rgbRed;
            MinValues.rgbGreen = (pRowCurrent[j + 1].rgbGreen  < MinValues.rgbGreen) ? pRowCurrent[j + 1].rgbGreen : MinValues.rgbGreen;
            MinValues.rgbBlue = (pRowCurrent[j + 1].rgbBlue < MinValues.rgbBlue) ? pRowCurrent[j + 1].rgbBlue : MinValues.rgbBlue;
            
            MinValues.rgbRed = (pRowBelow[j].rgbRed < MinValues.rgbRed) ? pRowBelow[j].rgbRed : MinValues.rgbRed;
            MinValues.rgbGreen = (pRowBelow[j].rgbGreen  < MinValues.rgbGreen) ? pRowBelow[j].rgbGreen : MinValues.rgbGreen;
            MinValues.rgbBlue = (pRowBelow[j].rgbBlue < MinValues.rgbBlue) ? pRowBelow[j].rgbBlue : MinValues.rgbBlue;
            
            pRowOut[j] = MinValues;
        }
        
        pRowAbove = pRowCurrent;
        pRowCurrent = pRowBelow;
        pRowBelow += unWidth;
        pRowOut += unWidth;
    }
}

//////////////////////////////////////////////////////
// CMaxTransform
// Replaces pixel values with the maximum pixel value from
// a non-diagonal neighborhood (neighborhood looks like a cross).
CMaxTransform::CMaxTransform()
{
}

void CMaxTransform::Transform(UINT32 unWidth, UINT32 unHeight, RGBQUAD* pImageIn, RGBQUAD* pImageOut)
{
    RGBQUAD* pRowAbove = pImageIn;
    RGBQUAD* pRowCurrent = pImageIn + unWidth;
    RGBQUAD* pRowBelow = pImageIn + unWidth + unWidth;
    RGBQUAD* pRowOut = pImageOut + unWidth;
    for(UINT32 i = 1; i < unHeight - 1; i++)
    {
        for(UINT32 j = 1; j < unWidth - 1; j++)
        {
            RGBQUAD MaxValues;

            MaxValues = pRowAbove[j];
            
            MaxValues.rgbRed = (pRowCurrent[j - 1].rgbRed > MaxValues.rgbRed) ? pRowCurrent[j - 1].rgbRed : MaxValues.rgbRed;
            MaxValues.rgbGreen = (pRowCurrent[j - 1].rgbGreen > MaxValues.rgbGreen) ? pRowCurrent[j - 1].rgbGreen : MaxValues.rgbGreen;
            MaxValues.rgbBlue = (pRowCurrent[j - 1].rgbBlue > MaxValues.rgbBlue) ? pRowCurrent[j - 1].rgbBlue : MaxValues.rgbBlue;
            
            MaxValues.rgbRed = (pRowCurrent[j + 1].rgbRed > MaxValues.rgbRed) ? pRowCurrent[j + 1].rgbRed : MaxValues.rgbRed;
            MaxValues.rgbGreen = (pRowCurrent[j + 1].rgbGreen  > MaxValues.rgbGreen) ? pRowCurrent[j + 1].rgbGreen : MaxValues.rgbGreen;
            MaxValues.rgbBlue = (pRowCurrent[j + 1].rgbBlue > MaxValues.rgbBlue) ? pRowCurrent[j + 1].rgbBlue : MaxValues.rgbBlue;
            
            MaxValues.rgbRed = (pRowBelow[j].rgbRed > MaxValues.rgbRed) ? pRowBelow[j].rgbRed : MaxValues.rgbRed;
            MaxValues.rgbGreen = (pRowBelow[j].rgbGreen  > MaxValues.rgbGreen) ? pRowBelow[j].rgbGreen : MaxValues.rgbGreen;
            MaxValues.rgbBlue = (pRowBelow[j].rgbBlue > MaxValues.rgbBlue) ? pRowBelow[j].rgbBlue : MaxValues.rgbBlue;
            
            pRowOut[j] = MaxValues;
        }
            
        pRowAbove = pRowCurrent;
        pRowCurrent = pRowBelow;
        pRowBelow += unWidth;
        pRowOut += unWidth;
    }
}

//////////////////////////////////////////////////////
// CNoiseRemovalTransform
// Removes small noise from the image.
CNoiseRemovalTransform::CNoiseRemovalTransform()
{
}

void CNoiseRemovalTransform::Transform(UINT32 unWidth, UINT32 unHeight, RGBQUAD* pImageIn, RGBQUAD* pImageOut)
{
    size_t AllocLen;
    if( FAILED(SizeTMult(size_t(unWidth), size_t(unHeight), &AllocLen)) )
    {
        return;
    }
    
    RGBQUAD* pImageTemp = new RGBQUAD[AllocLen];

    CMaxTransform MaxTransform;
    CMinTransform MinTransform;

    // Applying Min/Max/Max/Min does a good job of removing small anomalies
    // in the image while not affecting the overall image quality too much.
    // Processing the image four times is pretty computationally expensive,
    // however.  Improvements could be made here if necessary.
    MinTransform.Transform(unWidth, unHeight, pImageIn, pImageTemp);
    MaxTransform.Transform(unWidth, unHeight, pImageTemp, pImageOut);
    MaxTransform.Transform(unWidth, unHeight, pImageOut, pImageTemp);
    MinTransform.Transform(unWidth, unHeight, pImageTemp, pImageOut);

    delete[] pImageTemp;
}

////////////////////////////////////////////////////
// CHistogramEqualizationTransform
// Improves the image contrast through histogram equalization.
CHistogramEqualizationTransform::CHistogramEqualizationTransform()
{
}

void CHistogramEqualizationTransform::Transform(UINT32 unWidth, UINT32 unHeight, RGBQUAD* pImageIn, RGBQUAD* pImageOut)
{
    UINT32 arrRedHist[256];
    UINT32 arrGreenHist[256];
    UINT32 arrBlueHist[256];

    ZeroMemory(arrRedHist, 256 * sizeof(UINT32));
    ZeroMemory(arrGreenHist, 256 * sizeof(UINT32));
    ZeroMemory(arrBlueHist, 256 * sizeof(UINT32));
    
    // Calculate the cumulative histogram for each color channel.  A histogram is the number
    // of pixels at a given color value.  A cumulative hisogram is the number of pixels
    // at a given color value plus the number of pixels at all lower color values.
    GenerateHistogram(unWidth, unHeight, pImageIn, arrRedHist, arrGreenHist, arrBlueHist);

    UINT32 arrRedCumulativeHist[256];
    GenerateCumulativeHistogram(arrRedHist, arrRedCumulativeHist);

    UINT32 arrGreenCumulativeHist[256];
    GenerateCumulativeHistogram(arrGreenHist, arrGreenCumulativeHist);

    UINT32 arrBlueCumulativeHist[256];
    GenerateCumulativeHistogram(arrBlueHist, arrBlueCumulativeHist);

    UINT32 unTotalSize = unWidth * unHeight;

    // Adjust colors based upon the cumulative histogram.  Color magnitude is calculated
    // as a percentage of full magnitude where the percentage is the number of pixels at
    // that color value over the number of total pixels.  Essentially, blacks will get
    // blacker and whites will get whiter, spreading out the color values used by the image.
    for(UINT32 i = 0; i < unHeight; i++)
    {
        for(UINT32 j = 0; j < unWidth; j++)
        {
            pImageOut->rgbRed = 256 * arrRedCumulativeHist[pImageIn->rgbRed] / unTotalSize;
            pImageOut->rgbGreen = 256 * arrGreenCumulativeHist[pImageIn->rgbGreen] / unTotalSize;
            pImageOut->rgbBlue = 256* arrBlueCumulativeHist[pImageIn->rgbBlue] / unTotalSize;
            pImageIn++;
            pImageOut++;
        }
    }
}

void CHistogramEqualizationTransform::GenerateHistogram(UINT32 unWidth, UINT32 unHeight, RGBQUAD* pImageIn, UINT32* pHistRedOut, UINT32* pHistGreenOut, UINT32* pHistBlueOut)
{
    for(UINT32 i = 0; i < unHeight; i++)
    {
        for(UINT32 j = 0; j < unWidth; j++)
        {
            pHistRedOut[pImageIn->rgbRed]++;
            pHistGreenOut[pImageIn->rgbGreen]++;
            pHistBlueOut[pImageIn->rgbBlue]++;
            pImageIn++;
       }
    }
}

void CHistogramEqualizationTransform::GenerateCumulativeHistogram(__in_bcount(256) UINT32* pHistIn, __inout_bcount(256) UINT32* pCumulativeHistOut)
{
    pCumulativeHistOut[0] = pHistIn[0];

    for(UINT32 i = 1; i < 256; i++)
    {
        pCumulativeHistOut[i] = pCumulativeHistOut[i - 1] + pHistIn[i];
    }
}
