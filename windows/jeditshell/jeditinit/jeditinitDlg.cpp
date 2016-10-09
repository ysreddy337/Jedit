/*
 * jeditinitDlg.cpp - part of jEditLauncher package
 * Copyright (C) 2001 John Gellene
 * jgellene@nyc.rr.com
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or any later version.
 *
 * Notwithstanding the terms of the General Public License, the author grants
 * permission to compile and link object code generated by the compilation of
 * this program with object code and libraries that are not subject to the
 * GNU General Public License, provided that the executable output of such
 * compilation shall be distributed with source code on substantially the
 * same basis as the jEditLauncher package of which this program is a part.
 * By way of example, a distribution would satisfy this condition if it
 * included a working makefile for any freely available make utility that
 * runs on the Windows family of operating systems. This condition does not
 * require a licensee of this software to distribute any proprietary software
 * (including header files and libraries) that is licensed under terms
 * prohibiting redistribution to third parties.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Id: jeditinitDlg.cpp,v 1.1.1.1 2001/07/03 13:14:19 jgellene Exp $
 */

// jeditinitDlg.cpp : Implementation of CJeditinitDlg

#include "stdafx.h"
#include "jeditinitDlg.h"

/////////////////////////////////////////////////////////////////////////////
// CJeditinitDlg


extern TCHAR *szVersion;

TCHAR *keyValues[] =  { _T("Java Executable"),
						_T("Java Options"),
						_T("jEdit Target"),
						_T("jEdit Options"),
						_T("jEdit Working Directory") };



UINT CALLBACK CJeditinitDlg::OFNHookProc(
  HWND hdlg,      // handle to child dialog box
  UINT uiMsg,     // message identifier
  WPARAM wParam,  // message parameter
  LPARAM lParam   // message parameter
)
{
	switch(uiMsg)
	{
		case WM_INITDIALOG:
			::SetDlgItemText(::GetParent(hdlg), IDOK, "&Select");
        	break;
	}
	return 0;
}

/*
BOOL CJeditinitDlg::DoDataExchange(BOOL bSendToDialog)
{
	if(bSendToDialog)
	{
		::SetWindowText(m_hwnds[JavaExec], (LPCTSTR)m_strings[JavaExec]);
		SetBatchElements(IsBatchFile());
		::SetWindowText(m_hwnds[JEditWorkingDir], (LPCTSTR)m_strings[JEditWorkingDir]);
		MakeCmdLine();
	}
	else
	{
		for( int i = 0; i < 5; ++i)
		{
			CStringBuf<> p(m_strings[i]);
			::GetWindowText(m_hwnds[i], p, p.size());
		}
	}
	return TRUE;
}
*/
BOOL CJeditinitDlg::DoRegistryExchange(BOOL bSendToDialog)
{
	CRegKey hVersionKey;
	int nResult = hVersionKey.Create(HKEY_CURRENT_USER, szVersion);
	if(nResult != ERROR_SUCCESS)
	{
		return FALSE;
	}
	if(bSendToDialog)
	{
		for(int i = 0; i < 5; ++i)
		{
			unsigned long len = MAX_PATH;
			hVersionKey.QueryValue(CStringBuf<>(m_strings[i]), keyValues[i], &len);
		}
	}
	else
	{
		for(int i = 0; i < 5; ++i)
		{
			hVersionKey.SetValue(m_strings[i], keyValues[i]);
		}
		// JAVA_SERVER_FILE variable parsed from jEdit options
		CString strServerFile;
		MakeServerFilePath(strServerFile);
		hVersionKey.SetValue(strServerFile, _T("jEdit Server File"));
	}
	hVersionKey.Close();
	return TRUE;
}


BOOL CJeditinitDlg::IsJarFile()
{
	TCHAR* pJar = _tcsstr(m_strings[JavaOptions], _T("-jar"));
	return (pJar != 0 &&
			(pJar == (LPCTSTR)m_strings[JavaOptions] || isspace(*(pJar - 1))) &&
			(_tcslen(pJar) == 4 || isspace(*(pJar + 4))));
}


BOOL CJeditinitDlg::IsBatchFile()
{
	TCHAR* pBat = _tcsstr(m_strings[JavaExec], _T(".bat"));
	return(pBat != 0 &&
		(_tcslen(pBat) == 4 ||
			(_tcslen(pBat) == 5 && *(pBat + 4) == _T('\"'))));
}


