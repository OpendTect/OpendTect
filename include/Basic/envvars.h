#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Aug 2005
________________________________________________________________________

-*/

#include "basicmod.h"
#include "gendefs.h"

class BufferStringSet;


mGlobal(Basic) const char* GetEnvVar(const char*);
		/*!< getenv or other source. Cannot be called before
		     SetProgramArgs is called. Use GetOSEnvVar if you wish
		     to use before SetProgramArgs.
		     \note Will return a pointer to a threadsafe static buffer.
			   Please copy if you want to keep result after next
			   call to this function.*/
mGlobal(Basic) bool GetEnvVarDirList(const char*,BufferStringSet&,
				     bool checkdirs);
/*!< Returns a list of directories, possibly filtering out the non-existing */
mGlobal(Basic) bool GetEnvVarYN(const char*,bool defltval=false);
/*!< Returns defltval if not set, false if set to 0, "no" or "false",
     otherwise true */
mGlobal(Basic) int GetEnvVarIVal(const char*,int defltval);
mGlobal(Basic) double GetEnvVarDVal(const char*,double defltval);
mGlobal(Basic) float GetEnvVarFVal(const char*,float defltval);
mGlobal(Basic) void UnsetOSEnvVar(const char*);
mGlobal(Basic) const char* GetOSEnvVar(const char*);
		/*!< Raw 'getenv' call.
		    \note Will return a pointer to a threadsafe static buffer.
			   Please copy if you want to keep result after next
			   call to this function. */
mGlobal(Basic) void SetEnvVar(const char* env,const char* val);
		/*!< sets environment variable to a value. */
mGlobal(Basic) void SetEnvVarDirList(const char* env,const BufferStringSet&,
				     bool appendnoerase);
mGlobal(Basic) bool WriteEnvVar(const char* env,const char* val);
		/*!< Writes environment variable to .od/envvars for user
		     or data/Envvars for SysAdm */
mGlobal(Basic) const char* GetEnvVarDirListWoOD(const char* ky,
						const char* filt=nullptr);
		/*!< Returns the value of an environment variable
		     without any path inside the OpendTect installation */
