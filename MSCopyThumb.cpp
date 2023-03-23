// MSCopyThumb.cpp : Defines the entry point for the application.
//

/*********************************************************************/
/* Project: RAMSES - CERN											 */
/* Software MSCopy on MS panel:										 */
/* - Moves measure log files (MSLog) to SUP servers					 */
/* - Moves event log files (WinCC) to SUP servers					 */
/* - Synchronizes the user account file								 */
/*																	 */
/* Author: Laurent MARTIN - ASSYSTEM								 */
/* Version: 1.00													 */
/* Date: 26/11/2014													 */
/* Panel type: MP270B												 */
/*********************************************************************/

#include "stdafx.h"
#include "Tlhelp32.h"

#include "CIniFile.h"
#include "CUsrSynchro.h"
#include "CLogFileMove.h"
#include "CStatusWrite.h"
#include "Global.h"

// Global variables
tstMSCopyStatus gmcsMSCopyStatus;
tstMSCopyCfg gmccMSCopyCfg; 

SOCKET listening_socket, client_socket;

// Function prototypes
unsigned char ReadPanelIniFile(char *strFilePath);
unsigned char CheckOtherInstances(char *strExeFileName, unsigned char *pucInstanceNb, unsigned long *ulProcessId);
unsigned char GetSleepDelayFromCurTime(ULONGLONG ul64WakeupTimeIn100ns, DWORD *pdwDelayInMs);
unsigned char GetSleepDelayFromVarTime(SYSTEMTIME stimeGMTCurTime, ULONGLONG ul64WakeupTimeIn100ns, DWORD *pdwDelayInMs);
unsigned char GetCurrentTimeIn100ns(ULONGLONG *ul64GMTCurTime);
bool ShutdownIsRequested(char *strStopCmdFilePath);

// Function definition

/*********************************************************************/
/* Function: WinMain			                                     */
/*                                                                   */
/* Parameters:                                                       */
/*		HINSTANCE hInstance [in]:                                    */
/*			Handle to the current instance of the application        */
/*                                                                   */
/*		HINSTANCE hPrevInstance [in]:                                */
/*			Handle to the previous instance of the application       */
/*                                                                   */
/*		LPSTR lpCmdLine [in]:										 */
/*			The command line for the application, excluding the		 */
/*			program name 											 */
/*                                                                   */
/*		int nCmdShow [in]:											 */
/*			Controls how the window is to be shown					 */
/*																	 */
/* Return value:                                                     */
/*      int:		                                                 */
/*			If the function succeeds, terminating when it receives a */
/*			WM_QUIT message, it should return the exit value		 */
/*			contained in that message's wParam parameter. If the	 */
/*			function terminates before entering the message loop, it */
/*			should return zero.										 */
/*                                                                   */
/* Remarks:                                                          */
/*		Application entry point										 */
/*********************************************************************/

