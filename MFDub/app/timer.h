#pragma once

#include <atlfile.h>

class CPerfTimer
{
public:
    CPerfTimer(CAtlString strFileName)
    {
        m_file.Create(strFileName, FILE_WRITE_DATA, FILE_SHARE_READ, CREATE_ALWAYS);
    }
    
    void TimeBegin()
    {
        QueryPerformanceFrequency(&m_liFreq);
        QueryPerformanceCounter(&m_liBegin);
    }
    
    void TimeEnd(CAtlString strTitle, bool fPrintBegin = false)
    {
        QueryPerformanceCounter(&m_liEnd);
        
        CAtlStringA strOut;
        if(fPrintBegin) strOut.Format("%I64d: %S : %f seconds\r\n", m_liBegin.QuadPart * 1000 / m_liFreq.QuadPart, strTitle, double(m_liEnd.QuadPart - m_liBegin.QuadPart) / m_liFreq.QuadPart);
        else strOut.Format("%S : %f seconds\r\n", strTitle, double(m_liEnd.QuadPart - m_liBegin.QuadPart) / m_liFreq.QuadPart);
        
        m_file.Write(strOut, strOut.GetLength());
    }
    
private:
    CAtlFile m_file;
    LARGE_INTEGER m_liFreq;
    LARGE_INTEGER m_liBegin;
    LARGE_INTEGER m_liEnd;
};