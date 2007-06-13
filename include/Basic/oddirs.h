#ifndef oddirs_h
#define oddirs_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		Aug 2005
 RCS:		$Id: oddirs.h,v 1.7 2007-06-13 16:29:34 cvsbert Exp $
________________________________________________________________________

For historic reasons, also dGB_ instead of DTECT_ or OD_ will be scanned.

Beware that all functions may return a pointer to _the_ _same_ static
buffer of length PATH_LENGTH!

-*/

#ifndef gendefs_H
#include "gendefs.h"
#endif

#ifdef __cpp__
extern "C" {
#endif


/*! Directory of the installed software = $DTECT_APPL 

 GetSoftwareDir returns the full path of the root of where the executables,
 program data, etc. is located.

 On windows:
    DTECT_WINAPPL
 On unix:
    DTECT_APPL

 If that fails, it will try to extract the location out of argv[0] 

*/
const char*	GetSoftwareDir(void);


/*! Site-specific site directory with setup files and scripts overruling
    current software release's. Tied to environment DTECT_SITE_DATA. If
    the environment is not set (see GetEnvVar), this function returns null.
*/

const char*	GetSiteDataDir(void);


/*! Platform subdirectory for platforms

  Tries PLFSUBDIR, if not set: binsubdir, if not set: HDIR
 
 */
const char*	GetPlfSubDir(void);


/*! Location of launch script for external programs

    local:
	GetSoftwareDir()/bin/od_exec     on *nix
	GetSoftwareDir()\bin\od_exec.bat on win32
    remote:
	GetSoftwareDir()/bin/od_exec_rmt     on *nix
	GetSoftwareDir()\bin\od_exec_rmt.bat on win32

    In the above, if a GetSiteDataDir()/bin/od_xx script exists, this will be
    returned.

*/
const char*	GetExecScript(int remote);
#define		mGetExecScript()	GetExecScript(0)

/*! DTECT_USER

 returns the value as set in DTECT_USER.
 This may be null - in fact it is most often null!

 /sa GetSoftwareDir(void)

*/
const char*	GetSoftwareUser(void);


/*! gets user's home directory.

 This gets the user's home directory, if available.

 Since this is not guaranteed on Windows, it is recommended
 to use GetSettingsDir() or GetPersonalDir(), which will
 map to "Application Data" resp. "My Documents" if a home directory
 is not availabile.

 The home directory is linked to environment variables:

 On windows:

    DTECT_WINHOME
    DTECT_HOME	-> converted from unix to windows path (cygpath -w)
    HOME	-> ""
    HOMEDRIVE+HOMEPATH if exists and _not_ equal to "C:\"

    else returns 0;

 in that order.

 On unix:

    DTECT_HOME
    HOME

 in that order.

*/
const char*	_GetHomeDir(void);


/*! Home dir or "Application Data"

  Returns OD_SETTINGS_DIR if set.

  else:
  
    On Unix: returns _GetHomeDir();

    On Windows:
	returns 

	    if OD_PREFER_HOME is set: _GetHomeDir()
	    APPDATA
	    DTECT_APPLICATION_DATA
	    _GetHomeDir()
	    location of "Application Data"

    in that order.

*/
const char*	GetSettingsDir(void);


/*! Home dir or "My Documents"

  Returns OD_PERSONAL_DIR if set.
  else returns _GetHomeDir() if set.

  else, on Windows:
    returns 

	USERPROFILE
	DTECT_USERPROFILE_DIR
	DTECT_MYDOCUMENTS_DIR
	location of "User Profile"

    in that order.

*/
const char*	GetPersonalDir(void);


const char*	GetDataFileName(const char*);
		/*!< Application data file in $DTECT_SITE_DATA/data (if set),
		     or $DTECT_APPL/data. Pass null for the $DTECT_APPL/data
		     directory itself. */

const char*	GetProcFileName(const char*);
		/*!< Job processing spec file in $DTECT_DATA/Proc.
		  	Pass null for directory name. */

const char*	SearchODFile(const char*);
		/*!< Search for a <configuration> file in:
		    $OD_FILES
                    GetPersonalDir() + ".od"
                    GetSettingsDir() + ".od"
		    $DTECT_APPL/data
		    $DTECT_APPL/bin
		    $DTECT_APPL
		    GetBaseDataDir()
		*/


/*! Data directory

 The data directory is linked to environment variables;
 the project data is first searched in:

 On windows:

    DTECT_WINDATA

 On unix:

    DTECT_DATA

 If it is not found, it will try to find it in the settings.

*/
const char*	GetDataDir(void);
const char*	GetBaseDataDir(void);

#ifdef __cpp__
}
#endif

#endif
