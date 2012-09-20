#ifndef envvars_h
#define envvars_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		Aug 2005
 RCS:		$Id$
________________________________________________________________________

-*/

#ifndef gendefs_h
#include "basicmod.h"
#include "gendefs.h"
#endif

#ifdef __cpp__
extern "C" {
#endif

mGlobal(Basic) const char* GetEnvVar(const char*);
		/*!< getenv or other source */
mGlobal(Basic) int GetEnvVarYN(const char*);
		/*!< Returns 0=NO if not set or explicitly on 0 or "no". */
mGlobal(Basic) int GetEnvVarIVal(const char*,int defltval);
mGlobal(Basic) double GetEnvVarDVal(const char*,double defltval);

mGlobal(Basic) char* GetOSEnvVar(const char*);
		/*!< Raw 'getenv' call */
mGlobal(Basic) int SetEnvVar(const char* env,const char* val);
		/*!< sets environment variable to a value. */
mGlobal(Basic) int WriteEnvVar(const char* env,const char* val);
		/*!< Writes environment variable to .od/envvars for user
		     or data/Envvars for SysAdm */

mGlobal(Basic) char GetEnvSeparChar(void);
		/*!< The character separating entries in an env variable */

#ifdef __cpp__
}
#endif


#endif

