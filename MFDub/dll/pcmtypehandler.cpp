// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE
//
// Copyright (c) Microsoft Corporation.  All rights reserved.

#include "stdafx.h"

#include "basemft.h"
#include "pcmtypehandler.h"
#include "mferror.h"

/////////////////////////////////////////////////////////////
// CPCM1in1outTypeHandler
// A simple type handler that works with either PCM or Float
// audio.
CPCM1in1outTypeHandler::CPCM1in1outTypeHandler()
{
    CComPtr<IMFMediaType> spOutAvType;
    if(FAILED(MFCreateMediaType(&spOutAvType))) return;
    spOutAvType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
    spOutAvType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
    spOutAvType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);

    SetOutputAvTypeCount(1);
    SetOutputAvType(0, spOutAvType);
    
    CComPtr<IMFMediaType> spInputAvType;
    if(FAILED(MFCreateMediaType(&spInputAvType))) return;
    spInputAvType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
    spInputAvType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);

    CComPtr<IMFMediaType> spInputAvType1;
    if(FAILED(MFCreateMediaType(&spInputAvType1))) return;
    spInputAvType1->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
    spInputAvType1->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_Float);
    
    SetInputAvTypeCount(1);
    SetInputAvType(0, spInputAvType);
    SetInputAvType(1, spInputAvType);
}

CPCM1in1outTypeHandler::~CPCM1in1outTypeHandler()
{
}

void CPCM1in1outTypeHandler::OnInputTypeChanged()
{
    m_cbInputSize = MFGetAttributeUINT32(m_spInputType, MF_MT_AUDIO_BLOCK_ALIGNMENT, 0);
    m_spInputType->CopyAllItems(m_pOutAvTypes[0]);
}

void CPCM1in1outTypeHandler::OnOutputTypeChanged()
{
    m_cbOutputSize = MFGetAttributeUINT32(m_spOutputType, MF_MT_AUDIO_BLOCK_ALIGNMENT, 0);
}
