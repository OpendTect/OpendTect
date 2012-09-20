#ifndef winterminate_h
#define winterminate_h
/*+
 * AUTHOR   : Arend Lammertink
 * DATE     : Aug 2003
 * SOURCE   : http://support.microsoft.com/kb/178893
 * RCS      : $Id$
-*/

#include <windows.h>

#define TA_FAILED 0
#define TA_SUCCESS_CLEAN 1
#define TA_SUCCESS_KILL 2

DWORD WINAPI TerminateApp( DWORD dwPID, DWORD dwTimeout );

#endif