int WINAPI WinMain(	HINSTANCE hInstance,
					HINSTANCE hPrevInstance,
					LPTSTR    lpCmdLine,
					int       nCmdShow)
{
	long lPathLength, i;
	int iBackslashSignPosition;
	unsigned char ucRetValue;
	unsigned char ucInstanceNb = 0;
	bool bMSCopyStatusModified;
	bool bTreatmentDatesAreValid;

	HANDLE hMSCopyProcess;
	DWORD dwProcessExitCode;
	DWORD dwSleepDelay;
	DWORD dwSleepDelayLFM;
	DWORD dwSleepDelaySW;
	DWORD dwSleepDelayUS;
	ULONGLONG ul64GMTFirstStartTime;
	ULONGLONG ul64GMTCurTime;
	SYSTEMTIME stimeGMTDateTime;
	FILETIME ftimeGMTDateTime;
	
	CUsrSynchro *pcusUsrSynchro;
	CLogFileMove *pclfmLogFileMove;
	CStatusWrite *pswStatusWrite;
	tstMSCopyStatus mcsPrevMSCopyStatus;

	char strIniFilePath[100];
	char strStopCmdFilePath[100];
	char strTempString[200];
	char strTempString2[200];
	char strExeFilePath[100];
	char strExeFileName[100];
	unsigned long ulMSCopyProcessId[10];
	WCHAR wstrTempString[100];

	// Initialization of the MSCopy trace parameters
	gmccMSCopyCfg.bTraceMode = true;
	gmccMSCopyCfg.lTraceLevel = 2;
	gmccMSCopyCfg.lTraceFileMaxSize = 1024;
	strcpy(gmccMSCopyCfg.strTraceDirPath, "\\Storage Card2\\MSCopy");
		
	// Initialization of the MSCopy process IDs
	for (i = 0; i < 10; i++)
	{
		ulMSCopyProcessId[i] = 0;
	}
	
	// Recovery of the MSCopy executable file path
	lPathLength = GetModuleFileName(NULL, wstrTempString, 100);
	
	// Type conversion WCHAR to char
	for (i = 0; i < lPathLength; i++)
	{
		strExeFilePath[i] = (char)wstrTempString[i];
	}
	strExeFilePath[i] = '\0';

	// Recovery of the MSCopy executable file name
	for (i = strlen(strExeFilePath) - 1; i >= 0; i--)
	{
		if (strExeFilePath[i] == '\\')
		{
			break;
		}
	}
	strcpy(strExeFileName, &(strExeFilePath[i + 1]));

	// Building of the "MSPANEL.INI" file path from the "MSCopy.exe" path.
	// The file "MSPANEL.INI" is located in the flash card root directory.
	// => "<Flash card dir>\MSPanel.ini"
	strcpy(strIniFilePath, strExeFilePath);
	iBackslashSignPosition = strcspn(&(strIniFilePath[1]), "\\");
	strncpy(strTempString, strIniFilePath, iBackslashSignPosition + 1);
	strTempString[iBackslashSignPosition + 1] = '\0';
	strcat(strTempString, "\\MSPANEL.INI");
	strcpy(strIniFilePath, strTempString);

	// Reading of the file "MSPANEL.INI"
	ucRetValue = ReadPanelIniFile(strIniFilePath);
	
	if (ucRetValue != RET_OK)
	{
		sprintf(strTempString, "[MAIN] Reading of the file '%s' failed (local error code = %d)", strIniFilePath, ucRetValue);
		LogTrace(strTempString, 1);
		return(0);
	}

	// MS is started with reading of "MSPANEL.INI" successful (the trace
	// file path is initialized)
	LogTrace("[MAIN] MSCopy started", 1);

	// Building of the "STOP.CMD" file path located in the same directory 
	// as "MSCopy.exe"
	for (i = strlen(strExeFilePath) - 1; i >= 0; i--)
	{
		if (strExeFilePath[i] == '\\')
		{
			strncpy(strStopCmdFilePath, strExeFilePath, i + 1);
			strStopCmdFilePath[i + 1] = '\0';
			break;
		}
	}
	strcat(strStopCmdFilePath, "STOP.CMD");

	// Reading of the WinMain command arguments

	// Type conversion WCHAR to char
	i = 0;
	while ((lpCmdLine[i] != 0) && (i < 99))
	{
		strTempString[i] = (char)lpCmdLine[i];
		i++;
	}
	strTempString[i] = '\0';

	// Extracts the first argument
	sscanf(strTempString, "%s", strTempString2);
	
	if (strcmp(strTempString2, "kill") == 0)
	{		
		// The "stop" argument requests to kill all running instances of the 
		// MSCopy process
		
		// Checks if another instance of MSCopy is running
		ucRetValue = CheckOtherInstances(strExeFileName, &ucInstanceNb, ulMSCopyProcessId);
		
		if (ucRetValue == RET_OK)
		{			
			// Terminates each running instance
			for (i = 0; i < ucInstanceNb; i++)
			{				
				// Recovers the process handle
				hMSCopyProcess = OpenProcess(0, FALSE, ulMSCopyProcessId[i]);
							
				if (hMSCopyProcess != NULL)
				{					
					// Recovers the process exit code
					if (GetExitCodeProcess(hMSCopyProcess, &dwProcessExitCode) == FALSE)
					{
						sprintf(strTempString, "[MAIN] Stopping of the MSCopy instance no %d failed (impossible to get the exit code of the process handle 0x%08X)", i + 1, hMSCopyProcess);
						LogTrace(strTempString, 1);
					}
					else
					{
						// Terminates the process
						if (TerminateProcess(hMSCopyProcess, dwProcessExitCode) == FALSE)
						{
							sprintf(strTempString, "[MAIN] Stopping of the MSCopy instance no %d failed (process handle 0x%08X)", i + 1, hMSCopyProcess);
							LogTrace(strTempString, 1);
						}
						else
						{
							sprintf(strTempString, "[MAIN] MSCopy instance no %d terminated (process handle 0x%08X)", i + 1, hMSCopyProcess);
							LogTrace(strTempString, 1);
						}
					}
				}
			}
		}
		else
		{
			sprintf(strTempString, "[MAIN] Checking of other MSCopy instances failed (local error code = %d)", ucRetValue);
			LogTrace(strTempString, 1);
		}

		// MSCopy shutdown
		LogTrace("[MAIN] MSCopy shutdown", 1);
		return(0);
	}
		
	// Checks if another instance of MSCopy is running
	ucRetValue = CheckOtherInstances(strExeFileName, &ucInstanceNb, ulMSCopyProcessId);
	
	if (ucRetValue == RET_OK)
	{		
		if (ucInstanceNb >= 1)
		{
			// Another instance of MSCopy is running => log trace and shutdown
			sprintf(strTempString, "[MAIN] Another instance of MSCopy is already running");
			LogTrace(strTempString, 1);
			LogTrace("[MAIN] MSCopy shutdown", 1);
			return(0);
		}
	}
	else
	{
		sprintf(strTempString, "[MAIN] Checking of another MSCopy instance failed (local error code = %d)", ucRetValue);
		LogTrace(strTempString, 1);
	}
	
	// Checks if shutdown is requested before sleep time
	if (ShutdownIsRequested(strStopCmdFilePath) == true)
	{
		LogTrace("[MAIN] MSCopy shutdown", 1);
		return(0);
	}
	
	WSADATA wsaData; //Filled by call to WSAStartup


	int wsaret = WSAStartup(0x101, &wsaData);

	if(wsaret!=0) {
		//printf("Error in call to WSAStartup, error code: %d.\nExiting in 3 seconds\n", wsaret);
		//Sleep(3000);
		return -1;
	}

	sockaddr_in local_address; //Specifies address of socket

	local_address.sin_family = AF_INET; //Address family
	local_address.sin_addr.s_addr = INADDR_ANY; // Accept connection from any IP
	local_address.sin_port = htons((u_short)20248); //Port to use

	listening_socket = socket(AF_INET, SOCK_STREAM, 0);

	if( listening_socket == INVALID_SOCKET ) {
		//printf("Error in creating listening socket. Exiting in 2 seconds\n");
		//printf("Error code is %d\nExiting in 2 seconds\n", GetLastError());
		//Sleep(2000);
        return -2;
	}

	if( bind(listening_socket, (sockaddr*)&local_address, sizeof(local_address)) !=0 ) {
		//printf("\nError in binding the socket to the port 20248.\n");
		//printf("Error code is %d\nExiting in 2 seconds\n", GetLastError());
		//Sleep(2000);
        return -3;
	}

	u_long iMode = 1;

	int iResult = ioctlsocket(listening_socket, FIONBIO, &iMode); //Third parameter != 0 means non blocking

	if(iResult != NO_ERROR) {
		//printf("Error in setting listening socket as non blocking\n");
		//printf("Error code is %d\nExiting in 2 seconds\n", GetLastError());
		//Sleep(2000);
        return -4;
	}

    //listen instructs the socket to listen for incoming 
    //connections from clients. The second arg is the backlog
    if(listen(listening_socket, 10)!=0)
    {
		//printf("Error in using the socket for listening. Exiting in 2 seconds\n");
		//printf("Error code is %d\nExiting in 2 seconds\n", GetLastError());
		//Sleep(2000);
        return -5;
    }

	// Initialization of the MSCopy status structure
	gmcsMSCopyStatus.bLogFileMoveError = false;
	gmcsMSCopyStatus.bLogFileMoveRecovery = false;
	gmcsMSCopyStatus.bUsrSynchroError = false;

	strcpy(gmcsMSCopyStatus.strLogFileServerUsed, "No server");
	gmcsMSCopyStatus.fMSCopyVersion = MSCOPY_VERSION;

	// Initialization of the 3 main cylic treatments (object creation)
	// -> User file synchronisation
	pcusUsrSynchro = new CUsrSynchro(&gmccMSCopyCfg);
	// -> Moving of log files to server
	pclfmLogFileMove = new CLogFileMove(&gmccMSCopyCfg);
	// -> Writing of the status file
	pswStatusWrite = new CStatusWrite(&gmccMSCopyCfg);


	//Wait upto 5 seconds to establish a connection

	fd_set rfds, wfds;

	struct timeval tv;

	FD_ZERO(&rfds);
	FD_SET(listening_socket, &rfds);

	FD_ZERO(&wfds);
	FD_SET(listening_socket, &wfds);

	tv.tv_sec = 5;
	tv.tv_usec = 0;

	//printf("Waiting upto 5 seconds to receive a connection in the listening queue\n");

	iResult = select(listening_socket + 1, &rfds, &wfds, NULL, &tv);
	
	client_socket = INVALID_SOCKET;
	sockaddr_in client_addr;
	int client_addr_len = sizeof(client_addr);

	if(iResult == 1 && FD_ISSET(listening_socket, &rfds) ) {

		//Socket is readable => there is atleast one complete connection in queue

		client_socket = accept(listening_socket, (struct sockaddr*) &client_addr, &client_addr_len);

		if(client_socket == INVALID_SOCKET) {
			//printf("Error in accepting the client connection\n");
		} else {
			//printf("Connected successfully to a client\n");

			int iResult = ioctlsocket(client_socket, FIONBIO, &iMode); //Third parameter != 0 means non blocking

			if(iResult != NO_ERROR) {
				//printf("Error in setting client socket as non blocking\n");
				//printf("Error code is %d\nExiting in 2 seconds\n", GetLastError());
				//Sleep(2000);
				return -4;
			}

			//printf("Succesfully set client socket as non blocking\n");
		}
	}
	
	// Waits for the next passage at 15s 0ms
	ucRetValue = GetSleepDelayFromCurTime(0, &dwSleepDelay);
	
	if (ucRetValue == RET_OK)
	{
		sprintf(strTempString, "[MAIN] Wait for the next passage at 15s. Sleep delay = %d ms.", dwSleepDelay + 1000);
		LogTrace(strTempString, 2);

		//printf("[MAIN] Wait for the next passage at 15s. Sleep delay = %d ms.\n", dwSleepDelay + 1000);

		// Sleeps during the calculated time + 1s to take in account the lack
		// of precision of the Windows CE timers
		
		//TODO: UNCOMMENT THIS LINE ********************************
		Sleep(dwSleepDelay + 1000);

		// After wakeup, in case of panel time synchronisation, checks if the 
		// current time is between 15 and 30s. If not, waits for the next
		// passage at 15s one time.
		GetSystemTime(&stimeGMTDateTime);

		//printf("Woke up at stime second: %d\n",stimeGMTDateTime.wSecond);
			//TODO - UNCOMMENT THE FOLLOWING LINES ************************9
		
		if (!((stimeGMTDateTime.wSecond >= 15) && (stimeGMTDateTime.wSecond <= 30)))
		{
			// Exception: the status file writing is mandatory for the first
			// MSLog reading
			pswStatusWrite->ExecuteTreatment(&gmcsMSCopyStatus);
			
			// Calculates and applies the next sleep time
			LogTrace("[MAIN] The wakeup time is not between 15s and 30s. Waits for the next passage at 15s", 1);
			//printf("[MAIN] The wakeup time is not between 15s and 30s. Waits for the next passage at 15s\n");
			ucRetValue = GetSleepDelayFromCurTime(0, &dwSleepDelay);
			
			if (ucRetValue == RET_OK)
			{
				sprintf(strTempString, "[MAIN] Sleep delay = %d ms", dwSleepDelay + 1000);
				LogTrace(strTempString, 2);

				// Sleeps during the calculated time + 1s to take in account the 
				// lack of precision of the Windows CE timers
				Sleep(dwSleepDelay + 1000);
			}
			else
			{
				sprintf(strTempString, 
						"[MAIN] Failed to calculate the sleep delay to wake up at the next 15s 0ms (local error code = %d). Treatment continued.",
						ucRetValue);
				LogTrace(strTempString, 1);
			}
		}
	}
	else
	{
		sprintf(strTempString, 
				"[MAIN] Failed to calculate the first sleep delay to wake up at the next 15s 0ms (local error code = %d). Value applied by default is 1min.",
				ucRetValue);
		LogTrace(strTempString, 1);

		Sleep(60000);
	}

	// First execution of the 3 main cyclic treatments
	// -> User file synchronisation
	//TODO: UNCOMMMENT THE FOLLOWING LINE!
	pcusUsrSynchro->ExecuteTreatment(&gmcsMSCopyStatus);
	// -> Moving of log files to server
	pclfmLogFileMove->ExecuteTreatment(&gmcsMSCopyStatus);	
	// -> Writing of the status file
	pswStatusWrite->ExecuteTreatment(&gmcsMSCopyStatus);
	
	// Checks if shutdown is requested before each sleep time
	while(ShutdownIsRequested(strStopCmdFilePath) == false)
	{
		// No shutdown is requested

		// Reads the current system time in GMT format
		GetSystemTime(&stimeGMTDateTime);

		// Checks if the 3 treatment start dates are valid 
		// (<=> waiting delay < time period configured in "MSPANEL.INI" + 1min)
		bTreatmentDatesAreValid = true;
		
		ucRetValue = GetSleepDelayFromVarTime(stimeGMTDateTime, pclfmLogFileMove->m_ul64NextStartTime, &dwSleepDelayLFM);
		ucRetValue |= GetSleepDelayFromVarTime(stimeGMTDateTime, pcusUsrSynchro->m_ul64NextStartTime, &dwSleepDelayUS);
		ucRetValue |= GetSleepDelayFromVarTime(stimeGMTDateTime, pswStatusWrite->m_ul64NextStartTime, &dwSleepDelaySW);

		if (ucRetValue == RET_OK) 
		{
			if (((dwSleepDelayLFM > (DWORD)((gmccMSCopyCfg.lMovePeriod + 1) * MINUTE_MS)) && (dwSleepDelayLFM > (DWORD)((gmccMSCopyCfg.lSrvScrutingDelay + 1) * MINUTE_MS)))
				|| (dwSleepDelayUS > (DWORD)((gmccMSCopyCfg.lUsrSynchroPeriod + 1) * MINUTE_MS))
				|| (dwSleepDelaySW > (DWORD)((gmccMSCopyCfg.lStatusWritePeriod + 1) * MINUTE_MS)))
			{
				bTreatmentDatesAreValid = false;
			}

			if (bTreatmentDatesAreValid == true)
			{
				// Looks for the earliest date among the 3 next treatment start dates		
				if (dwSleepDelayLFM < dwSleepDelayUS)
				{
					if (dwSleepDelayLFM < dwSleepDelaySW)
					{
						dwSleepDelay = dwSleepDelayLFM;
						ul64GMTFirstStartTime = pclfmLogFileMove->m_ul64NextStartTime;
					}
					else
					{
						dwSleepDelay = dwSleepDelaySW;
						ul64GMTFirstStartTime = pswStatusWrite->m_ul64NextStartTime;
					}	
				}
				else
				{
					if (dwSleepDelayUS < dwSleepDelaySW)
					{
						dwSleepDelay = dwSleepDelayUS;
						ul64GMTFirstStartTime = pcusUsrSynchro->m_ul64NextStartTime;
					}
					else
					{
						dwSleepDelay = dwSleepDelaySW;
						ul64GMTFirstStartTime = pswStatusWrite->m_ul64NextStartTime;
					}
				}
			}
			else
			{
				ucRetValue = GetSleepDelayFromCurTime(0, &dwSleepDelay);
			}
		}
		
		// Waits for the earliest treatment start date
		if (ucRetValue == RET_OK)
		{
			if (gmccMSCopyCfg.bTraceMode == true)
			{
				if (bTreatmentDatesAreValid == true)
				{
					ftimeGMTDateTime.dwLowDateTime = (DWORD)(ul64GMTFirstStartTime);
					ftimeGMTDateTime.dwHighDateTime = (DWORD)(ul64GMTFirstStartTime >> 32);
					FileTimeToSystemTime(&ftimeGMTDateTime, &stimeGMTDateTime);
					sprintf(strTempString, 
							"[MAIN] Wait for the next treatment. Start date = %02u/%02u/%02u %02u:%02u:%02u (GMT), sleep delay = %d ms.",
							stimeGMTDateTime.wDay,
							stimeGMTDateTime.wMonth,
							stimeGMTDateTime.wYear,
							stimeGMTDateTime.wHour,
							stimeGMTDateTime.wMinute,
							stimeGMTDateTime.wSecond,
							dwSleepDelay + 1000);
					LogTrace(strTempString, 2);
				}
				else
				{
					sprintf(strTempString, "[MAIN] Invalid treatement start date detected. Wait for the next passage at 15s to start the 3 treatments. Sleep delay = %d ms.", dwSleepDelay);
					LogTrace(strTempString, 1);
				}
			}
	
			// Sleeps during the calculated time + 1s to take in account the lack
			// of precision of the Windows CE timers
			Sleep(dwSleepDelay + 1000);

			// After wakeup, in case of panel time synchronisation, checks if the 
			// current time is between 15 and 30s. If not, waits for the next
			// passage at 15s one time.
			GetSystemTime(&stimeGMTDateTime);

			if (!((stimeGMTDateTime.wSecond >= 15) && (stimeGMTDateTime.wSecond <= 30)))
			{
				// Exception: in STARTING mode, the status file is written at least
				// once per minute systematically for the first MSLog reading
				if (pswStatusWrite->m_ucOperatingMode == STARTING)
				{
					pswStatusWrite->ExecuteTreatment(&gmcsMSCopyStatus);
				}

				// Calculate and apply the next sleep time
				LogTrace("[MAIN] The wakeup time is not between 15s and 30s. Waits for the next passage at 15s", 1);
				
				ucRetValue = GetSleepDelayFromCurTime(0, &dwSleepDelay);
								
				if (ucRetValue == RET_OK)
				{
					sprintf(strTempString, "[MAIN] Wait for the next passage at 15s. Sleep delay = %d ms.", dwSleepDelay + 1000);
					LogTrace(strTempString, 2);
					
					// Sleeps during the calculated time + 1s to take in account the 
					// lack of precision of the Windows CE timers
					Sleep(dwSleepDelay + 1000);
				}
				else
				{
					sprintf(strTempString, 
							"[MAIN] Failed to calculate the sleep delay to wake up at the next 15s 0ms (local error code = %d). Treatment continued.",
							ucRetValue);
					LogTrace(strTempString, 1);
				}
			}
		}	
		else
		{
			sprintf(strTempString, 
					"[MAIN] Failed to calculate the next sleep delay (local error code = %d). Value applied by default is 1min.",
					ucRetValue);
			LogTrace(strTempString, 1);

			Sleep(60000);
		}
		
		// Stores the initial MSCopy status before starting some cyclic treatment(s)
		mcsPrevMSCopyStatus.bLogFileMoveError = gmcsMSCopyStatus.bLogFileMoveError;
		mcsPrevMSCopyStatus.bLogFileMoveRecovery = gmcsMSCopyStatus.bLogFileMoveRecovery;
		mcsPrevMSCopyStatus.bUsrSynchroError = gmcsMSCopyStatus.bUsrSynchroError;
		strcpy(mcsPrevMSCopyStatus.strLogFileServerUsed, gmcsMSCopyStatus.strLogFileServerUsed);
		
		// Checks the validity of the 3 treatment start dates. If at least one date
		// is not valid, the 3 treatments are executed to update their start date.
		if (bTreatmentDatesAreValid == false)
		{
			// Execution of the 3 main cyclic treatments
			// -> Moving of log files to server
			pclfmLogFileMove->ExecuteTreatment(&gmcsMSCopyStatus);	
			// -> User file synchronisation
			pcusUsrSynchro->ExecuteTreatment(&gmcsMSCopyStatus);
			// -> Writing of the status file
			pswStatusWrite->ExecuteTreatment(&gmcsMSCopyStatus);
		}
		else
		{
			// Execution of the treatments whose start date is reached
		
			// Recovers the current UTC time
			ucRetValue = GetCurrentTimeIn100ns(&ul64GMTCurTime);

			if (ucRetValue != RET_OK)
			{
				sprintf(strTempString, 
						"[MAIN] Failed to get the current system time in 100ns (local error code = %d). Max possible value applied by default.",
						ucRetValue);
				LogTrace(strTempString, 1);
				
				ul64GMTCurTime = 0xFFFFFFFFFFFFFFFF;
			}

			// Checks if the current time reaches the next start time of each treatment according to the following 
			// priority:
			// 1. Log file move
			// 2. User profile synchronisation
			// 3. MSCopy status writing

			// 1. Log file move		
			if (pclfmLogFileMove->m_ul64NextStartTime <= ul64GMTCurTime)
			{
				pclfmLogFileMove->ExecuteTreatment(&gmcsMSCopyStatus);
			}

			// 2. User profile synchronisation
			if (pcusUsrSynchro->m_ul64NextStartTime <= ul64GMTCurTime)
			{		
				pcusUsrSynchro->ExecuteTreatment(&gmcsMSCopyStatus);
			}

			// 3. MSCopy status writing
			// -> Checks if at least one status parameters has been changed since the beginning of the wake up 
			//    period comparing the current values with the initial stored ones.
			if ((mcsPrevMSCopyStatus.bLogFileMoveError != gmcsMSCopyStatus.bLogFileMoveError)
				|| (mcsPrevMSCopyStatus.bLogFileMoveRecovery != gmcsMSCopyStatus.bLogFileMoveRecovery)
				|| (mcsPrevMSCopyStatus.bUsrSynchroError != gmcsMSCopyStatus.bUsrSynchroError)
				|| strcmp(mcsPrevMSCopyStatus.strLogFileServerUsed, gmcsMSCopyStatus.strLogFileServerUsed) != 0)
			{
				bMSCopyStatusModified = true;
			}
			else
			{
				bMSCopyStatusModified = false;
			}

			// -> If at least one parameter has been changed, or if the next start time is reached, the Status
			//	  Write treatment is started
			if ((pswStatusWrite->m_ul64NextStartTime <= ul64GMTCurTime) || (bMSCopyStatusModified == true))
			{			
				pswStatusWrite->ExecuteTreatment(&gmcsMSCopyStatus);
			}
		}
	}
	
	delete pcusUsrSynchro;
	delete pclfmLogFileMove;
	delete pswStatusWrite;

	// MSCopy shutdown
	LogTrace("[MAIN] MSCopy shutdown", 1);
	return(0);
}

