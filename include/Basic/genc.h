#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		23-10-1996
________________________________________________________________________

Some general utilities, that need to be accessible in many places:

-*/

#ifndef gendefs_h
#include "basicmod.h"
#include "gendefs.h"
#endif

# include "string2.h"

extern "C" {

mGlobal(Basic) const char* GetProjectVersionName(void);
		/*!< "dTect Vx.x" */

mGlobal(Basic) int GetPID(void);
		/*!< returns process ID */

mGlobal(Basic) const char* GetLocalHostName(void);
		/*!< returns (as expected) local host name */

mGlobal(Basic) const char* GetFullExecutablePath(void);
		/*!< returns full path to executable. setProgramArgs
		     must be called for it to work. */

mGlobal(Basic) const char* GetOSIdentifier(void);

mGlobal(Basic) char** GetArgV(void);

mGlobal(Basic) int GetArgC(void);

mGlobal(Basic) bool AreProgramArgsSet(void);

mGlobal(Basic) void SetProgramArgs(int argc, char** argv);

mGlobal(Basic) bool isProcessAlive(int pid);
		/*!< returns 1 if the process is still running */
mGlobal(Basic) const char* getProcessNameForPID(int pid);
		/*!< returns null if process not found, otherwise returns 
		     the executable name
		*/

mGlobal(Basic) int ExitProgram( int ret );
		/*!< Win32: kills progam itself and ignores ret.
		     Unix: uses exit(ret).
		     Return value is convenience only, so you can use like:
		     return exitProgram( retval );
                */

typedef void (*PtrAllVoidFn)(void);
mGlobal(Basic) void NotifyExitProgram(PtrAllVoidFn);
		/*!< Function will be called on 'ExitProgram' */

mGlobal(Basic) void PutIsLittleEndian(unsigned char*);
		/*!< Puts into 1 byte: 0=SunSparc/SGI (big), 1=PC (little) */

mGlobal(Basic) void SwapBytes(void*,int nbytes);
		/*!< nbytes=2,4,... e.g. nbytes=4: abcd becomes cdab */

mGlobal(Basic) int InSysAdmMode(void);
		/*!< returns 0 unless in sysadm mode */

mGlobal(Basic) void sleepSeconds(double);
		/*!< puts current thread to sleep for (fraction of) seconds */


mGlobal(Basic) const char* GetVCSVersion(void);
		/*!< Returns Subversion revision number or git commit hash */

mGlobal( Basic ) const char* GetLastSystemErrorMessage(void);

mGlobal( Basic ) void ForkProcess(void);


mGlobal( Basic ) int InSysAdmMode(void);
mGlobal( Basic ) void SetInSysAdmMode(void);


inline void EmptyFunction()			{}
/* Used in some macros and ifdefs */

}
