#ifndef genc_H
#define genc_H

/*@+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		23-10-1996
 Contents:	Extension of gendefs.h to C generalities
 RCS:		$Id: genc.h,v 1.2 2000-03-02 15:24:29 bert Exp $
________________________________________________________________________

General utilities:
* Survey related
* Data and software directories
* Two 'PowerOf' functions (1) integer (2) consistent for negative numbers
* Misc (generally not interesting)

-*/

#ifndef gendefs_H
#include <gendefs.h>
#endif

#ifdef __cpp__
extern "C" {
#endif

const char*	GetProjectVersionName(void);
		/* dGB-GDI Vx.x */

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
		/* Application data in $dGB_APPL/data */

const char*	GetDataDir(void);
		/* User data: $dGB_DATA/project */

double		IntPowerOf(double,int);
double		PowerOf(double,double);
		/* PowerOf(-2,2) returns -4 */


/* Misc stuff */
const char*	errno_message();

		/* Platform for binary data. 0 = Sun/IBM, 1 = Linux, 2 = SGI */
int		get_platform(int);
void		put_platform(int*);
void		swap_bytes(void*,int);

#ifdef ibm
#include <stdio.h>
extern FILE *popen(const char *, const char *);
extern int  pclose(FILE *);
extern int  getpid();
#endif


#ifdef __cpp__
}
#else
/* C only */

typedef char	FileNameString[PATH_LENGTH+1];
typedef char	UserIDString[mMaxUserIDLength+1];
typedef char	UnitIDString[mMaxUnitIDLength+1];

#endif


#endif
