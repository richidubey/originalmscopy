// CIniFile.cpp: implementation of the CIniFile class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CIniFile.h"

#include "Global.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CIniFile::CIniFile(char *strFilePath)
{
	strcpy(m_strFilePath, strFilePath);
}

CIniFile::~CIniFile()
{
}

///////////////////////////////////////////////////////////////////////
// CIniFile member functions

/*********************************************************************/
/* Function: ReadLongValue                                           */
/*                                                                   */
/* Parameters:                                                       */
/*		char* strSection [in]:                                       */
/*			Name of the section where is located the requested       */
/*			parameter                                                */
/*		char* strParameter [in]:                                     */
/*			Name of the requested parameter                          */
/*		long* plValue [out]:                                         */
/*			Parameter value read in file                             */
/*                                                                   */
/* Return value:                                                     */
/*      unsigned char:                                               */
/*			0 if success, >0 if error						         */
/*                                                                   */
/* Remarks:                                                          */
/*		Read a long parameter in the ini file initialized in the     */
/*		class constructor                                            */
/*********************************************************************/

unsigned char CIniFile::ReadLongValue(char *strSection, char *strParameter, long *plValue)
{
	unsigned char ucRetErrorCode;
	unsigned char ucFieldTypeSearched;
	int iEqualsSignPosition;
	bool bReqValueIsFound;
	
	char strBuffer[100];
	char strTempBuffer[100];

	FILE *pfiIniFile;
		
	// File opening
	pfiIniFile = fopen(m_strFilePath, "r");
	
	if (pfiIniFile != NULL)
	{
		// Firstly, the section line is searched, except if the section string is empty
		if (strlen(strSection) > 0)
		{
			ucFieldTypeSearched = SECTION;
		}
		else
		{
			ucFieldTypeSearched = PARAMETER;
		}
		bReqValueIsFound = false;

		do 
		{
			// Buffer data reset
			strncpy(strBuffer, "\0", 100);
			strncpy(strTempBuffer, "\0", 100);
			
			// Reading of each line in the file opened
			if (fgets(strBuffer, 100, pfiIniFile) != NULL)
			{					
				switch (ucFieldTypeSearched)
				{
					case SECTION:
					{
						// Deletes the char "\n"
						if (strlen(strBuffer) > 1)
						{
							strncpy(strTempBuffer, strBuffer, strlen(strBuffer) - 1);
							strTempBuffer[strlen(strBuffer) - 1] = '\0';
						}
						else
						{
							strTempBuffer[0] = '\0';
						}
												
						// Deletes all spaces on both sides of the read string
						TrimString(strTempBuffer, 100);

						// Compares the read string with the requested section
						if (strcmp(strTempBuffer, strSection) == 0)
						{
							// The requested section is found: the requested 
							// parameter will be searched the next time
							ucFieldTypeSearched = PARAMETER;
						}
						break;
					}
					
					case PARAMETER:	
					{
						// Extraction of the parameter name in the read line
						iEqualsSignPosition = strcspn(strBuffer, "=");
						strncpy(strTempBuffer, strBuffer, iEqualsSignPosition);
						strTempBuffer[iEqualsSignPosition] = '\0';
						
						// Deletes all spaces on both sides of the read string
						TrimString(strTempBuffer, 100);
												
						// Checks if the parameter read is the next section. If yes, the
						// target paramater is missing.
						if (strTempBuffer[0] == '[')
						{
							fseek(pfiIniFile, 0, SEEK_END);
						}
						else
						{
							// Compares the read string with the requested parameter name
							if (strcmp(strTempBuffer, strParameter) == 0) 
							{		
								// Extraction of the parameter value
								strcpy(strBuffer, (strBuffer + iEqualsSignPosition + 1));
								
								// Value assignment
								*plValue = atol(strBuffer);
								bReqValueIsFound = true;
								ucRetErrorCode = RET_OK;
							}
						}
						break;
					}
				}
			}
			else 
			{ 
				// End of file or error found
				ucRetErrorCode = RET_ERR_MISSING_PARAM;
				break; 
			}

		} while (!bReqValueIsFound);

		// File closing
		fclose(pfiIniFile);
	}
	else
	{
		// File not found or opening error
		ucRetErrorCode = RET_ERR_FILE_NOT_FOUND;
	}

	return ucRetErrorCode;
}

/*********************************************************************/
/* Function: ReadStringValue                                         */
/*                                                                   */
/* Parameters:                                                       */
/*		char* strSection [in]:                                       */
/*			Name of the section where is located the requested       */
/*			parameter                                                */
/*		char* strParameter [in]:                                     */
/*			Name of the requested parameter                          */
/*		char* strValue [out]:                                        */
/*			Parameter value read in file                             */
/*                                                                   */
/* Return value:                                                     */
/*      unsigned char:                                               */
/*			0 if success, >0 if error						         */
/*                                                                   */
/* Remarks:                                                          */
/*		Read a string parameter in the ini file initialized in the   */
/*		class constructor                                            */
/*********************************************************************/

