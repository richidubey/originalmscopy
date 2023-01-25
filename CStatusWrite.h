// CStatusWrite.h: interface for the CStatusWrite class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CSTATUSWRITE_H__410373CC_BE4C_486C_8018_0C831388989D__INCLUDED_)
#define AFX_CSTATUSWRITE_H__410373CC_BE4C_486C_8018_0C831388989D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Global.h"
#include "CIniFile.h"

class CStatusWrite  
{
	public:
		CStatusWrite(tstMSCopyCfg* pmccMSCopyCfg);
		virtual ~CStatusWrite();

		unsigned char m_ucOperatingMode;
		ULONGLONG m_ul64NextStartTime;

		unsigned char ExecuteTreatment(tstMSCopyStatus* pmcsMSCopyStatus);
		
	private:
		unsigned long m_ulStatusWritePeriod;
		CIniFile *m_pcifPanelStartupFile;
		char m_strStatusFilePath[100];
};

#endif // !defined(AFX_CSTATUSWRITE_H__410373CC_BE4C_486C_8018_0C831388989D__INCLUDED_)