int SelectReadUptoNSeconds(SOCKET socket, int Nseconds) {

		fd_set rfds;
		struct timeval tv;

		FD_ZERO(&rfds);
		FD_SET(socket, &rfds);
		
		tv.tv_sec = Nseconds;
		tv.tv_usec = 0;

		int iRetSelect = select(socket + 1, &rfds, NULL, NULL, &tv);

		return iRetSelect;
}

int Socket_Handshake() {

	int iRetSelect;
	int iRetRecv;
	int iRetSend;
	char temp[1024];

	//Handshake Procedure

	int rand_sent = rand() % 100;
	
	sprintf(temp, "%d", rand_sent);
	
	//Send random number
	iRetSend = send(client_socket, temp, strlen(temp), 0);

	if(iRetSend < 0) {
		//Error in sending to the driver
		//Close the connection.

		//printf("Error in sending random number for handshake\n");
		closesocket(client_socket);
		client_socket = INVALID_SOCKET;
		return -1;
	}

	//printf("Sent random number %d\n", rand_sent);

	//Wait upto 5 seconds to receive random_number + 1;
	iRetSelect = SelectReadUptoNSeconds(client_socket, 5);

	if(iRetSelect <= 0) {
		//Did not receive a reply from the driver
		//Close the connection.
		//printf("Did not receive rand+1 within 5 seconds or error in select\n");
		closesocket(client_socket);
		client_socket = INVALID_SOCKET;
		return -1;
	}
	
	memset(temp, 0, sizeof(temp));
	iRetRecv = recv(client_socket, temp, 3, 0);

	if(iRetRecv <= 0) {
		//Did not receive a reply from the driver
		//Close the connection.
		//printf("recv: Error in receiving random number + 1 \n");
		closesocket(client_socket);
		client_socket = INVALID_SOCKET;
		return -1;
	}
	
	//printf("Succesfully received %s\n", temp);

	if(atoi(temp) != rand_sent + 1) {
		//Received an invalid response. 
		//Close the connection.
		//printf("recv: Received number is not equal to random number + 1 \n");
		closesocket(client_socket);
		client_socket = INVALID_SOCKET;
		return -1;
	}

	//printf("Success: Handshake completed\n");

	//Handshake completed successfully
	return 0;
}


