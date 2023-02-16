#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CIniFile  
{
	public:
		CIniFile(char *strFilePath);
		virtual ~CIniFile();

		unsigned char ReadLongValue(char *strSection, char *strParameter, long *plValue);
		unsigned char ReadStringValue(char *strSection, char *strParameter, char *strValue);
		unsigned char TrimString(char *strBuffer, unsigned char ucBufferSize);

	private:
		char m_strFilePath[200];
};