// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF   
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO   
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A   
// PARTICULAR PURPOSE.   
//   
// Copyright (c) Microsoft Corporation. All rights reserved   
   
#include "common.h"   
#include "ProfileBuilder.h"   
#include <shlwapi.h>   
#include <Objidl.h>   
#include <atlbase.h>   
#include <msxml6.h>   
#include <atlcoll.h>   
   
static const ContainerNameToGUIDMapEntry g_ContainerNameToGUIDMap[] =   
{   
        {L"MPEG4", MFTranscodeContainerType_MPEG4},    
        {L"ASF", MFTranscodeContainerType_ASF},   
        {L"3GP", MFTranscodeContainerType_3GP},   
        {L"MP3", MFTranscodeContainerType_MP3}   
};   
   
///////////////////////////////////////////////////////////////////////////////   
CProfileBuilder::CProfileBuilder()   
{   
    m_pProfile = NULL;   
}   
   
///////////////////////////////////////////////////////////////////////////////   
CProfileBuilder::~CProfileBuilder()   
{   
    Reset();   
}   
   
///////////////////////////////////////////////////////////////////////////////   
HRESULT CProfileBuilder::Load( __in LPCWSTR pszFilePath )   
{   
    HRESULT hr = S_OK;   
    LPWSTR ext = PathFindExtension( pszFilePath );   
    if( 0 == wcscmp( ext, L".xml" ) )   
    {   
       CHECK_HR( hr = LoadMFProfile( pszFilePath ) );   
    }   
    else       
    {   
       CHECK_HR( hr = E_UNEXPECTED );   
    }   
   
done:   
    return hr;   
}   
   
///////////////////////////////////////////////////////////////////////////////   
HRESULT CProfileBuilder::CreateProfile( __in IMFPresentationDescriptor* pPD, TranscodeMode enumTranscodeMode, __in LPCWSTR strContainerName )   
{   
    HRESULT hr = S_OK;   
    CComPtr<IMFTranscodeProfile> spProfile;   
    GUID guidContainerType;   
    CComPtr<IMFAttributes> spContainerAttrs;   
    CComPtr<IMFAttributes> spAudioAttrs;   
    CComPtr<IMFAttributes> spVideoAttrs;   
       
    SAFE_RELEASE( m_pProfile );   
   
    CHECK_HR( hr = MapContainerNameToGUID( strContainerName, &guidContainerType ) );    
   
    CHECK_HR( hr = MFCreateTranscodeProfile( &spProfile ) );   
   
    CHECK_HR( hr = MFCreateAttributes( &spContainerAttrs, 3 ) );   
   
    //   
    // Config which container that the generated file will be put    
    //   
    CHECK_HR( hr = spContainerAttrs->SetGUID(MF_TRANSCODE_CONTAINERTYPE, guidContainerType ) );   
   
    //   
    // If the h/w transcode is allowed   
    //   
    CHECK_HR( hr = spContainerAttrs->SetUINT32( MF_TRANSCODE_TOPOLOGYMODE, MF_TRANSCODE_TOPOLOGYMODE_HARDWARE_ALLOWED ) );   
       
    switch( enumTranscodeMode )   
    {   
    case TranscodeMode_Remux:   
    {   
        HRESULT hrAudio = CreateRemuxProfile( pPD, MFMediaType_Audio, &spAudioAttrs );   
        HRESULT hrVideo = CreateRemuxProfile( pPD, MFMediaType_Video, &spVideoAttrs );   
   
        // Need at least one stream to remux   
        if( ( S_OK != hrAudio ) && ( S_OK != hrVideo ) )   
        {   
            CHECK_HR( hr = E_UNEXPECTED );   
        }   
    }   
    break;   
           
    case TranscodeMode_SplitAudio:           
    {   
        CHECK_HR( hr = CreateRemuxProfile( pPD, MFMediaType_Audio, &spAudioAttrs ) );   
    }           
    break;   
           
    case TranscodeMode_SplitVideo:   
    {   
        CHECK_HR( hr = CreateRemuxProfile( pPD, MFMediaType_Video, &spVideoAttrs ) );   
    }           
    break;   
   
    default:   
        CHECK_HR( hr = E_UNEXPECTED );   
    }   
   
    CHECK_HR( hr = spProfile->SetAudioAttributes( spAudioAttrs ) );   
    CHECK_HR( hr = spProfile->SetVideoAttributes( spVideoAttrs ) );   
    CHECK_HR( hr = spProfile->SetContainerAttributes( spContainerAttrs ) );   
   
    m_pProfile = spProfile;   
    spProfile.Detach();   
   
done:           
    return hr;   
}   
   
