/*
 * JELInstaller.cpp - part of jEditLauncher package
 * Copyright (C) 2001 John Gellene
 * jgellene@nyc.rr.com
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
 * $Id: JELInstaller.cpp,v 1.11 2001/09/07 15:20:23 jgellene Exp $
 */

#include "stdafx.h"
#include <string.h>
#include "InstallData.h"
#include "DeferFileOps.h"
#include "CtxMenuDlg.h"
#include "StringPtr.h"
#include "InstallerLog.h"
#include "JELRegInstaller.h"
#include "JELInstaller.h"

#include <stdlib.h> // for itoa in debug

/* Implementation of JELFileInstaller */

JELFileInstaller::JELFileInstaller(const InstallData *ptrData,
								   Installer *ptrOwner)
	: pData(ptrData), pOwner(ptrOwner),
	  pDFO(0), bRebootNeeded(false) {}

JELFileInstaller::~JELFileInstaller()
{
	delete pDFO;
}


// if returns S_FALSE; set bRebootRequired to true
// if returns S_OK set strInstallPath to strInstallFinalPath

HRESULT JELFileInstaller::Install()
{
	bRebootNeeded = false;
	DWORD dwError = 0;
	HRESULT hr;
	if(pData->nIndexOverwrittenVer != -1)
	{
		if(pData->strInstallPath.Compare(pData->strOldPath) == 0)
		{
			InstallerLog::Log("Shell extension is up to date: no copying required.\n");
			// nothing to do here
			return S_OK;
		}
		// do copy/overwrite
		InstallerLog::Log("Overwriting shell extension with new version.\n");
		if(!CopyFile(pData->strInstallPath, pData->strInstallFinalPath, FALSE))
		{
			dwError = GetLastError();
			if(ERROR_SHARING_VIOLATION != dwError)
			{
				InstallerLog::Log("Overwrite failed: unspecified error.\n");
				hr = E_FAIL;
			}
			else
			{
				InstallerLog::Log("Overwrite failed: reboot required.\n");
				if(pData->bIsWinNT)
					pDFO = new DeferFileOpsNT;
				else
					pDFO = new DeferFileOps95;
				pDFO->Read();
				pDFO->Add(pData->strInstallFinalPath, pData->strInstallPath);
				if(FAILED(pDFO->Write()))
				{
					InstallerLog::Log("Could not set up file copy after reboot.\n");
					hr = E_FAIL;
				}
				else
				{
					bRebootNeeded = true;
					hr = S_FALSE;
				}
				delete pDFO;
			}
		}
		else hr = S_OK;
	}
	else
	{
		// do move/delete
		InstallerLog::Log("Renaming shell extension to jeshlstb.dll.\n");
		if(!MoveFile(pData->strInstallPath, pData->strInstallFinalPath))
		{
			dwError = GetLastError();
			if(ERROR_SHARING_VIOLATION != dwError)
			{
				InstallerLog::Log("Rename failed: error code %d.\n", dwError);
				hr = E_FAIL;
			}
			else
			{
				InstallerLog::Log("Rename failed: reboot required.\n");
				if(pData->bIsWinNT)
					pDFO = new DeferFileOpsNT;
				else
					pDFO = new DeferFileOps95;
				pDFO->Read();
				pDFO->Add(pData->strInstallFinalPath, pData->strInstallPath);
				if(FAILED(pDFO->Write()))
				{
					InstallerLog::Log("Could not set up file rename after reboot.\n");
					hr = E_FAIL;
				}
				else
				{
					bRebootNeeded = true;
					hr = S_FALSE;
				}
				delete pDFO;
			}
		}
		else
		{
			InstallerLog::Log("Shell extension installed; no reboot required.\n");
			hr = S_OK;
		}
	}
	return hr;
}

HRESULT JELFileInstaller::Uninstall()
{
	bRebootNeeded = false;
	WIN32_DIR_WALK finddata;
	lstrcpy(finddata.cDirMask, pData->strInstallDir);
	lstrcat(finddata.cDirMask, _T("\\*.*"));
	if(pData->bIsWinNT)
	{
		pDFO = new DeferFileOpsNT();
	}
	else
	{
		pDFO = new DeferFileOps95();
	}
	pDFO->Read();
	EmptyDirectory(&finddata, pDFO);
	// remove parent directory
	if(!RemoveDirectory(pData->strInstallDir))
	{
		bRebootNeeded = true;
		pDFO->Add(0, pData->strInstallDir);
	}
	if(bRebootNeeded)
	{
		pDFO->Write();
	}
	delete pDFO;
	return (bRebootNeeded ? S_FALSE : S_OK);
}

