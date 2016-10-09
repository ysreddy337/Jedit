/*
 * LauncherLog.h - part of jEditLauncher package
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
 * $Id: LauncherLog.h,v 1.1 2002/01/18 19:31:55 jgellene Exp $
 */

#include "..\ltslog\ltslog.h"

#if !defined(__LAUNCHER_LOG_H__)
#define __LAUNCHER_LOG_H__

class LauncherLog
{

	/* Operations */
public:
	static void Init();
	static void Log(MsgLevel level, const char* lpszFormat, ...);
	static void Exit();

	/* Implementation */
protected:
private:
	static HMODULE hLogModule;
	static LogArgsFunc logProc;
	static CLtslog* pLog;

	/* No copying */
private:
	LauncherLog();
	~LauncherLog();
	LauncherLog(const LauncherLog&);
	LauncherLog& operator=(const LauncherLog&);
};



#endif        //  #if !defined(__LAUNCHER_LOG_H__)