/*********************************************************************/
/* Function: ReadPanelIniFile                                        */
/*                                                                   */
/* Parameters:                                                       */
/*		char* strFilePath [in]:                                      */
/*			Path of the panel INI file to read                       */
/*                                                                   */
/* Return value:                                                     */
/*      unsigned char:                                               */
/*			0 if success, >0 if error						         */
/*                                                                   */
/* Remarks:                                                          */
/*		Read all parameters in the panel INI file useful for MSCopy  */
/*********************************************************************/

unsigned char ReadPanelIniFile(char *strFilePath)
{
	unsigned char ucErrorCode = 0;
	long lReadValue;

	CIniFile *pcifIniFile;

	pcifIniFile = new CIniFile(strFilePath);

	// Log files
	ucErrorCode |= pcifIniFile->ReadLongValue("[LogFile]", "SuccessiveMove", &(gmccMSCopyCfg.lSuccessiveMove));
	ucErrorCode |= pcifIniFile->ReadLongValue("[LogFile]", "MovePeriodInMin", &(gmccMSCopyCfg.lMovePeriod));
	ucErrorCode |= pcifIniFile->ReadStringValue("[LogFile]", "ServerName1", gmccMSCopyCfg.strServerName1);
	ucErrorCode |= pcifIniFile->ReadStringValue("[LogFile]", "ServerName2", gmccMSCopyCfg.strServerName2);	
	ucErrorCode |= pcifIniFile->ReadStringValue("[LogFile]", "MeasLocalDirPath", gmccMSCopyCfg.strMeasPathLocal);
	ucErrorCode |= pcifIniFile->ReadStringValue("[LogFile]", "MeasServerDirPath", gmccMSCopyCfg.strMeasPathServer);	
	ucErrorCode |= pcifIniFile->ReadStringValue("[LogFile]", "EvtLocalDirPath", gmccMSCopyCfg.strEvtPathLocal);
	ucErrorCode |= pcifIniFile->ReadStringValue("[LogFile]", "EvtServerDirPath", gmccMSCopyCfg.strEvtPathServer);	
	ucErrorCode |= pcifIniFile->ReadLongValue("[LogFile]", "ServerScrutingDelayInMin", &(gmccMSCopyCfg.lSrvScrutingDelay));
	
	// User profiles
	ucErrorCode |= pcifIniFile->ReadStringValue("[UserProfile]", "LocalFilePath", gmccMSCopyCfg.strUsrPathLocal);	
	ucErrorCode |= pcifIniFile->ReadStringValue("[UserProfile]", "RefFilePath", gmccMSCopyCfg.strUsrPathServer);	
	ucErrorCode |= pcifIniFile->ReadLongValue("[UserProfile]", "SynchroPeriodInMin", &(gmccMSCopyCfg.lUsrSynchroPeriod));
	
	// Panel startup
	ucErrorCode |= pcifIniFile->ReadStringValue("[PanelStartup]", "FilePath", gmccMSCopyCfg.strPanelStartupFilePath);	

	// MSCopy status file
	ucErrorCode |= pcifIniFile->ReadStringValue("[MSCopyStatusFile]", "FilePath", gmccMSCopyCfg.strStatusFilePath);	
	ucErrorCode |= pcifIniFile->ReadLongValue("[MSCopyStatusFile]", "WritePeriodInMin", &(gmccMSCopyCfg.lStatusWritePeriod));
	
	// Debug mode
	ucErrorCode |= pcifIniFile->ReadLongValue("[MSCopyDebug]", "TraceLevel", &(gmccMSCopyCfg.lTraceLevel));
	ucErrorCode |= pcifIniFile->ReadStringValue("[MSCopyDebug]", "DirPath", gmccMSCopyCfg.strTraceDirPath);
	ucErrorCode |= pcifIniFile->ReadLongValue("[MSCopyDebug]", "MaxFileSizeInKb", &(gmccMSCopyCfg.lTraceFileMaxSize));
	ucErrorCode |= pcifIniFile->ReadLongValue("[MSCopyDebug]", "TraceMode", &lReadValue);
	
	// Update the trace mode value only if no error occured, in order to force the report of any problem 
	// about the reading of INI parameters in the trace file
	if (ucErrorCode == RET_OK)
	{
		if (lReadValue == 1)
		{
			gmccMSCopyCfg.bTraceMode = true;
		}
		else
		{
			gmccMSCopyCfg.bTraceMode = false;
		}
	}

	delete pcifIniFile;

	return(ucErrorCode);
}

