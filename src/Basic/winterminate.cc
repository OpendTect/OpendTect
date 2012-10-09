/*+
 * AUTHOR   : Arend Lammertink
 * DATE     : Aug 2003
 * SOURCE   : http://support.microsoft.com/kb/178893
-*/

static const char* rcsID = "$Id$";
#include "commondefs.h"
#ifdef __win__
#include "winterminate.h"


// Declare Callback Enum Functions.
BOOL CALLBACK TerminateAppEnum( HWND hwnd, LPARAM lParam );

/*----------------------------------------------------------------
DWORD WINAPI TerminateApp( DWORD dwPID, DWORD dwTimeout )

Purpose:
  Shut down a 32-Bit Process (or 16-bit process under Windows 95)

Parameters:
  dwPID
     Process ID of the process to shut down.

  dwTimeout
     Wait time in milliseconds before shutting down the process.

Return Value:
  TA_FAILED - If the shutdown failed.
  TA_SUCCESS_CLEAN - If the process was shutdown using WM_CLOSE.
  TA_SUCCESS_KILL - if the process was shut down with
     TerminateProcess().
  NOTE:  See header for these defines.
----------------------------------------------------------------*/ 

DWORD WINAPI TerminateApp( DWORD dwPID, DWORD dwTimeout )
{

    // If we can't open the process with PROCESS_TERMINATE rights,
    // then we give up immediately.
    HANDLE hProc = OpenProcess( SYNCHRONIZE|PROCESS_TERMINATE, FALSE, dwPID );
    if ( hProc == NULL )
	return TA_FAILED;

    // TerminateAppEnum() posts WM_CLOSE to all windows whose PID
    // matches your process's.
    EnumWindows( (WNDENUMPROC)TerminateAppEnum, (LPARAM) dwPID );

    // Wait on the handle. If it signals, great. If it times out,
    // then you kill it.
    DWORD dwRet;
    if ( WaitForSingleObject(hProc,dwTimeout) != WAIT_OBJECT_0 )
	dwRet=(TerminateProcess(hProc,0)?TA_SUCCESS_KILL:TA_FAILED);
    else
	dwRet = TA_SUCCESS_CLEAN;

    CloseHandle( hProc );

    return dwRet;
}


BOOL CALLBACK TerminateAppEnum( HWND hwnd, LPARAM lParam )
{
    DWORD dwID;
    GetWindowThreadProcessId( hwnd, &dwID );
    if ( dwID == (DWORD)lParam )
	PostMessage( hwnd, WM_CLOSE, 0, 0 );

    return TRUE;
}
#endif // __win__
