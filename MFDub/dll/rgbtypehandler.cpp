// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE
//
// Copyright (c) Microsoft Corporation.  All rights reserved.

#include "stdafx.h"

#include "basemft.h"
#include "rgbtypehandler.h"
#include "mferror.h"
#include "rgbmft.h"

HRESULT CalculateImageSizeFromType(IMFMediaType* pType, UINT32* pOutput)
{
    UINT32 unWidth, unHeight;
    MFGetAttributeSize(pType, MF_MT_FRAME_SIZE, &unWidth, &unHeight);

    GUID gidSubtype;
    pType->GetGUID(MF_MT_SUBTYPE, &gidSubtype);

    MFCalculateImageSize(gidSubtype, unWidth, unHeight, pOutput);

    return S_OK;
}

/////////////////////////////////////////////////////////////
// CRGB1in1outTypeHandler
// Simple video type handler for RGB32 input and output.
CRGB1in1outTypeHandler::CRGB1in1outTypeHandler()
{
    CComPtr<IMFMediaType> spOutAvType;
    if(FAILED(MFCreateMediaType(&spOutAvType))) return;
    spOutAvType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    spOutAvType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
    spOutAvType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);

    SetOutputAvTypeCount(1);
    SetOutputAvType(0, spOutAvType);
    
    CComPtr<IMFMediaType> spInputAvType;
    if(FAILED(MFCreateMediaType(&spInputAvType))) return;
    spInputAvType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    spInputAvType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
    
    SetInputAvTypeCount(1);
    SetInputAvType(0, spInputAvType);
}

CRGB1in1outTypeHandler::~CRGB1in1outTypeHandler()
{
}

void CRGB1in1outTypeHandler::SetInputFrameSize(UINT32 unWidth, UINT32 unHeight)
{
    MFSetAttributeSize(m_pInputAvTypes[0], MF_MT_FRAME_SIZE, unWidth, unHeight);
}

void CRGB1in1outTypeHandler::SetOutputFrameSize(UINT32 unWidth, UINT32 unHeight)
{
    MFSetAttributeSize(m_pOutAvTypes[0], MF_MT_FRAME_SIZE, unWidth, unHeight);
}


void CRGB1in1outTypeHandler::OnInputTypeChanged()
{
    CalculateImageSizeFromType(m_spInputType, (UINT32*) &m_cbInputSize);
}

void CRGB1in1outTypeHandler::OnOutputTypeChanged()
{
    CalculateImageSizeFromType(m_spOutputType, (UINT32*) &m_cbOutputSize);
}

////////////////////////////////////////////
//

CRGB1in1outAutoCopyTypeHandler::CRGB1in1outAutoCopyTypeHandler()
{
}

CRGB1in1outAutoCopyTypeHandler::~CRGB1in1outAutoCopyTypeHandler()
{
}

void CRGB1in1outAutoCopyTypeHandler::OnInputTypeChanged()
{
    CRGB1in1outTypeHandler::OnInputTypeChanged();
    m_spInputType->CopyAllItems(m_pOutAvTypes[0]);
}

///////////////////////////////////
//

CResizeTypeHandler::CResizeTypeHandler(CResizeCropMFT* pMFT)
    : m_pMFT(pMFT)
{
}

CResizeTypeHandler::~CResizeTypeHandler()
{
}

void CResizeTypeHandler::OnInputTypeChanged()
{
    CRGB1in1outTypeHandler::OnInputTypeChanged();
    
    UINT32 unWidth, unHeight;
    MFGetAttributeSize(m_pOutAvTypes[0], MF_MT_FRAME_SIZE, &unWidth, &unHeight);
    m_spInputType->CopyAllItems(m_pOutAvTypes[0]);
    MFSetAttributeSize(m_pOutAvTypes[0], MF_MT_FRAME_SIZE, unWidth, unHeight);
    m_pOutAvTypes[0]->SetUINT32(MF_MT_SAMPLE_SIZE, unWidth * unHeight * 4);
    m_pOutAvTypes[0]->SetUINT32(MF_MT_DEFAULT_STRIDE, unWidth * 4);
    
    m_pMFT->HandleInputTypeSet();
}

void CResizeTypeHandler::OnOutputTypeChanged()
{
    CRGB1in1outTypeHandler::OnOutputTypeChanged();
    
    m_pMFT->HandleOutputTypeSet();
}