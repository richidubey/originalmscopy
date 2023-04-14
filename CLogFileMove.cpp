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

	m_ulMovePeriod = pmccMSCopyCfg->lMovePeriod;
	m_ulSrvScrutingDelay = pmccMSCopyCfg->lSrvScrutingDelay;
	m_ulSuccessiveMove = pmccMSCopyCfg->lSuccessiveMove;
	
	// Type conversion char to TCHAR
	wsprintf(m_tstrMeasPathLocal, _T("%S"), pmccMSCopyCfg->strMeasPathLocal);
	wsprintf(m_tstrEvtPathLocal, _T("%S"), pmccMSCopyCfg->strEvtPathLocal);
}

CLogFileMove::~CLogFileMove()
{
}

int SendFilesOverSocket(int fCount, TCHAR fileToSend[][25], TCHAR *tstrDirPath, unsigned long MaxNbOfFileToMove){
	
	int sCount;
	unsigned long ulNbOfFileMoved;
	
	int iRetSend;
	int iRetSelect;
	int iRetRecv;

	FILE *fp;

	char temp[1024];
	char subtemp[15];

	TCHAR tstrTempString[100];
	char strTempString[100];

	ulNbOfFileMoved = 0;

	for(sCount = fCount - 1; sCount >= 0; sCount--) //Send files if fcount!=0 
	{
		/*
				Send Filename
					|
					v
				Wait for confirmation.
				After receiving Confirmation
					|
					v
				Send File content
					|
					v
				Send EOF marker
					|
					v
				Wait for confirmation
					|
					v
				Delete File.
		*/
		//////////////////

	/*	printf("(Telnet)Random Check: Before sending Final Marker, checking if I can receive something from socket already\n");
			
		//Wait upto 5 seconds to receive confirmation
		iRetSelect = SelectReadUptoNSeconds(client_socket, 5);
		
		if(iRetSelect <= 0) {
			//Did not receive a reply from the driver
			//Close the connection.
			printf("(Telnet) Did not receive any message within 5 seconds or error in select\n");

		} else {
			printf("(Telnet) Yes, we can receive\n");
			recv(client_socket, temp, 15, 0);

			//printf("(Telnet) Received: %s\nMandatory 3 seconds sleep\n", temp);
			//Sleep(3000);
		}
*/
		//////////////////////////

		//printf("Sending name: %S\n", fileToSend[sCount]);

		sprintf(temp, "%S", fileToSend[sCount]);

		//Send filename to driver
		iRetSend = send(client_socket, temp, strlen(temp), 0);

		if(iRetSend <= 0) {
			//Error in sending filename. Abort this Log File Movement treatment.
			//printf("Error in sending filename. Error code is %d\n", GetLastError());
			return -1; 
		}

//		printf("Going to wait for confirmation  Mandatory 5 second wait\n");
//		Sleep(5);

		//Wait upto 5 seconds to receive confirmation
		iRetSelect = SelectReadUptoNSeconds(client_socket, 5);

		if(iRetSelect <= 0) {
			//Did not receive a reply from the driver
			//Close the connection.
			//printf("Did not receive any message within 5 seconds or error in select\n");
			closesocket(client_socket);
			client_socket = INVALID_SOCKET;
			return -1;
		}

		memset(temp, 0, sizeof(temp));
		iRetRecv = recv(client_socket, temp, 15, 0);

		if(iRetRecv <= 0) {
			//Did not receive a reply from the driver
			//Close the connection.
			//printf("recv: Error in receiving confirmation\n");
			closesocket(client_socket);
			client_socket = INVALID_SOCKET;
			return -1;
		}
		
		//printf("Succesfully received %s\n", temp);
		
		if(strlen(temp) < 13) {
			//Message size less than expected. Received an invalid confirmation.
			//printf("Invalid confirmation message from client. Abort connection\n");
			closesocket(client_socket);
			client_socket = INVALID_SOCKET;
			return -1;
		}

		memcpy(subtemp, &temp[strlen(temp) - 13], 11); 
		subtemp[11] = '\0'; //strcmp compares upto \n or EOS message in the source/destination string

		if( strcmp("##DRV_ACK##", subtemp) != 0 ) {
			//Received an invalid confirmation. 
			//printf("Invalid confirmation message from client. Abort connection\n");
			closesocket(client_socket);
			client_socket = INVALID_SOCKET;
			return -1;
		}

		lstrcpy(tstrTempString, tstrDirPath);
		lstrcat(tstrTempString, _T("\\"));
		lstrcat(tstrTempString, fileToSend[sCount]);

		sprintf(strTempString, "%S", tstrTempString);

		//printf("Trying to open in read mode: \n%s\n", strTempString);

		fp = fopen(strTempString, "r");

		if(fp == NULL) {
			//Opening file to read failed. Skip this file
			//printf("Opening file in read mode failed. Skip this file\nError code is %d\n", GetLastError());

			//Send final marker to indicate EOF.
			sprintf(temp, "##PNL_ACK##");
			iRetSend = send(client_socket, temp, strlen(temp), 0);

			if(iRetSend <= 0) {
				//Error in sending final marker. Abort this Log File Movement treatment.
				//printf("Error in sending final marker.\n");
				closesocket(client_socket);
				client_socket = INVALID_SOCKET;
				return -1; 
			}

			//printf("Mandatory 4 seconds sleep\n");
			//Sleep(4000);
			continue;
		}

		while(fgets(temp, sizeof(temp), fp)) {
			//printf("Line read:\n %s",temp);

			iRetSend = send(client_socket, temp, strlen(temp), 0);
			
			if(iRetSend <= 0) {
				//Error in sending file content. Abort this Log File Movement treatment.
				//printf("Error in sending file content.\n");
				return -1; 
			}
		}
		//Finished sending file. Send marker

		//////////////////

		//printf("Before sending Final Marker, checking if I can receive something from socket already\n");
			
		//Wait upto 5 seconds to receive confirmation
	//	iRetSelect = SelectReadUptoNSeconds(client_socket, 5);
		
	//	if(iRetSelect <= 0) {
			//Did not receive a reply from the driver
			//Close the connection.
	//		printf("Did not receive any message within 5 seconds or error in select\n");
	//	} else {
	//		printf("Yes, we can receive\n");
	//		recv(client_socket, temp, 10, 0);

		//	printf("Received: %s\nMandatory 3 seconds sleep\n", temp);
		//	Sleep(3000);
	//	}

		//////////////////////////

		sprintf(temp, "##PNL_ACK##");
		iRetSend = send(client_socket, temp, strlen(temp), 0);

		if(iRetSend <= 0) {
			//Error in sending final marker. Abort this Log File Movement treatment.
			//printf("Error in sending final marker.\n");
			closesocket(client_socket);
			client_socket = INVALID_SOCKET;
			return -1; 
		}

		
	//	printf("(Debug) Going to wait for confirmation mandatory 5 second wait\n");
	//Sleep(5);

		//Wait upto 5 seconds to receive confirmation from the driver
		iRetSelect = SelectReadUptoNSeconds(client_socket, 5);

		if(iRetSelect <= 0) {
			//Did not receive a reply from the driver
			//Close the connection.
			//printf("Did not receive any message within 5 seconds or error in select\n");
			closesocket(client_socket);
			client_socket = INVALID_SOCKET;
			return -1;
		}

		memset(temp, 0, sizeof(temp));
		iRetRecv = recv(client_socket, temp, 15, 0);

		if(iRetRecv <= 0) {
			//Did not receive a reply from the driver
			//Close the connection.
			//printf("recv: Error in receiving confirmation\n");
			closesocket(client_socket);
			client_socket = INVALID_SOCKET;
			return -1;
		}
		
		//printf("Succesfully received %s\n", temp);
		
		if(strlen(temp) < 13) {
			//Message size less than expected. Received an invalid confirmation.
			//printf("Invalid confirmation message from client. Abort connection\n");
			closesocket(client_socket);
			client_socket = INVALID_SOCKET;
			return -1;
		}

		memcpy(subtemp, &temp[strlen(temp) - 13], 11); 
		subtemp[11] = '\0'; //strcmp compares upto \n or EOS message in the source/destination string

		if( strcmp("##DRV_ACK##", subtemp) != 0 ) {
			//Received an invalid confirmation. 
			//printf("Invalid confirmation message from client. Abort connection\n");
			closesocket(client_socket);
			client_socket = INVALID_SOCKET;
			return -1;
		}

		//Final marker received from client. Delete the file.

		fclose(fp);

		if(DeleteFile(tstrTempString) == 0) {
			//Error in deleting file
			//printf("Error in deleting file. Error code: %d\n", GetLastError());
		}

		ulNbOfFileMoved = ulNbOfFileMoved + 1;

		if(ulNbOfFileMoved == MaxNbOfFileToMove) 
			break;
	}

	return ulNbOfFileMoved;
}

