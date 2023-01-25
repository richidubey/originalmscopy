// CLogFileMove.cpp: implementation of the CLogFileMove class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CLogFileMove.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CLogFileMove::CLogFileMove(tstMSCopyCfg* pmccMSCopyCfg)
{
	// Initialization of member variables 
	m_ul64NextStartTime = 0;
	m_ul64NextSwitchToServer1 = 0;
	m_ucCurServer = SERVER1;

	m_ulMovePeriod = pmccMSCopyCfg->lMovePeriod;
	m_ulSrvScrutingDelay = pmccMSCopyCfg->lSrvScrutingDelay;
	m_ulSuccessiveMove = pmccMSCopyCfg->lSuccessiveMove;
	
	// Type conversion char to TCHAR
	wsprintf(m_tstrMeasPathLocal, _T("%S"), pmccMSCopyCfg->strMeasPathLocal);
	wsprintf(m_tstrMeasPathServer1, _T("\\\\%S\\%S"), pmccMSCopyCfg->strServerName1, pmccMSCopyCfg->strMeasPathServer);
	wsprintf(m_tstrMeasPathServer2, _T("\\\\%S\\%S"), pmccMSCopyCfg->strServerName2, pmccMSCopyCfg->strMeasPathServer);
	wsprintf(m_tstrEvtPathLocal, _T("%S"), pmccMSCopyCfg->strEvtPathLocal);
	wsprintf(m_tstrEvtPathServer1, _T("\\\\%S\\%S"), pmccMSCopyCfg->strServerName1, pmccMSCopyCfg->strEvtPathServer);
	wsprintf(m_tstrEvtPathServer2, _T("\\\\%S\\%S"), pmccMSCopyCfg->strServerName2, pmccMSCopyCfg->strEvtPathServer);

	// Server name extraction
	strcpy(m_strNameServer1, pmccMSCopyCfg->strServerName1);
	strcpy(m_strNameServer2, pmccMSCopyCfg->strServerName2);
}

CLogFileMove::~CLogFileMove()
{
}

///////////////////////////////////////////////////////////////////////
// CLogFileMove member functions

/*********************************************************************/
/* Function: ExecuteTreatment                                        */
/*                                                                   */
/* Parameters:                                                       */
/*		tstMSCopyStatus* pmcstMSCopyStatus [out]:                    */
/*			Structure containing MSCopy status data			         */
/*                                                                   */
/* Return value:                                                     */
/*      unsigned char:                                               */
/*			0 if success, >0 if error						         */
/*                                                                   */
/* Remarks:                                                          */
/*		Manages the move of measure and event log files to the main  */
/*		server. When a lot of files are waiting, the oldest files    */
/*		are sent by block as specified in the "PANEL.INI" file with  */
/*		the most recent log file. If the current server is the	     */
/*		number 2, checks if it's time to do an attempt to server 1.  */
/*		At the end, calculates the next treatment start date.		 */
/*********************************************************************/

