#pragma once

class CPCMSampleTransform
{
public:
    virtual void Transform(UINT32 nBitsPerSample, UINT32 nChannels, UINT32 nSamplesPerSecond, UINT32 cbData, BYTE* pSamplesIn, BYTE* pSamplesOut) = 0;
};

class CVolumeCompressionTransform
    : public CPCMSampleTransform
{
public:
    CVolumeCompressionTransform(float flFactor);

    void SetFactor(float flFactor) { _flFactor = flFactor; }

    void Transform(UINT32 nBitsPerSample, UINT32 nChannels, UINT32 nSamplesPerSecond, UINT32 cbData, BYTE* pSamplesIn, BYTE* pSamplesOut);

private:
   float _flFactor;
};