unsigned int CLogFileMove::SendMeasFiles(tstMSCopyStatus* pmcsMSCopyStatus) {
	
	// -> Sends the requested number of the oldest files (if sufficient number of files stored
	//    in flash card)

	HANDLE hSearchFile;
	TCHAR tstrTempString[100];
	TCHAR tstrDirPath[100];
	SYSTEMTIME stimeGMTCurTime;
	FILETIME ftimeGMTCurTime;

	ULONGLONG ul64GMTCurTime;
	ULONGLONG ul64GMTFileWTime;
	ULONGLONG ul64CurTimeFileWTimeDiff;

	unsigned long ulTotNbOfFileMoved;
	unsigned long ulNbOfFileMoved;

	WIN32_FIND_DATA ffd;

	TCHAR fileToSend[720][25];

	int fCount;

	ulTotNbOfFileMoved = 0;

	// Browses the MSLog directories from "1" (most recent files) to "11" (oldest files)
	for (unsigned char i = 1; i < 12; i++)
	{
		fCount = 0;
		
		lstrcpy(tstrDirPath, m_tstrMeasPathLocal);
		wsprintf(tstrTempString, _T("\\%d"), i);
		lstrcat(tstrDirPath, tstrTempString);
		
		lstrcpy(tstrTempString, tstrDirPath);
		lstrcat(tstrTempString, _T("\\*.log"));

		//printf("Checking files in folder num %d and full path is \n%S\n",i, tstrTempString);
	
	//	printf("Mandatory 2 second sleep\n");
//		Sleep(2000);
		
		// In the current directory, searches log file names sorted by alphabetic order to browse 
		// it from the oldest to the more recent file
		hSearchFile = FindFirstFile(tstrTempString, &ffd);

		if (hSearchFile != INVALID_HANDLE_VALUE)
		{

			do{
				if(i == 1) { //Check file write time only for the first folder
					GetSystemTime(&stimeGMTCurTime);
					if (SystemTimeToFileTime(&stimeGMTCurTime, &ftimeGMTCurTime) != FALSE) {

						ul64GMTCurTime = (((ULONGLONG)(ftimeGMTCurTime.dwHighDateTime)) << 32) + ftimeGMTCurTime.dwLowDateTime;
						ul64GMTFileWTime = (((ULONGLONG)(ffd.ftLastWriteTime.dwHighDateTime)) << 32) + ffd.ftLastWriteTime.dwLowDateTime;

						if (CompareFileTime(&(ffd.ftLastWriteTime), &ftimeGMTCurTime) == -1) {	
							ul64CurTimeFileWTimeDiff = ul64GMTCurTime - ul64GMTFileWTime;
						}
						else {	
							ul64CurTimeFileWTimeDiff = ul64GMTFileWTime - ul64GMTCurTime;
						}

						if (ul64CurTimeFileWTimeDiff > 5 * SECOND_100NS) {
							
							//printf("File modification time correct, increasing file count\n");
							lstrcpy(fileToSend[fCount], ffd.cFileName);
							fCount++;

						} else {
							//printf("***Modified time of this file in last 5 seconds skipping this file\n");
							//printf("Name of the file was: %S\n",ffd.cFileName);
						}
					} else {
						//printf("Could not convert SystemTime to FileTime, Send current file\n");
						lstrcpy(fileToSend[fCount], ffd.cFileName);
						fCount++;
						}
				} else {
					//We dont need to check file modification time for files in other folders.
					lstrcpy(fileToSend[fCount], ffd.cFileName);
					fCount++; }
			} while ( FindNextFile(hSearchFile, &ffd) == TRUE);
		} else {
			//No file in this folder
		//		printf("INVALID HANDLE to get file. Mandatory 2 second sleep\n");
		//		Sleep(2000);
		
				//printf("There are no files in folder %d or error code is %d\n", i, GetLastError());
		}

		//printf("Sending %d files from this folder\n", fCount);

		ulNbOfFileMoved = SendFilesOverSocket(fCount, fileToSend, tstrDirPath, m_ulSuccessiveMove - ulTotNbOfFileMoved);
		
		if( ulNbOfFileMoved < 0 ) {
			return -1;
		}

		ulTotNbOfFileMoved = ulTotNbOfFileMoved + ulNbOfFileMoved;

		if( ulTotNbOfFileMoved == m_ulSuccessiveMove ) { 
			pmcsMSCopyStatus->bLogFileMoveRecovery = true;	
			break;
		}
	}

	return 0;
}

