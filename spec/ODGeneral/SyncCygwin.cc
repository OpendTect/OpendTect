/*+
 * AUTHOR   : A.H. Lammertink
 * DATE     : January 2004
 * FUNCTION : Synchronize OpendTect sys tools with cygwin, if installed.
-*/

static const char* rcsID = "$Id: SyncCygwin.cc,v 1.4 2011/12/14 13:16:41 cvsbert Exp $";


#include "prog.h"
#include "file.h"
#include "filepath.h"

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

    FilePath fp( cygdir, "bin" );
    if ( !File::isDirectory(fp.fullPath()) ) return 2;

    static const char* cygdllnm = "cygwin1.dll";
    fp.add( cygdllnm );
    BufferString cygdllpath = fp.fullPath();
    if ( !File::exists(cygdllpath) ) return 3;

    fp.set( File::getCurrentDir() ); fp.add( "sys" );
    if ( !File::isDirectory(fp.fullPath()) ) return 4;

    fp.add( cygdllnm );
    BufferString tofile = fp.fullPath();
    fp.setFileName( "cygwin1_old.dll" );
    BufferString oldfile = fp.fullPath();
    if ( File::exists(oldfile) && !File::remove(oldfile, false) )
	return 5;
    else if ( !File::rename( tofile, oldfile ) )
	return 6;

    if ( !File::copy(cygdllpath,tofile,false) ) 
    {
	if ( !File::rename( oldfile, tofile ) )
	    return 7;
	else
	    return 8;
    }

    return 0;
}