unsigned char CLogFileMove::ExecuteTreatment(tstMSCopyStatus* pmcsMSCopyStatus)
{
	unsigned char i;
	unsigned char ucRet;
	unsigned long ulNbOfMeasFileMoved;
	unsigned long ulNbOfEvtFileMoved;
	bool bOtherFilesToMoveInCurDir;

	HANDLE hSearchFile;
	WIN32_FIND_DATA wfdFileData;
	SYSTEMTIME stimeGMTCurTime;
	FILETIME ftimeGMTCurTime;
	ULONGLONG ul64GMTCurTime;
	ULONGLONG ul64GMTFileWTime;
	ULONGLONG ul64CurTimeFileWTimeDiff;

	char strTempString[200];
	TCHAR tstrTempString[100];
	TCHAR tstrDirPath[100];

	// Checks if the switch to server 1 is needed
	if (m_ucCurServer == SERVER2)
	{
		GetSystemTime(&stimeGMTCurTime);
		if (SystemTimeToFileTime(&stimeGMTCurTime, &ftimeGMTCurTime) == FALSE)
		{
			LogTrace("[LOGFILE MOVE] Failed to convert the current system time in file time. The current server remains Server 2", 1);
		}
		else
		{
			ul64GMTCurTime = (((ULONGLONG)(ftimeGMTCurTime.dwHighDateTime)) << 32) + ftimeGMTCurTime.dwLowDateTime;
			
			if (ul64GMTCurTime >= m_ul64NextSwitchToServer1)
			{
				m_ucCurServer = SERVER1;
			}
		}
	}
	
	// Status and flag initialization
	pmcsMSCopyStatus->bLogFileMoveRecovery = false;
	ulNbOfMeasFileMoved = 0;
	ulNbOfEvtFileMoved = 0;

	// MEASURE LOG FILES
	
	// -> Moves the requested number of the oldest files (if sufficient number of files stored
	//    in flash card)

	// Browses the MSLog directories from "11" (oldest files) to "1" (more recent files)
	for (i = 11; i > 0; i--)
	{
		lstrcpy(tstrDirPath, m_tstrMeasPathLocal);
		wsprintf(tstrTempString, _T("\\%d"), i);
		lstrcat(tstrDirPath, tstrTempString);
		
		lstrcpy(tstrTempString, tstrDirPath);
		lstrcat(tstrTempString, _T("\\*.log"));
		
		// In the current directory, searches log file names sorted by alphabetic order to browse 
		// it from the oldest to the more recent file
		hSearchFile = FindFirstFile(tstrTempString, &wfdFileData);

		if (hSearchFile != INVALID_HANDLE_VALUE)
		{
			// The 1st file is found. 
			
			// Checks if its last write time is different than the current time
			// (<=> time difference >= 1s)
			GetSystemTime(&stimeGMTCurTime);
			if (SystemTimeToFileTime(&stimeGMTCurTime, &ftimeGMTCurTime) == FALSE)
			{
				LogTrace("[LOGFILE MOVE] Failed to convert the current system time in file time. Current log file not moved.", 1);
			}
			else
			{
				if (CompareFileTime(&(wfdFileData.ftLastWriteTime), &ftimeGMTCurTime) != 0)
				{
					// Moves it to server
					ucRet = MoveLogFiles(tstrDirPath, wfdFileData.cFileName, MEASURE);

					if (ucRet == RET_OK)
					{
						ulNbOfMeasFileMoved++;
					}
					else
					{
						pmcsMSCopyStatus->bLogFileMoveError = true;
						strcpy(pmcsMSCopyStatus->strLogFileServerUsed, "No server");
						FindClose(hSearchFile);
						return(ucRet);
					}
				}
			}

			// Searches the requested number of next files if existing and moves them to server
			bOtherFilesToMoveInCurDir = true;
			while ((bOtherFilesToMoveInCurDir == true) && (ulNbOfMeasFileMoved < m_ulSuccessiveMove))
			{
				if (FindNextFile(hSearchFile, &wfdFileData) != FALSE)
				{
					// The next file is found

					// Checks if its last write time is different than the current time 
					// (<=> time difference >= 1s)
					GetSystemTime(&stimeGMTCurTime);
					if (SystemTimeToFileTime(&stimeGMTCurTime, &ftimeGMTCurTime) == FALSE)
					{
						LogTrace("[LOGFILE MOVE] Failed to convert the current system time in file time. Current log file not moved.", 1);
					}
					else
					{
						if (CompareFileTime(&(wfdFileData.ftLastWriteTime), &ftimeGMTCurTime) != 0)
						{
							// Moves it to server					
							ucRet = MoveLogFiles(tstrDirPath, wfdFileData.cFileName, MEASURE);

							if (ucRet == RET_OK)
							{
								ulNbOfMeasFileMoved++;
							}
							else
							{
								pmcsMSCopyStatus->bLogFileMoveError = true;
								strcpy(pmcsMSCopyStatus->strLogFileServerUsed, "No server");
								FindClose(hSearchFile);
								return(ucRet);
							}
						}
					}
				}
				else
				{
					bOtherFilesToMoveInCurDir = false;
				}
			}

			FindClose(hSearchFile);
			
			// If the threshold of moved files is reached, the MSLog directories browsing is
			// stopped. Otherwise, all files stored in the current directory have been moved so
			// it's necessary to browse the next directory.
			if (ulNbOfMeasFileMoved == m_ulSuccessiveMove)
			{
				break;
			}
		}
	}
				
	// -> Move the most recent file
	if (ulNbOfMeasFileMoved == m_ulSuccessiveMove)
	{
		lstrcpy(tstrDirPath, m_tstrMeasPathLocal);
		wsprintf(tstrTempString, _T("\\1"), i);
		lstrcat(tstrDirPath, tstrTempString);
		
		lstrcpy(tstrTempString, tstrDirPath);
		lstrcat(tstrTempString, _T("\\*.log"));
		
		hSearchFile = FindFirstFile(tstrTempString, &wfdFileData);

		// In the directory "1", searches log file names sorted by alphabetic order and moves 
		// the last file which is the more recent
		if (hSearchFile != INVALID_HANDLE_VALUE)
		{
			while (FindNextFile(hSearchFile, &wfdFileData) != FALSE);

			// Checks if the last file write time is different than the current time
			// (<=> time difference >= 1s)
			GetSystemTime(&stimeGMTCurTime);
			if (SystemTimeToFileTime(&stimeGMTCurTime, &ftimeGMTCurTime) == FALSE)
			{
				LogTrace("[LOGFILE MOVE] Failed to convert the current system time in file time. Current log file not moved.", 1);
			}
			else
			{
				if (CompareFileTime(&(wfdFileData.ftLastWriteTime), &ftimeGMTCurTime) != 0)
				{
					// Moves it to server
					ucRet = MoveLogFiles(tstrDirPath, wfdFileData.cFileName, MEASURE);

					if (ucRet == RET_OK)
					{
						ulNbOfMeasFileMoved++;
					}
					else
					{
						pmcsMSCopyStatus->bLogFileMoveError = true;
						strcpy(pmcsMSCopyStatus->strLogFileServerUsed, "No server");
						FindClose(hSearchFile);
						return(ucRet);
					}
				}
			}

			FindClose(hSearchFile);
		}
	}
	
	// EVENT LOG FILES

	// -> Move the requested number of the oldest files (if sufficient number of files stored 
	//	  in flash card)
	
	lstrcpy(tstrTempString, m_tstrEvtPathLocal);
	lstrcat(tstrTempString, _T("\\*.log"));
		
	hSearchFile = FindFirstFile(tstrTempString, &wfdFileData);

	// In the local event directory, searches log file names sorted by alphabetic order to browse 
	// it from the oldest to the more recent file
	if (hSearchFile != INVALID_HANDLE_VALUE)
	{
		// The 1st file is found

		// Checks if the time difference between last file write time and current time is more
		// than 1min
		GetSystemTime(&stimeGMTCurTime);
		if (SystemTimeToFileTime(&stimeGMTCurTime, &ftimeGMTCurTime) == FALSE)
		{
			LogTrace("[LOGFILE MOVE] Failed to convert the current system time in file time. Current log file not moved.", 1);
		}
		else
		{
			ul64GMTCurTime = (((ULONGLONG)(ftimeGMTCurTime.dwHighDateTime)) << 32) + ftimeGMTCurTime.dwLowDateTime;
			ul64GMTFileWTime = (((ULONGLONG)(wfdFileData.ftLastWriteTime.dwHighDateTime)) << 32) + wfdFileData.ftLastWriteTime.dwLowDateTime;
			
			if (CompareFileTime(&(wfdFileData.ftLastWriteTime), &ftimeGMTCurTime) == -1)
			{	
				ul64CurTimeFileWTimeDiff = ul64GMTCurTime - ul64GMTFileWTime;
			}
			else
			{	
				ul64CurTimeFileWTimeDiff = ul64GMTFileWTime - ul64GMTCurTime;
			}

			if (ul64CurTimeFileWTimeDiff > MINUTE_100NS)		
			{
				// Moves it to server
				ucRet = MoveLogFiles(m_tstrEvtPathLocal, wfdFileData.cFileName, EVENT);

				if (ucRet == RET_OK)
				{
					ulNbOfEvtFileMoved++;
				}
				else
				{
					pmcsMSCopyStatus->bLogFileMoveError = true;
					strcpy(pmcsMSCopyStatus->strLogFileServerUsed, "No server");
					FindClose(hSearchFile);
					return(ucRet);
				}
			}
		}

		// Searches the requested number of next files if existing and moves them to server
		bOtherFilesToMoveInCurDir = true;
		while ((bOtherFilesToMoveInCurDir == true) && (ulNbOfEvtFileMoved < m_ulSuccessiveMove))
		{
			if (FindNextFile(hSearchFile, &wfdFileData) != FALSE)
			{
				// The next file is found

				// Checks if the time difference between last file write time and current time is more
				// than 1min
				GetSystemTime(&stimeGMTCurTime);
				if (SystemTimeToFileTime(&stimeGMTCurTime, &ftimeGMTCurTime) == FALSE)
				{
					LogTrace("[LOGFILE MOVE] Failed to convert the current system time in file time. Current log file not moved.", 1);
				}
				else
				{
					ul64GMTCurTime = (((ULONGLONG)(ftimeGMTCurTime.dwHighDateTime)) << 32) + ftimeGMTCurTime.dwLowDateTime;
					ul64GMTFileWTime = (((ULONGLONG)(wfdFileData.ftLastWriteTime.dwHighDateTime)) << 32) + wfdFileData.ftLastWriteTime.dwLowDateTime;
					
					if (CompareFileTime(&(wfdFileData.ftLastWriteTime), &ftimeGMTCurTime) == -1)
					{	
						ul64CurTimeFileWTimeDiff = ul64GMTCurTime - ul64GMTFileWTime;
					}
					else
					{	
						ul64CurTimeFileWTimeDiff = ul64GMTFileWTime - ul64GMTCurTime;
					}

					if (ul64CurTimeFileWTimeDiff > MINUTE_100NS)		
					{
						// Moves it to server
						ucRet = MoveLogFiles(m_tstrEvtPathLocal, wfdFileData.cFileName, EVENT);

						if (ucRet == RET_OK)
						{
							ulNbOfEvtFileMoved++;
						}
						else
						{
							pmcsMSCopyStatus->bLogFileMoveError = true;
							strcpy(pmcsMSCopyStatus->strLogFileServerUsed, "No server");
							FindClose(hSearchFile);
							return(ucRet);
						}
					}
				}
			}
			else
			{
				bOtherFilesToMoveInCurDir = false;
			}
		}

		FindClose(hSearchFile);
	}
				
	// -> Move the most recent file
	if (ulNbOfEvtFileMoved == m_ulSuccessiveMove)
	{
		lstrcpy(tstrTempString, m_tstrEvtPathLocal);
		lstrcat(tstrTempString, _T("\\*.log"));
		
		hSearchFile = FindFirstFile(tstrTempString, &wfdFileData);

		// In the local event directory, searches log file names sorted by alphabetic order and 
		// moves the last file which is the more recent
		if (hSearchFile != INVALID_HANDLE_VALUE)
		{
			while (FindNextFile(hSearchFile, &wfdFileData) != FALSE);

			// Checks if the time difference between last file write time and current time is more
			// than 1min
			GetSystemTime(&stimeGMTCurTime);
			if (SystemTimeToFileTime(&stimeGMTCurTime, &ftimeGMTCurTime) == FALSE)
			{
				LogTrace("[LOGFILE MOVE] Failed to convert the current system time in file time. Current log file not moved.", 1);
			}
			else
			{
				ul64GMTCurTime = (((ULONGLONG)(ftimeGMTCurTime.dwHighDateTime)) << 32) + ftimeGMTCurTime.dwLowDateTime;
				ul64GMTFileWTime = (((ULONGLONG)(wfdFileData.ftLastWriteTime.dwHighDateTime)) << 32) + wfdFileData.ftLastWriteTime.dwLowDateTime;
					
				if (CompareFileTime(&(wfdFileData.ftLastWriteTime), &ftimeGMTCurTime) == -1)
				{	
					ul64CurTimeFileWTimeDiff = ul64GMTCurTime - ul64GMTFileWTime;
				}
				else
				{	
					ul64CurTimeFileWTimeDiff = ul64GMTFileWTime - ul64GMTCurTime;
				}

				if (ul64CurTimeFileWTimeDiff > MINUTE_100NS)		
				{
					// Moves it to server	
					ucRet = MoveLogFiles(m_tstrEvtPathLocal, wfdFileData.cFileName, EVENT);

					if (ucRet == RET_OK)
					{
						ulNbOfEvtFileMoved++;
					}
					else
					{
						pmcsMSCopyStatus->bLogFileMoveError = true;
						strcpy(pmcsMSCopyStatus->strLogFileServerUsed, "No server");
						FindClose(hSearchFile);
						return(ucRet);
					}
				}
			}

			FindClose(hSearchFile);
		}
	}

	// Update of the MSCopy status
	if ((ulNbOfMeasFileMoved == m_ulSuccessiveMove + 1) || (ulNbOfEvtFileMoved == m_ulSuccessiveMove + 1)) 
	{
		pmcsMSCopyStatus->bLogFileMoveRecovery = true;
	}
	else
	{
		// Displays a trace if no file to move
		if ((ulNbOfMeasFileMoved == 0) && (ulNbOfEvtFileMoved == 0)) 
		{
			LogTrace("[LOGFILE MOVE] No file to move", 2);
		}
	}

	pmcsMSCopyStatus->bLogFileMoveError = false;
	
	if (m_ucCurServer == SERVER1)
	{
		strcpy(pmcsMSCopyStatus->strLogFileServerUsed, m_strNameServer1);
	}
	else
	{
		strcpy(pmcsMSCopyStatus->strLogFileServerUsed, m_strNameServer2);
	}

	// Calculate the next treatement start time
	ucRet = GetNextStartTime(m_ulMovePeriod, &m_ul64NextStartTime);
	
	if (ucRet != RET_OK)
	{
		// Start time calculation failed
		// Reset the next start time to restart the treatment in 1 minute
		m_ul64NextStartTime = 0;
		
		sprintf(strTempString,
				"[LOGFILE MOVE] Failed to compute the next start time (local error code = %d). Retry in 1 minute.",
				ucRet);
		LogTrace(strTempString, 1);
	}

	return(RET_OK);
}