void JELFileInstaller::EmptyDirectory(PWIN32_DIR_WALK pFinddata,
								      DeferFileOps *pDFO)
{
	TCHAR *pDirMarker = pFinddata->cDirMask
					+ lstrlen(pFinddata->cDirMask) - 4;

	HANDLE findHandle = FindFirstFile(pFinddata->cDirMask, pFinddata);

	if(findHandle == INVALID_HANDLE_VALUE)
		return;

	BOOL findResult = TRUE;
	while (findResult)
	{
		if(pFinddata->dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)
		{
			// call MakeChildMask/MakeParentMask
			// to recurse into and delete child directories
			int len = lstrlen(pFinddata->cFileName) - 1;
			if(pFinddata->cFileName[len] != _T('.'))
			{
				MakeChildMask(pFinddata->cDirMask, pFinddata->cFileName);
				EmptyDirectory(pFinddata, pDFO);
				TCHAR *pChildDirMarker = pFinddata->cDirMask
					+ lstrlen(pFinddata->cDirMask) - 4;
				*pChildDirMarker = 0;
				if(!RemoveDirectory(pFinddata->cDirMask))
				{
					bRebootNeeded = true;
					pDFO->Add(0, pFinddata->cDirMask);
				}
				*pChildDirMarker = _T('\\');
				MakeParentMask(pFinddata->cDirMask);
			}
		}
		else
		{
			*(pDirMarker + 1) = 0;
			lstrcat(pDirMarker, pFinddata->cFileName);
			if(!DeleteFile(pFinddata->cDirMask))
			{
				bRebootNeeded = true;
				pDFO->Add(0, pFinddata->cDirMask);
			}
			*(pDirMarker + 1) = 0;
			lstrcat(pFinddata->cDirMask, _T("*.*"));
		}
		findResult = FindNextFile(findHandle, pFinddata);
	}
	FindClose(findHandle);
}

bool JELFileInstaller::MakeParentMask(LPTSTR lpszMask)
{
	if(lpszMask == 0)
		return false;
	int nIndexLastNoMask = lstrlen(lpszMask) - 4;
	if(nIndexLastNoMask < 2)
		return false;
	TCHAR *pLastNoMask = lpszMask + nIndexLastNoMask;
	if(*pLastNoMask == _T('\\'))
		*pLastNoMask = 0;
	TCHAR *p = lpszMask;
	TCHAR *s = 0;
	while(*(++p) != 0)
	{
		if(*p == _T('\\'))
			s = p;
	}
	if(s != 0)
		*s = 0;
	lstrcat(lpszMask, _T("\\*.*"));
	return true;
}

bool JELFileInstaller::MakeChildMask(LPTSTR lpszMask, LPCTSTR lpszChildDir)
{
	if(lpszMask == 0 || lpszChildDir == 0)
		return false;
	int nIndexLastSlash = lstrlen(lpszMask) - 3;
	if(nIndexLastSlash < 3)
		return false;
	*(lpszMask + nIndexLastSlash) = 0;
	lstrcat(lpszMask, lpszChildDir);
	lstrcat(lpszMask, _T("\\*.*"));
	return true;
}


HRESULT JELFileInstaller::SendMessage(LPVOID p)
{
	p;
	return E_NOTIMPL;
}

/* Implementation of JELShortcutInstaller */


JELShortcutInstaller::JELShortcutInstaller(const InstallData *ptrData,
										   Installer *ptrOwner)
	: pData(ptrData), pOwner(ptrOwner) {}

JELShortcutInstaller::~JELShortcutInstaller() {}

