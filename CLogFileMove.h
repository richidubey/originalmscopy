// CLogFileMove.h: interface for the CLogFileMove class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CLOGFILEMOVE_H__A22AD16C_2627_46D1_A925_E853C9D36F55__INCLUDED_)
#define AFX_CLOGFILEMOVE_H__A22AD16C_2627_46D1_A925_E853C9D36F55__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Global.h"

class CLogFileMove  
{
	public:
		CLogFileMove(tstMSCopyCfg* pmccMSCopyCfg);
		virtual ~CLogFileMove();

		ULONGLONG m_ul64NextStartTime;		// From FILETIME format: time unit = 100ns

		unsigned char ExecuteTreatment(tstMSCopyStatus* pmcsMSCopyStatus);
		
	private:
		unsigned long m_ulMovePeriod;
		unsigned long m_ulSrvScrutingDelay;
		unsigned long m_ulSuccessiveMove;
		unsigned char m_ucCurServer;
		ULONGLONG m_ul64NextSwitchToServer1;
		
		char m_strNameServer1[100];
		char m_strNameServer2[100];

		TCHAR m_tstrMeasPathLocal[100];
		TCHAR m_tstrMeasPathServer1[100];
		TCHAR m_tstrMeasPathServer2[100];
		TCHAR m_tstrEvtPathLocal[100];
		TCHAR m_tstrEvtPathServer1[100];
		TCHAR m_tstrEvtPathServer2[100];

		unsigned char CLogFileMove::MoveLogFiles(TCHAR* tstrLocalDirPath, TCHAR* tstrFileName, unsigned char ucFileType);
};

#endif // !defined(AFX_CLOGFILEMOVE_H__A22AD16C_2627_46D1_A925_E853C9D36F55__INCLUDED_)
