#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Aug 2005
________________________________________________________________________

OpendTect directories.

Beware that all functions may return a pointer to *the same* static buffer!

-*/

#include "basicmod.h"
#include "gendefs.h"

extern "C" {

/* Functions delivering files/directies in the 'survey data' scope */

mGlobal(Basic) const char* GetBaseDataDir(void);
/*!< Base data directory: DTECT_[WIN]DATA or from User Settings. */

mGlobal(Basic) const char* GetDataDir(void);
/*!< Survey directory: GetBaseDataDir()/<current survey> */

mGlobal(Basic) const char* GetProcFileName(const char*);
/*!< Returns file name in GetDataDir()/Proc. Pass null for directory. */

mGlobal(Basic) const char* GetLastSurveyFileName(void);
/*!< File with last visited survey name */

mGlobal(Basic) const char* GetLastSurveyDirName(void);
/*!< last visited survey directory name */


/* Functions delivering files/directies in the 'sytem' scope */

mGlobal(Basic) const char* GetSoftwareDir(bool acceptnone);
/*!< Directory of the installed software = $DTECT_[WIN]APPL

 GetSoftwareDir returns the full path of the root of the release.
 If no DTECT_[WIN]APPL set, the location will be determined from argv[0] or
 the 'bundle' location (Mac only).

 If acceptnone is false, program will terminate if none is found.
*/

mGlobal(Basic) bool isDeveloperBuild();
/*!< Thus is running from a developers build. Checks the presence of
     CMakeCache.txt file at the location of GetSoftwareDir */


mGlobal(Basic) const char* GetBinSubDir(void);
/*!< Directory for the release-type, i.e. Debug, Release, ... */

mGlobal(Basic) const char* GetApplSetupDir(void);
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

mGlobal(Basic) const char* GetSetupDataFileDir(ODSetupLocType,bool acceptnone);
/*!< Returns the name of the "data" subdir of the release or the
     site setup directory.
     If acceptnone is false, program will terminate if none is found.
*/
#define mGetApplSetupDataDir() GetSetupDataFileDir(ODSetupLoc_ApplSetupOnly,0)
#define mGetSWDirDataDir() GetSetupDataFileDir(ODSetupLoc_SWDirOnly,0)

mGlobal(Basic) const char* GetSetupDataFileName(ODSetupLocType,const char*,
					 bool acceptnone);
/*!< Returns the name of a file in the "data" subdir of the release or the
     appl setup directory.

     For the 'Pref' types, it returns the first existing
     file.

     If acceptnone is false, program will terminate if none is found.
*/
#define mGetSetupFileName(x) GetSetupDataFileName(ODSetupLoc_ApplSetupPref,x,0)
/*!< Usual choice: first look in ApplSetup, if not there, look in release */


mGlobal(Basic) const char* GetPlfSubDir(void);
/*!< Platform subdirectory for platforms

  Tries PLFSUBDIR, if not set: binsubdir, if not set: HDIR
  Ouput medio 2009 is one of lux32, lux64, sol32, mac, win32 or win64.
*/


mGlobal(Basic) const char* GetExecPlfDir(void);
/*!< Platform dependent executable directory
  Returns full path to bin dir:
  GetSoftwareDir()/bin/GetPlfSubDir()/GetBinSubDir() on Windows and Linux,
  and GetSoftwareDir()/Contents/MacOS on Mac.
*/

mGlobal(Basic) const char* GetScriptDir(void);
/*!< Returns full path to directory with scripts (def=GetSoftwareDir()/bin). */

mGlobal(Basic) const char* GetShellScript(const char*);
/*!< Returns full path to script, pass null to get the directory */

mGlobal(Basic) const char* GetPythonScript(const char*);
/*!< Returns full path to script if existing. */

mGlobal(Basic) const char* GetLibPlfDir(void);
/*!< Platform dependent library directory
 Returns full path to bin dir:
 GetSoftwareDir()/bin/GetPlfSubDir()/GetBinSubDir() on Windows and Linux,
 and GetSoftwareDir()/Contents/Frameworks on Mac.
 */

mGlobal(Basic) const char* GetDocFileDir(const char* filedir);
/*!< Location of Documentation */
#define mGetUserDocDir()	GetDocFileDir("userdoc")
#define mGetProgrammerDocDir()	GetDocFileDir("Programmer")
#define mGetSysAdmDocDir()	GetDocFileDir("SysAdm")


mGlobal(Basic) const char* GetUnixExecScript();
/*!< Location of exec_prog script to launch od executables externally
  In GetSiteDataDir() or GetSoftwareDir(): bin/od_exec[_rmt][.bat]
*/

mGlobal(Basic) const char* GetODExternalScript();
/* Location of od_external script to launch commands free of any OpendTect
   environment
*/

mGlobal(Basic) const char* GetSoftwareUser(void);
/*!< Sub-user of software: $DTECT_USER

 When multiple people want to run under a single account, they need to
 set DTECT_USER. The output of GetSoftwareUser() will therefore most often
 be null!
*/

mGlobal(Basic) const char* GetUserNm(void);
/*!< user's name */


    /* Functions delivering files/directies in the 'user-specific' scope */
mGlobal(Basic) const char* GetPersonalDir(void);
/*!< Directory for personal settings: 'Home directory'

 This gets the user's home directory. Even on Windows, something will be
 assigned as being the home directory.

Windows:
    DTECT_WINHOME
    DTECT_HOME	-> converted from unix to windows path (cygpath -w)
    HOMEDRIVE+HOMEPATH if exists and not equal to "C:\"
    That should be it. If that still fails, we try HOME, USERPROFILE, APPDATA,
    DTECT_USERPROFILE_DIR. As a last resort, getSpecialFolderLocation is used.

UNIX:
    DTECT_HOME, HOME; in that order.

 Before anything else, DTECT_PERSONAL_DIR is checked. If it's set, that is
 returned. If all else fails, "C:\\" or "/tmp" are returned.

*/

mGlobal(Basic) const char* GetSettingsDir(void);
/*!< Directory with the user settings

  Returns GetPersonalDir()/.od, unless DTECT_PERSONAL_DIR is set.

*/

mGlobal(Basic) const char* GetSettingsFileName(const char*);
/*!< Returns GetSettingsDir()/filenm */


mGlobal(Basic) const char* GetCmdDriverScript(const char*);
/* pass null to get the default CmdDriver script directory */


} // extern "C"