HRESULT JELShortcutInstaller::Install()
{
	InstallerLog::Log("Installing shortcuts. . .\n");
	// Always install shortcuts for the primary version
	int nIndex = pData->nIndexNewPrimaryVer;
	CString strPrimaryDir = (nIndex != -1) ?
		pData->arPathNames[nIndex] : pData->strInstallDir;
	// Place shortcuts on the "Start" menu and desktop
	CString strFile = strPrimaryDir + "\\unlaunch.exe";
	CString strUninstallIconPath = strFile;
	CString strUninstallDesc((LPCSTR)IDS_SHORTCUT_UNINSTALL);
	CreateShortcut(strFile, strUninstallDesc, strUninstallIconPath,
		ProgramsMenu, strUninstallDesc, strPrimaryDir, 0);
	strFile = strPrimaryDir + "\\jedinit.exe";
	CreateShortcut(strFile, "Set jEdit Parameters", strFile,
		ProgramsMenu, CString((LPCSTR)IDS_SHORTCUT_JEDINIT),
		strPrimaryDir, 0);
	strFile = strPrimaryDir + "\\jedit.exe";
	CString strJEditDesc((LPCSTR)IDS_SHORTCUT_JEDIT);
	CreateShortcut(strFile, "jEdit", strFile, ProgramsMenu,
		strJEditDesc, strPrimaryDir, 0);
	// for primary version
	CreateShortcut(strFile, "jEdit", strFile, StartMenu,
		strJEditDesc, strPrimaryDir, 0);
	CreateShortcut(strFile, "jEdit", strFile, Desktop,
		strJEditDesc, strPrimaryDir, 0);
	return S_OK;
}


HRESULT JELShortcutInstaller::CreateShortcut(LPCSTR lpszPathObj,
 											 LPCSTR lpszLinkName,
											 LPCSTR lpszPathIcon,
											 LinkLocation location,
											 LPCSTR lpszDesc,
											 LPCSTR lpszWorkingDir,
											 LPCSTR lpszParams)
{
	ITEMIDLIST *id;
	char szLinkPath[MAX_PATH];
	ZeroMemory(szLinkPath, sizeof(szLinkPath));
	SHGetSpecialFolderLocation(NULL, location, &id);
	SHGetPathFromIDList(id, szLinkPath);
	if(location == ProgramsMenu)
	{
		char szLinkParent[MAX_PATH];
		strcpy(szLinkParent, szLinkPath);
		strcat(szLinkPath, "\\jEdit");
		if(-1 == GetFileAttributes(szLinkPath)
			&& !CreateDirectoryEx(szLinkParent, szLinkPath, 0))
			return E_FAIL;
	}
	strcat(szLinkPath, "\\");
	strcat(szLinkPath, lpszLinkName);
	strcat(szLinkPath, ".lnk");
	InstallerLog::Log("Creating shortcut at %s.\n", szLinkPath);

    HRESULT hres;
    IShellLink* psl;

    // Get a pointer to the IShellLink interface.
    hres = CoCreateInstance(CLSID_ShellLink, NULL,
        CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID *) &psl);
    if (SUCCEEDED(hres)) {
        IPersistFile* ppf;

        // Set the path to the shortcut target and add the
        // description.
        psl->SetPath(lpszPathObj);
        psl->SetDescription(lpszDesc);
		psl->SetIconLocation(lpszPathIcon, 0);
		psl->SetWorkingDirectory(lpszWorkingDir);
		if(lpszParams)
			psl->SetArguments(lpszParams);

        // Query IShellLink for the IPersistFile interface
	    // for saving the shortcut in persistent storage.
        hres = psl->QueryInterface(__uuidof(IPersistFile), (void**) &ppf);

        if (SUCCEEDED(hres)) {
            WCHAR wsz[MAX_PATH];

            // Ensure that the string is Unicode.
            MultiByteToWideChar(CP_ACP, 0, szLinkPath, -1,
                wsz, MAX_PATH);

            // Save the link by calling IPersistFile::Save.
            hres = ppf->Save(wsz, TRUE);
            ppf->Release();
        }
        psl->Release();
    }
	InstallerLog::Log("Shortcut creation %s.\n",
		(hres == S_OK ? "succeeded" : "failed"));
    return hres;
}

HRESULT JELShortcutInstaller::Uninstall()
{
	if(pData->nIndexNewPrimaryVer != -1)
		return Install();
	DeleteShortcut("\\jEdit.lnk", Desktop);
	DeleteShortcut("\\jEdit.lnk", StartMenu);
	int nResult = DeleteShortcut("\\jEdit\\jEdit.lnk", ProgramsMenu);
	nResult &= DeleteShortcut("\\jEdit\\Set jEdit parameters.lnk", ProgramsMenu);
	nResult &= DeleteShortcut("\\jEdit\\Uninstall jEdit.lnk", ProgramsMenu);
	if(nResult == 1)
	{
		ITEMIDLIST *id;
		char szLinkPath[MAX_PATH];
		ZeroMemory(szLinkPath, sizeof(szLinkPath));
		SHGetSpecialFolderLocation(NULL, ProgramsMenu, &id);
		SHGetPathFromIDList(id, szLinkPath);
		strcat(szLinkPath, "\\jEdit");
		RemoveDirectory(szLinkPath);
	}
	return S_OK;
}

