#ifndef genc_H
#define genc_H

/*@+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		23-10-1996
 Contents:	Extension of gendefs.h to C generalities
 RCS:		$Id: genc.h,v 1.5 2001-03-19 10:17:47 bert Exp $
________________________________________________________________________

-*/

/*!
General utilities:
* Survey related
* Data and software directories
* Two 'PowerOf' functions (1) integer (2) consistent for negative numbers
* Misc (generally not interesting)
*/

#ifndef gendefs_H
#include <gendefs.h>
#endif

#ifdef __cpp__
extern "C" {
#endif

const char*	GetProjectVersionName(void);
		/*!< usually dGB-GDI Vx.x */

/*

 GetSoftwareDir : returns the full path of the root of where the executables,
		  program data, etc. is located.
 GetUserDir     : returns the full path of the root of where the user data
		  files will be located.
 Both functions return a pointer to the same static buffer of
 length PATH_LENGTH.

*/
const char*	GetSoftwareDir(void);
const char*	GetDataFileName(const char*);
		/*!< Application data in $dGB_APPL/data */

const char*	GetDataDir(void);
		/*!< User data: $dGB_DATA/project */

int		isFinite(double);
double		IntPowerOf(double,int);
double		PowerOf(double,double);
		/*!< PowerOf(-2,2) returns -4 */


/* Misc stuff */
const char*	errno_message();

		/* Platform for binary data. 0 = SunSparc, 1 = Win/Linux,
		   2 = SGI(Irix) */

void		put_platform(unsigned char*);
void		swap_bytes(void*,int);


#ifdef __cpp__
}
#else
/* C only */

typedef char	FileNameString[PATH_LENGTH+1];
typedef char	UserIDString[mMaxUserIDLength+1];
typedef char	UnitIDString[mMaxUnitIDLength+1];

#endif


#endif
