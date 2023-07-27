#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#ifndef gendefs_h
#include "basicmod.h"
#include "gendefs.h"
#endif

extern "C" {

/* Functions delivering files/directories in the 'survey data' scope */

mGlobal(Basic) const char* GetBaseDataDir(void);
/*!< Base data directory: DTECT_[WIN]DATA or from User Settings. */

mGlobal(Basic) const char* GetDataDir(void);
/*!< Survey directory: GetBaseDataDir()/\<current survey\> */

mGlobal(Basic) const char* GetProcFileName(const char*);
/*!< Returns file name in GetDataDir()/Proc. Pass null for directory. */


/* Functions delivering files/directories in the 'system' scope */

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
/*!< Platform dependent script directory
  Returns full path to bin dir:
  GetSoftwareDir()/bin on Windows and Linux,
  and GetSoftwareDir()/Contents/MacOS on Mac.
*/

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

mGlobal(Basic) inline const char* GetBinPlfDir(void)
{ return GetExecPlfDir(); }
//!<Old don't use

mGlobal(Basic) const char* GetDocFileDir(const char* filedir);
/*!< Location of Documentation */
#define mGetUserDocDir()	GetDocFileDir("userdoc")
#define mGetProgrammerDocDir()	GetDocFileDir("Programmer")
#define mGetSysAdmDocDir()	GetDocFileDir("SysAdm")


mGlobal(Basic) const char* GetExecScript(int remote);
/*!< Location of launch script for external programs

  In GetSiteDataDir() or GetSoftwareDir(): bin/od_exec[_rmt][.bat]
*/
#define		mGetExecScript()	GetExecScript(0)

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


    /* Functions delivering files/directories in the 'user-specific' scope */
mGlobal(Basic) const char* GetPersonalDir(void);
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

mGlobal(Basic) const char* GetSettingsDir(void);
/*!< Directory with the user settings

  Returns GetPersonalDir()/.od, unless DTECT_PERSONAL_DIR is set.

*/

mGlobal(Basic) const char* GetSettingsFileName(const char*);
/*!< Returns GetSettingsDir()/filenm */


mGlobal(Basic) const char* GetScriptsDir();
mGlobal(Basic) const char* GetScriptsLogDir();
mGlobal(Basic) const char* GetScriptsPicturesDir();

mGlobal(Basic) const char* GetSurveyExportDir();
mGlobal(Basic) const char* GetSurveyPicturesDir();
mGlobal(Basic) const char* GetSurveyScriptsDir();
mGlobal(Basic) const char* GetSurveyScriptsLogDir();
mGlobal(Basic) const char* GetSurveyTempDir();
mGlobal(Basic) const char* GetSurveyProcDir();

mGlobal(Basic) const char* GetImportFromDir();
mGlobal(Basic) void SetImportFromDir(const char*);
mGlobal(Basic) const char* GetExportToDir();
mGlobal(Basic) void SetExportToDir(const char*);
mGlobal(Basic) const char* GetPicturesDir();
mGlobal(Basic) void SetPicturesDir(const char*);
mGlobal(Basic) void ResetDefaultDirs();


} // extern "C"
