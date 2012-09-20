/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Lammertink
 * DATE     : November 2004
 * FUNCTION : Utilities for win32, amongst others path conversion
-*/

static const char* rcsID mUnusedVar = "$Id$";


#include "winutils.h"
#include "bufstring.h"
#include "envvars.h"
#include "file.h"
#include "debugmasks.h"
#include "string2.h"
#include "staticstring.h"
#include "strmprov.h"
#ifdef __win_
# include <windows.h>
# include <shlobj.h>
# include <process.h>

// registry stuff
# include <regstr.h>
# include <winreg.h>

#include <iostream>

#endif

#include <ctype.h>
#include <string.h>

static const char* drvstr="/cygdrive/";

const char* getCleanUnxPath( const char* path )
{
    if ( !path || !*path ) return 0;

    static StaticStringManager stm;
    BufferString& res = stm.getString();

    BufferString buf; buf = path;
    char* ptr = buf.buf();
    mTrimBlanks( ptr );
    replaceCharacter( ptr, '\\' , '/' );
    replaceCharacter( ptr, ';' , ':' );

    char* drivesep = strstr( ptr, ":" );
    if ( !drivesep ) { res = ptr; return res; }
    *drivesep = '\0';

    res = drvstr;
    *ptr = tolower(*ptr);
    res += ptr;
    res += ++drivesep;

    return res;
}

#define mRet(ret) \
    replaceCharacter( ret.buf(), '/', '\\' ); \
    if ( __do_debug_cleanpath ) \
    { \
        BufferString msg("getCleanWinPath for:"); \
	msg += path; \
	msg += " : "; \
	msg += ret; \
        od_debug_message( msg ); \
    } \
    return ret;  

const char* getCleanWinPath( const char* path )
{
    if ( !path || !*path ) return 0;

    static StaticStringManager stm;
    BufferString& ret = stm.getString();
    ret = path;
    replaceCharacter( ret.buf(), ';' , ':' );

    BufferString buf( ret );
    char* ptr = buf.buf();

    mTrimBlanks( ptr );

    static bool __do_debug_cleanpath = GetEnvVarYN("DTECT_DEBUG_WINPATH");

    if ( *(ptr+1) == ':' ) // already in windows style.
	{ ret = ptr;  mRet( ret ); }

    bool isabs = *ptr == '/';
                
    char* cygdrv = strstr( ptr, drvstr );
    if ( cygdrv )
    {
	char* drv = cygdrv + strlen( drvstr );
	char* buffer = ret.buf();

	*buffer = *drv; *(buffer+1) = ':'; *(buffer+2) = '\0';
	ret += ++drv;
    }

    char* drivesep = strstr( ret.buf(), ":" );
    if ( isabs && !drivesep ) 
    {
	const char* cygdir =
#ifdef __win__
				getCygDir();
#else 
				0;
#endif
	if ( !cygdir || !*cygdir )
	{
	    if ( GetEnvVar("CYGWIN_DIR") ) // anyone got a better idea?
		cygdir = GetEnvVar("CYGWIN_DIR");
	    else
		cygdir = "c:\\cygwin";
	}

	ret = cygdir;

	ret += ptr;
    }

    mRet( ret );
}


#ifdef __win__

const char* getCygDir()
{
    static StaticStringManager stm;
    BufferString& answer = stm.getString();
    if ( !answer.isEmpty() )  return answer;

    HKEY hKeyRoot = HKEY_CURRENT_USER;
    LPTSTR subkey="Software\\Cygnus Solutions\\Cygwin\\mounts v2\\/";
    LPTSTR Value="native"; 

    BYTE Value_data[256];
    DWORD Value_size = sizeof(Value_data);

    HKEY hKeyNew = 0;
    DWORD retcode = 0;
    DWORD Value_type = 0;

    retcode = RegOpenKeyEx ( hKeyRoot, subkey, 0, KEY_QUERY_VALUE, &hKeyNew );

    if (retcode != ERROR_SUCCESS)
    {
	hKeyRoot = HKEY_LOCAL_MACHINE;
	subkey="Software\\Cygnus Solutions\\Cygwin\\mounts v2\\/";

	retcode = RegOpenKeyEx( hKeyRoot, subkey, 0, KEY_QUERY_VALUE, &hKeyNew);
	if (retcode != ERROR_SUCCESS) return 0;
    }

    retcode = RegQueryValueEx( hKeyNew, Value, NULL, &Value_type, Value_data,
                               &Value_size);

    if (retcode != ERROR_SUCCESS) return 0;

    answer = (const char*) Value_data;
    return answer;
}