/*********************************************************************/
/* Function: LogTrace                                                */
/*                                                                   */
/* Parameters:                                                       */
/*		char* strMessage [in]:                                       */
/*			Message to add in the MSCopy log file                    */
/*																	 */
/*		unsigned char ucTraceLevel[in]:                              */
/*			Level of details associated with the message content     */
/*			1 = Errors and important information					 */
/*			2 = All details											 */
/*                                                                   */
/* Return value:                                                     */
/*      unsigned char:                                               */
/*			0 if success, >0 if error						         */
/*                                                                   */
/* Remarks:                                                          */
/*		Write the user message in the MSCopy log file in accordance  */
/*		with the defined trace mode and level values                 */
/*********************************************************************/

unsigned char LogTrace(char *strMessage, unsigned char ucTraceLevel)
{
	HANDLE hSearchFile;
	WIN32_FIND_DATA wfdFileData;
	
	ULONGLONG ul64LastFileWriteTime_0, ul64LastFileWriteTime_1;
	ULONGLONG ul64FileSize_0, ul64FileSize_1;
	SYSTEMTIME stimeGMTDateTime;
	FILE *pfiTraceFile;
	
	char strTraceFilePath_0[100];
	char strTraceFilePath_1[100];
	TCHAR tstrTraceFilePath_0[100];
	TCHAR tstrTraceFilePath_1[100];

	// Checks if the trace mode is enabled
	if (gmccMSCopyCfg.bTraceMode == true)
	{
		// Checks if the selected trace level is filtered (the value
		// increases with more details)
		if (ucTraceLevel <= (unsigned char)gmccMSCopyCfg.lTraceLevel)
		{
			// Reads the current system time in GMT format
			GetSystemTime(&stimeGMTDateTime);
			
			// Searches the exiting trace files and stores the last write time and size attributes
			strcpy(strTraceFilePath_0, gmccMSCopyCfg.strTraceDirPath);
			strcat(strTraceFilePath_0, "\\Trace0.txt");
			strcpy(strTraceFilePath_1, gmccMSCopyCfg.strTraceDirPath);
			strcat(strTraceFilePath_1, "\\Trace1.txt");

			wsprintf(tstrTraceFilePath_0, _T("%S"), strTraceFilePath_0);
			wsprintf(tstrTraceFilePath_1, _T("%S"), strTraceFilePath_1);

			ul64LastFileWriteTime_0 = 0;
			ul64LastFileWriteTime_1 = 0;
			ul64FileSize_0 = 0;
			ul64FileSize_1 = 0;

			hSearchFile = FindFirstFile(tstrTraceFilePath_0, &wfdFileData);
			if (hSearchFile != INVALID_HANDLE_VALUE)
			{
				ul64LastFileWriteTime_0 = (((ULONGLONG)(wfdFileData.ftLastWriteTime.dwHighDateTime)) << 32) + wfdFileData.ftLastWriteTime.dwLowDateTime;
				ul64FileSize_0 = (((ULONGLONG)(wfdFileData.nFileSizeHigh)) << 32) + wfdFileData.nFileSizeLow;
				FindClose(hSearchFile);
			}

			hSearchFile = FindFirstFile(tstrTraceFilePath_1, &wfdFileData);
			if (hSearchFile != INVALID_HANDLE_VALUE)
			{
				ul64LastFileWriteTime_1 = (((ULONGLONG)(wfdFileData.ftLastWriteTime.dwHighDateTime)) << 32) + wfdFileData.ftLastWriteTime.dwLowDateTime;
				ul64FileSize_1 = (((ULONGLONG)(wfdFileData.nFileSizeHigh)) << 32) + wfdFileData.nFileSizeLow;
				FindClose(hSearchFile);
			}
			
			// Compares the size and the last write time attributes of the 2 files "Trace0.txt" and
			// "Trace1.txt". The most recent file is written first if its size doesn't exceed
			// the max size configured. However the other file is selected.
			if (ul64LastFileWriteTime_0 >= ul64LastFileWriteTime_1)
			{
				if (ul64FileSize_0 < (gmccMSCopyCfg.lTraceFileMaxSize * 1024))
				{
					// File "Trace0.txt" opening in "append" mode
					pfiTraceFile = fopen(strTraceFilePath_0, "a+");
				}
				else
				{
					// File "Trace1.txt" opening in "write" mode (destroyed if existing)
					pfiTraceFile = fopen(strTraceFilePath_1, "w+");
				}
			}
			else
			{
				if (ul64FileSize_1 < (gmccMSCopyCfg.lTraceFileMaxSize * 1024))
				{
					// File "Trace1.txt" opening in "append" mode
					pfiTraceFile = fopen(strTraceFilePath_1, "a+");
				}
				else
				{
					// File "Trace0.txt" opening in "write" mode (destroyed if existing)
					pfiTraceFile = fopen(strTraceFilePath_0, "w+");
				}
			}

			if (pfiTraceFile != NULL)
			{
				// Writing of the message preceded by the current system time
				if (fprintf(pfiTraceFile, 
							"%02u/%02u/%02u %02u:%02u:%02u (GMT) - %s\n", 
							stimeGMTDateTime.wDay, 
							stimeGMTDateTime.wMonth, 
							stimeGMTDateTime.wYear, 
							stimeGMTDateTime.wHour,
							stimeGMTDateTime.wMinute, 
							stimeGMTDateTime.wSecond, 
							strMessage) > 0)
				{
					fclose(pfiTraceFile);
				}
				else
				{
					fclose(pfiTraceFile);
					return(RET_ERR_FILE_WRITING);
				}
			}
			else
			{
				return(RET_ERR_FILE_OPENING);
			}
		}
	}

	return(RET_OK);
}