/*********************************************************************/
/* Function: MoveLogFiles                                            */
/*                                                                   */
/* Parameters:                                                       */
/*		TCHAR* tstrLocalDirPath [in]:								 */
/*			Source directory path of the file to move				 */
/*																	 */
/*		TCHAR* tstrFileName [in]:									 */
/*			File name to move										 */
/*																	 */
/*		unsigned char ucFileType[in]:								 */
/*			Type of file											 */
/*          0 = Measure												 */
/*			1 = Event                                                */
/*																	 */
/* Return value:                                                     */
/*      unsigned char:                                               */
/*			0 if success, >0 if error						         */
/*                                                                   */
/* Remarks:                                                          */
/*		Moves a log file from its local directory to the current     */
/*		main server and attempts to the other server in case of      */
/*		failure. If the new main server is the number 2, calculates  */
/*		the date when it will be necessary to switch to server 1. If */
/*		the move failed to each server, suspends the treatment and   */
/*		calculates the date for the next attempt.				     */
/*********************************************************************/

unsigned char CLogFileMove::MoveLogFiles(TCHAR* tstrLocalDirPath, TCHAR* tstrFileName, unsigned char ucFileType)
{
	unsigned char i, j;
	unsigned char ucRet;
	char strTempString[200];
	char strLocalFilePath[100];
	
	TCHAR tstrLocalFilePath[100];
	TCHAR tstrServerFilePath[100];
	TCHAR* tcServerDirPath[2][2];

	tcServerDirPath[SERVER1][MEASURE] = m_tstrMeasPathServer1;
	tcServerDirPath[SERVER2][MEASURE] = m_tstrMeasPathServer2;
	tcServerDirPath[SERVER1][EVENT] = m_tstrEvtPathServer1;
	tcServerDirPath[SERVER2][EVENT] = m_tstrEvtPathServer2;
	
	// Checks if the log file path length is valid (99 char max)
	if ((lstrlen(tstrLocalDirPath) + lstrlen(tstrFileName)) >= 99)
	{
		m_ul64NextStartTime = 0;
		
		for (i = 0; i < lstrlen(tstrLocalDirPath); i++)
		{
			strLocalFilePath[i] = (char)tstrLocalDirPath[i];
		}
		strLocalFilePath[i] = '\\';
		
		for (j = i + 1; j < 99; j++)
		{
			strLocalFilePath[j] = (char)tstrFileName[j - i - 1];
		}
		strLocalFilePath[99] = '\0';
		
		sprintf(strTempString,
		"[LOGFILE MOVE] Failed to move the log file '%s' (truncated value). Local file path too long (99 char max). Retry in 1 minute.",
		strLocalFilePath);
		LogTrace(strTempString, 1);
		return(RET_ERR_LOG_FILE_PATH);
	}
	
	lstrcpy(tstrLocalFilePath, tstrLocalDirPath);
	lstrcat(tstrLocalFilePath, _T("\\"));
	lstrcat(tstrLocalFilePath, tstrFileName);
	
	for (i = 0; i <= lstrlen(tstrLocalFilePath); i++)
	{
		strLocalFilePath[i] = (char)tstrLocalFilePath[i];
	}
	
	switch(m_ucCurServer) 
	{	
		case SERVER1:
		{
			// The main server is the number 1
			lstrcpy(tstrServerFilePath, tcServerDirPath[SERVER1][ucFileType]);
			lstrcat(tstrServerFilePath, _T("\\"));
			lstrcat(tstrServerFilePath, tstrFileName);
			
			// 1st attempt to server 1
			if (CopyFile(tstrLocalFilePath, tstrServerFilePath, FALSE) != FALSE) 
			{
				if (DeleteFile(tstrLocalFilePath) == FALSE)
				{
					m_ul64NextStartTime = 0;
					
					sprintf(strTempString,
					"[LOGFILE MOVE] Failed to delete the log file '%s'. Retry in 1 minute.",
					strLocalFilePath);
					LogTrace(strTempString, 1);
					return(RET_ERR_LOG_FILE_DEL);
				}

				// File move successful
				sprintf(strTempString, "[LOGFILE MOVE] Move file '%s' to server 1 OK", strLocalFilePath);
				LogTrace(strTempString, 2);				
			}
			else
			{
				// File move failed
				sprintf(strTempString, "[LOGFILE MOVE] Move file '%s' to server 1 NOK. Attempt with server 2.", strLocalFilePath);
				LogTrace(strTempString, 2);				

				lstrcpy(tstrServerFilePath, tcServerDirPath[SERVER2][ucFileType]);
				lstrcat(tstrServerFilePath, _T("\\"));
				lstrcat(tstrServerFilePath, tstrFileName);
		
				// 2nd attempt to server 2
				if (CopyFile(tstrLocalFilePath, tstrServerFilePath, FALSE) != FALSE) 
				{
					if (DeleteFile(tstrLocalFilePath) == FALSE)
					{
						m_ul64NextStartTime = 0;
					
						sprintf(strTempString,
						"[LOGFILE MOVE] Failed to delete the log file '%s'. Retry in 1 minute.",
						strLocalFilePath);
						LogTrace(strTempString, 1);
						return(RET_ERR_LOG_FILE_DEL);
					}

					// File move successful, the new main server is the number 2
					m_ucCurServer = SERVER2;
					
					// Calculate the date when it will be necessary to switch to server 1
					ucRet = GetNextStartTime(m_ulSrvScrutingDelay, &m_ul64NextSwitchToServer1);
					if (ucRet != RET_OK)
					{
						m_ul64NextSwitchToServer1 = 0;
		
						sprintf(strTempString,
						"[LOGFILE MOVE] Failed to compute the next time to switch to server 1 (local error code = %d). Switch to server 1 at the next treatment.",
						ucRet);
						LogTrace(strTempString, 1);
					}

					sprintf(strTempString, "[LOGFILE MOVE] Move file '%s' to server 2 OK", strLocalFilePath);
					LogTrace(strTempString, 2);
				}
				else
				{
					// File move failed, treatment suspended for the configured delay
					// Calculates the date for the next attempt
					ucRet = GetNextStartTime(m_ulSrvScrutingDelay, &m_ul64NextStartTime);
					if (ucRet != RET_OK)
					{
						m_ul64NextStartTime = 0;
		
						sprintf(strTempString,
						"[LOGFILE MOVE] Failed to compute the next start time (local error code = %d). Retry in 1 minute.",
						ucRet);
						LogTrace(strTempString, 1);
					}

					sprintf(strTempString,
					"[LOGFILE MOVE] Failed to move the log file '%s' to servers. Treatment suspended for %d minutes.",
					strLocalFilePath,
					m_ulSrvScrutingDelay);
					LogTrace(strTempString, 1);
					
					return(RET_ERR_LOG_FILE_MOVE);
				}
			}
		} 
		break;

		case SERVER2:
		{
			// The main server is the number 2
			lstrcpy(tstrServerFilePath, tcServerDirPath[SERVER2][ucFileType]);
			lstrcat(tstrServerFilePath, _T("\\"));
			lstrcat(tstrServerFilePath, tstrFileName);
			
			// 1st attempt to server 2
			if (CopyFile(tstrLocalFilePath, tstrServerFilePath, FALSE) != FALSE) 
			{
				if (DeleteFile(tstrLocalFilePath) == FALSE)
				{
					m_ul64NextStartTime = 0;
					
					sprintf(strTempString,
					"[LOGFILE MOVE] Failed to delete the log file '%s'. Retry in 1 minute.",
					strLocalFilePath);
					LogTrace(strTempString, 1);
					return(RET_ERR_LOG_FILE_DEL);
				}

				// File move successful
				sprintf(strTempString, "[LOGFILE MOVE] Move file '%s' to server 2 OK", strLocalFilePath);
				LogTrace(strTempString, 2);				
			}
			else
			{
				// File move failed
				sprintf(strTempString, "[LOGFILE MOVE] Move file '%s' to server 2 NOK. Attempt with server 1.", strLocalFilePath);
				LogTrace(strTempString, 2);
				
				lstrcpy(tstrServerFilePath, tcServerDirPath[SERVER1][ucFileType]);
				lstrcat(tstrServerFilePath, _T("\\"));
				lstrcat(tstrServerFilePath, tstrFileName);
		
				// 2nd attempt to server 1
				if (CopyFile(tstrLocalFilePath, tstrServerFilePath, FALSE) != FALSE) 
				{
					if (DeleteFile(tstrLocalFilePath) == FALSE)
					{
						m_ul64NextStartTime = 0;
					
						sprintf(strTempString,
						"[LOGFILE MOVE] Failed to delete the log file '%s'. Retry in 1 minute.",
						strLocalFilePath);
						LogTrace(strTempString, 1);
						return(RET_ERR_LOG_FILE_DEL);
					}

					// File move successful, the new main server is the number 1
					m_ucCurServer = SERVER1;
					m_ul64NextSwitchToServer1 = 0;

					sprintf(strTempString, "[LOGFILE MOVE] Move file '%s' to server 1 OK", strLocalFilePath);
					LogTrace(strTempString, 2);
				}
				else
				{
					// File move failed, treatment suspended for the configured delay
					// Calculates the date for the next attempt
					ucRet = GetNextStartTime(m_ulSrvScrutingDelay, &m_ul64NextStartTime);
					if (ucRet != RET_OK)
					{
						m_ul64NextStartTime = 0;
		
						sprintf(strTempString,
						"[LOGFILE MOVE] Failed to compute the next start time (local error code = %d). Retry in 1 minute.",
						ucRet);
						LogTrace(strTempString, 1);
					}

					m_ucCurServer = SERVER1;
					
					sprintf(strTempString,
					"[LOGFILE MOVE] Failed to move the log file '%s' to servers. Treatment suspended for %d minutes.",
					strLocalFilePath,
					m_ulSrvScrutingDelay);
					LogTrace(strTempString, 1);
					
					return(RET_ERR_LOG_FILE_MOVE);
				}
			}
		} 
		break;
	}

	return(RET_OK);
}