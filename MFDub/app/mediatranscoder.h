#pragma once

#include "wmcontainer.h"
#include "transformapplier.h"

class CMediaTranscodeEventHandler
{
public:
    virtual void HandleFinished() = 0;
    virtual void HandleError(HRESULT hr) = 0;
};

class ISampleHandler
    : public IUnknown
{
public:
    virtual DWORD WaitUntilSampleRequest() = 0;
    virtual void ProcessSample(DWORD dwStreamIndex, IMFSample* pSample) = 0;
    virtual void NotifyEndOfStream() = 0;
    virtual void NotifyError(HRESULT hr) = 0;
};

class CTranscodeThread
{
public:
    CTranscodeThread(IMFSourceReader* pSourceReader, IMFSinkWriter* pSinkWriter, DWORD dwAudioStreamIndex, 
        DWORD dwVideoStreamIndex, CVideoTransformApplier* pApplier, CAudioTransformApplier* pAudioTransformApplier,
        CMediaTranscodeEventHandler* pEventHandler, HRESULT& hr);
    ~CTranscodeThread();

    void StartProcessing();

    static DWORD WINAPI TranscodeThreadProc(LPVOID lpParam);

private:
    CComPtr<IMFSourceReader> m_spSourceReader;
    CComPtr<IMFSinkWriter> m_spSinkWriter;
    const DWORD m_dwAudioStreamIndex;
    const DWORD m_dwVideoStreamIndex;
    CVideoTransformApplier* m_pTransformApplier;
    CAudioTransformApplier* m_pAudioTransformApplier;
    CMediaTranscodeEventHandler* m_pEventHandler;

    HANDLE m_hThread;
    HANDLE m_hStartEvent;
};

class CMediaTranscoder
{
public:
    CMediaTranscoder(LPCWSTR szInputURL, CVideoTransformApplier* pTransformApplier, CAudioTransformApplier* pAudioTransformApplier,
        UINT32 unFrameRateN, UINT32 unFrameRateD, LPCWSTR szOutputUrl, int iEncodeQuality, CMediaTranscodeEventHandler* pEventHandler, 
        HRESULT& hr);
    ~CMediaTranscoder();
    
    HRESULT BeginTranscode();

    HRESULT GetPresentationTime(MFTIME* phnsTime);
    HRESULT GetPresentationDuration(MFTIME* phnsDuration);
    
    // for callbacks
    LONG AddRef();
    LONG Release();
    
protected:
    HRESULT GetContainerTypesForOutputFile(LPCWSTR szOutputFile, GUID& guidAudioSubtype, GUID& guidVideoSubtype);
    HRESULT ConfigureSourceReader(const GUID& guidDesiredAudioSubtype, const GUID& guidDesiredVideoSubtype);
    HRESULT ConfigureSinkWriter(const GUID& guidInputAudioSubtype, const GUID& guidInputVideoSubtype, const GUID& guidTargetAudioSubtype, const GUID& guidTargetVideoSubtype, UINT32 unFrameRateN, UINT32 unFrameRateD);

    HRESULT MakeTargetAudioType(IMFMediaType* pInputType, const GUID& guidTargetAudioSubtype, IMFMediaType** ppTargetType);
    bool IsBetterAudioTypeMatch(IMFMediaType* pInputType, IMFMediaType* pPossibleType, IMFMediaType* pCurrentType);
    HRESULT MakeTargetVideoType(IMFMediaType* pInputType, const GUID& guidTargetVideoSubtype, IMFMediaType** ppTargetType);

private:
    LONG m_cRef;

    struct CONTAINER_MAP
    {
        DWORD cExtensions;
        LPCWSTR szExtensions[5];
        GUID guidAudioSubtype;
        GUID guidVideoSubtype;
    };
    static CONTAINER_MAP m_ContainerMap[];

    CComPtr<IMFSourceReader> m_spSourceReader;
    CComPtr<IMFSinkWriter> m_spSinkWriter;
    CVideoTransformApplier* m_pTransformApplier;
    CAudioTransformApplier* m_pAudioTransformApplier;

    DWORD m_dwAudioStreamIndex;
    DWORD m_dwVideoStreamIndex;

    int m_iEncodeQuality;

    CMediaTranscodeEventHandler* m_pEventHandler;

    CAutoPtr<CTranscodeThread> m_spTranscodeThread;
};