/*********************************************************************/
/* Function: CheckOtherInstances                                     */
/*                                                                   */
/* Parameters:                                                       */
/*																	 */
/*		char* strExeFileName [in]:									 */
/*			Name of the MSCopy executable file						 */
/*																	 */
/*		unsigned char* pucInstanceNb [out]:                          */
/*			Number of the other MSCopy running instance(s)		     */
/*																	 */
/*		unsigned long* ulProcessId [out]:							 */
/*			List of the process ID(s) associated with each MSCopy    */
/*			instance												 */
/*																	 */
/* Return value:                                                     */
/*      unsigned char:                                               */
/*			0 if success, >0 if error						         */
/*                                                                   */
/* Remarks:                                                          */
/*		Counts the number of process running instance(s) associated  */
/*		with "MSCopy.exe" other than the currrent process, and gives */
/*		the associated process ID (10 processes max)				 */
/*********************************************************************/

unsigned char CheckOtherInstances(char *strExeFileName, unsigned char *pucInstanceNb, unsigned long *ulProcessId)
{
	unsigned char i;
	unsigned char ucInstanceNb;
	DWORD dwCurProcessID;

	HANDLE hSysSnapshot;
	PROCESSENTRY32 pe32ProcessProperties;

	char strTempString[100];
	
	// Recovers the current process ID
	dwCurProcessID = GetCurrentProcessId();

	// Creates a system snapshot with all running processes
	hSysSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	
	if (hSysSnapshot != NULL)
	{		
		// Initialization of the counter of MSCopy instance(s)
		ucInstanceNb = 0;
		
		pe32ProcessProperties.dwSize = sizeof(PROCESSENTRY32);

		// Reads the properties of the first process in the snapshot list
		if (Process32First(hSysSnapshot, &pe32ProcessProperties) == TRUE)
		{
			i = 0;
			
			// Excludes the read process if it's the current one
			if (pe32ProcessProperties.th32ProcessID != dwCurProcessID)
			{
				// Checks if the exe file path is the same as "MSCopy.exe"
				while ((pe32ProcessProperties.szExeFile[i] != 0) && (i < 99))
				{
					strTempString[i] = (char)pe32ProcessProperties.szExeFile[i];
					i++;
				}
				strTempString[i] = '\0';

				if (strcmp(strTempString, strExeFileName) == 0)
				{
					// The exe file path is the same as "MSCopy.exe", so the 
					// read process is an instance of the MSCopy process
					if (ucInstanceNb < 10)
					{
						ulProcessId[ucInstanceNb] = (unsigned long)pe32ProcessProperties.th32ProcessID;
						ucInstanceNb++;
					}
				}
			}

			// Reads the properties of the next processes in the snapshot 
			// list
			while (Process32Next(hSysSnapshot, &pe32ProcessProperties) == TRUE)
			{
				i = 0;

				// Excludes the read process if it's the current one
				if (pe32ProcessProperties.th32ProcessID != dwCurProcessID)
				{
					// Checks if the exe file path is the same as "MSCopy.exe"
					while ((pe32ProcessProperties.szExeFile[i] != 0) && (i < 99))
					{
						strTempString[i] = (char)pe32ProcessProperties.szExeFile[i];
						i++;
					}
					strTempString[i] = '\0';
					
					if (strcmp(strTempString, strExeFileName) == 0)
					{
						// The exe file path is the same as "MSCopy.exe", so the 
						// read process is an instance of the MSCopy process
						if (ucInstanceNb < 10)
						{
							ulProcessId[ucInstanceNb] = (unsigned long)pe32ProcessProperties.th32ProcessID;
							ucInstanceNb++;
						}
					}
				}
			}

			// All snapshot processes have been read. Number of instances
			// updated.
			*pucInstanceNb = ucInstanceNb;

			CloseToolhelp32Snapshot(hSysSnapshot);
		}
		else
		{
			// Impossible to recover the properties of the first process => it's 
			// a failure because at least the current process is running
			CloseToolhelp32Snapshot(hSysSnapshot);
			return(RET_ERR_PROCESS_READING);
		}
	}
	else
	{
		// Snapshot creation error
		return(RET_ERR_SYS_SNAPSHOT);
	}

	return (RET_OK);
}

