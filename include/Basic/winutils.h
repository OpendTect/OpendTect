#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Lammertink
 Date:		09-10-2003
 Contents:	Utilities for win32 support
________________________________________________________________________


-*/


#include "basicmod.h"
#include "gendefs.h"

#ifdef __win__

# include "shlobj.h"


namespace WinUtils
{

mGlobal(Basic) bool		copy(const char* from,const char* to,
				     bool isfile,bool ismove=false);
mGlobal(Basic) bool		removeDir(const char* dirnm);
mGlobal(Basic) bool		fileInUse(const char* fnm);
mGlobal(Basic) bool		belongsToStdUser(const char* fnm);
mGlobal(Basic) bool		belongsToAdmin(const char* fnm);
mGlobal(Basic) bool		belongsToTrusterInstaller(const char* fnm);
mGlobal(Basic) bool		pathContainsTrustedInstaller(const char* fnm);

mGlobal(Basic) unsigned int	getWinVersion();
mGlobal(Basic) unsigned int     getWinMinorVersion();
mGlobal(Basic) const char*	getFullWinVersion();
mGlobal(Basic) const char*      getWinBuildNumber();
mGlobal(Basic) const char*      getWinDisplayName();
mGlobal(Basic) const char*	getWinEdition();
mGlobal(Basic) const char*	getWinProductName();

mGlobal(Basic) bool		canHaveAppLocker();
mGlobal(Basic) bool		hasAppLocker();
mGlobal(Basic) bool		serviceIsRunning(const char* nm);
						/* See SERVICE_STATUS_PROCESS.dwCurrentState
							for possible values*/
mGlobal(Basic) int		getServiceStatus(const char* nm);
mGlobal(Basic) const char*	getSpecialFolderLocation(int csidl);
mGlobal(Basic) const char*	getCygDir();
mGlobal(Basic) bool		getDefaultBrowser(BufferString& cmd,
						  BufferString& errmsg);

				/*<! Depends on the running process
				    Does not return true if the process is
				    not start with elevated privileges */
mGlobal(Basic) bool		IsUserAnAdmin();
mGlobal(Basic) bool		NTUserBelongsToAdminGrp();

mGlobal(Basic) bool		setRegKeyVal(const char* ky, const char* vanrnm,
					     const char *val);
mGlobal(Basic) bool		removeRegKey(const char*);
mGlobal(Basic) bool		readKey(const HKEY,const char* path,
					const char* ky,BufferString&,
					LPDWORD dwFlags=NULL,
					LPDWORD dwType=NULL);

} // namespace WinUtils

#endif /* __win__ */