///////////////////////////////////////////////////////////////////////////////   
HRESULT CProfileBuilder::CreateRemuxProfile( __in IMFPresentationDescriptor* pPD, __in REFGUID guidMajorType, __out IMFAttributes** ppAttrs )   
{   
    HRESULT hr = S_OK;   
    CComPtr<IMFMediaType> spSrcStreamMT;   
    CComPtr<IMFAttributes> spAttrs;   
    UINT32 unCount;    
   
    CHECK_HR( hr = GetSourceStreamMediaType( pPD, guidMajorType, &spSrcStreamMT ) );   
    CHECK_HR( hr = spSrcStreamMT->GetCount( &unCount ) );   
    CHECK_HR( hr = MFCreateAttributes( &spAttrs, unCount+1 ) );         
    CHECK_HR( hr = spSrcStreamMT->CopyAllItems( spAttrs ) );    
   
    // Add the attribute to skip the encoder since this is for remux.    
    CHECK_HR( hr = spAttrs->SetUINT32( MF_TRANSCODE_DONOT_INSERT_ENCODER, 1 ) );   
   
    *ppAttrs = spAttrs.Detach();   
   
done:   
    return hr;   
}   
   
///////////////////////////////////////////////////////////////////////////////   
HRESULT CProfileBuilder::GetSourceStreamMediaType( __in IMFPresentationDescriptor* pPD, __in REFGUID guidMajorType, __out IMFMediaType** ppMT )   
{   
    HRESULT hr = S_OK;   
    CComPtr<IMFStreamDescriptor> spSD;   
    CComPtr<IMFMediaTypeHandler> spMTHandler;   
    CComPtr<IMFMediaType> spMT;   
    DWORD  dwSDCount = 0;   
    DWORD  dwIndex;   
       
    CHECK_HR( hr = pPD->GetStreamDescriptorCount( &dwSDCount ) );   
   
    for( dwIndex = 0; dwIndex < dwSDCount; dwIndex++ )   
    {   
        BOOL fSelect = FALSE;   
           
        spSD = NULL;   
        spMTHandler = NULL;   
        spMT = NULL;   
           
        CHECK_HR( hr = pPD->GetStreamDescriptorByIndex( dwIndex, &fSelect, &spSD ) );    
   
        if( fSelect )   
        {   
            GUID guidType;   
            CHECK_HR( hr = spSD->GetMediaTypeHandler( &spMTHandler ) );   
            if( NULL == spMTHandler.p )   
            {   
                hr = E_UNEXPECTED;   
                goto done;   
            }   
   
            CHECK_HR( hr = spMTHandler->GetMajorType( &guidType ) );    
            if( guidType != guidMajorType )   
            {   
                continue;   
            }               
               
            HRESULT hrRes = spMTHandler->GetCurrentMediaType( &spMT );   
            if( FAILED( hrRes ) )   
            {   
                spMT = NULL;   
                CHECK_HR( hr = spMTHandler->GetMediaTypeByIndex( 0, &spMT ) );   
            }   
   
            break;               
        }   
    }   
   
    if( dwIndex == dwSDCount )   
    {   
        hr = E_UNEXPECTED;   
    }   
       
    CHECK_HR( hr );   
   
    *ppMT = spMT;   
    spMT.Detach();   
   
done:   
    return hr;   
}   
   
///////////////////////////////////////////////////////////////////////////////   
HRESULT CProfileBuilder::MapContainerNameToGUID( __in LPCWSTR strContainerName, __out GUID* pGuid )   
{   
    HRESULT hr = S_OK;   
    DWORD dwCount = sizeof( g_ContainerNameToGUIDMap ) /sizeof( ContainerNameToGUIDMapEntry );   
    DWORD dwIndex;   
       
    for( dwIndex = 0; dwIndex < dwCount; dwIndex++ )   
    {   
        if( 0 == wcscmp( strContainerName, g_ContainerNameToGUIDMap[dwIndex].szContainerName ) )   
        {   
            *pGuid = g_ContainerNameToGUIDMap[dwIndex].guidContainerType;   
            break;   
        }   
    }   
   
    if( dwIndex == dwCount )   
    {   
        hr = E_UNEXPECTED;           
    }   
   
    return hr;   
}   
   