/*********************************************************************/
/* Function: GetNextStartTime										 */
/*                                                                   */
/* Parameters:                                                       */
/*																	 */
/*		unsigned long ulTimePeriodInMin [in]:						 */
/*			Time period to add in minutes							 */
/*																	 */
/*		ULONGLONG* pul64NextStartTime [out]:                         */
/*			Result time factor of 100ns							     */
/*																	 */
/* Return value:                                                     */
/*      unsigned char:                                               */
/*			0 if success, >0 if error						         */
/*                                                                   */
/* Remarks:                                                          */
/*		Calculate the next start time for a specific treatment		 */
/*		adding the associated time period to the current system time */
/*		and applying a shift to be synchronised at 15s 0ms			 */
/*********************************************************************/

unsigned char GetNextStartTime(unsigned long ulTimePeriodInMin, ULONGLONG *pul64NextStartTime)
{
	SYSTEMTIME stimeGMTDateTime;
	FILETIME ftimeGMTCurTime, ftimeGMTNewTime;
	ULONGLONG ul64GMTNewTime;

	// The time period must be > 0
	if (ulTimePeriodInMin == 0)
	{
		return(RET_ERR_TIME_PERIOD_0);
	}
	
	// Reads the current system time in GMT format
	GetSystemTime(&stimeGMTDateTime);
	if (SystemTimeToFileTime(&stimeGMTDateTime, &ftimeGMTCurTime) == FALSE)
	{
		return(RET_ERR_SYS_TO_FILE_TIME);
	}
	
	// Shift time to have a value at 15s 0ms (at the same minute)
	stimeGMTDateTime.wSecond = 15;
	stimeGMTDateTime.wMilliseconds = 0;
	if (SystemTimeToFileTime(&stimeGMTDateTime, &ftimeGMTNewTime) == FALSE)
	{
		return(RET_ERR_SYS_TO_FILE_TIME);
	}

	// Compare the shift time with the current time to establish the chronology and
	// to begin at the next "+15s 0ms" occurence
	ul64GMTNewTime = (((ULONGLONG)(ftimeGMTNewTime.dwHighDateTime)) << 32) + ftimeGMTNewTime.dwLowDateTime;

	if ((CompareFileTime(&ftimeGMTCurTime, &ftimeGMTNewTime) == 1)
		|| (CompareFileTime(&ftimeGMTCurTime, &ftimeGMTNewTime) == 0))
	{
		ul64GMTNewTime += MINUTE_100NS;
	}

	// Add the time period in minutes removing the current minute
	ul64GMTNewTime += (ulTimePeriodInMin - 1) * MINUTE_100NS;
	
	// Update the output value
	*pul64NextStartTime = ul64GMTNewTime;

	return(RET_OK);
}

/*********************************************************************/
/* Function: GetSleepDelayFromCurTime                                */
/*                                                                   */
/* Parameters:                                                       */
/*		ULONGLONG ul64WakeupTimeIn100ns [in]:                        */
/*			End UTC time of the delay in FILETIME format	    	 */
/*                                                                   */
/*		DWORD *pdwDelayInMs [out]:                                   */
/*			Delay calculated in ms									 */
/*																     */
/* Return value:                                                     */
/*      unsigned char:                                               */
/*			0 if success, >0 if error						         */
/*                                                                   */
/* Remarks:                                                          */
/*		Calculates the delay in ms between the current time and the  */
/*      end time (UTC format). If the end time is before the current */
/*	    one, calculates a delay ending at the next 15s passage.	The	 */
/*		max delay value is 1h.										 */
/*********************************************************************/

unsigned char GetSleepDelayFromCurTime(ULONGLONG ul64WakeupTimeIn100ns, DWORD *pdwDelayInMs)
{
	char strTempString[200];
	
	DWORD dwRetDelay;
	SYSTEMTIME stimeGMTDateTime;
	FILETIME ftimeGMTCurTime, ftimeGMTNewTime;
	ULONGLONG ul64GMTCurTime, ul64GMTNewTime;

	// Reads the current system time in GMT format
	GetSystemTime(&stimeGMTDateTime);
	if (SystemTimeToFileTime(&stimeGMTDateTime, &ftimeGMTCurTime) == FALSE)
	{
		return(RET_ERR_SYS_TO_FILE_TIME);
	}

	ul64GMTCurTime = (((ULONGLONG)(ftimeGMTCurTime.dwHighDateTime)) << 32) + ftimeGMTCurTime.dwLowDateTime;

	if (ul64WakeupTimeIn100ns < ul64GMTCurTime)
	{
		// Shift time to have a value at 15s 0ms (at the same minute)
		stimeGMTDateTime.wSecond = 15;
		stimeGMTDateTime.wMilliseconds = 0;
		if (SystemTimeToFileTime(&stimeGMTDateTime, &ftimeGMTNewTime) == FALSE)
		{
			return(RET_ERR_SYS_TO_FILE_TIME);
		}
	
		// Compare the shift time with the current time to establish the chronology 
		// and to begin at the next "+15s 0ms" occurence
		ul64GMTNewTime = (((ULONGLONG)(ftimeGMTNewTime.dwHighDateTime)) << 32) + ftimeGMTNewTime.dwLowDateTime;
	
		if (CompareFileTime(&ftimeGMTCurTime, &ftimeGMTNewTime) == 1)
		{
			ul64GMTNewTime += MINUTE_100NS;
		}

		dwRetDelay = (DWORD)((ul64GMTNewTime - ul64GMTCurTime) / 10000);
	}
	else
	{
		dwRetDelay = (DWORD)((ul64WakeupTimeIn100ns - ul64GMTCurTime) / 10000);
	}

	// Checks if the calculated delay is greater than 1h (in case of panel time 
	// synchronisation). If yes, set the value 1h.
	if (dwRetDelay > HOUR_MS)
	{
		sprintf(strTempString, "[MAIN] The calculated delay is greater than 1h (%d ms). Set the value 1h.", dwRetDelay);
		LogTrace(strTempString, 1);

		*pdwDelayInMs = HOUR_MS;
	}
	else
	{
		*pdwDelayInMs = dwRetDelay;
	}

	return(RET_OK);
}