unsigned int CLogFileMove::SendEventFiles(tstMSCopyStatus* pmcsMSCopyStatus)
{
	HANDLE hSearchFile;
	TCHAR tstrTempString[100];
	SYSTEMTIME stimeGMTCurTime;
	FILETIME ftimeGMTCurTime;

	unsigned long ulNbOfFileMoved;

	ULONGLONG ul64GMTCurTime;
	ULONGLONG ul64GMTFileWTime;
	ULONGLONG ul64CurTimeFileWTimeDiff;

	WIN32_FIND_DATA ffd;

	TCHAR fileToSend[720][25];
	int fCount;

	fCount = 0;
	lstrcpy(tstrTempString, m_tstrEvtPathLocal);
	lstrcat(tstrTempString, _T("\\*.log"));

	//printf("Checking files in folder\n%S\nMandatory 2 second sleep\n", tstrTempString);
	//Sleep(2000);
		
	// In the current directory, searches log file names sorted by alphabetic order to browse 
	// it from the oldest to the more recent file
	hSearchFile = FindFirstFile(tstrTempString, &ffd);

	if (hSearchFile != INVALID_HANDLE_VALUE){
		do{
			GetSystemTime(&stimeGMTCurTime);

			if (SystemTimeToFileTime(&stimeGMTCurTime, &ftimeGMTCurTime) != FALSE) {

				ul64GMTCurTime = (((ULONGLONG)(ftimeGMTCurTime.dwHighDateTime)) << 32) + ftimeGMTCurTime.dwLowDateTime;
				ul64GMTFileWTime = (((ULONGLONG)(ffd.ftLastWriteTime.dwHighDateTime)) << 32) + ffd.ftLastWriteTime.dwLowDateTime;

				if (CompareFileTime(&(ffd.ftLastWriteTime), &ftimeGMTCurTime) == -1) {	
					ul64CurTimeFileWTimeDiff = ul64GMTCurTime - ul64GMTFileWTime;
				}
				else {	
					ul64CurTimeFileWTimeDiff = ul64GMTFileWTime - ul64GMTCurTime;
				}

				if (ul64CurTimeFileWTimeDiff > 60 * SECOND_100NS) {
					//printf("File modification time correct, increasing file count\n");
					lstrcpy(fileToSend[fCount], ffd.cFileName);
					fCount++;
				} else {
					//printf("***Modified time of this file in last 60 seconds skipping this file\n");
					//printf("Name of the file was: %S\n",ffd.cFileName);
				}
			} else {
				//printf("Could not convert SystemTime to FileTime, Send current file\n");
				lstrcpy(fileToSend[fCount], ffd.cFileName);
				fCount++;
				}
		} while ( FindNextFile(hSearchFile, &ffd) == TRUE);
	} else {
		//No file in this folder
		//printf("INVALID HANDLE to get file. Mandatory 2 second sleep\n");
		//Sleep(2000);
		//printf("There are no files in the events folder\n");
	}

	ulNbOfFileMoved = SendFilesOverSocket(fCount, fileToSend, m_tstrEvtPathLocal, m_ulSuccessiveMove);
		
	if( ulNbOfFileMoved < 0 ) {
		return -1;
	}

	if( ulNbOfFileMoved == m_ulSuccessiveMove ) { 
		pmcsMSCopyStatus->bLogFileMoveRecovery = true;
	}

	return 0;
}

