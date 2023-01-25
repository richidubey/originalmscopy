// CUsrSynchro.cpp: implementation of the CUsrSynchro class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CUsrSynchro.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CUsrSynchro::CUsrSynchro(tstMSCopyCfg* pmccMSCopyCfg)
{
	// Initialization of member variables 
	m_ul64NextStartTime = 0;
	m_ulTimePeriod = pmccMSCopyCfg->lUsrSynchroPeriod;
	strcpy(m_strUsrPathLocal, pmccMSCopyCfg->strUsrPathLocal);
	strcpy(m_strUsrPathServer, pmccMSCopyCfg->strUsrPathServer);

	// Type conversion char to TCHAR
	wsprintf(m_tstrUsrPathLocal, _T("%S"), m_strUsrPathLocal);
	wsprintf(m_tstrUsrPathServer, _T("%S"), m_strUsrPathServer);
}

CUsrSynchro::~CUsrSynchro()
{
}

///////////////////////////////////////////////////////////////////////
// CIniFile member functions

/*********************************************************************/
/* Function: ExecuteTreatment                                        */
/*                                                                   */
/* Parameters:                                                       */
/*		tstMSCopyStatus* pmcsMSCopyStatus [out]:                     */
/*			Structure containing MSCopy status data			         */
/*                                                                   */
/* Return value:                                                     */
/*      unsigned char:                                               */
/*			0 if success, >0 if error						         */
/*                                                                   */
/* Remarks:                                                          */
/*		Synchronize the local user file with the reference user file */
/*		stored in the CWS server. Copy the reference file only if	 */
/*		the 2 files have different data. At the end, calculate the   */
/*		next treatment start date.									 */
/*********************************************************************/