/*********************************************************************/
/* Function: GetSleepDelayFromVarTime                                */
/*                                                                   */
/* Parameters:                                                       */
/*		SYSTEMTIME stimeGMTCurTime [in]:							 */
/*			Start UTC time of the delay in SYSTEMTIME format	     */
/*																	 */
/*		ULONGLONG ul64WakeupTimeIn100ns [in]:                        */
/*			End UTC time of the delay in FILETIME format	    	 */
/*                                                                   */
/*		DWORD *pdwDelayInMs [out]:                                   */
/*			Delay calculated in ms									 */
/*																     */
/* Return value:                                                     */
/*      unsigned char:                                               */
/*			0 if success, >0 if error						         */
/*                                                                   */
/* Remarks:                                                          */
/*		Calculates the delay in ms between the start and the end     */
/*      time (UTC format). If the end time is before the current one */
/*	    calculates a delay ending at the next 15s passage.	The	 max */
/*		delay value is 1h.										     */
/*********************************************************************/

unsigned char GetSleepDelayFromVarTime(SYSTEMTIME stimeGMTCurTime, ULONGLONG ul64WakeupTimeIn100ns, DWORD *pdwDelayInMs)
{
	char strTempString[200];
	
	DWORD dwRetDelay;
	FILETIME ftimeGMTCurTime, ftimeGMTNewTime;
	ULONGLONG ul64GMTCurTime, ul64GMTNewTime;

	// Reads the current system time in GMT format
	if (SystemTimeToFileTime(&stimeGMTCurTime, &ftimeGMTCurTime) == FALSE)
	{
		return(RET_ERR_SYS_TO_FILE_TIME);
	}

	ul64GMTCurTime = (((ULONGLONG)(ftimeGMTCurTime.dwHighDateTime)) << 32) + ftimeGMTCurTime.dwLowDateTime;

	if (ul64WakeupTimeIn100ns < ul64GMTCurTime)
	{
		// Shift time to have a value at 15s 0ms (at the same minute)
		stimeGMTCurTime.wSecond = 15;
		stimeGMTCurTime.wMilliseconds = 0;
		if (SystemTimeToFileTime(&stimeGMTCurTime, &ftimeGMTNewTime) == FALSE)
		{
			return(RET_ERR_SYS_TO_FILE_TIME);
		}
	
		// Compare the shift time with the current time to establish the chronology 
		// and to begin at the next "+15s 0ms" occurence
		ul64GMTNewTime = (((ULONGLONG)(ftimeGMTNewTime.dwHighDateTime)) << 32) + ftimeGMTNewTime.dwLowDateTime;
	
		if (CompareFileTime(&ftimeGMTCurTime, &ftimeGMTNewTime) == 1)
		{
			ul64GMTNewTime += MINUTE_100NS;
		}

		dwRetDelay = (DWORD)((ul64GMTNewTime - ul64GMTCurTime) / 10000);
	}
	else
	{
		dwRetDelay = (DWORD)((ul64WakeupTimeIn100ns - ul64GMTCurTime) / 10000);
	}

	// Checks if the calculated delay is greater than 1h (in case of panel time 
	// synchronisation). If yes, set the value 1h.
	if (dwRetDelay > HOUR_MS)
	{
		sprintf(strTempString, "[MAIN] The calculated delay is greater than 1h (%d ms). Set the value 1h.", dwRetDelay);
		LogTrace(strTempString, 1);

		*pdwDelayInMs = HOUR_MS;
	}
	else
	{
		*pdwDelayInMs = dwRetDelay;
	}

	return(RET_OK);
}

/*********************************************************************/
/* Function: GetCurrentTimeIn100ns                                   */
/*                                                                   */
/* Parameters:                                                       */
/*		ULONGLONG *ul64GMTCurTime [out]:                             */
/*			Current UTC time in FILETIME format	    				 */
/*                                                                   */
/* Return value:                                                     */
/*      unsigned char:                                               */
/*			0 if success, >0 if error						         */
/*                                                                   */
/* Remarks:                                                          */
/*		Converts the current UTC time to FILETIME 64bits format      */
/*		(number of 100-nanosecond intervals since January 1, 1601)   */
/*********************************************************************/

unsigned char GetCurrentTimeIn100ns(ULONGLONG *ul64GMTCurTime)
{	
	SYSTEMTIME stimeGMTCurTime;
	FILETIME ftimeGMTCurTime;

	// Reads the current system time in GMT format
	GetSystemTime(&stimeGMTCurTime);
	if (SystemTimeToFileTime(&stimeGMTCurTime, &ftimeGMTCurTime) == FALSE)
	{
		return(RET_ERR_SYS_TO_FILE_TIME);
	}

	*ul64GMTCurTime = (((ULONGLONG)(ftimeGMTCurTime.dwHighDateTime)) << 32) + ftimeGMTCurTime.dwLowDateTime;

	return(RET_OK);
}

/*********************************************************************/
/* Function: ShutdownIsRequested                                     */
/*                                                                   */
/* Parameters:                                                       */
/*		char *strStopCmdFilePath [in]:                               */
/*			Path of the "STOP.CMD" file stored in the executable	 */
/*	        file directory											 */
/*                                                                   */
/* Return value:                                                     */
/*      bool:													     */
/*			true = the file "STOP.CMD" is found						 */
/*			false = the file "STOP.CMD" is not found				 */
/*                                                                   */
/* Remarks:                                                          */
/*		Searches the "STOP.CMD" file which means that MSCopy must be */
/*		stopped. Delete the file if found.							 */
/*********************************************************************/

bool ShutdownIsRequested(char *strStopCmdFilePath)
{
	HANDLE hSearchFile;
	WIN32_FIND_DATA wfdFileData;

	TCHAR tstrStopCmdFilePath[100];

	wsprintf(tstrStopCmdFilePath, _T("%S"), strStopCmdFilePath);
	hSearchFile = FindFirstFile(tstrStopCmdFilePath, &wfdFileData);

	if (hSearchFile != INVALID_HANDLE_VALUE)
	{
		FindClose(hSearchFile);
		DeleteFile(tstrStopCmdFilePath);
		return(true);
	}

	return(false);
}