void CJeditinitDlg::SetJarElements(BOOL bCheck)
{
	CheckDlgButton(IDC_CHECK_JAR, bCheck ? 1 : 0);
	::EnableWindow(GetDlgItem(IDC_BUTTON_JEDIT_TARGET), bCheck);
	::SetWindowText(m_hwnds[JEditTarget], bCheck ?
		(LPCTSTR)m_strings[JEditTarget] : _T("org.gjt.sp.jedit.jEdit"));
	::SendMessage(m_hwnds[JEditTarget], EM_SETREADONLY,
		bCheck ? 0 : 1, 0);
}


void CJeditinitDlg::SetBatchElements(BOOL bCheck)
{
	CheckDlgButton(IDC_CHECK_BATCH, bCheck);
	SetControlReadOnly(JavaOptions, bCheck);
	SetControlReadOnly(JEditTarget, bCheck);
	SetControlReadOnly(JEditOptions, bCheck);
	if(bCheck)
	{
		CheckDlgButton(IDC_CHECK_JAR, 0);
		::SetWindowText(m_hwnds[JavaOptions], _T(""));
		::SetWindowText(m_hwnds[JEditTarget], _T(""));
		::SetWindowText(m_hwnds[JEditOptions], _T(""));
	}
	else
	{
		::SetWindowText(m_hwnds[JavaOptions], (LPCTSTR)m_strings[JavaOptions]);
		SetJarElements(IsJarFile());
		::SetWindowText(m_hwnds[JEditOptions], (LPCTSTR)m_strings[JEditOptions]);
	}
}


void CJeditinitDlg::SetControlReadOnly(int nIndex, BOOL bCheck)
{
	::EnableWindow(m_hwnds[nIndex], !bCheck);
	::SendMessage(m_hwnds[nIndex], EM_SETREADONLY, bCheck, 0);
}


// pDest must be null-terminated and must reside
// in buffer with sufficient capacity
// pDest should not end with whitespace
TCHAR* CJeditinitDlg::trimcpy(TCHAR* pDest, const TCHAR* pSource)
{
	if(*pDest != 0)
		_tcscat(pDest, _T(" "));
	while(*pSource != 0 && isspace(*pSource))
		++pSource;
	const TCHAR *pEnd = pSource + _tcslen(pSource);
	while(pSource != pEnd--)
	{
		if(!isspace(*pEnd))
			break;
	}
	_tcsncat(pDest, pSource, pEnd-pSource+1);
	return pDest;
}


void CJeditinitDlg::MakeCmdLine()
{
	CStringBuf<MAX_PATH*2> strPtr(m_strings[CmdLine]);
	*((LPTSTR)strPtr) = 0;
	for(int i = 0; i < 4; ++i)
	{
		trimcpy(strPtr, m_strings[i]);
	}
	::SetWindowText(m_hwnds[CmdLine], strPtr);
}


