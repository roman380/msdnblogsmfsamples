// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF   
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO   
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A   
// PARTICULAR PURPOSE.   
//   
// Copyright (c) Microsoft Corporation. All rights reserved   
   
// MFSimpleEncode.cpp : Defines the entry point for the console application.   
//   
#include "common.h"   
#include "encodeengine.h"   
#include "Windows.h"   
   
#define POLLING_PERIOD 3000   
   
void Usage()   
{   
    wprintf_s(L"MFSimpleEncode.exe -i INPUTFILE -o OUTPUTFILE [-p PROFILE] [-t DURATION] [-splitAudio CONTAINER] [-splitVideo CONTAINER] [-remux CONTAINER].\n");   
    wprintf_s(L"INPUTFILE - any supported input file format: ASF, AVI, WAV, MPEG4, 3GP, MP3.\n");   
    wprintf_s(L"OUTPUTFILE - any supported output file format: ASF, MPEG4, 3GP, MP3.\n");   
    wprintf_s(L"PROFILE - transcode profile in XML format.\n");   
    wprintf_s(L"DURATION - encode cut off time in milliseconds.\n");   
    wprintf_s(L"CONTAINER - one of: ASF, MPEG4, 3GP, MP3.\n");   
    wprintf_s(L"The following options are mutually exclusive: -p, -splitAudio, -splitVideo, -remux.\n");   
}   
   
HRESULT ParseCommandLine( __in int argc,   
                          __in_ecount( argc ) WCHAR *argv[],   
                          __out CEncodeConfig *pConfig,   
                          __out QWORD *pqwCutOffTime   
                          )   
{   
    HRESULT hr = S_OK;   
    DWORD dwIndex;   
   
    *pqwCutOffTime = 0;   
   
    if ( 1 == argc )   
    {   
        CHECK_HR( hr = E_INVALIDARG );   
    }   
   
    for( dwIndex = 1; dwIndex < (DWORD)argc; )   
    {   
        if( ( 0 == wcscmp( argv[dwIndex], L"-i" ) ) ||   
            ( 0 == wcscmp( argv[dwIndex], L"/i" ) ) )   
        {   
            if( (DWORD)argc < ( dwIndex + 2 ) )   
            {   
                CHECK_HR( hr = E_INVALIDARG );   
            }   
   
            pConfig->strInputFile.SetString( argv[dwIndex+1] );           
            dwIndex += 2;   
        }   
        else if( ( 0 == wcscmp( argv[dwIndex], L"-o" ) ) ||   
            ( 0 == wcscmp( argv[dwIndex], L"/o" ) ) )   
        {   
            if( (DWORD)argc < ( dwIndex + 2 ) )   
            {   
                CHECK_HR( hr = E_INVALIDARG );   
            }   
   
            pConfig->strOutputFile.SetString( argv[dwIndex+1] );   
            dwIndex += 2;   
        }   
        else if( ( 0 == wcscmp( argv[dwIndex], L"-p" ) ) ||   
            ( 0 == wcscmp( argv[dwIndex], L"/p" ) ) )   
        {   
            if( (DWORD)argc < ( dwIndex + 2 ) )   
            {   
                CHECK_HR( hr = E_INVALIDARG );   
            }   
   
            pConfig->strProfile.SetString( argv[dwIndex+1] );   
            dwIndex += 2;   
        }   
        else if( ( 0 == wcscmp( argv[dwIndex], L"-t" ) ) ||   
            ( 0 == wcscmp( argv[dwIndex], L"/t" ) ) )   
        {   
            if( (DWORD)argc < ( dwIndex + 2 ) )   
            {   
                CHECK_HR( hr = E_INVALIDARG );   
            }   
   
            *pqwCutOffTime = _wtoi64( argv[dwIndex+1] );   
            *pqwCutOffTime *= 10000; // ms -> hns               
            dwIndex += 2;   
        }   
        else if( ( 0 == wcscmp( argv[dwIndex], L"-splitAudio" ) ) ||   
            ( 0 == wcscmp( argv[dwIndex], L"/splitAudio" ) ) )   
        {   
            if( (DWORD)argc < ( dwIndex + 2 ) )   
            {   
                CHECK_HR( hr = E_INVALIDARG );   
            }   
           
            pConfig->enumTranscodeMode = TranscodeMode_SplitAudio;   
            pConfig->strContainerName.SetString( argv[dwIndex+1] );   
            dwIndex += 2;                  
        }   
        else if( ( 0 == wcscmp( argv[dwIndex], L"-splitVideo" ) ) ||   
            ( 0== wcscmp( argv[dwIndex], L"/splitVideo" ) ) )   
        {   
            if( (DWORD)argc < ( dwIndex + 2 ) )   
            {   
                CHECK_HR( hr = E_INVALIDARG );   
            }   
           
            pConfig->enumTranscodeMode = TranscodeMode_SplitVideo;   
            pConfig->strContainerName.SetString( argv[dwIndex+1] );   
            dwIndex += 2;                  
        }   
        else if( ( 0 == wcscmp( argv[dwIndex], L"-remux" ) ) ||   
            ( 0== wcscmp( argv[dwIndex], L"/remux" ) ) )   
        {   
            if( (DWORD)argc < ( dwIndex + 2 ) )   
            {   
                CHECK_HR( hr = E_INVALIDARG );   
            }   
           
            pConfig->enumTranscodeMode = TranscodeMode_Remux;   
            pConfig->strContainerName.SetString( argv[dwIndex+1] );   
            dwIndex += 2;                  
        }           
        else if( ( 0 == wcscmp( argv[ dwIndex ], L"-h" ) ) ||   
                 ( 0 == wcscmp( argv[ dwIndex ], L"/h" ) ) ||   
                 ( 0 == wcscmp( argv[ dwIndex ], L"-?" ) ) ||   
                 ( 0 == wcscmp( argv[ dwIndex ], L"/?" ) ) )   
        {   
            hr = S_FALSE;   
            goto done;   
        }   
        else   
        {   
            CHECK_HR( hr = E_INVALIDARG );   
        }   
    }   
   
done:   
    return hr;   
}   
   
