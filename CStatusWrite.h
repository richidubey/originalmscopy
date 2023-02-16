
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
