// CUsrSynchro.h: interface for the CUsrSynchro class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_USRSYNCHRO_H__F45E9879_6663_451E_96F5_464B4D95DAE2__INCLUDED_)
#define AFX_USRSYNCHRO_H__F45E9879_6663_451E_96F5_464B4D95DAE2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Global.h"

class CUsrSynchro  
{
	public:
		CUsrSynchro(tstMSCopyCfg* pmccMSCopyCfg);
		virtual ~CUsrSynchro();

		ULONGLONG m_ul64NextStartTime;		// From FILETIME format: time unit = 100ns

		unsigned char ExecuteTreatment(tstMSCopyStatus* pmcsMSCopyStatus);

	private:
		unsigned long m_ulTimePeriod;		// Time unit = min
		char m_strUsrPathLocal[100];
		char m_strUsrPathServer[100];
		TCHAR m_tstrUsrPathLocal[100];
		TCHAR m_tstrUsrPathServer[100];
};

#endif // !defined(AFX_USRSYNCHRO_H__F45E9879_6663_451E_96F5_464B4D95DAE2__INCLUDED_)
