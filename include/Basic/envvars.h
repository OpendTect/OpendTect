#ifndef envvars_h
#define envvars_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		Aug 2005
 RCS:		$Id: envvars.h,v 1.2 2008-06-20 12:23:52 cvsraman Exp $
________________________________________________________________________

-*/

#ifndef gendefs_H
#include "gendefs.h"
#endif

#ifdef __cpp__
extern "C" {
#endif

const char*	GetEnvVar(const char*);
		/*!< getenv or other source */
int		GetEnvVarYN(const char*);
		/*!< Returns 0=NO if not set or explicitly on 0 or "no". */
int		GetEnvVarIVal(const char*,int defltval);
double		GetEnvVarDVal(const char*,double defltval);

char*		GetOSEnvVar(const char*);
		/*!< Raw 'getenv' call */
int		SetEnvVar(const char* env,const char* val);
		/*!< sets environment variable to a value. */

char		GetEnvSeparChar();
		/*!< The character separating entries in an env variable */

#ifdef __cpp__
}
#endif


#endif
