#pragma once

#include "basemft.h"

class CResizeCropMFT;

class CRGB1in1outTypeHandler
    : public C1in1outTypeHandler
{
public:
    CRGB1in1outTypeHandler();
    virtual ~CRGB1in1outTypeHandler();

    void SetInputFrameSize(UINT32 unWidth, UINT32 unHeight);
    void SetOutputFrameSize(UINT32 unWidth, UINT32 unHeight);
    
protected:
    virtual void OnInputTypeChanged();
    virtual void OnOutputTypeChanged();
};

class CRGB1in1outAutoCopyTypeHandler
    : public CRGB1in1outTypeHandler
{
public:
    CRGB1in1outAutoCopyTypeHandler();
    ~CRGB1in1outAutoCopyTypeHandler();
    
protected:
    void OnInputTypeChanged();
};

class CResizeTypeHandler
    : public CRGB1in1outTypeHandler
{
public:
    CResizeTypeHandler(CResizeCropMFT* m_pMFT);
    ~CResizeTypeHandler();
    
protected:
    void OnInputTypeChanged();
    void OnOutputTypeChanged();
    
private:
    CResizeCropMFT* m_pMFT;
};