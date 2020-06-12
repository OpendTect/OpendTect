/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Lammertink
 * DATE     : November 2004
 * FUNCTION : Utilities for win32, amongst others path conversion
-*/



#include "winutils.h"

#include "bufstring.h"
#include "genc.h"
#include "debug.h"
#include "envvars.h"
#include "file.h"
#include "staticstring.h"
#include "string2.h"

#ifdef __win__
// These get rid of warnings about casting HINSTANCE to int
#pragma warning( disable : 4302 )
#pragma warning( disable : 4311 )

# include <windows.h>
# include <shlobj.h>
# include <process.h>

// registry stuff
# include <regstr.h>
# include <winreg.h>

# include <iostream>
# include <QSettings>
#endif


static const char* cygdrvstr="/cygdrive/";
static const int cygdrvstringlen=10;

void DisableAutoSleep()
{
#ifdef __win__
    /* Prevents the machine from sleeping
	https://msdn.microsoft.com/en-us/library/windows/desktop/
		aa373208(v=vs.85).aspx */
    SetThreadExecutionState( ES_CONTINUOUS | ES_SYSTEM_REQUIRED );
#else
   //TODO
#endif
}

void EnableAutoSleep()
{
#ifdef __win__
   SetThreadExecutionState( ES_CONTINUOUS );
#else
   //TODO
#endif
}


const char* GetCleanUnxPath( const char* path )
{
    if ( !path || !*path )
	return 0;

    mDeclStaticString( ret );

    BufferString buf = path;
    buf.trimBlanks();
    buf.replace( '\\' , '/' );
    buf.replace( ';', ':' );

    char* drivesep = buf.find( ':' );
    if ( !drivesep )
	{ ret = buf; return ret.buf(); }

    *drivesep = '\0';

    ret = cygdrvstr;
    char* ptr = buf.getCStr();
    *ptr = (char)tolower(*ptr);
    ret += ptr;
    ret += ++drivesep;

    return ret;
}

#define mRet(ret) \
    ret.replace( '/', '\\' ); \
    if ( __do_debug_cleanpath ) \
    { \
        BufferString msg("GetCleanWinPath for: ",path," : "); \
	msg += ret; \
        od_debug_message( msg ); \
    } \
    return ret;

