/*+
 * AUTHOR   : A.H. Lammertink
 * DATE     : July 2004
 * FUNCTION : Get cygwin directory if installed
-*/

static const char* rcsID = "$Id: GetCygDir.cc,v 1.1 2004-09-09 10:23:10 arend Exp $";


#include "prog.h"
#include "filegen.h"
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

    std::cout << cygdir << std::endl;

    return 0;
}