int JELShortcutInstaller::DeleteShortcut(const char* szName,
										 LinkLocation location)
{
	// delete shortcuts if resolves to files in current directory
	//
	ITEMIDLIST *id;
	char szLinkPath[MAX_PATH];
	ZeroMemory(szLinkPath, sizeof(szLinkPath));
	SHGetSpecialFolderLocation(NULL, location, &id);
	SHGetPathFromIDList(id, szLinkPath);
	strcat(szLinkPath, szName);
	if(-1 == GetFileAttributes(szLinkPath))
		return 1;
	// does this resolve to current directory?

	IShellLink *psl;
	HRESULT hres;
	WIN32_FIND_DATA wfd;
	char szTargetPath[MAX_PATH];
	IPersistFile *ppf;

	hres = CoCreateInstance(CLSID_ShellLink, NULL,
		CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID *)&psl);
	if (SUCCEEDED(hres))
	{
		hres = psl->QueryInterface(__uuidof(IPersistFile), (LPVOID *)&ppf);
		if (SUCCEEDED(hres))
		{
			WORD wsz[MAX_PATH];
		    MultiByteToWideChar(CP_ACP, 0, szLinkPath, -1, wsz, MAX_PATH);
		    hres = ppf->Load(wsz, STGM_READ);
			if (SUCCEEDED(hres))
			{
				hres = psl->Resolve(0, SLR_NO_UI | SLR_NOSEARCH | SLR_NOTRACK
					| SLR_NOUPDATE);
				if (SUCCEEDED(hres))
				{
				    hres = psl->GetPath(szTargetPath, MAX_PATH,
						(WIN32_FIND_DATA *)&wfd, 0);//SLGP_SHORTPATH );
					if(SUCCEEDED(hres))
					{
						char szDir[MAX_PATH];
						strcpy(szDir, szTargetPath);
						InstallData::ShortenToDirectory(szDir);
						if(strcmp(szDir, pData->arPathNames[pData->nIndexSubjectVer])
							== 0)
						{
							hres = DeleteFile(szLinkPath) ? S_OK : E_FAIL;
						}
					}
				}
				else
				{
					hres = DeleteFile(szLinkPath) ? S_OK : E_FAIL;
				}
			}
			ppf->Release();
		}
		psl->Release();
	}
	return (hres == S_OK) ? 1 : 0;
}

HRESULT JELShortcutInstaller::SendMessage(LPVOID p)
{
	p;
	return E_NOTIMPL;
}


/* Implementation of JELApplicationInstaller */

JELApplicationInstaller::JELApplicationInstaller(const char* ptrJavaHome,
												 const char* ptrInstallDir,
												 BOOL bLeaveFiles)
	: pData(0), pFileInstaller(0), pRegistryInstaller(0),
	  pShortcutInstaller(0), szJavaHome(ptrJavaHome),
	  szInstallDir(ptrInstallDir), leaveFiles(bLeaveFiles), 
	  hrCOM(E_FAIL), pLog(0) 
{
	pLog = new InstallerLog();
}

JELApplicationInstaller::~JELApplicationInstaller()
{
	InstallerLog::Log("End installation.\n");
	delete pLog;
	Cleanup();
}

bool JELApplicationInstaller::Init()
{
	Cleanup();
	hrCOM = CoInitialize(0);
	if(FAILED(hrCOM))
	{
		InstallerLog::Log("Could not initialize COM facilities.\n");
		return false;
	}
	const char *pEmpty = _T("");
	if(szJavaHome == 0)
		szJavaHome = pEmpty;
	pData = new InstallData(szJavaHome, szInstallDir);
	if(pData == 0 || !pData->Init())
	{
		InstallerLog::Log("Could not initialize installation data.\n");
		return false;
	}
	pFileInstaller = new JELFileInstaller(pData, this);
	pRegistryInstaller = new JELRegistryInstaller(pData, this);
	pShortcutInstaller = new JELShortcutInstaller(pData, this);
	return pFileInstaller != 0 && pRegistryInstaller != 0
		&& pShortcutInstaller != 0;
}


void JELApplicationInstaller::Cleanup()
{
	delete pData;
	delete pFileInstaller;
	delete pRegistryInstaller;
	delete pShortcutInstaller;
	pData = 0;
	pFileInstaller = 0;
	pRegistryInstaller = 0;
	pShortcutInstaller = 0;
	if(hrCOM == S_OK)
	{
		CoUninitialize();
		hrCOM = E_FAIL;
	}
}

