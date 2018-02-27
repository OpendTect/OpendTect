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


mGlobal(Basic) bool		execProg(const char* comm,
					       const char* parm,
					       const char* runin=0);
mGlobal(Basic) bool		execShellCmd(const char* comm,const char* parm,
					     const char* runin );
mGlobal(Basic) bool		execProc(const char* comm,bool inconsole,
					 bool inbg,const char* runin);

mGlobal(Basic) unsigned int	getWinVersion();
mGlobal(Basic) const char*	getFullWinVersion();
mGlobal(Basic) const char*	getSpecialFolderLocation(int csidl);
mGlobal(Basic) const char*	getCygDir();
mGlobal(Basic) bool		getDefaultBrowser(BufferString& cmd,
						  BufferString& errmsg);

mGlobal(Basic) bool		setRegKeyVal(const char* ky, const char* vanrnm,
					     const char *val);
mGlobal(Basic) bool		removeRegKey(const char*);

} // namespace WinUtils

#endif /* __win__ */
