#ifndef genc_H
#define genc_H

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		23-10-1996
 Contents:	Extension of gendefs.h to C generalities
 RCS:		$Id: genc.h,v 1.19 2004-07-22 16:13:19 bert Exp $
________________________________________________________________________


Some general utilities, that need to be accessible in many places:

* Data and software directories
* A few 'math' thingies
* Misc (generally not interesting)

-*/

#ifndef gendefs_H
#include <gendefs.h>
#endif

#ifdef __cpp__
extern "C" {
#endif

const char*	GetProjectVersionName(void);
		/*!< "dTect Vx.x" */

/*! Directory of the installed software = $dGB|DTECT_APPL 

 GetSoftwareDir returns the full path of the root of where the executables,
 program data, etc. is located.


 On windows:
    DTECT_WINDATA
    dGB_WINDATA

 in that order.
 On unix:

    DTECT_DATA
    dGB_DATA

 in that order.

 If that fails, it will try to extract the location out of argv[0] 


 Beware that all functions may return a pointer to _the_ _same_ static
 buffer of length PATH_LENGTH!

*/
const char*	GetSoftwareDir(void);


/*! Location of binaries on local platform

    GetSoftwareDir()/bin		on *nix
    GetSoftwareDir()\bin		on win32
    GetSoftwareDir()/Contents/MacOS	on Mac OS/X

*/
const char*	GetBinDir(void);


/*! Location of launch script for external programs

    local:
	GetSoftwareDir()/bin/od_exec     on *nix
	GetSoftwareDir()\bin\od_exec.bat on win32
    remote:
	GetSoftwareDir()/bin/od_exec_rmt     on *nix
	GetSoftwareDir()\bin\od_exec_rmt.bat on win32

*/
const char*	GetExecScript(int remote);
#define		mGetExecScript()	GetExecScript(0)

/*! DTECT_USER (or dGB_USER, etc.).

 returns the value as set in DTECT_USER (or dGB_USER, etc.).
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
    dGB_WINHOME
    HOMEDRIVE+HOMEPATH if available and _not_ equal to "C:\"

    else returns 0;

 in that order.

 On unix:

    DTECT_HOME
    dGB_HOME
    HOME

 in that order.

*/
const char*	_GetHomeDir(void);


/*! Home dir or "Application Data"

 returns _GetHomeDir() by default.

 On Windows, it returns "Application Data" if home directory not defined.

*/
const char*	GetSettingsDir(void);


/*! Home dir or "My Documents"

 returns _GetHomeDir() by default.

 On Windows, it returns "My Documents" if home directory not defined.

*/
const char*	GetPersonalDir(void);


const char*	GetDataFileName(const char*);
		/*!< Application data file in $dGB|DTECT_APPL/data */

/*! Data directory

 The data directory is linked to environment variables;
 the project data is first searched in:

 On windows:

    DTECT_WINDATA
    dGB_WINDATA

 in that order.

 On unix:

    DTECT_DATA
    dGB_DATA

 in that order.

 If it is not found, it will try to find it in the settings.

*/
const char*	GetDataDir(void);
const char*	GetBaseDataDir(void);


int		isFinite(double);
		/*!< Returns 0 for infinite, NaN, and that sort of crap */
double		IntPowerOf(double,int);
double		PowerOf(double,double);
		/*!< PowerOf(-2,2) returns -4. This may be mathematically
		  incorrect, it delivers continuity with negative numbers */

/* Misc stuff */

const char*     errno_message();
                /*!< Will not return meaningful string on Windows */

void		put_platform(unsigned char*);
		/*!< Puts into single byte: 0 = Sun/SGI, 1 = Win/Linux */

void		swap_bytes(void*,int nbytes);
		/*!< nbytes=2,4,... e.g. nbytes=4: abcd becomes cdab */

int		getPID();
		/*!< returns process ID */

int		setEnvVar(const char* env,const char* val);
		/*!< sets environment variable to a value. */

int		exitProgram( int ret );
		/*!< Win32: kills progam itself and ignores ret.
		     Unix: uses exit(ret). return value is there to keep
		     compiler satisfied, like: return exitProgram( retval );
                */


#ifdef __cpp__
}
#else
/* C only */

typedef char	FileNameString[PATH_LENGTH+1];
typedef char	UserIDString[mMaxUserIDLength+1];

#endif


#endif
