#ifndef genc_H
#define genc_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		23-10-1996
 Contents:	Extension of gendefs.h to C generalities
 RCS:		$Id: genc.h,v 1.10 2003-09-26 16:24:47 bert Exp $
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

/*

 Where to get data and application data is linked to environment variables.
 For example, the project data is searched in:
 DTECT_WINDATA (windows only)
 dGB_WINDATA (windows only)
 DTECT_DATA
 dGB_DATA
 in that order.

 GetSoftwareDir : returns the full path of the root of where the executables,
		  program data, etc. is located.
 GetSoftwareUser: returns the value as set in DTECT_USER (or dGB_USER, etc.).
 		  This may be null - in fact it is most often null!
 GetUserDir     : returns the full path of the root of where the user data
		  files will be located.

 Beware that all functions will likely return a pointer to _the_ _same_ static
 buffer of length PATH_LENGTH!

*/
const char*	GetSoftwareDir(void);
		/*!< Directory of the installed software = $dGB|DTECT_APPL */
const char*	GetSoftwareUser(void);
		/*!< $dGB|DTECT_USER */
const char*	GetHomeDir(void);
		/*!< $HOME / $HOMEDRIVE/$HOMEPATH */
const char*	GetDataFileName(const char*);
		/*!< Application data file in $dGB|DTECT_APPL/data */

const char*	GetDataDir(void);
		/*!< User data: $dGB|DTECT_DATA/project */

int		isFinite(double);
		/*!< Returns 0 for infinite, NaN, and that sort of crap */
double		IntPowerOf(double,int);
double		PowerOf(double,double);
		/*!< PowerOf(-2,2) returns -4. This may be mathematically
		  incorrect, it delivers continuity with negative numbers */


/* Misc stuff */
const char*	errno_message();
		/*!< Will not return meaningful string on Windows */

void		put_platform(unsigned char*);
		/*!< Puts into single byte: 0 = Sun/SGI, 1 = Win/Linux */

void		swap_bytes(void*,int nbytes);
		/*!< nbytes=2,4,... e.g. nbytes=4: abcd becomes cdab */

int		getPID();
		/*!< returns process ID */


#ifdef __cpp__
}
#else
/* C only */

typedef char	FileNameString[PATH_LENGTH+1];
typedef char	UserIDString[mMaxUserIDLength+1];
typedef char	UnitIDString[mMaxUnitIDLength+1];

#endif


#endif
