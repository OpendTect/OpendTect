#ifndef winterminate_h
#define winterminate_h
/*+
 * AUTHOR   : Arend Lammertink
 * DATE     : Aug 2003
 * SOURCE   : http://support.microsoft.com/kb/178893
 * RCS      : $Id: winterminate.h,v 1.3 2010/09/06 09:40:53 cvsnanne Exp $
-*/

#include <windows.h>

#define TA_FAILED 0
#define TA_SUCCESS_CLEAN 1
#define TA_SUCCESS_KILL 2

DWORD WINAPI TerminateApp( DWORD dwPID, DWORD dwTimeout );

#endif