unsigned char CUsrSynchro::ExecuteTreatment(tstMSCopyStatus* pmcsMSCopyStatus)
{
	bool bEndOfLocalFile;
	bool bEndOfServerFile;
	bool bReadErrorInLocalFile;
	bool bReadErrorInServerFile;
	bool bFilesAreIdentical;
	bool bNeedToCopyServerFile;
	unsigned char ucRetValue;
	unsigned char ucRetValue2;
	DWORD dwErrorCode;

	HANDLE hSearchFile;
	WIN32_FIND_DATA wfdFileData;
	FILE *pfiLocalFile;
	FILE *pfiServerFile;

	char strTempString[200];
	char strTempString2[200];

	bNeedToCopyServerFile = false;
	ucRetValue = RET_OK;
	
	// Checks if a local version of the user file version exists
	hSearchFile = FindFirstFile(m_tstrUsrPathLocal, &wfdFileData);

	if (hSearchFile == INVALID_HANDLE_VALUE)
	{
		// No local version found => copy of the reference file from
		// server requested
		dwErrorCode = GetLastError();
		sprintf(strTempString, 
				"[USR SYNC] Failed to find the local user file (win32 error code = %d). File copy requested.", 
				dwErrorCode);
		LogTrace(strTempString, 1);

		bNeedToCopyServerFile = true;
	}
	else
	{
		// Local user file found => compare data between local and server 
		// file
			
		// Opening of the local file
		pfiLocalFile = fopen(m_strUsrPathLocal, "r");

		if (pfiLocalFile != NULL)
		{
			// Opening of the server file
			pfiServerFile = fopen(m_strUsrPathServer, "r");
			
			if (pfiServerFile != NULL)
			{	
				// Reset EOF and read error flags
				bEndOfLocalFile = false;
				bReadErrorInLocalFile = false;
				bEndOfServerFile = false;
				bReadErrorInServerFile = false;
				
				bFilesAreIdentical = true;

				// Comparison of each line read from the 2 files
				while(!(bEndOfLocalFile || bReadErrorInLocalFile
					  || bEndOfServerFile || bReadErrorInServerFile))
				{
					// Buffer data reset
					strTempString[0] = '\0';
					strTempString2[0] = '\0';
							
					// Reading of one line in each file
					if (fgets(strTempString, 200, pfiLocalFile) == NULL)
					{
						// End of file or error found
						if (feof(pfiLocalFile))
						{
							bEndOfLocalFile = true;
						}
						else
						{
							bReadErrorInLocalFile = true;
							LogTrace("[USR SYNC] Local user file reading error", 1);
						}
					}

					if (fgets(strTempString2, 200, pfiServerFile) == NULL)
					{
						// End of file or error found
						if (feof(pfiServerFile))
						{
							bEndOfServerFile = true;
						}
						else
						{
							bReadErrorInServerFile = true;
							LogTrace("[USR SYNC] Server user file reading error", 1);
						}
					}						
					
					// If file reading interrupted, checks if it's
					// necessary to synchronize the data
					if ((bEndOfLocalFile == !bEndOfServerFile)
						|| (bReadErrorInLocalFile == true)
						|| (bReadErrorInServerFile == true))
					{
						// Different number of lines between files or 
						// reading error found (impossible to compare next
						// data) => need to synchronize user data
						bFilesAreIdentical = false;
						bNeedToCopyServerFile = true;
					}
					else
					{
						// Comparison of the read lines
						if ((bEndOfLocalFile == false) && (bEndOfServerFile == false))
						{
							if (strcmp(strTempString, strTempString2) != 0)
							{
								// Difference found => need to synchronize user data
								bFilesAreIdentical = false;
								bNeedToCopyServerFile = true;
								break;
							}
						}
					}
				}
				
				// Reset the user synchronisation default if necessary, and display
				// traces
				if (bFilesAreIdentical)
				{
					// Reset the user file synchronisation default
					pmcsMSCopyStatus->bUsrSynchroError = false;
					LogTrace("[USR SYNC] Local and server user files are identical. File copy not requested.", 2);
				}
				else
				{
					LogTrace("[USR SYNC] Local and server user files are not identical. File copy requested.", 2);
				}

				fclose(pfiServerFile);
			}
			else
			{
				// Opening of the server file failed
				bNeedToCopyServerFile = true;
				LogTrace("[USR SYNC] Opening of the server user file for comparison failed. File copy requested.", 2);
			}
			
			fclose(pfiLocalFile);
		}
		else
		{
			// Opening of the local file failed
			bNeedToCopyServerFile = true;
			LogTrace("[USR SYNC] Opening of the local user file for comparison failed. File copy requested.", 2);
		}

		FindClose(hSearchFile);
	}

	// Copy of the server file to the local directory if requested
	if (bNeedToCopyServerFile)
	{
		if (CopyFile(m_tstrUsrPathServer, m_tstrUsrPathLocal, FALSE))
		{
			// File copy succeeded
			LogTrace("[USR SYNC] User file copy from server OK", 2);

			// Reset the user file synchronisation default
			pmcsMSCopyStatus->bUsrSynchroError = false;
		}
		else
		{
			// File copy failed
			dwErrorCode = GetLastError();
			sprintf(strTempString, 
					"[USR SYNC] Failed to copy the user file from server (win32 error code = %d)", 
					dwErrorCode);
			LogTrace(strTempString, 1);

			// Set the user file synchronisation default
			pmcsMSCopyStatus->bUsrSynchroError = true;
			
			ucRetValue = RET_ERR_USR_FILE_COPY;
		}
	}

	// Calculate the next treatement start time
	ucRetValue2 = GetNextStartTime(m_ulTimePeriod, &m_ul64NextStartTime);
		
	if (ucRetValue2 != RET_OK)
	{
		// Start time calculation failed
		// Reset the next start time to restart the treatment in 1 minute
		m_ul64NextStartTime = 0;
		
		sprintf(strTempString,
				"[USR SYNC] Failed to compute the next start time (local error code = %d). Retry in 1 minute.",
				ucRetValue2);
		LogTrace(strTempString, 1);
	}

	return(ucRetValue);
}
