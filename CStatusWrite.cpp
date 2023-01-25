// CStatusWrite.cpp: implementation of the CStatusWrite class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CStatusWrite.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CStatusWrite::CStatusWrite(tstMSCopyCfg* pmccMSCopyCfg)
{
	// Initialization of member variables
	m_ul64NextStartTime = 0;
	m_ulStatusWritePeriod = pmccMSCopyCfg->lStatusWritePeriod;
	
	strcpy(m_strStatusFilePath, pmccMSCopyCfg->strStatusFilePath);
	m_pcifPanelStartupFile = new CIniFile(pmccMSCopyCfg->strPanelStartupFilePath);

	m_ucOperatingMode = STARTING;
	LogTrace("[STATUS WRITE] Operating mode = STARTING", 2);
}

CStatusWrite::~CStatusWrite()
{
}

///////////////////////////////////////////////////////////////////////
// CIniFile member functions

/*********************************************************************/
/* Function: ExecuteTreatment                                        */
/*                                                                   */
/* Parameters:                                                       */
/*		tstMSCopyStatus* pmcsMSCopyStatus [in]:                      */
/*			Structure containing MSCopy status data			         */
/*                                                                   */
/* Return value:                                                     */
/*      unsigned char:                                               */
/*			0 if success, >0 if error						         */
/*                                                                   */
/* Remarks:                                                          */
/*		Write the MSCopy status file to the flash card. At the end,  */
/*      calculate the next treatment start date.	                 */
/*********************************************************************/

unsigned char CStatusWrite::ExecuteTreatment(tstMSCopyStatus* pmcsMSCopyStatus)
{
	unsigned char ucRet;
	unsigned long ulNextWritePeriod;
	long lPanelStartupStatus;
	char strTempString[200];

	FILE *pfiStatusFile;
	
	// File opening in read/write mode
	pfiStatusFile = fopen(m_strStatusFilePath, "w+");

	if (pfiStatusFile != NULL)
	{
		// MSCopy context
		fprintf(pfiStatusFile, "[Global]\n");
		fprintf(pfiStatusFile, "PrgVersion=%.2f\n\n", pmcsMSCopyStatus->fMSCopyVersion);

		// Log files move status
		fprintf(pfiStatusFile, "[LogFile]\n");
		fprintf(pfiStatusFile, "ServerUsed=%s\n", pmcsMSCopyStatus->strLogFileServerUsed);
		
		if (pmcsMSCopyStatus->bLogFileMoveRecovery == true)
		{
			fprintf(pfiStatusFile, "MoveRecovery=1\n");
		}
		else
		{
			fprintf(pfiStatusFile, "MoveRecovery=0\n");
		}

		if (pmcsMSCopyStatus->bLogFileMoveError == true)
		{
			fprintf(pfiStatusFile, "MoveError=1\n\n");
		}
		else
		{
			fprintf(pfiStatusFile, "MoveError=0\n\n");
		}

		// User profile synchronisation status 
		fprintf(pfiStatusFile, "[UserProfile]\n");

		if (pmcsMSCopyStatus->bUsrSynchroError == true)
		{
			fprintf(pfiStatusFile, "SynchroError=1\n");
		}
		else
		{
			fprintf(pfiStatusFile, "SynchroError=0\n");
		}

		fclose(pfiStatusFile);

		LogTrace("[STATUS WRITE] Status file updated", 2);
	}

	// Calculate the next treatement start time according to the current operating
	// mode. While the mode value is STARTING, the MSCopy status file will be
	// rewritten each minute. When the panel startup is finished (=> synchronized 
	// by NTP), the mode changes to NORMAL.
	if (m_ucOperatingMode == STARTING)
	{
		ulNextWritePeriod = 1;

		ucRet = m_pcifPanelStartupFile->ReadLongValue("", "startup_status", &lPanelStartupStatus);

		if (ucRet == RET_OK)
		{
			if ((lPanelStartupStatus == 1) || (lPanelStartupStatus == 2))
			{
				m_ucOperatingMode = NORMAL;
				ulNextWritePeriod = m_ulStatusWritePeriod;
				LogTrace("[STATUS WRITE] Operating mode = NORMAL", 2);
			}
		}
	}
	else
	{
		ulNextWritePeriod = m_ulStatusWritePeriod;
	}

	// Next treatement start time
	ucRet = GetNextStartTime(ulNextWritePeriod, &m_ul64NextStartTime);
	
	if (ucRet != RET_OK)
	{
		// Start time calculation failed
		// Reset the next start time to restart the treatment in 1 minute
		m_ul64NextStartTime = 0;
		
		sprintf(strTempString,
				"[STATUS WRITE] Failed to compute the next start time (local error code = %d). Retry in 1 minute.",
				ucRet);
		LogTrace(strTempString, 1);
	}
	
	return(RET_OK);
}