int CLogFileMove::ReceiveACKfromClient()
{
	char temp[1024], subtemp[15];
	int iRetSelect;
	int iRetRecv;

	//printf("Waiting upto 5 seconds to receive ACK\n");

	iRetSelect = SelectReadUptoNSeconds(client_socket, 5);

	if(iRetSelect <= 0) {
		//Did not receive a reply from the driver
		//Close the connection.
		//printf("Did not receive any message within 5 seconds or error in select\n");
		return -1;
	}

	memset(temp, 0, sizeof(temp));
	iRetRecv = recv(client_socket, temp, 15, 0);

	if(iRetRecv <= 0) {
		//Did not receive a reply from the driver
		//Close the connection.
		//printf("recv: Error in receiving confirmation\n");
		return -1;
	}
	
	//printf("Succesfully received %s\n", temp);
	
	if(strlen(temp) < 13) {
		//Message size less than expected. Received an invalid confirmation.
		//printf("Invalid confirmation message from client. Abort connection\n");
		return -1;
	}

	memcpy(subtemp, &temp[strlen(temp) - 13], 11); 
	subtemp[11] = '\0'; //strcmp compares upto \n or EOS message in the source/destination string

	if( strcmp("##DRV_ACK##", subtemp) != 0 ) {
		//Received an invalid confirmation. 
		//printf("Invalid confirmation message from client. Abort connection\n");
		return -1;
	}


	//printf("ACK received successfully\n");
	//ACK received successfully;
	return 0;
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

	unsigned char ucRet, ucRetValue;
	unsigned long ulNbOfMeasFileMoved;
	unsigned long ulNbOfEvtFileMoved;

	int iRetHandshake;
	int iRetSend;
	int iRetAck;

	char temp[1024];

	char strTempString[200];
	bool skip_treatment = false;
	
	sockaddr_in from;
    int fromlen = sizeof(from);


	//printf("Inside Log File Move Treatment\n");

	// Status and flag initialization
	pmcsMSCopyStatus->bLogFileMoveError = false;
	ulNbOfMeasFileMoved = 0;
	ulNbOfEvtFileMoved = 0;

	if(client_socket == INVALID_SOCKET) {
		//Client is not connected. Wait upto 5 seconds on the listening queue
		//printf("Client is not connected. Wait upto 5 seconds on the listening queue\n");
		int iRetSelect = SelectReadUptoNSeconds(listening_socket, 5);

		if(iRetSelect > 0) {
			client_socket = accept(listening_socket, (struct sockaddr*) &from, &fromlen);
			
			if(client_socket > 0) {
				//printf("Succesfully connected to the driver within a 5 seconds wait \n");
				ioctlsocket(listening_socket, FIONBIO, &iMode);
			} else {
				//Could not accept connection to driver. Skip User Synchronization treatment.
				//printf("Could not accept connection to driver. Skip Log File Synchronization treatment.\n");
				skip_treatment = true;
				pmcsMSCopyStatus->bLogFileMoveError = true;
			}
		} else {
			//Could not connect to driver. Skip User Synchronization treatment.
			//printf("Could not connect to driver in last 5 seconds. Skip Log File Synchronization treatment.");
			skip_treatment = true;
			pmcsMSCopyStatus->bLogFileMoveError = true;
		}
	}

	if(!skip_treatment) {

		iRetHandshake = Socket_Handshake();

		if(iRetHandshake == 0) {

			sprintf(temp, "LogFile");
	
			//Send 'LogFile' message to indicate start of Log File Move Treatment
			//printf("To peer send 'LogFile' message to indicate start of Log File Move Treatment\n");
			iRetSend = send(client_socket, temp, strlen(temp), 0);

		//	printf("Mandatory 2 seconds wait\n");
	//		Sleep(2000);

			if(iRetSend > 0)
				//Receive ACK for LogFile Message
				iRetAck = ReceiveACKfromClient();

			
			if(iRetSend > 0 && iRetAck == 0) {
				if( SendMeasFiles(pmcsMSCopyStatus) == 0) {
					//Mesaurement files sent succesfully. Send log files now.
					//printf("Measurement files sent successfully\n");
					
					sprintf(temp, "Event");
					//Send 'Event' message to indicate sending of Event Files 
					//printf("To peer send 'Event' message to indicate sending of Event Files."); //Mandatory 2 secs wait\n");
					//Sleep(2000);
					iRetSend = send(client_socket, temp, strlen(temp), 0);

					if(iRetSend > 0)
						//Receive ACK for Event Message
						iRetAck = ReceiveACKfromClient();

					if(iRetSend > 0 && iRetAck == 0) {
						if(SendEventFiles(pmcsMSCopyStatus)==0) {
							//Both measurement and event files sent successfully. 
							//Sleep(2000);
							sprintf(temp, "##PNL_ACK##");
							iRetSend = send(client_socket, temp, strlen(temp), 0);

							if(iRetSend <= 0) {
								//Error in sending final marker. Abort this Log File Movement treatment.
								//printf("Error in sending final marker.\n");
								closesocket(client_socket);
								client_socket = INVALID_SOCKET;
								pmcsMSCopyStatus->bLogFileMoveError = true;
							} else {
								pmcsMSCopyStatus->bLogFileMoveError = false;
							}
						} else {
							//printf("Error in sending Event Files to driver\n");
							pmcsMSCopyStatus->bLogFileMoveError = true;
							ucRetValue = RET_ERR_LOG_FILE_MOVE;
						}
					} else {
						//Error in sending 'Event' message to the driver
						//Close the connection.

						//if(iRetSend <= 0)
							//printf("Error in sending 'Event' message to peer to start sending log files");
				//		else
							//printf("Error in getting ACK of 'Event' message from peer");

						closesocket(client_socket);
						client_socket = INVALID_SOCKET;
						pmcsMSCopyStatus->bLogFileMoveError = true;
						ucRetValue = RET_ERR_LOG_FILE_MOVE;
					}
				} else {
					//printf("Error in sending Measurement Files to driver\n");
					pmcsMSCopyStatus->bLogFileMoveError = true;
					ucRetValue = RET_ERR_LOG_FILE_MOVE;
				}
			} else {
				//Error in sending 'LogFile' message to the driver or in Receiving ACK
				//Close the connection.
				
				//if(iRetSend <= 0)
					//printf("Error in sending 'LogFile' message to peer to start sending log files");
				//else
					//printf("Error in getting ACK of 'LogFile' message from peer");
				
				closesocket(client_socket);
				client_socket = INVALID_SOCKET;
				pmcsMSCopyStatus->bLogFileMoveError = true;
				ucRetValue = RET_ERR_LOG_FILE_MOVE;
			}
		} else {
			//Error in handshake.
			//Close the connection.
			//printf("Error in application level socket handshake with the peer\n");
			closesocket(client_socket);
			client_socket = INVALID_SOCKET;
			pmcsMSCopyStatus->bLogFileMoveError = true;
			ucRetValue = RET_ERR_LOG_FILE_MOVE;
		}
	}

	if(pmcsMSCopyStatus->bLogFileMoveError) {
		// Next treatement start time in 1 minute
		ucRet = GetNextStartTime(1, &m_ul64NextStartTime);
	} else {
		// Calculate the next treatement start time
		ucRet = GetNextStartTime(m_ulMovePeriod, &m_ul64NextStartTime);
	}

	if (ucRet != RET_OK) {
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