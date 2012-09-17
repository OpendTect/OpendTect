#ifndef oddirs_h
#define oddirs_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		Aug 2005
 RCS:		$Id: oddirs.h,v 1.21 2010/10/28 06:01:48 cvsranojay Exp $
________________________________________________________________________

OpendTect directories.

Beware that all functions may return a pointer to *the same* static buffer!

-*/

#ifndef gendefs_h
#include "gendefs.h"
#endif

#ifdef __cpp__
extern "C" {
#endif

    /* Functions delivering files/directies in the 'survey data' scope */

mGlobal const char* GetBaseDataDir(void);
/*!< Base data directory: DTECT_[WIN]DATA or from User Settings. */

mGlobal const char* GetDataDir(void);
/*!< Survey directory: GetBaseDataDir()/<current survey> */

mGlobal const char* GetProcFileName(const char*);
/*!< Returns file name in GetDataDir()/Proc. Pass null for directory. */


    /* Functions delivering files/directies in the 'sytem' scope */

mGlobal const char* GetSoftwareDir(int acceptnone);
/*!< Directory of the installed software = $DTECT_[WIN]APPL 

 GetSoftwareDir returns the full path of the root of the release.
 If no DTECT_[WIN]APPL set, the location will be determined from argv[0] or
 the 'bundle' location (Mac only).

 If acceptnone is false, program will terminate if none is found.
*/

mGlobal const char* GetApplSetupDir(void);
/*!< Directory with setup files and scripts overruling current software
     release's default setup files.
    
    Tied to environment DTECT_[WIN]APPL_SETUP. If
    the environment is not set (see GetEnvVar), this function returns null.
*/

typedef enum
{
    ODSetupLoc_ApplSetupOnly,
    ODSetupLoc_ApplSetupPref, /* Usual choice for GetSetupDataFileName */
    ODSetupLoc_SWDirPref,
    ODSetupLoc_SWDirOnly
} ODSetupLocType;

mGlobal const char* GetSetupDataFileDir(ODSetupLocType,int acceptnone);
/*!< Returns the name of the "data" subdir of the release or the
     site setup directory.
     If acceptnone is false, program will terminate if none is found.
*/
#define mGetApplSetupDataDir() GetSetupDataFileDir(ODSetupLoc_ApplSetupOnly,0)
#define mGetSWDirDataDir() GetSetupDataFileDir(ODSetupLoc_SWDirOnly,0)

mGlobal const char* GetSetupDataFileName(ODSetupLocType,const char*,
					 int acceptnone);
/*!< Returns the name of a file in the "data" subdir of the release or the
     appl setup directory.

     For the 'Pref' types, it returns the first existing
     file.

     If acceptnone is false, program will terminate if none is found.
*/
#define mGetSetupFileName(x) GetSetupDataFileName(ODSetupLoc_ApplSetupPref,x,0)
/*!< Usual choice: first look in ApplSetup, if not there, look in release */

mGlobal const char* GetPlfSubDir(void);
/*!< Platform subdirectory for platforms

  Tries PLFSUBDIR, if not set: binsubdir, if not set: HDIR
  Ouput medio 2009 is one of lux32, lux64, sol32, mac, win32 or win64.
*/

mGlobal const char* GetBinPlfDir(void);
/*!< Platform dependent bin directory
  Returns full path to bin dir: GetSoftwareDir()/bin/GetPlfSubDir
*/

mGlobal const char* GetDocFileDir(const char* filedir);
/*!< Location of Documentation */
#define mGetUserDocDir()	GetDocFileDir("User")
#define mGetProgrammerDocDir()	GetDocFileDir("Programmer")
#define mGetSysAdmDocDir()	GetDocFileDir("SysAdm")


mGlobal const char* GetExecScript(int remote);
/*!< Location of launch script for external programs
  
  In GetSiteDataDir() or GetSoftwareDir(): bin/od_exec[_rmt][.bat]
*/
#define		mGetExecScript()	GetExecScript(0)

mGlobal const char* GetSoftwareUser(void);
/*!< Sub-user of software: $DTECT_USER

 When multiple people want to run under a single account, they need to
 set DTECT_USER. The output of GetSoftwareUser() will therefore most often
 be null!
*/

mGlobal const char* GetUserNm(void);
/*!< user's name */


    /* Functions delivering files/directies in the 'user-specific' scope */
mGlobal const char* GetPersonalDir(void);
/*!< Directory for personal settings: 'Home directory'

 This gets the user's home directory. Even on Windows, something will be
 assigned as being the home directory.

Windows:
    DTECT_WINHOME
    DTECT_HOME	-> converted from unix to windows path (cygpath -w)
    HOMEDRIVE+HOMEPATH if exists and not equal to "C:\"
    That should be it. If that still fails, we try HOME, USERPROFILE, APPDATA,
    DTECT_USERPROFILE_DIR. As a last resort, GetSpecialFolderLocation is used.

UNIX:
    DTECT_HOME, HOME; in that order.

 Before anything else, DTECT_PERSONAL_DIR is checked. If it's set, that is
 returned. If all else fails, "C:\\" or "/tmp" are returned.

*/

mGlobal const char* GetSettingsDir(void);
/*!< Directory with the user settings

  Returns GetPersonalDir()/.od, unless DTECT_PERSONAL_DIR is set.

*/

mGlobal const char* GetSettingsFileName(const char*);
/*!< Returns GetSettingsDir()/filenm */


mGlobal const char* GetScriptsDir(const char*);


#ifdef __cpp__
}
#endif

#endif
