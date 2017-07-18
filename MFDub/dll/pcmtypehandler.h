#pragma once

#include "basemft.h"

class CPCM1in1outTypeHandler
    : public C1in1outTypeHandler
{
public:
    CPCM1in1outTypeHandler();
    virtual ~CPCM1in1outTypeHandler();

protected:
    void OnInputTypeChanged();
    void OnOutputTypeChanged();
};