///////////////////////////////////////////////////////////////////////////////   
HRESULT CProfileBuilder::GetProfile( __out IMFTranscodeProfile** ppProfile )   
{   
    HRESULT hr = S_OK;   
       
    if( NULL == ppProfile )   
    {   
        hr = E_INVALIDARG;   
        goto done;   
    }   
       
    *ppProfile = m_pProfile;   
    if( m_pProfile )   
    {   
        m_pProfile->AddRef();   
    }   
   
done:   
    return hr;   
}   
   
///////////////////////////////////////////////////////////////////////////////   
void CProfileBuilder::Reset()   
{   
    SAFE_RELEASE( m_pProfile );   
}   
   
///////////////////////////////////////////////////////////////////////////////   
HRESULT CProfileBuilder::LoadMFProfile( __in LPCWSTR pszFilePath )   
{   
    HRESULT hr = S_OK;   
    CComPtr<IXMLDOMElement> spXMLRootElement;   
    IMFTranscodeProfile* pProfile = NULL;   
   
    SAFE_RELEASE( m_pProfile );   
   
    CHECK_HR( hr = LoadXML( pszFilePath, &spXMLRootElement ) );   
    CHECK_HR( hr = CreateTranscodeProfile( spXMLRootElement, &pProfile ) );   
   
    m_pProfile = pProfile;   
    pProfile = NULL;   
   
done:   
    SAFE_RELEASE( pProfile );   
    return hr;   
}   
   
///////////////////////////////////////////////////////////////////////////////   
HRESULT CProfileBuilder::LoadXML( __in LPCWSTR pszFilePath, __out IXMLDOMElement **ppElemRoot )   
{   
    HRESULT hr = S_OK;   
    VARIANT_BOOL vbStatus;   
    CComPtr<IXMLDOMDocument> spXMLDoc;   
   
    assert( ppElemRoot );   
    *ppElemRoot = NULL;   
   
    // Open the XML file   
    CHECK_HR( hr = spXMLDoc.CoCreateInstance(CLSID_DOMDocument) );   
    CHECK_HR( hr = spXMLDoc->put_async(VARIANT_FALSE) );   
    CHECK_HR( hr = spXMLDoc->put_validateOnParse(VARIANT_FALSE) );   
    CHECK_HR( hr = spXMLDoc->put_resolveExternals(VARIANT_FALSE) );   
    CHECK_HR( hr = spXMLDoc->load(CComVariant(pszFilePath), &vbStatus) );   
    if( vbStatus!=VARIANT_TRUE )   
    {   
        // Display the error (probably an XML file not respecting the DTD)   
        CComPtr<IXMLDOMParseError> spParseError;   
        CComBSTR bstrReason;   
        LONG errorCode;   
        LONG line;   
        LONG linePos;   
        CHECK_HR( hr = spXMLDoc->get_parseError( &spParseError ) );   
        CHECK_HR( hr = spParseError->get_errorCode( &errorCode ) );   
        CHECK_HR( hr = spParseError->get_reason( &bstrReason ) );   
        CHECK_HR( hr = spParseError->get_line( &line ) );   
        CHECK_HR( hr = spParseError->get_filepos( &linePos ) );   
        wprintf_s( L"XML parsing error:\n" );   
        wprintf_s( L"  Code = 0x%x\n", errorCode );   
        wprintf_s( L"  Source = Line : %ld; Char : %ld\n", line, linePos );   
        wprintf_s( L"  Description = %s\n", bstrReason );   
   
        CHECK_HR( hr = E_UNEXPECTED );   
    }   
   
    // Get the root node   
    CHECK_HR( hr = spXMLDoc->get_documentElement( ppElemRoot) );   
   
done:   
    return hr;   
}   
   
