/*+
 * AUTHOR   : Arend Lammertink
 * DATE     : Aug 2003
 * SOURCE   : http://support.microsoft.com/default.aspx?scid=http://support.microsoft.com:80/support/kb/articles/Q178/8/93.ASP&NoWebContent=1
 * RCS      : $Id: winterminate.h,v 1.2 2003-11-07 10:04:25 bert Exp $
-*/

#include <windows.h>

#define TA_FAILED 0
#define TA_SUCCESS_CLEAN 1
#define TA_SUCCESS_KILL 2
#define TA_SUCCESS_16 3

DWORD WINAPI TerminateApp( DWORD dwPID, DWORD dwTimeout ) ;
DWORD WINAPI Terminate16App( DWORD dwPID, DWORD dwThread,
			     WORD w16Task, DWORD dwTimeout );
