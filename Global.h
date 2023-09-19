#ifndef MSCOPY_GLOBAL_H
#define MSCOPY_GLOBAL_H

// Global constants

// MSCopy version number
#define MSCOPY_VERSION				2.01

// MSCopy error codes
#define RET_OK						0x00
#define RET_ERR_MISSING_PARAM		0x01
#define RET_ERR_FILE_NOT_FOUND		0x02
#define RET_ERR_NO_NULL_CHAR		0x03
#define RET_ERR_SPACES_ONLY			0x04
#define RET_ERR_FILE_OPENING		0x05
#define RET_ERR_FILE_WRITING		0x06
#define RET_ERR_SYS_SNAPSHOT		0x07
#define RET_ERR_PROCESS_READING		0x08
#define RET_ERR_USR_FILE_COPY		0x09
#define RET_ERR_SYS_TO_FILE_TIME	0x0A
#define RET_ERR_TIME_PERIOD_0		0x0B
#define RET_ERR_LOG_FILE_DEL		0x0C
#define RET_ERR_LOG_FILE_MOVE		0x0D
#define RET_ERR_LOG_FILE_PATH		0x0E

// INI file
#define SECTION						1
#define PARAMETER					2

// Log file move
#define SERVER1						0
#define SERVER2						1
#define MEASURE						0
#define EVENT						1

// Standard time units expressed in ms
#define MINUTE_MS					60000
#define HOUR_MS						(60 * MINUTE_MS)

// Standard time units expressed as factor of 100ns
#define SECOND_100NS				((ULONGLONG)10000000)
#define MINUTE_100NS				(60 * SECOND_100NS)
#define HOUR_100NS					(60 * MINUTE_100NS)
#define DAY_100NS					(24 * DAY_100NS)

// Operating mode for the status write treatment
#define STARTING					0
#define NORMAL						1

// Global user types
typedef struct
{
	float fMSCopyVersion;
	bool bLogFileMoveRecovery;
	bool bLogFileMoveError;
	bool bUsrSynchroError;
	char strLogFileServerUsed[30];
} tstMSCopyStatus;

typedef struct
{
	long lSuccessiveMove;
	long lMovePeriod;
	long lSrvScrutingDelay;
	long lUsrSynchroPeriod;
	long lStatusWritePeriod;
	bool bTraceMode;
	long lTraceLevel;
	long lTraceFileMaxSize;
	char strServerName1[100];
	char strServerName2[100];
	char strMeasPathLocal[100];
	char strMeasPathServer[100];
	char strEvtPathLocal[100];
	char strEvtPathServer[100];
	char strUsrPathLocal[100];
	char strUsrPathServer[100];
	char strPanelStartupFilePath[100];
	char strStatusFilePath[100];
	char strTraceDirPath[100];
} tstMSCopyCfg;

// Global function prototypes
unsigned char LogTrace(char *strMessage, unsigned char ucTraceLevel);
unsigned char GetNextStartTime(unsigned long ulTimePeriodInMin, ULONGLONG *pul64NextStartTime);
int SelectReadUptoNSeconds(SOCKET socket, int Nseconds);
int Socket_Handshake();

#endif MSCOPY_GLOBAL_H