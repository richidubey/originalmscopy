#include "Global.h"

extern SOCKET listening_socket, client_socket;

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
		char m_strUsrTempPathLocal[100];
		char m_strUsrEncTempPathLocal[100];
		TCHAR m_tstrUsrPathLocal[100];
		TCHAR m_tstrUsrPathServer[100];
		TCHAR m_tstrUsrTempPathLocal[100];

		int CUsrSynchro::GetUserFile();
};