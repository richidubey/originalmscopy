// CIniFile.h: interface for the CIniFile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CINIFILE_H__690B312C_24D2_4AA8_A781_AE2B291278BD__INCLUDED_)
#define AFX_CINIFILE_H__690B312C_24D2_4AA8_A781_AE2B291278BD__INCLUDED_

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

#endif // !defined(AFX_CINIFILE_H__690B312C_24D2_4AA8_A781_AE2B291278BD__INCLUDED_)
