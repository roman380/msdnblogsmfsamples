#include "stdafx.h"

#include "sourcesamplecacher.h"

////////////////////////////////////////////////////////////////////
//

CSourceStreamReader::CSourceStreamReader()
{
}

CSourceStreamReader::~CSourceStreamReader()
{
}

HRESULT CSourceStreamReader::LoadStream(IMFMediaStream* pStream)
{
    m_spStream = pStream;
    
    IFC( m_spStream->BeginGetEvent(&m_xOnStreamEvent, NULL) );
    IFC( m_spStream->RequestSample(NULL) );
    
done:
    return hr;
}

void CSourceStreamReader::
///////////////////////////////////////////////////////////////////
//

CSourceSampleCacher::CSourceSampleCacher()
{

}

CSourceSampleCacher::~CSourceSampleCacher()
{

}

HRESULT CSourceSampleCacher::LoadSource(IMFMediaSource* pSource)
{
    m_spCurrentSource = pSource;
    
    m_spCurrentSource->CreatePresentationDescriptor( &m_spPD );
    IFC( MFSelectAllStreams(m_spPD) );
    
    IFC( m_spCurrentSource->BeginGetEvent(&m_xOnSourceEvent, NULL) );
 
    PROPVARIANT varStartPosition;
    varStartPosition.vt = VT_EMPTY;
    IFC( m_spCurrentSource->Start(m_spPD, GUID_NULL, &varStartPosition) );
}


void CSourceSampleCacher::OnSourceEvent(IMFAsyncResult* pResult)
{
    CComPtr<IMFMediaEvent> spEvent;
    
    IFC( m_spCurrentSource->EndGetEvent(pResult, &spEvent) );
    IFC( HandleEvent(spEvent) );
    IFC( m_spCurrentSource->BeginGetEvent(&m_xOnSourceEvent, NULL) );
    
done:
}

HRESULT CSourceSampleCacher::HandleEvent(IMFMediaEvent* pEvent)
{
    HRESULT hr = S_OK;
    HRESULT hrEvent = S_OK;
    MediaEventType met;
    PROPVARIANT var;

    PropVariantInit(&var);
    
    IFC( pEvent->GetType(&met) );
    IFC( pEvent->GetStatus(&hrEvent) );
    IFC( pEvent->GetValue(&var) );
    
    switch(met)
    {
        case MENewStream:
        
    }
}
