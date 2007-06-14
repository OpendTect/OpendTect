#ifndef oddirs_h
#define oddirs_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		Aug 2005
 RCS:		$Id: oddirs.h,v 1.8 2007-06-14 11:22:37 cvsbert Exp $
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


/*! Directory for personal settings: 'Home directory'

 This gets the user's home directory. Even on Windows, something will be
 assigned as being the home directory.

 The personal directory is linked to environment variables:

 First of all, DTECT_PERSONAL_DIR is checked. If it's set, that is returned.

 On windows:

    DTECT_WINHOME
    DTECT_HOME	-> converted from unix to windows path (cygpath -w)
    HOMEDRIVE+HOMEPATH if exists and not equal to "C:\"

    That should be it. If that still fails, we try HOME, USERPROFILE, APPDATA,
    DTECT_USERPROFILE_DIR. As a last resort,
    GetSpecialFolderLocation( CSIDL_PROFILE ) is used.

 On unix:

    DTECT_HOME, HOME; in that order.

 If all else fails, "C:\\" or "/tmp" are returned.

*/
const char*	GetPersonalDir(void);


/*! Directory with the user settings

  Returns GetPersonalDir()/.od, unless DTECT_PERSONAL_DIR is set.

*/
const char*	GetSettingsDir(void);


const char*	GetDataFileDir();
		/*!< Returns the directory where the release's data files
		  are stored. This will be $DTECT_APPL/data. Beware that in
		  many cases you also need to look in GetSiteDataDir() */

const char*	GetDataFileName(const char*);
		/*!< This function will search in first the GetSiteDataDir()
		     and then GetDataFileDir() for a file. It returns the first
		     that exists. */

const char*	GetProcFileName(const char*);
		/*!< Job processing spec file in $DTECT_DATA/Proc.
		  	Pass null for directory name. */

const char*	SearchODFile(const char*);
		/*!< Search for a <configuration> file in:
                    GetPersonalDir() + ".od"
                    GetSettingsDir() + ".od"
		    GetBaseDataDir()
		    $DTECT_SITE_DATA/data
		    $DTECT_APPL/data
		    $DTECT_SITE_DATA/bin
		    $DTECT_APPL/bin
		    $DTECT_SITE_DATA
		    $DTECT_APPL
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
