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

int CUsrSynchro::GetUserFile() {
	
	int iRetSelect;
	int iRetRecv;
	char temp[1025];

	/*//Wait upto 5 seconds to receive UserFile
	iRetSelect = SelectReadUptoNSeconds(client_socket, 5);
	
	if(iRetSelect <= 0) {
		//Did not receive a reply from the driver
		//Close the connection.
		closesocket(client_socket);
		client_socket = INVALID_SOCKET;
		return -1;
	}*/
	
	
	strcpy(m_strUsrTempPathLocal, m_strUsrPathLocal); //Parameter: Destination, Source

	//Replace User.dat with UserTemp.dat in the temp name. Find the location of last '/' first
	int i;
	for(i = strlen(m_strUsrTempPathLocal); i>=0 ; i--) {
		if(m_strUsrTempPathLocal[i] == '\\') {
			break;
		}
	}
	strcpy(&m_strUsrTempPathLocal[i+1], "UserTemp.dat");

	//printf("Location of User file is :\n%s\n", m_strUsrPathLocal);
	//printf("Location of temp User file being created is :\n%s\n", m_strUsrTempPathLocal);

	FILE *nfile = fopen(m_strUsrTempPathLocal ,"w");

	//printf("Received UserFileContent: \n\n");
	char subtemp[8];

	strcpy(subtemp, "Hello");
	while(1) {
		//Wait upto 5 seconds to receive UserFile content
		iRetSelect = SelectReadUptoNSeconds(client_socket, 5);

		memset(temp, 0, sizeof(temp));
		iRetRecv = recv(client_socket, temp, 1024, 0);

		if(iRetRecv <= 0) {
			//Did not receive a reply from the driver
			//Close the connection.
			closesocket(client_socket);
			client_socket = INVALID_SOCKET;
			return -1;
		}

		if(iRetRecv == 1024) {
			temp[1024] = '\0';
		}

		//printf("%s\n",temp);

		if(strlen(temp) >=13) {
			//printf("The last 5 characters of the message is:\n");

			//char r = '\0';
			//printf("Printing null character looks like : %c\n",r);

			//printf("Position %d : %c\n", strlen(temp)-1, temp[strlen(temp)-1]);
			//printf("Position %d : %c\n", strlen(temp)-2, temp[strlen(temp)-2]);
			//printf("Position %d : %c\n", strlen(temp)-3, temp[strlen(temp)-3]);
			//printf("Position %d : %c\n", strlen(temp)-4, temp[strlen(temp)-4]);
			//printf("Position %d : %c\n", strlen(temp)-5, temp[strlen(temp)-5]);
		
			//printf("Strlen temp is :%d\n", strlen(temp));
			memcpy(subtemp, &temp[strlen(temp) - 13], 11); //Destination, Source, size
			//Account for 2 extra characters at the end.
			subtemp[11] = '\0';

			//printf("\nLast 11 character of temp are:  %s\n",subtemp);
		}

		if( strcmp("##DRV_ACK##", subtemp) == 0 ) {
			//printf("Received the entire file\n");
			temp[strlen(temp) - 13] = '\0';
			fprintf(nfile, "%s", temp);
			fclose(nfile);
			break;
		}

		fprintf(nfile, "%s", temp);
	}

	return 0;
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
	bool bNeedToCopyServerFile;
	unsigned char ucRetValue;
	unsigned char ucRetValue2;

	int iRetSelect;
	int iRetHandshake;
	int iRetSend;

	char temp[1024];

	char strTempString[200];

	bNeedToCopyServerFile = false;
	ucRetValue = RET_OK;

	bool skip_treatment= false;

	sockaddr_in from;
    int fromlen = sizeof(from);


	//printf("Inside User Synchronization Treatment\n");

	if(client_socket == INVALID_SOCKET) {
		
		//Client is not connected. Wait upto 5 seconds on the listening queue

		iRetSelect = SelectReadUptoNSeconds(listening_socket, 5);

		//printf("Client is not connected. Will wait upto 5 seconds for possible incoming connections\n");

		if(iRetSelect > 0) {
			client_socket = accept(listening_socket, (struct sockaddr*) &from, &fromlen); //Todo - Update the 2nd and 3rd parameter

			if(client_socket > 0) {
				//printf("Succesfully connected to the driver within a 5 seconds wait \n");
				ioctlsocket(client_socket, FIONBIO, &iMode); //Set connection as non blocking
			} else {
				//Could not connect to driver. Skip User Synchronization treatment.
				//printf("Error in accepting the client connection\n");
				skip_treatment = true;
			}
		} else {
			//Could not connect to driver. Skip User Synchronization treatment.
			//printf("No client connected in last 5 seconds or error in select\n");
			skip_treatment = true;
		}
	}

	if(!skip_treatment) {

		//printf("Starting treatment with handshake\n");

		iRetHandshake = Socket_Handshake();

		if(iRetHandshake == 0) {

			//printf("Handshake completed, Sending 'User' Message\n");

			sprintf(temp, "User");
	
			//Send 'User' message to indicate start of User File Synchronization Treatment
			iRetSend = send(client_socket, temp, strlen(temp), 0);

		//	printf("Mandatory 2 seconds wait\n");
	//		Sleep(2000);

			if(iRetSend > 0) {
				if( GetUserFile() == 0) {
					//User Temp file has been generated completely.
					//Replace actual user file with the UserTemp file

					//printf("Successfully received the entire file\n");
					wsprintf(m_tstrUsrTempPathLocal, _T("%S"), m_strUsrTempPathLocal);
					if ( CopyFile(m_tstrUsrTempPathLocal, m_tstrUsrPathLocal,FALSE) ) { //Source, Destination, FailIfExists
						pmcsMSCopyStatus->bUsrSynchroError = false;
						//printf("Successfully replaced the original user file with the temporary file\n");
						//printf("Mandatory 2 second sleep\n");
						//Sleep(2000);

					} else {
						//Error in replacing original user file with the temporary file
						//printf("Error in replacing original user file with the temporary file\nError Code: %d\n\n", GetLastError());
						pmcsMSCopyStatus->bUsrSynchroError = true;
						ucRetValue = RET_ERR_USR_FILE_COPY;
					}
				} else {
					//printf("Error in getting User File from peer\n");
					pmcsMSCopyStatus->bUsrSynchroError = true;
					ucRetValue = RET_ERR_USR_FILE_COPY;
				}
			} else {
				//Error in sending 'User' message to the driver
				//Close the connection.
				//printf("Error in sending 'User' message to peer to start User file exchange");
				closesocket(client_socket);
				client_socket = INVALID_SOCKET;
				pmcsMSCopyStatus->bUsrSynchroError = true;
				ucRetValue = RET_ERR_USR_FILE_COPY;
			}
		} else {
			//Error in handshake.
			//Close the connection.
			//printf("Error in application level socket handshake with the peer\n");
			closesocket(client_socket);
			client_socket = INVALID_SOCKET;
			pmcsMSCopyStatus->bUsrSynchroError = true;
			ucRetValue = RET_ERR_USR_FILE_COPY;
		}
	} else {
		pmcsMSCopyStatus->bUsrSynchroError = true;
	}

	if(pmcsMSCopyStatus->bUsrSynchroError) {
		// If there is a failure in Synchronization, Retry treatment in 1 minute
		ucRetValue2 = GetNextStartTime(1, &m_ul64NextStartTime);
	} else {
		ucRetValue2 = GetNextStartTime(m_ulTimePeriod, &m_ul64NextStartTime);
		//printf("User Synchronization completed succesfully\n");
	}
		
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