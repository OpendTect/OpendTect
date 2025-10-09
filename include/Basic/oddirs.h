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

class BufferStringSet;

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

mGlobal(Basic) const char* GetLibraryFnm(const void* fn);
/*!< returns the full path to the runtime library implementing a function
     Should not be called/used when building a static library */

namespace OD
{

mGlobal(Basic) bool isDeveloperBuild();
/*!< This is running from a developers build. Checks the presence of
     CMakeCache.txt/CMakeFiles/external at the location of GetSoftwareDir */

mGlobal(Basic) bool isDeveloperInstallation();
/*!< This is running from an installed developers build. Checks the presence of
     CMakeCache.txt/CMakeFiles/external at the location of GetSoftwareDir */

} // namespace OD

mGlobal(Basic) const char* GetBinSubDir(void);
/*!< Directory for the release-type, i.e. Debug, Release, ... */

typedef enum
{
    ODSetupLoc_ApplSetupOnly,
    ODSetupLoc_ApplSetupPref, /* Usual choice for GetSetupDataFileName */
    ODSetupLoc_SWDirPref,
    ODSetupLoc_SWDirOnly,
    ODSetupLoc_UserPluginDirOnly
} ODSetupLocType;

mGlobal(Basic) bool addCustomShareFolder(const char* path);
/*!< Adds a share folder that is treated as an additional lookup folder for
     the enum values ODSetupLoc_SWDirPref and ODSetupLoc_SWDirOnly */

mGlobal(Basic) const char* GetSetupDataFileDir(
					ODSetupLocType=ODSetupLoc_ApplSetupPref,
					bool acceptnone=false);
/*!< Returns the name of the "share" subdir of the release or the
     site setup directory.
     If acceptnone is false, program will terminate if none is found.
*/

mGlobal(Basic) const char* GetSWDirDataDir();
//!< GetSetupDataFileDir for ODSetupLoc_SWDirOnly
mGlobal(Basic) const char* GetApplSetupDataDir();
//!< GetSetupDataFileDir for ODSetupLoc_ApplSetupOnly
mGlobal(Basic) const char* GetUserPluginDataDir();
//!< GetSetupDataFileDir for ODSetupLoc_UserPluginDirOnly

mGlobal(Basic) const char* GetSetupShareFileName(const char* filenm,
				ODSetupLocType=ODSetupLoc_ApplSetupPref,
				bool acceptnone=true);
/*!< Returns the name of a file or directory in the "share" subdir of the
     release or the appl setup directory.
     By default, first look in ApplSetup, if not there, look in release .

     For the 'Pref' types, it returns the first existing file.
     If acceptnone is false, program will terminate if none is found.
*/

mGlobal(Basic) const char* GetSWSetupShareFileName(const char* filenm,
						   bool acceptnone=false);
/*!< GetSetupShareFileName for ODSetupLoc_SWDirOnly
     Exclusively for resource files which are no configuration */

mGlobal(Basic) bool GetSetupShareFileNames( const char* searchkey,
					    BufferStringSet&,
					    bool acceptnone=false);
//!< Returns a list of file names matching a pattern in the 'share' directories

mGlobal(Basic) bool GetSetupShareDirNames( const char* searchkey,
					   BufferStringSet&,
					   bool acceptnone=false);
//!< Returns a list of folder names matching a pattern in the 'share' dirs

mGlobal(Basic) const char* GetSetupShareFileInDir( const char* subdir,
						   const char* filenm,
						   bool acceptnone=false);
/*!< Returns the name of a file or folder in a subfolder
     of the 'share' directories */

mGlobal(Basic) bool GetSetupShareFilesInDir( const char* subdir,
					     const char* searchkey,
					     BufferStringSet&,
					     bool acceptnone=false);
/*!< Returns a list of files names matching a pattern in a sub directory of
     the 'share' directories */

mGlobal(Basic) const char* GetPlfSubDir(void);
/*!< Platform subdirectory for platforms

  Tries PLFSUBDIR, if not set: binsubdir, if not set: HDIR
  Ouput medio 2009 is one of lux32, lux64, sol32, mac, win32 or win64.
*/


mGlobal(Basic) const char* GetExecPlfDir(void);
/*!< Platform dependent executable directory
  Returns full path to bin dir:
  GetSoftwareDir()/bin/GetPlfSubDir()/GetBinSubDir() on Windows and Linux,
  and GetSoftwareDir()/Contents/MacOS on macOS.
*/

mGlobal(Basic) const char* GetScriptDir(void);
/*!< Platform dependent script directory
  Returns full path to bin dir:
  GetSoftwareDir()/bin on Windows and Linux,
  and GetSoftwareDir()/Contents/Resources/bin on macOS.
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

mGlobal(Basic) bool addCustomDocFolder(const char* path);
/*!< Add a custom location for the Documentation */

mGlobal(Basic) const char* GetDocFileDir(const char* filedir);
/*!< Location of Documentation */

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

mGlobal(Basic) const char* GetInterpreterName(void);
/*!< Returns $DTECT_USER. Incase $DTECT_USER is null, returns user's name */

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

mGlobal(Basic) const char* GetLogsDir(void);
/*!< $HOME\AppData\Local\[ORGNAME]\[APPNAME]\logs on Windows
     $HOME/.local/share/[ORGNAME]/[APPNAME]/logs on Linux
     $HOME/Library/Application Support/[DOMAIN]/[APPNAME]/logs on macOS
     The organization name, domain and the application name values are read
     from the ApplicationData class, and may thus vary with the application */

mGlobal(Basic) const char* GetDownloadsDir(void);
/*!< User's Downloads directory, typically $HOME/Downloads. */

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


// Deprecated functions and macros

mDeprecated("Use GetApplSetupDataDir()")
mGlobal(Basic) const char* GetApplSetupDir(void);
/*!< Directory with setup files and scripts overruling current software
     release's default setup files.

    Tied to environment DTECT_[WIN]APPL_SETUP. If
    the environment is not set (see GetEnvVar), this function returns null.
*/

mDeprecated("Use GetSetupShareFileName")
mGlobal(Basic) const char* GetSetupDataFileName(ODSetupLocType,const char*,
					 bool acceptnone);

#define mGetSWDirDataDir() GetSWDirDataDir()
#define mGetApplSetupDataDir() GetApplSetupDataDir()
#define mGetUserPluginDataDir() GetUserPluginDataDir()

#define mGetSetupFileName(x) GetSetupDataFileName(ODSetupLoc_ApplSetupPref,x,0)
/*!< Old macro, use GetSetupShareFileName */

#define mGetUserDocDir()	GetDocFileDir("userdoc")
#define mGetProgrammerDocDir()	GetDocFileDir("Programmer")
#define mGetSysAdmDocDir()	GetDocFileDir("SysAdm")

} // extern "C"