HRESULT JELApplicationInstaller::Install()
{
	InstallerLog::Log("Commencing install. . . .\n");
	if(!Init())
		return E_FAIL;
	HRESULT hrFile = pFileInstaller->Install();
	if(hrFile == E_FAIL)
		return E_FAIL;
	HRESULT hr = pRegistryInstaller->Install();
	if(hr == E_FAIL)
	{
		// recover by undoing PendingFileOperations
		return E_FAIL;
	}
	hr = pShortcutInstaller->Install();
	if(hrFile == S_FALSE)
	{
		InstallerLog::Log("Reboot required to complete installation.\n");
		QueryReboot();
	}
	CString strMsg(_T("Installation of jEditLauncher "));
	strMsg += hr == E_FAIL ? _T("failed.") : _T("was successful.");
	MessageBox(0, strMsg, "jEditLauncher",
		hr == E_FAIL ? MB_ICONERROR : MB_ICONINFORMATION);
	InstallerLog::Log("%s\n", (LPCTSTR)strMsg);
	return hr;
}


HRESULT JELApplicationInstaller::Uninstall()
{
	if(!Init())
		return E_FAIL;
	// no logging of uninstall
	delete pLog;
	CString strConfirm;
	// Note: resource file has commented version of query
	// for use when full uninstall is activated
	UINT nMsg = leaveFiles ? IDS_QUERY_UNREGISTER : IDS_QUERY_UNINSTALL_FULL;
	strConfirm.Format(nMsg, pData->strInstallVersion, pData->strInstallDir);
	if(IDOK != MessageBox(0, strConfirm, pData->strApp,
		MB_ICONQUESTION | MB_OKCANCEL | MB_DEFBUTTON2))
	{
		MessageBox(0, "The uninstallation of jEditLauncher was cancelled.",
			"jEditLauncher", MB_ICONINFORMATION);
		return S_FALSE;
	}
	HRESULT hr = pRegistryInstaller->Uninstall();
	hr |= pShortcutInstaller->Uninstall();
	// File deletion is disabled when uninstall
	// is invoked in jedit.exe and
	// enabled when doing full uninstall with unlaunch.exe
	HRESULT hrFile = S_OK;
	if(!leaveFiles)
		hrFile = pFileInstaller->Uninstall();
	if(hrFile == E_FAIL)
		return E_FAIL;
	if(hrFile == S_FALSE)
	{
		QueryReboot();
	}
	CString strMsg(_T("The uninstallation of jEditLauncher "));
	strMsg += hr == E_FAIL ? _T("failed.") : _T("was successful.");
	MessageBox(0, strMsg, "jEditLauncher",
		hr == E_FAIL ? MB_ICONERROR : MB_ICONINFORMATION);
	return hr;
}


HRESULT JELApplicationInstaller::SendMessage(LPVOID p)
{
	// message is from registry installer
	LPDWORD pMsg = (LPDWORD)p;
	int nVersion = (int)(*pMsg & 0xFFFF);
	int nMessage = (int)((*pMsg & 0xFFFF0000) >> 16);
	DWORD dwChangedVersion = (1 << nVersion);
	switch(nMessage)
	{
		case MsgChangePrimary:
			pData->nIndexNewPrimaryVer = nVersion;
			break;
		case MsgInstall:
			pData->dwInstalledVersions |= dwChangedVersion;
			break;
		case MsgUninstall:
			--(pData->nInstalledVersions);
			pData->dwInstalledVersions &= ~dwChangedVersion;
			if(pData->nInstalledVersions == 0)
				pData->nIndexNewPrimaryVer = -1;
	}
	return S_OK;
}


BOOL WINAPI ExitWindowsNT(DWORD dwOptions);
BOOL WINAPI ExitWindows9x(DWORD dwOptions);
#if !defined(EWX_FORCEIFHUNG)
#define EWX_FORCEIFHUNG      0x00000010
#endif

void JELApplicationInstaller::QueryReboot()
{
	BOOL bRet = FALSE;
	if(IDYES == MessageBox(0, CString((LPCSTR)IDS_QUERY_REBOOT),
		pData->strApp, MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON1))
	{
		// reboot routine:
		if(pData->bIsWinNT)
			bRet = ExitWindowsNT(EWX_REBOOT | EWX_FORCEIFHUNG);
		else
			bRet = ExitWindows9x(EWX_REBOOT | EWX_FORCE);
		if(!bRet)
		{
			MessageBox(0, "The system could not reboot. Please do so as soon as possible.",
				"jEditLauncher", MB_ICONEXCLAMATION);
		}
	}
	else
	{
		MessageBox(0, "Please reboot to complete installation as soon as possible.",
			"jEditLauncher", MB_ICONEXCLAMATION);
	}
}