const char* GetCleanWinPath( const char* path )
{
    if ( !path || !*path ) return 0;

    mDeclStaticString( ret );
    ret = path;
    ret.replace( ';', ':' );

    BufferString buf( ret );
    char* ptr = buf.getCStr();

    mTrimBlanks( ptr );

    mDefineStaticLocalObject( bool, __do_debug_cleanpath,
			      = GetEnvVarYN("DTECT_DEBUG_WINPATH") );

    if ( *(ptr+1) == ':' ) // already in windows style.
	{ ret = ptr;  mRet( ret ); }

    bool isabs = *ptr == '/';

    char* cygdrv = firstOcc( ptr, cygdrvstr );
    if ( cygdrv )
    {
	char* buffer = ret.getCStr();
	char* drv = cygdrv + cygdrvstringlen;
	*buffer = *drv; *(buffer+1) = ':'; *(buffer+2) = '\0';
	ret += ++drv;
    }

    char* drivesep = ret.find( ":" );
    if ( isabs && !drivesep )
    {
	const char* cygdir =
#ifdef __win__
				WinUtils::getCygDir();
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

const char* WinUtils::getCygDir()
{
    mDeclStaticString( answer );
    if ( !answer.isEmpty() ) return answer;

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


const char* WinUtils::getSpecialFolderLocation( int nFolder )
{
    LPITEMIDLIST pidl;
    HRESULT hr = SHGetSpecialFolderLocation( NULL, nFolder, &pidl );
    if ( !SUCCEEDED(hr) )
	return 0;

    char szPath[_MAX_PATH];
    if ( !SHGetPathFromIDList(pidl,szPath) )
	return 0;

    mDeclStaticString( ret );
    ret = szPath;
    return ret;
}


bool WinUtils::copy( const char* from, const char* to, bool isfile,
			bool ismove )
{
    if ( isfile && File::getKbSize(from) < 1024 )
    {
	OS::MachineCommand mc( "copy", "/Y", from, to );
	return mc.execute();
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


bool WinUtils::removeDir( const char* dirnm )
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


unsigned int WinUtils::getWinVersion()
{
    DWORD dwVersion = 0;
    DWORD dwMajorVersion = 0;
    DWORD dwMinorVersion = 0;
#pragma warning( push )
#pragma warning( disable:4996 )
    dwVersion = GetVersion();
#pragma warning( pop )
    dwMajorVersion = (DWORD)(LOBYTE(LOWORD(dwVersion)));
    dwMinorVersion = (DWORD)(HIBYTE(LOWORD(dwVersion)));
    return dwMajorVersion;
}


const char* WinUtils::getFullWinVersion()
{
    DWORD dwVersion = 0;
    DWORD dwMajorVersion = 0;
    DWORD dwMinorVersion = 0;
#pragma warning( push )
#pragma warning( disable:4996 )
    dwVersion = GetVersion();
#pragma warning( pop )
    dwMajorVersion = (DWORD)(LOBYTE(LOWORD(dwVersion)));
    dwMinorVersion = (DWORD)(HIBYTE(LOWORD(dwVersion)));

    const int majorVersion = dwMajorVersion;
    const int minorVersion = dwMinorVersion;
    mDeclStaticString( winversion );
    winversion.add( majorVersion );
    winversion.add( "." );
    winversion.add( minorVersion );
    return winversion.buf();
}


bool WinUtils::execShellCmd( const char* comm, const char* parm,
			     const char* runin )
{
    int res = (int)ShellExecute( NULL, "runas", comm, parm, runin, SW_SHOW );
    return res > 32;
}


bool WinUtils::execProc( const char* comm, bool inconsole, bool inbg,
			 const char* runin )
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
	const_cast<char*>( runin ),	// Use parent's starting directory if
					// runin is NULL.
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


bool WinUtils::execProg( const char* comm, const char* parm, const char* runin )
{
     if ( !comm || !*comm ) return false;

     unsigned int winversion = WinUtils::getWinVersion();
     if ( winversion < 6 )
     {
	 BufferString com( comm, " " );
	 com += parm;
	 return WinUtils::execProc( com, true, true, runin );
     }

     return WinUtils::execShellCmd( comm, parm, runin );
}


static bool getDefaultApplication( const char* filetype,
				   BufferString& cmd, BufferString& errmsg )
{
    cmd = errmsg = "";

    HKEY handle;
    LONG res = 0;
    const BufferString subkey( filetype, "\\Shell\\Open\\Command" );
    res = RegOpenKeyEx( HKEY_CLASSES_ROOT, subkey.buf(), 0, KEY_READ, &handle );
    if ( res != ERROR_SUCCESS )
    {
	errmsg = "Cannot open registry";
	return false;
    }

    CHAR value[512];
    DWORD bufsz = sizeof( value );
    res = RegQueryValueEx( handle, NULL, NULL, NULL, (LPBYTE)value, &bufsz );
    if ( res != ERROR_SUCCESS )
    {
	errmsg =  "Cannot query registry for default browser";
	return false;
    }

    cmd = value;
    RegCloseKey( handle );
    return true;
}


bool WinUtils::getDefaultBrowser( BufferString& cmd, BufferString& errmsg )
{
    return getDefaultApplication( "HTTP", cmd, errmsg );
}


bool WinUtils::setRegKeyVal( const char* ky, const char* vanrnm,
			     const char *val )
{
    QSettings regkey( ky, QSettings::NativeFormat );
    regkey.setValue("Default", "");
    regkey.setValue( vanrnm, val );
    regkey.sync();
    return regkey.status() == QSettings::NoError;
}


bool WinUtils::removeRegKey( const char* ky )
{
    QSettings regkey( ky, QSettings::NativeFormat );
    regkey.clear();
    regkey.sync();
    return regkey.status() == QSettings::NoError;
}

#endif // __win__