const char* GetSpecialFolderLocation(int nFolder)
{
    static StaticStringManager stm;
    BufferString& Result = stm.getString();

    LPITEMIDLIST pidl;
    HRESULT hr = SHGetSpecialFolderLocation(NULL, nFolder, &pidl);
    
    if ( !SUCCEEDED(hr) ) return 0;

    char szPath[_MAX_PATH];

    if ( !SHGetPathFromIDList(pidl, szPath)) return 0;

    Result = szPath;

    return Result;
}


bool winCopy( const char* from, const char* to, bool isfile, bool ismove )
{
    if ( isfile && File::getKbSize(from) < 1024 )
    {
	BufferString cmd;
	cmd = "copy /Y";
	cmd.add(" \"").add(from).add("\" \"").add(to).add("\"");
	const bool ret = system( cmd ) != -1;
	return ret;
    }

    SHFILEOPSTRUCT fileop;
    BufferString frm( from );
    if ( !isfile )
	frm += "\\*";

    const int sz = frm.size();
    frm[sz+1] = '\0';
     
    ZeroMemory( &fileop, sizeof(fileop) );
    fileop.hwnd = NULL; fileop.wFunc = ismove ? FO_MOVE : FO_COPY;
    fileop.pFrom = frm; fileop.pTo = to; 
    fileop.fFlags = ( isfile ? FOF_FILESONLY : FOF_MULTIDESTFILES )
			       | FOF_NOCONFIRMMKDIR | FOF_NOCONFIRMATION;
			     //  | FOF_SILENT;

    int res = SHFileOperation( &fileop );
    return !res;
}


bool winRemoveDir( const char* dirnm )
{
    SHFILEOPSTRUCT fileop;
    BufferString frm( dirnm );
    const int sz = frm.size();
    frm[sz+1] = '\0';
    ZeroMemory( &fileop, sizeof(fileop) );
    fileop.hwnd = NULL;
    fileop.wFunc = FO_DELETE;
    fileop.pFrom = frm;
    fileop.fFlags = FOF_MULTIDESTFILES | FOF_NOCONFIRMATION | FOF_SILENT;
    int res = SHFileOperation( &fileop );
    return !res;
}


unsigned int getWinVersion()
{
    DWORD dwVersion = 0; 
    DWORD dwMajorVersion = 0;
    DWORD dwMinorVersion = 0; 
    dwVersion = GetVersion();
    dwMajorVersion = (DWORD)(LOBYTE(LOWORD(dwVersion)));
    dwMinorVersion = (DWORD)(HIBYTE(LOWORD(dwVersion)));
    return dwMajorVersion;
}


bool execShellCmd( const char* comm, const char* parm, const char* runin )
{
   int res = (int) ShellExecute( NULL, "runas",
				 comm,
				 parm,    // params
				 runin, // directory
				 SW_SHOW );
    return res > 32;
}


bool execProc( const char* comm, bool inconsole, bool inbg, const char* runin )
{
    if ( !comm || !*comm ) return false;

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(STARTUPINFO));
    ZeroMemory( &pi, sizeof(pi) );
    si.cb = sizeof(STARTUPINFO);

    if ( !inconsole )
    {
	si.dwFlags = STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;
	si.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);	
	si.wShowWindow = SW_HIDE;
    }
    
   //Start the child process. 
    int res = CreateProcess( NULL,	// No module name (use command line). 
        const_cast<char*>( comm ),
        NULL,				// Process handle not inheritable. 
        NULL,				// Thread handle not inheritable. 
        FALSE,				// Set handle inheritance to FALSE. 
        0,				// Creation flags. 
        NULL,				// Use parent's environment block. 
        const_cast<char*>( runin ), 	// Use parent's starting directory if runin is NULL. 
        &si, &pi );
	
    if ( res )
    {
	if ( !inbg )  WaitForSingleObject( pi.hProcess, INFINITE );
	CloseHandle( pi.hProcess );
	CloseHandle( pi.hThread );
    }
    else
    {
	char *ptr = NULL;
	FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER |
	    FORMAT_MESSAGE_FROM_SYSTEM,
	    0, GetLastError(), 0, (char *)&ptr, 1024, NULL);

	fprintf(stderr, "\nError: %s\n", ptr);
	LocalFree(ptr);
    }

    return res;
}


bool executeWinProg( const char* comm, const char* parm, const char* runin )
{
	 if ( !comm || !*comm ) return false;
	 unsigned int winversion = getWinVersion();
	 if ( winversion < 6 )
	 {
	     BufferString com( comm, " " );
	     com += parm;
	     return execProc( com, true, true, runin );
	 }
	 return execShellCmd( comm, parm, runin );
}

#endif // __win__

