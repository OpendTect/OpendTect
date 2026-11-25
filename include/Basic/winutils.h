#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"

#include "commondefs.h"

class BufferString;

#ifdef __win__
# include "shtypes.h"
# include "minwindef.h"
# include <ctime>
# include <iostream>
#endif

extern "C"
{
    mGlobal(Basic) const char*	getCleanUnixPath(const char* path);
    mGlobal(Basic) const char*	getCleanWinPath(const char* path);
}

mGlobal(Basic) bool		readRegKeyVal(const char* ky,const char* varnm,
					      BufferString&);
mGlobal(Basic) bool		setRegKeyVal(const char* ky, const char* vanrnm,
					     const char *val);
mGlobal(Basic) bool		removeRegKey(const char*);
mGlobal(Basic) bool		removeRegKeyVal(const char* ky,
						const char* varnm);

#ifdef __win__

extern "C"
{
    mGlobal(Basic) const char*  GetKnownFolderLocation(REFKNOWNFOLDERID);
	/*!< Returns the path to a known folder. see Knownfolders.h */
    mDeprecated("Use GetKnownFolderLocation")
    mGlobal(Basic) const char*	GetSpecialFolderLocation(int csidl);
}

mGlobal(Basic) bool		winCopy(const char* from,const char* to,
					bool isfile,bool ismove=false);
mGlobal(Basic) bool		winRemoveDir(const char* dirnm);

namespace WinUtils {

mGlobal(Basic) bool		isFileInUse(const char* fnm);
mGlobal(Basic) bool		belongsToStdUser(const char* fnm);
mGlobal(Basic) bool		belongsToAdmin(const char* fnm);
mGlobal(Basic) bool		belongsToTrusterInstaller(const char* fnm);
mGlobal(Basic) bool		pathContainsTrustedInstaller(const char* fnm);

				/*<! Depends on the running process
				    Does not return true if the process is
				    not start with elevated privileges */
mGlobal(Basic) bool		IsUserAnAdmin();
mGlobal(Basic) bool		NTUserBelongsToAdminGrp();
mGlobal(Basic) void		GetUserName(BufferString&);
				/*!< Retrieves the user name running
				     the current process */
mGlobal(Basic) bool		GetSessionUserNm(PSID,BufferString&);
mGlobal(Basic) bool		GetDisplayName(PSID,BufferString&);
mGlobal(Basic) bool		GetUserName(const char* fnm,BufferString& owner,
					    BufferString* groupnm,
					    bool resolve=true);
mGlobal(Basic) bool		resolveSessionUserNm(const char*,BufferString&);
mGlobal(Basic) bool		serviceIsRunning(const char* nm);
				/* See SERVICE_STATUS_PROCESS.dwCurrentState
				    for possible values*/
mGlobal(Basic) int		getServiceStatus(const char* nm);
mGlobal(Basic) void		TimespecToFileTime(const std::timespec&,
						   FILETIME&);
mGlobal(Basic) void		FileTimeToTimet(const FILETIME&,std::timespec&);

mGlobal(Basic) void		copyWString(LPCTSTR,BufferString&);
mGlobal(Basic) bool		writeUtf8ToStdConsole(const char* txt,
						      std::ostream&);
mGlobal(Basic) void		useUtf8StdConsoleInput();

}

#define mDeprecatedMachCmd mDeprecated("Use MachineCommand")

mDeprecatedMachCmd mGlobal(Basic) bool executeWinProg(const char* comm,
					       const char* parm,
					       const char* runin=0);
mDeprecatedMachCmd mGlobal(Basic) bool execShellCmd(const char* comm,
						const char* parm,
						const char* runin );
mDeprecatedMachCmd mGlobal(Basic) bool execProc(const char* comm,bool inconsole,
					 bool inbg,const char* runin);

mGlobal(Basic) const char*	getWinVersion();
mGlobal(Basic) const char*	getWinMinorVersion();
mGlobal(Basic) const char*	getFullWinVersion();
mGlobal(Basic) const char*	getWinBuildNumber();
mGlobal(Basic) const char*	getWinDisplayName();
mGlobal(Basic) const char*	getWinEdition();
mGlobal(Basic) const char*	getWinProductName();
mGlobal(Basic) bool		IsWindowsServer();

mGlobal(Basic) bool		canHaveAppLocker();
mGlobal(Basic) bool		hasAppLocker();
mGlobal(Basic) bool		getDefaultBrowser(BufferString& cmd,
						  BufferString& errmsg);
mGlobal(Basic) bool		getDefaultApplication( const char* scheme,
					BufferString& cmd,
					BufferString& errmsg);

mDeprecated("Use readRegKeyVal")
mGlobal(Basic) bool		readKey(const HKEY,const char* path,
					const char* ky,BufferString&,
					LPDWORD dwFlags=NULL,
					LPDWORD dwType=NULL);

mGlobal(Basic) void		disableAutoSleep();
mGlobal(Basic) void		enableAutoSleep();

#endif // __win__