// signatures for NT functions used by ExitWindowsNT()

typedef BOOL(WINAPI *OPTFunc)(HANDLE, DWORD, PHANDLE);
typedef BOOL(WINAPI *LPVFunc)(LPCTSTR, LPCTSTR, PLUID);
typedef BOOL(WINAPI *ATPFunc)(HANDLE, BOOL, PTOKEN_PRIVILEGES,
							  DWORD, PTOKEN_PRIVILEGES, PDWORD);

BOOL WINAPI ExitWindowsNT(DWORD dwOptions)
{
	// set shutdown privilege for current process
	HANDLE hToken;
	TOKEN_PRIVILEGES tkp;

	HINSTANCE hModule = ::LoadLibrary("advapi32.dll");
	if(hModule == 0)
		return FALSE;
	OPTFunc optProc = (OPTFunc)GetProcAddress(hModule, "OpenProcessToken");
	LPVFunc lpvProc = (LPVFunc)GetProcAddress(hModule, "LookupPrivilegeValueA");
	ATPFunc atpProc = (ATPFunc)GetProcAddress(hModule, "AdjustTokenPrivileges");
	DWORD dwError = GetLastError();

	BOOL bRet = (optProc != 0) && (optProc)(GetCurrentProcess(),
		TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)
		&& (lpvProc != 0) && (atpProc != 0);
	if(!bRet)
	{
		::FreeLibrary(hModule);
		return FALSE;
	}
	(lpvProc)(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);
	tkp.PrivilegeCount = 1;  // one privilege to set
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	(atpProc)(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);
	::FreeLibrary(hModule);
	return GetLastError() == ERROR_SUCCESS
		&& ExitWindowsEx(dwOptions, 0);
}


#include <Tlhelp32.h>

// signatures for functions ised by ExitWindows9x()

typedef HANDLE (WINAPI *CTHFunc)(DWORD, DWORD);
typedef BOOL (WINAPI *PFFunc)(HANDLE, PPROCESSENTRY32);
typedef BOOL (WINAPI *PNFunc)(HANDLE, PPROCESSENTRY32);

BOOL WINAPI ExitWindows9x(DWORD dwOptions)
{
	if (dwOptions & EWX_FORCE)
	{
		// Terminate all processes named "Explorer" before calling
		// ExitWindowEx() to prevent log-in window from appearing

		HMODULE hModule = LoadLibrary("kernel32.dll");
		if(hModule == 0)
			return FALSE;
		CTHFunc ctsProc = (CTHFunc)GetProcAddress(hModule, "CreateToolhelp32Snapshot");
		PFFunc pfProc = (PFFunc)GetProcAddress(hModule, "Process32First");
		PNFunc pnProc = (PNFunc)GetProcAddress(hModule, "Process32Next");
		if( ctsProc == 0 || pfProc == 0 || pnProc == 0)
		{
			::FreeLibrary(hModule);
			return FALSE;
		}

		// ToolHelp snapshot of the system's processes.
		HANDLE hSnapShot = (ctsProc)(TH32CS_SNAPPROCESS, 0);
		if( hSnapShot == INVALID_HANDLE_VALUE )
		{
			::FreeLibrary(hModule);
			return FALSE;
		}

		// Get process information and terminate "explorer.exe"
		PROCESSENTRY32 processEntry;
		processEntry.dwSize = sizeof(PROCESSENTRY32);
		BOOL bFound = (pfProc)(hSnapShot, &processEntry);
		while(bFound)
		{
			HANDLE hProcess;
			int nPos = lstrlen(processEntry.szExeFile);
			if (nPos)
			{
				while (processEntry.szExeFile[--nPos] != '\\' );
				if(!lstrcmpi("explorer.exe", &(processEntry.szExeFile[nPos+1])))
				{
					// Terminate the process.
					hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE,
                                           processEntry.th32ProcessID);
					TerminateProcess(hProcess, 1);
					CloseHandle(hProcess);
				}
			}
			processEntry.dwSize = sizeof(PROCESSENTRY32) ;
			bFound = (pnProc)(hSnapShot, &processEntry);
		}
		CloseHandle(hSnapShot);
		FreeLibrary(hModule);
	}
	return ExitWindowsEx(dwOptions, 0);
}