unsigned char CIniFile::ReadStringValue(char *strSection, char *strParameter, char *strValue)
{
	unsigned char ucRetErrorCode;
	unsigned char ucFieldTypeSearched;
	int iEqualsSignPosition;
	bool bReqValueIsFound;
	
	char strBuffer[100];
	char strReadString[100];
	char strTempBuffer[100];
	
	FILE *pfiIniFile;

	// File opening
	pfiIniFile = fopen(m_strFilePath, "r");
	
	if (pfiIniFile != NULL) 
	{
		// Firstly, the section line is searched, except if the section string is empty
		if (strlen(strSection) > 0)
		{
			ucFieldTypeSearched = SECTION;
		}
		else
		{
			ucFieldTypeSearched = PARAMETER;
		}
		bReqValueIsFound = false;
		
		do 
		{
			// Buffer data reset
			strncpy(strBuffer, "\0", 100);
			strncpy(strTempBuffer, "\0", 100);
			strncpy(strReadString, "\0", 100);
			
			// Reading of each line in the file opened
			if (fgets(strBuffer, 100, pfiIniFile) != NULL)
			{	
				switch (ucFieldTypeSearched) 
				{
					case SECTION:
					{
						// Deletes the char "\n"
						if (strlen(strBuffer) > 1)
						{
							strncpy(strTempBuffer, strBuffer, strlen(strBuffer) - 1);
							strTempBuffer[strlen(strBuffer) - 1] = '\0';
						}
						else
						{
							strTempBuffer[0] = '\0';
						}
						
						// Deletes all spaces on both sides of the read string
						TrimString(strTempBuffer, 100);
												
						// Compares the read string with the requested section
						if (strcmp(strTempBuffer, strSection) == 0)
						{
							// The requested section is found: the requested 
							// parameter will be searched the next time
							ucFieldTypeSearched = PARAMETER;
						}
						break;
					}

					case PARAMETER:
					{	
						// Extraction of the parameter name in the read line
						iEqualsSignPosition = strcspn(strBuffer, "=");
						strncpy(strTempBuffer, strBuffer, iEqualsSignPosition);
						strTempBuffer[iEqualsSignPosition] = '\0';
						
						// Deletes all spaces on both sides of the read string
						TrimString(strTempBuffer, 100);
						
						// Checks if the parameter read is the next section. If yes, the
						// target parameter is missing.
						if (strTempBuffer[0] == '[')
						{
							fseek(pfiIniFile, 0, SEEK_END);
						}
						else
						{
							// Compares the read string with the requested parameter name
							if (strcmp(strTempBuffer, strParameter) == 0) 
							{		
								// Extraction of the parameter value
								strcpy(strBuffer, (strBuffer + iEqualsSignPosition + 1));
								
								// Deletes the char "\n"
								if (strlen(strBuffer) > 1)
								{
									strncpy(strReadString, strBuffer, strlen(strBuffer) - 1);
									strReadString[strlen(strBuffer) - 1] = '\0';
								}
								else
								{
									strReadString[0] = '\0';
								}
								
								// Deletes all spaces on both sides of the read string
								TrimString(strReadString, 100);

								// Value assignment
								strcpy(strValue, strReadString);
								bReqValueIsFound = true;
								ucRetErrorCode = RET_OK;
							}
						}
						break;
					}
				}
			}
			else 
			{ 
				// End of file or error found
				ucRetErrorCode = RET_ERR_MISSING_PARAM;
				break; 
			}
		} while (!bReqValueIsFound);

		// File closing
		fclose(pfiIniFile);
	}
	else
	{
		// File not found or opening error
		ucRetErrorCode = RET_ERR_FILE_NOT_FOUND;
	}

	return ucRetErrorCode;
}

/*********************************************************************/
/* Function: TrimString                                              */
/*                                                                   */
/* Parameters:                                                       */
/*		char* strBuffer [in/out]:                                    */
/*			String array to modify                                   */
/*		unsigned char ucBufferSize [in]:                             */
/*			Size of the string array                                 */
/*                                                                   */
/* Return value:                                                     */
/*      unsigned char:                                               */
/*			0 if success, >0 if error								 */
/*                                                                   */
/* Remarks:                                                          */
/*		Delete the space characters before and after the string in   */
/*		the buffer                                                   */
/*********************************************************************/

unsigned char CIniFile::TrimString(char *strBuffer, unsigned char ucBufferSize)
{
	unsigned char i;
	unsigned char j;
	unsigned char ucFirstIndexWoSpc;
	char strBufferTemp[255];
		
	// Empty string case
	if (strBuffer[0] == '\0')
	{
		return(RET_OK);
	}

	// Initialization of the first string position without space
	ucFirstIndexWoSpc = 255;

	// Reading of the buffer from the beginning to find the first character which 
	// is not a space
	for (i = 0; i < ucBufferSize; i++)
	{
		if (strBuffer[i] != ' ')
		{
			ucFirstIndexWoSpc = i;
			break;
		}
	}
	
	if (ucFirstIndexWoSpc < 255)
	{
		// Case of string with 'space' characters only
		if (strBuffer[ucFirstIndexWoSpc] == '\0')
		{
			strBuffer[0] = '\0';
			return(RET_OK);
		}

		// Reading of the buffer from the first no space character to the end in
		// order to find the end of the string
		for (i = ucFirstIndexWoSpc; i < ucBufferSize; i++)
		{
			if (strBuffer[i] == '\0')
			{								
				// Reading of the buffer from the end of the string to the beginning
				// in order to find the first character which is not a space
				for (j = i - 1; j >= ucFirstIndexWoSpc; j--)
				{
					if (strBuffer[j] != ' ')
					{						
						// Stores the truncated string and copy it to the buffer
						strncpy(strBufferTemp, &strBuffer[ucFirstIndexWoSpc], j - ucFirstIndexWoSpc + 1);
						strBufferTemp[j - ucFirstIndexWoSpc + 1] = '\0';
						strcpy(strBuffer, strBufferTemp);
						return(RET_OK);
					}
				}
			}
		}

		return(RET_ERR_NO_NULL_CHAR);
	}

	return(RET_ERR_SPACES_ONLY);
}