int __cdecl wmain( __in int argc, __in_ecount( argc ) WCHAR* argv[])   
{   
    HRESULT hr = S_OK;   
    CEncodeConfig config;   
    CEncodeEngine* pEncoder = NULL;   
    QWORD qwDuration = 0;   
    QWORD qwTime = 0;   
    BOOL fCoInit = FALSE;   
    BOOL fMFInit = FALSE;   
    QWORD qwCutOffTime;   
    BOOL fEncodeCutOff = FALSE;   
    HRESULT hrError = S_OK;   
   
    HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);   
      
    //   
    // Get the encode config   
    //   
   
    hr = ParseCommandLine( argc, argv, &config, &qwCutOffTime );   
   
    if( S_OK != hr )   
    {   
        Usage();   
        goto done;   
    }   
   
    //   
    // Init MF   
    //   
    CHECK_HR( hr = CoInitializeEx( NULL, COINIT_APARTMENTTHREADED ) );   
    fCoInit = TRUE;   
   
    CHECK_HR( hr = MFStartup( MF_VERSION ) );   
    fMFInit = TRUE;       
   
    //   
    // Init encode engine   
    //   
    pEncoder = new CEncodeEngine();   
    if( NULL == pEncoder )   
    {   
        CHECK_HR( hr = E_OUTOFMEMORY );   
    }   
       
    pEncoder->AddRef();   
   
    CHECK_HR( hr = pEncoder->Init() );   
       
    //   
    // Apply the encode config   
    //   
    CHECK_HR( hr = pEncoder->Load( &config ) );   
   
    CHECK_HR( hr = pEncoder->GetDuration( &qwDuration ) );   
   
    //   
    // Encode session starts.   
    //   
    CHECK_HR( hr = pEncoder->Start() );   
       
    wprintf_s(L"Encode Starts, duration=%dms \n", (DWORD)(qwDuration/10000) );   
       
    do   
    {      
        Sleep( POLLING_PERIOD );   
   
        CHECK_HR( hr = pEncoder->GetTime( &qwTime ) );   
        CHECK_HR( hr = pEncoder->GetErrorStatus( &hrError ) );   
   
        if( FAILED( hrError ) )   
        {   
            break;   
        }   
   
        if( qwTime != 0 )   
        {   
            if( ( qwCutOffTime != 0 ) && ( qwTime > qwCutOffTime ) )   
            {   
                fEncodeCutOff = TRUE;   
                wprintf_s(L"--------Percent %d @%dms \n", 100, (DWORD)(qwTime/10000) );   
                break;   
            }   
            else   
            {   
                DWORD dwPercent = 0;   
               
                if( qwDuration != 0 )   
                {   
                    dwPercent = (DWORD) ((qwTime * 100 ) / (qwCutOffTime ? qwCutOffTime : qwDuration));   
                }   
                   
                wprintf_s(L"--------Percent %d @%dms \n", dwPercent, (DWORD)(qwTime/10000) );   
            }   
        }   
        else   
        {   
            wprintf_s(L"--------Percent %d @%dms \n", 100, (DWORD)(qwDuration/10000) );   
        }   
           
    }while( qwTime != 0 );   
   
    //   
    // Encode session ends.   
    //   
    if( SUCCEEDED( hrError ) )   
    {   
        if( fEncodeCutOff )   
        {   
            CHECK_HR( hr = pEncoder->Stop() );   
        }   
           
        CHECK_HR( hr = pEncoder->Close() );   
   
        CHECK_HR( hr = pEncoder->Shutdown() );   
   
        wprintf_s(L"Encode Ends Successfully\n" );       
    }   
    else   
    {   
        CHECK_HR( hr = pEncoder->Shutdown() );   
    }   
   
done:   
       
    SAFE_RELEASE( pEncoder );   
   
    if( fMFInit )   
    {   
        MFShutdown();   
    }   
       
    if( fCoInit )   
    {   
        CoUninitialize();   
    }   
   
    if( FAILED( hrError ) )   
    {   
        hr = hrError;   
    }   
   
    if( FAILED( hr ) )   
    {   
        wprintf_s(L"Encode Failed, hr=%x \n", hr );       
    }   
   
    return hr;   
}   