BOOL  CJeditinitDlg::ValidateData()
{
	CString strCaption((LPCTSTR)IDS_ERR_CAPTION);
	CString strError;
	CString strTest(m_strings[JavaExec]);

	// check java exec
	if(strTest.IsEmpty())
	{
		strError.LoadString(IDS_ERR_EMPTY_EXEC);
		::MessageBox(m_hWnd, strError, strCaption, MB_ICONEXCLAMATION);
		::SetFocus(GetDlgItem(IDC_EDIT_JAVA_EXEC));
		return FALSE;
	}
	strTest.Remove(_T('\"'));
	if(-1 == GetFileAttributes(strTest))
	{
		strError.Format(IDS_ERR_NOFILE, m_strings[JavaExec]);
		::MessageBox(m_hWnd, strError, strCaption, MB_ICONEXCLAMATION);
		::SetFocus(GetDlgItem(IDC_EDIT_JAVA_EXEC));
		return FALSE;
	}
	strTest.MakeLower();
	if(-1 == strTest.Find(_T(".exe")) && !IsBatchFile())
	{
		strError.Format(IDS_ERR_BADEXEC, m_strings[JavaExec]);
		::MessageBox(m_hWnd, strError, strCaption, MB_ICONEXCLAMATION);
		::SetFocus(m_hwnds[JavaExec]);
		return FALSE;
	}

	// check java target if a -jar file
	if(IsJarFile())
	{
		strTest = m_strings[JEditTarget];
		if(strTest.GetAt(0) == _T('\"'))
		{
			strTest.Remove(_T('\"'));
		}
		if(-1 == GetFileAttributes(strTest))
		{
			strError.Format(IDS_ERR_NOFILE, m_strings[JEditTarget]);
			::MessageBox(m_hWnd, strError, strCaption, MB_ICONEXCLAMATION);
			::SetFocus(m_hwnds[JEditTarget]);
			return FALSE;
		}
		strTest.MakeLower();
		if( -1 == strTest.Find(_T("jedit.jar")))
		{
			strError.Format(IDS_ERR_BADJAR, m_strings[JEditTarget]);
			::MessageBox(m_hWnd, strError, strCaption, MB_ICONEXCLAMATION);
			::SetFocus(m_hwnds[JavaExec]);
			return FALSE;
		}
	}

	// check working directory unless it's empty
	if(m_strings[JEditWorkingDir].GetLength() != 0)
	{
		strTest = m_strings[JEditWorkingDir];
		if(strTest.GetAt(0) == _T('\"'))
		{
			strTest.Remove(_T('\"'));
		}
		DWORD dwAttributes = GetFileAttributes(strTest);
		if(dwAttributes == -1 || (dwAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
		{
			strError.Format(IDS_ERR_BADDIR, m_strings[JEditWorkingDir]);
			::MessageBox(m_hWnd, strError, strCaption, MB_ICONEXCLAMATION);
			::SetFocus(m_hwnds[JEditWorkingDir]);
			return FALSE;
		}
	}

	// warn if edit server is being turned off
	if(-1 != m_strings[JavaOptions].Find(_T("-noserver "))  &&
		IDYES != ::MessageBox(m_hWnd, CString((LPCTSTR)IDS_MSG_CONFIRM_NOSERVER),
			strCaption, MB_YESNO | MB_ICONQUESTION))
	{
		::SetFocus(m_hwnds[JavaOptions]);
		return FALSE;
	}


	return TRUE;
}

// through some parsing and use of defaults, we can generate
// the JAVA_SERVER_FILE variable from the contents of the
// Java Options data without requiring a separate entry

BOOL CJeditinitDlg::MakeServerFilePath(CString& strFileName)
{
	CStringBuf<>strPtr(strFileName);
	ZeroMemory((LPBYTE)strPtr, strPtr.size());
	int nSettingsIndex = m_strings[JEditOptions].Find(_T("-settings="));
	if(nSettingsIndex != -1)
	{
		TCHAR *pDest = (LPTSTR)strPtr;
		const TCHAR *pSrc = (LPCTSTR)m_strings[JEditOptions] + nSettingsIndex + 9;
		while(*++pSrc && isspace(*pSrc));
		TCHAR settingsStop = (*pSrc == _T('\"')) ? _T('\"') : _T(' ');
		if(*pSrc != 0)
			*pDest++ = *pSrc++;
		while(*pSrc && *pSrc != settingsStop)
			*pDest++ = *pSrc++;
		if(*pSrc != _T('\\') || (*pSrc == _T('\"') && *(pSrc - 1) != _T('\\')))
			*pDest = _T('\\');
	}

	int nServerNameIndex = m_strings[JEditOptions].Find(_T("-server="));
	if(nServerNameIndex != -1)
	{
		if(nSettingsIndex == -1)
		{
			// if there is a -server without a -settings, get the default settings
			// directory: %USERPROFILE%  for NT/2000, %HOME% or %WINDIR% for 95/98/Me
			OSVERSIONINFO osver;
			osver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
			GetVersionEx(&osver);
			if(osver.dwPlatformId == VER_PLATFORM_WIN32_NT)
			{
				GetEnvironmentVariableA("USERPROFILE", strPtr, strPtr.size());
			}
			else  // Win95/98/Me
			{
				if(0 == GetEnvironmentVariableA("HOME", strPtr, strPtr.size()))
					GetEnvironmentVariableA("WINDIR", strPtr, strPtr.size());
			}
			_tcscat(strPtr, _T("\\.jedit\\"));
		}
		TCHAR *pDest = (LPTSTR)strPtr + _tcslen(strPtr);
		const TCHAR *pSrc = (LPCTSTR)m_strings[JEditOptions] + nServerNameIndex + 7;
		while(*++pSrc && isspace(*pSrc));
		TCHAR settingsStop = (*pSrc == _T('\"')) ? _T('\"') : _T(' ');
		if(*pSrc != 0)
			*pDest++ = *pSrc++;
		while(*pSrc && *pSrc != settingsStop)
			*pDest++ = *pSrc++;
	}
	else if(nSettingsIndex != -1)
		_tcscat((LPTSTR)strPtr, _T("server"));
	if(*(LPTSTR)strPtr == _T('\"'))
		_tcscat((LPTSTR)strPtr, _T("\""));

	return( nSettingsIndex != -1 || nServerNameIndex != -1);
}
