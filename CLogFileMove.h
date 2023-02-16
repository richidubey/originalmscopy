#include "Global.h"

extern SOCKET listening_socket, client_socket;

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

		TCHAR m_tstrMeasPathLocal[100];
		TCHAR m_tstrEvtPathLocal[100];

		unsigned int CLogFileMove::SendMeasFiles(tstMSCopyStatus* pmcsMSCopyStatus);
		unsigned int CLogFileMove::SendEventFiles(tstMSCopyStatus* pmcsMSCopyStatus);
};
