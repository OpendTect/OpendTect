/*+
 * AUTHOR   : A.H. Lammertink
 * DATE     : January 2004
 * FUNCTION : Synchronize OpendTect sys tools with cygwin, if installed.
-*/

static const char* rcsID = "$Id: SyncCygwin.cc,v 1.1 2004-01-22 10:14:23 dgb Exp $";


#include "prog.h"
#include "filegen.h"

#include <windows.h>
#include <regstr.h>
#include <ctype.h>
#include <winreg.h>
#include <iostream>


const char* getCygDir()
{
    static BufferString answer;

    HKEY hKeyRoot = HKEY_LOCAL_MACHINE;
    LPCTSTR subkey="SOFTWARE\\Cygnus Solutions\\Cygwin\\mounts v2\\/";
    LPTSTR Value="native";

    BYTE* Value_data = new BYTE[80];
    DWORD Value_size = 80;

    HKEY hKeyNew=0;
    DWORD retcode=0;
    DWORD Value_type=0;

    retcode = RegOpenKeyEx ( hKeyRoot, subkey, 0, KEY_QUERY_VALUE, &hKeyNew);

    if (retcode != ERROR_SUCCESS) return 0;


    retcode = RegQueryValueEx( hKeyNew, Value, NULL, &Value_type, Value_data,
			       &Value_size);

    if (retcode != ERROR_SUCCESS) return 0;

    answer = (const char*) Value_data;
    return answer;
}


int main( int argc, char** argv )
{
    const char* cygdir = getCygDir();
    if ( !cygdir || !*cygdir ) return 1;

    BufferString bindir = File_getFullPath( cygdir, "bin" );
    if ( !File_isDirectory(bindir) ) return 2;

    BufferString cygdll = File_getFullPath( bindir, "cygwin1.dll" );
    if ( !File_exists(cygdll) ) return 3;

    BufferString todir = File_getFullPath( File_getCurrentDir(), "sys" );
    if ( !File_isDirectory(todir) ) return 4;

    BufferString tofile = File_getFullPath( todir, "cygwin1.dll" );

    if ( !File_copy(cygdll,tofile,false) ) return 5;

    return 0;
}

