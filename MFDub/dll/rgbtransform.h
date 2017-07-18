#pragma once

class CRGBImageTransform
{
public:
    virtual void Transform(UINT32 unWidth, UINT32 unHeight, RGBQUAD* pImageIn, RGBQUAD* pImageOut) = 0;
};

class CLocalAverageTransform
    : public CRGBImageTransform
{
public:
    CLocalAverageTransform(UINT32 unLocalSize);

    void Transform(UINT32 unWidth, UINT32 unHeight, RGBQUAD* pImageIn, RGBQUAD* pImageOut);

private:
    UINT32 m_unLocalSize;
};

class CLocalMedianTransform
    : public CRGBImageTransform
{
public:
    CLocalMedianTransform(UINT32 unLocalSize);

    void Transform(UINT32 unWidth, UINT32 unHeight, RGBQUAD* pImageIn, RGBQUAD* pImageOut);

private:
    UINT32 m_unLocalSize;
};

class CUnsharpTransform
    : public CRGBImageTransform
{
public:
    CUnsharpTransform(UINT32 unLocalSize, float gamma);

    void Transform(UINT32 unWidth, UINT32 unHeight, RGBQUAD* pImageIn, RGBQUAD* pImageOut);
    void SetGamma(float gamma) { m_gamma = gamma; }
private:
    UINT32 m_unLocalSize;
    float m_gamma;
};

class CMinTransform
    : public CRGBImageTransform
{
public:
    CMinTransform();
    
    void Transform(UINT32 unWidth, UINT32 unHeight, RGBQUAD* pImageIn, RGBQUAD* pImageOut);
};

class CMaxTransform
    : public CRGBImageTransform
{
public:
    CMaxTransform();
    
    void Transform(UINT32 unWidth, UINT32 unHeight, RGBQUAD* pImageIn, RGBQUAD* pImageOut);
};

class CNoiseRemovalTransform
    : public CRGBImageTransform
{
public:
    CNoiseRemovalTransform();

    void Transform(UINT32 unWidth, UINT32 unHeight, RGBQUAD* pImageIn, RGBQUAD* pImageOut);
};

class CHistogramEqualizationTransform
    : public CRGBImageTransform
{
public:
    CHistogramEqualizationTransform();

    void Transform(UINT32 unWidth, UINT32 unHeight, RGBQUAD* pImageIn, RGBQUAD* pImageOut);    

protected:
    void GenerateHistogram(UINT32 unWidth, UINT32 unHeight, RGBQUAD* pImageIn, UINT32* pHistRedOut, UINT32* pHistGreenOut, UINT32* pHistBlueOut);
    void GenerateCumulativeHistogram(__in_bcount(256) UINT32* pHistIn, __inout_bcount(256) UINT32* pCumulativeHistOut);
};