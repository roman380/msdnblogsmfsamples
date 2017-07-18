#pragma once

class CSampleProvider
{
public:
    CSampleProvider();
    ~CSampleProvider();

    HRESULT LoadSource(LPCWSTR szSourceURL);
    
    HRESULT GetVideoSample(MFTIME tNext, IMFSample** ppSample, DWORD* pdwSampleNum, bool* pfIsKey);
    HRESULT GetVideoSample(DWORD dwFrameNumber, IMFSample** ppSample, DWORD* pdwSampleNum, bool* pfIsKey);
    
    // Metadata information
    HRESULT GetMediaDuration(MFTIME* ptDuration);
    HRESULT GetSampleSize(UINT32* punSampleWidth, UINT32* punSampleHeight);
    IMFMediaType* GetAudioMediaType();
    IMFMediaType* GetVideoMediaType();
    IMFSourceReader* GetSourceReader();

protected:
    HRESULT CreateOutputMediaType(IMFMediaType* pMediaTypeIn, IMFMediaType** ppMediaTypeOut);
    HRESULT SeekTo(MFTIME hnsNext, IMFSample** ppSample);

private:
    CComPtr<IMFSourceReader> m_spSourceReader;
    CComPtr<IMFMediaType> m_spAudioType;
    CComPtr<IMFMediaType> m_spVideoType;
    UINT32 m_unSampleWidth;
    UINT32 m_unSampleHeight;
    MFTIME m_tLastSample;
    DWORD m_dwVideoStreamId;
    DWORD m_dwAudioStreamId;
    DWORD m_dwCurrentFrameNumber;
};