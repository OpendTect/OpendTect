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

mGlobal(Basic) const char* GetEnvVar(const char*);
		/*!< getenv or other source. Cannot be called before
		     SetProgramArgs is called. Use GetOSEnvVar if you wish
		     to use before SetProgramArgs.
		     \note Will return a pointer to a threadsafe static buffer.
			   Please copy if you want to keep result after next
			   call to this function.*/
mGlobal(Basic) int GetEnvVarYN(const char*, int defltval=0);
/*!< Returns defltval if not set, 0 if set to 0 or "no", otherwise 1 */
mGlobal(Basic) int GetEnvVarIVal(const char*,int defltval);
mGlobal(Basic) double GetEnvVarDVal(const char*,double defltval);
mGlobal(Basic) float GetEnvVarFVal(const char*,float defltval);

mGlobal(Basic) const char* GetOSEnvVar(const char*);
		/*!< Raw 'getenv' call.
		    \note Will return a pointer to a threadsafe static buffer.
			   Please copy if you want to keep result after next
			   call to this function. */
mGlobal(Basic) void SetEnvVar(const char* env,const char* val);
		/*!< sets environment variable to a value. */
mGlobal(Basic) bool WriteEnvVar(const char* env,const char* val);
		/*!< Writes environment variable to .od/envvars for user
		     or data/Envvars for SysAdm */


#endif

