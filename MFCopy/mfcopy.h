// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#pragma once

typedef void (* PFN_MFCOPY_PROGRESS_CALLBACK )( __in DWORD dwPercentComplete, __in_opt LPVOID pvContext );

class CMFCopy
{
public:
    static HRESULT CreateInstance( __out CMFCopy **ppMFCopy );

    ULONG AddRef();
    ULONG Release();

    //
    // Main copy routine
    //
    HRESULT Copy( __in LPCWSTR wszSourceFilename,
                  __in LPCWSTR wszTargetFilename );

    //
    // Call this to set a callback function that will receive progress updates
    //
    void SetProgressCallback( __in_opt PFN_MFCOPY_PROGRESS_CALLBACK pfnCallback,
                              __in_opt LPVOID pvContext );

    //
    // Optionally specify a start position and duration
    //
    HRESULT SetStartPosition( __in LONGLONG llTimestamp );

    HRESULT SetDuration( __in ULONGLONG ullDuration );

    //
    // Optional target audio stream configuration
    //
    HRESULT SetTargetAudioFormat( __in REFGUID guidSubtype );

    HRESULT SetAudioNumChannels( __in UINT32 unNumChannels );

    HRESULT SetAudioSampleRate( __in UINT32 unSampleRate );

    //
    // Optional target video stream configuration
    //
    HRESULT SetTargetVideoFormat( __in REFGUID guidSubtype );

    HRESULT SetVideoFrameSize( __in UINT32 unWidth,
                               __in UINT32 unHeight );

    HRESULT SetVideoFrameRate( __in UINT32 unNumerator,
                            __in UINT32 unDenominator );

    HRESULT SetVideoInterlaceMode( __in UINT32 unInterlaceMode );

    HRESULT SetVideoAverageBitrate( __in UINT32 unBitrate );

    //
    // Additional optional configuration
    //
    HRESULT ExcludeAudioStreams( __in BOOL fExclude );

    HRESULT ExcludeVideoStreams( __in BOOL fExclude );

    HRESULT ExcludeNonAVStreams( __in BOOL fExclude );

    HRESULT EnableHardwareTransforms( __in BOOL fEnable );

    //
    // Call this to reset all configuration back to the default settings
    //
    void Reset();

    //
    // Stream information and Statistics
    // Can be queried after a copy operation has completed
    //
    DWORD GetStreamCount();

    HRESULT GetStreamSelection( __in DWORD dwStreamIndex,
                                __out BOOL *pfSelected );

    HRESULT GetStatistics( __in DWORD dwStreamIndex,
                           __out MF_SINK_WRITER_STATISTICS *pStats );

    double GetFinalizationTime();

    double GetOverallProcessingTime();

    //
    // Error information
    //
    void GetErrorDetails( __out HRESULT *phr,
                          __out LPCWSTR *pwszError );

private:
    CMFCopy();
    ~CMFCopy();

    HRESULT _Initialize();

    HRESULT _CreateReaderAndWriter();

    HRESULT _DetermineDuration();

    HRESULT _ConfigureStreams();

    HRESULT _AllocateStreamInfoArray();

    HRESULT _GetSourceReaderStreamCount( __out DWORD *pcStreams );

    HRESULT _ConfigureStreamSelection();

    HRESULT _ConfigureStream( __in DWORD dwStreamIndex );

    HRESULT _CreateTargetAudioMediaType( __in IMFMediaType *pNativeMediaType,
                                         __out IMFMediaType **ppTargetMediaType );

    HRESULT _CreateWMALosslessMediaType( __in IMFMediaType *pNativeMediaType,
                                         __out IMFMediaType **ppTargetMediaType );

    HRESULT _CreateTargetVideoMediaType( __in IMFMediaType *pNativeMediaType,
                                         __out IMFMediaType **ppTargetMediaType );

    HRESULT _NegotiateAudioStreamFormat( __in DWORD dwStreamIndex );

    HRESULT _NegotiateVideoStreamFormat( __in DWORD dwStreamIndex );

    HRESULT _NegotiateStreamFormat( __in DWORD dwStreamIndex,
                                    __in REFGUID guidMajorType,
                                    __in DWORD cFormats,
                                    __in_ecount( cFormats ) const GUID **paFormats );

    HRESULT _ConfigureStartPosition();

    HRESULT _DetermineTimestampAdjustment();

    HRESULT _ProcessSamples();

    HRESULT _HandleFormatChange( __in DWORD dwInputStreamIndex,
                                 __in DWORD dwTargetStreamIndex );

    LONGLONG _GetQPCTime();

    //
    // Error handling
    //

    void _SetErrorDetails( __in HRESULT hrError,
                           __in LPCWSTR wszError );

private:
    LONG m_cRef;

    LPCWSTR m_wszSourceFilename;
    LPCWSTR m_wszTargetFilename;

    IMFSourceReader *m_pSourceReader;
    IMFSinkWriter *m_pSinkWriter;
    IMFAttributes *m_pAttributes;

    IMFAttributes *m_pAudioOverrides;
    IMFAttributes *m_pVideoOverrides;

    //
    // Timing information
    //
    struct TimingInfo
    {
        LONGLONG llStartTime;
        LONGLONG llFinalizeStartTime;
        LONGLONG llStopTime;
    } m_Timing;

    //
    // Stream information
    //
    enum StreamType
    {
        UNKNOWN_STREAM = 0,
        AUDIO_STREAM = 1,
        VIDEO_STREAM = 2,
        NON_AV_STREAM = 3,
    };

    struct StreamInfo
    {
        StreamType eStreamType;
        BOOL fSelected;
        DWORD dwOutputStreamIndex;
        DWORD cSamples;
        BOOL fEOS;
    };

    __field_ecount( m_cStreams ) StreamInfo *m_paStreamInfo;
    DWORD m_cStreams;
    DWORD m_cSelectedStreams;

    LONGLONG m_llTimestampAdjustment;
    BOOL m_fRemuxing;

    //
    // Progress callback
    //
    PFN_MFCOPY_PROGRESS_CALLBACK m_pfnProgressCallback;
    LPVOID m_pvContext;

    //
    // Error information
    //
    HRESULT m_hrError;
    LPCWSTR m_wszError;

    //
    // Options controlled by command line arguments
    //
    struct CmdLineOptions
    {
        BOOL fTranscodeAudio;
        BOOL fTranscodeVideo;
        BOOL fExcludeAudio;
        BOOL fExcludeVideo;
        BOOL fExcludeNonAVStreams;
        BOOL fEnableHardwareTransforms;
        LONGLONG llStartPosition;
        BOOL fDurationSpecified;
        ULONGLONG ullDuration;
    } m_Options;
};

