#ifndef genc_H
#define genc_H

/*@+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		23-10-1996
 Contents:	Extension of gendefs.h to C generalities
 RCS:		$Id: genc.h,v 1.1.1.2 1999-09-16 09:19:00 arend Exp $
________________________________________________________________________

@$*/

#ifndef gendefs_H
#include <gendefs.h>
#endif

#ifdef __cpp__
extern "C" {
#endif

/*$@
 The function GetProjectVersionName must be implemented by every program.
@$*/

const char*	GetProjectVersionName	Pargs( (void) );

/*$@
 GetSoftwareDir : returns the full path of the root of where the executables,
		  program data, etc. is located.
 GetUserDir     : returns the full path of the root of where the user data
		  files will be located.
 Both functions return a pointer to a static buffer of length PATH_LENGTH.
@$*/
const char*	GetSoftwareDir		Pargs( (void) );
		/* Application data */
const char*	GetDataFileName		Pargs( (const char*) );

		/* User data */
const char*	GetDataDir		Pargs( (void) );

const char*	errno_message();

		/* Platform for binary data. 0 = Sun/IBM, 1 = Linux, 2 = SGI */
int		get_platform		Pargs( (int) );
void		put_platform		Pargs( (int*) );
void		swap_bytes		Pargs( (void*,int) );

double		PowerOf			Pargs( (double,double) );
double		IntPowerOf		Pargs( (double,int) );

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


/* End C only */
#endif


/*$-*/
#endif