///////////////////////////////////////////////////////////////////////////////   
HRESULT CProfileBuilder::CreateTranscodeProfile( __in IXMLDOMElement *pElemRoot, __out IMFTranscodeProfile **ppProfile )   
{   
    HRESULT hr = S_OK;   
    CComPtr<IXMLDOMNode> spNode;   
    CComPtr<IMFTranscodeProfile> spProfile;   
    CComPtr<IMFMediaType> spAudioMT;   
    CComPtr<IMFMediaType> spVideoMT;   
    CComPtr<IMFMediaType> spContMT;   
   
    assert( pElemRoot );   
    assert( ppProfile );   
    *ppProfile = NULL;   
   
    // Read the media types   
    CHECK_HR( hr = pElemRoot->get_firstChild(&spNode) );   
       
    while( spNode )   
    {   
        CComPtr<IXMLDOMElement> spElem;   
        CComBSTR bstr;   
        CComPtr<IXMLDOMNode> spNextNode;   
   
        if( SUCCEEDED( spNode.QueryInterface( &spElem ) )   
            && SUCCEEDED( spElem->get_tagName( &bstr.m_str ) ) )   
        {   
            if( bstr == L"audio" )   
            {   
                CHECK_HR( hr = CreateMediaType( spElem, &spAudioMT ) );   
            }   
            else if(bstr == L"video")   
            {   
                CHECK_HR( hr = CreateMediaType( spElem, &spVideoMT ) );   
            }   
            else if(bstr == L"container")   
            {   
                CHECK_HR( hr = CreateMediaType( spElem, &spContMT ) );   
            }   
            else   
            {   
                wprintf_s( L"Warning: ignoring unknown media type '%s'", bstr );   
            }   
        }   
   
        bstr.Empty();   
   
        // Next node   
        hr = spNode->get_nextSibling( &spNextNode);   
        if( S_FALSE == hr )   
        {   
            hr = S_OK;   
        }   
   
        CHECK_HR( hr );   
   
        spNode = spNextNode;   
        spNextNode.Detach();   
    }   
   
    if( !spAudioMT )   
    {   
        wprintf_s( L"Warning: no audio media type found" );   
    }   
    if( !spVideoMT )   
    {   
        wprintf_s( L"Warning: no video media type found" );   
    }   
    if( !spContMT )   
    {   
        wprintf_s( L"Warning: no container media type found" );   
    }   
   
    // Create the profile   
    CHECK_HR( hr = MFCreateTranscodeProfile( &spProfile ) );   
    CHECK_HR( hr = spProfile->SetAudioAttributes( spAudioMT ) );   
    CHECK_HR( hr = spProfile->SetVideoAttributes( spVideoMT ) );   
    CHECK_HR( hr = spProfile->SetContainerAttributes( spContMT ) );   
   
    *ppProfile = spProfile.Detach();   
   
done:   
    return hr;   
}   
   
///////////////////////////////////////////////////////////////////////////////   
HRESULT CProfileBuilder::CreateMediaType( __in IXMLDOMElement *pElemRoot, __out IMFMediaType **ppMT )   
{   
    HRESULT hr = S_OK;   
    CComPtr<IXMLDOMNode> spNode;   
    CComPtr<IMFMediaType> spMT;   
   
    assert( pElemRoot );   
    assert( ppMT );   
    *ppMT = NULL;   
   
    // Create the mediatype   
    CHECK_HR( hr = MFCreateMediaType( &spMT ) );   
   
    CHECK_HR( hr = pElemRoot->get_firstChild( &spNode) );   
    while( spNode )   
    {   
        CComPtr<IXMLDOMElement> spElem;   
        CComBSTR bstr;   
        CComPtr<IXMLDOMNode> spNextNode;   
   
        if( SUCCEEDED( spNode.QueryInterface(&spElem) )   
            && SUCCEEDED( spElem->get_tagName(&bstr.m_str) ) )   
        {   
            if( bstr == L"attribute" )   
            {   
                CHECK_HR( hr = AddAttribute( spElem, spMT ) );   
            }   
            else // Unknown element type   
            {   
                CHECK_HR( hr = E_UNEXPECTED );    
            }   
        }   
   
        bstr.Empty();   
   
        // Next node   
        hr = spNode->get_nextSibling( &spNextNode );   
        if( S_FALSE == hr )   
        {   
            hr = S_OK;   
        }   
   
        CHECK_HR( hr );   
   
        spNode = spNextNode;   
        spNextNode.Detach();   
    }   
   
    *ppMT = spMT.Detach();   
   
done:   
    return hr;   
}   
   
