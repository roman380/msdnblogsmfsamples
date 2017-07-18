// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE
//
// Copyright (c) Microsoft Corporation.  All rights reserved.

#include "stdafx.h"

#include "pcmtransform.h"
#include "intsafe.h"
#include "assert.h"

// CVolumeCompressionTransform
// Performs volume compression on the audio data by multiplying sample
// magnitudes by a factor.  This is very simple video compression with
// no effort to reduce the effect of clipping.
CVolumeCompressionTransform::CVolumeCompressionTransform(float flFactor)
: _flFactor(flFactor)
{
}

void CVolumeCompressionTransform::Transform(UINT32 nBitsPerSample, UINT32 nChannels, UINT32 nSamplesPerSecond, UINT32 cbData, BYTE* pbSamplesIn, BYTE* pbSamplesOut)
{
    assert(nBitsPerSample % 8 == 0);

    UINT32 cSamples = cbData * 8 / nBitsPerSample; 

    if(32 == nBitsPerSample)
    {
        for(UINT32 i = 0; i < cSamples; i++)
        {
            *(reinterpret_cast<float*>(pbSamplesOut)) = _flFactor * *(reinterpret_cast<float*>(pbSamplesIn));
            pbSamplesIn += 4;
            pbSamplesOut += 4;
        }
    }
    else if(16 == nBitsPerSample)
    {
        for(UINT32 i = 0; i < cSamples; i++)
        {
            *(reinterpret_cast<INT16*>(pbSamplesOut)) = static_cast<INT16>(_flFactor * *(reinterpret_cast<INT16*>(pbSamplesIn)));
            pbSamplesIn += 2;
            pbSamplesOut += 2;
        }
    }
    else if(8 == nBitsPerSample)
    {
        for(UINT32 i = 0; i < cSamples; i++)
        {
            *(reinterpret_cast<INT8*>(pbSamplesOut)) = static_cast<INT8>(_flFactor * *(reinterpret_cast<INT8*>(pbSamplesIn)));
            pbSamplesIn++;
            pbSamplesOut++;
        }
    }
    
}