///////////////////////////////////////////////////////////////////////////////   
HRESULT CProfileBuilder::AddAttribute( __in IXMLDOMElement* pElem, __inout IMFAttributes* pAttrib )   
{   
    HRESULT hr = S_OK;   
    CComVariant value(VT_EMPTY);   
    CAtlString strKey;   
    GUID guidKey;   
    CComBSTR bstrType;   
   
    assert( pElem );   
    assert( pAttrib );   
   
    // Key (required)   
    CHECK_HR( hr = pElem->getAttribute(CComBSTR(L"key"), &value ) );   
    if( value.vt != VT_BSTR )   
    {   
        CHECK_HR( hr = E_UNEXPECTED );   
    }   
    CHECK_HR( hr = CLSIDFromBSTR( value.bstrVal, &guidKey ) );   
   
    // Type (required)   
    value.Clear();   
    CHECK_HR( hr = pElem->getAttribute( CComBSTR(L"type"), &value) );   
    if( value.vt != VT_BSTR )   
    {   
        CHECK_HR( hr = E_UNEXPECTED );   
    }   
    bstrType = value.bstrVal;   
   
    // Value (required)   
    value.Clear();   
    CHECK_HR( hr = pElem->getAttribute( CComBSTR(L"value"), &value ) );   
    if( value.vt != VT_BSTR )   
    {   
        CHECK_HR( hr = E_UNEXPECTED );   
    }   
       
    // Parse the value based on its type and add it as an attribute to the sample   
    if( bstrType == L"UINT32" )   
    {   
        CHECK_HR( hr = pAttrib->SetUINT32( guidKey, (UINT32)_wtol(CComBSTR(value.bstrVal)) ) );   
    }   
    else if( bstrType == L"UINT64" )   
    {   
        CAtlStringW strValue = CComBSTR(value.bstrVal);   
        UINT64 unValue;   
   
        INT pos = strValue.Find(L",");   
        if( (pos > 0) && (pos < strValue.GetLength()-1))   
        { // A UINT64 given as "UINT32,UINT32"   
            UINT32 unHigh32 = (UINT32)_wtol( strValue.Left(pos) );   
            UINT32 unLow32  = (UINT32)_wtol( strValue.Right(strValue.GetLength()-pos-1) );   
            unValue = Pack2UINT32AsUINT64(unHigh32, unLow32);   
        }   
        else // A simple UINT64   
        {   
            unValue = (UINT64)_wtol(CComBSTR(value.bstrVal));   
        }   
   
        CHECK_HR( hr = pAttrib->SetUINT64( guidKey, unValue ) );   
    }   
    else if( bstrType == L"GUID" )   
    {   
        GUID guidValue;   
        CHECK_HR( hr = CLSIDFromBSTR( value.bstrVal, &guidValue ) );   
   
        CHECK_HR( hr = pAttrib->SetGUID( guidKey, guidValue ) );   
    }   
    else if( bstrType == L"STRING" )   
    {   
        CAtlStringW strValue = CComBSTR(value.bstrVal);   
   
        CHECK_HR( hr = pAttrib->SetString( guidKey, strValue ) );   
    }   
    else if( bstrType == L"BLOB" ) // Example of value: "44 4F 5F 46 4F 52 4D"   
    {   
        CAtlStringW strValue = CComBSTR(value.bstrVal);   
   
        // Convert string to bytes   
        CAtlArray<BYTE> abValue;   
        LPCWSTR pszStart = (LPCWSTR)strValue;   
        LPWSTR pszEnd = NULL;   
        for(;;)   
        {   
            BYTE bValue = (BYTE)wcstol(pszStart, &pszEnd, 16);   
            if( pszStart==pszEnd )   
            {   
                break;   
            }   
            abValue.Add( bValue );   
            pszStart = pszEnd;   
        }   
   
        CHECK_HR( hr = pAttrib->SetBlob( guidKey, abValue.GetData(), (UINT32)abValue.GetCount() ) );   
    }   
    else  // Unknown type   
    {   
        CHECK_HR( hr = E_UNEXPECTED );   
    }   
   
done:   
    return hr;   
}   
   
///////////////////////////////////////////////////////////////////////////////   
// Converts BSTR strings like "{464CBD74-3177-4593-ADC7-7F7AC6F29286}" or "464CBD74-3177-4593-ADC7-7F7AC6F29286" to GUIDs   
///////////////////////////////////////////////////////////////////////////////   
HRESULT CProfileBuilder::CLSIDFromBSTR( __in const CComBSTR &bstr, __out GUID *pguid )   
{   
    HRESULT hr = S_OK;   
    CAtlString str = bstr;   
   
    assert( pguid );   
   
    // Add surrounding '{' and '}' if missing   
    if( L'{' != str[0] )   
    {   
        str = L"{" + str + L"}";   
    }   
   
    CHECK_HR( hr = CLSIDFromString( str, pguid) );   
   
done:   
    return hr;   
}   

