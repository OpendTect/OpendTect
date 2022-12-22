/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "winutils.h"

#include "bufstring.h"
#include "debug.h"
#include "dirlist.h"
#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "genc.h"
#include "oscommand.h"
#include "perthreadrepos.h"
#include "ptrman.h"
#include "separstr.h"
#include "string2.h"

#ifdef __win__
# include <windows.h>
# include <process.h>
# include <aclapi.h>

// registry stuff
# include <regstr.h>
# include <winreg.h>

# include <iostream>
# include <QSettings>
#endif


static const char* cygdrvstr="/cygdrive/";
static const int cygdrvstrlen=10;


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


const char* getCleanUnxPath( const char* path )
{
    if ( !path || !*path ) return 0;

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
        BufferString msg("getCleanWinPath for: ",path," : "); \
	msg += ret; \
        od_debug_message( msg ); \
    } \
    return ret;

const char* getCleanWinPath( const char* path )
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
	char* drv = cygdrv + cygdrvstrlen;
	*buffer = *drv; *(buffer+1) = ':'; *(buffer+2) = '\0';
	ret += ++drv;
    }

    char* drivesep = ret.find( ":" );
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
    mDeclStaticString( answer );
    if ( !answer.isEmpty() )
	return answer;

    HKEY hKeyRoot = HKEY_CURRENT_USER;
    const BufferString subkey(
		    "Software\\Cygnus Solutions\\Cygwin\\mounts v2\\/" );
    const BufferString Value( "native" );

    BYTE Value_data[256];
    DWORD Value_size = sizeof(Value_data);

    HKEY hKeyNew = nullptr;
    DWORD retcode = 0;
    DWORD Value_type = 0;

    retcode = RegOpenKeyEx ( hKeyRoot, subkey.str(), 0, KEY_QUERY_VALUE,
			     &hKeyNew );

    if ( retcode != ERROR_SUCCESS )
    {
	hKeyRoot = HKEY_LOCAL_MACHINE;
	retcode = RegOpenKeyEx( hKeyRoot, subkey.buf(), 0, KEY_QUERY_VALUE,
				&hKeyNew );
	if (retcode != ERROR_SUCCESS)
	    return nullptr;
    }

    retcode = RegQueryValueEx( hKeyNew, Value.buf(), NULL, &Value_type,
			       Value_data, &Value_size );

    if ( retcode != ERROR_SUCCESS )
	return nullptr;

    answer = (const char*) Value_data;
    return answer;
}


const char* GetSpecialFolderLocation( int nFolder )
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


bool winCopy( const char* from, const char* to, bool isfile, bool ismove )
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


namespace WinUtils {

bool isFileInUse( const char* fnm )
{
    HANDLE handle = CreateFileA( fnm,
                                 GENERIC_READ | GENERIC_WRITE,
                                 0,
                                 0,
                                 OPEN_EXISTING,
                                 0,
                                 0 );
    const bool ret = handle == INVALID_HANDLE_VALUE;
    CloseHandle( handle );
    return ret;
}


mClass(Basic) SecurityID
{
public:
	    SecurityID( bool ismine )
		: ismine_(ismine)
	    {}
	    ~SecurityID()
	    {
		if ( ismine_ && sid_ )
		    FreeSid( sid_ );
	    }

    bool	isOK() const { return sid_; }

    bool operator==( const SecurityID& oth ) const
    { return EqualSid(sid_, oth.sid_); }
    bool operator!=( const SecurityID& oth ) const
    { return !(*this == oth); }

     PSID	 sid_ = nullptr;

private:
    bool	ismine_;

};

const SecurityID& getAdminSID()
{
    mDefineStaticLocalObject(PtrMan<SecurityID>, ret,
			     = new SecurityID(true) );
    if ( !ret->isOK() )
    {
	PSID sid = NULL;
	SID_IDENTIFIER_AUTHORITY ntAuth = SECURITY_NT_AUTHORITY;
	if ( AllocateAndInitializeSid(&ntAuth, 2,
	     SECURITY_BUILTIN_DOMAIN_RID,
	     DOMAIN_ALIAS_RID_ADMINS,
	     0, 0, 0, 0, 0, 0, &sid))
	    ret->sid_ = sid;
    }
    return *ret.ptr();
}

const SecurityID& getTrustedInstallerSID()
{
    mDefineStaticLocalObject(PtrMan<SecurityID>, ret,
			     = new SecurityID(true) );
    if ( !ret->isOK() )
    {
	PSID sid = NULL;
	SID_IDENTIFIER_AUTHORITY ntAuth = SECURITY_NT_AUTHORITY;
	if ( AllocateAndInitializeSid(&ntAuth, 6,
	    SECURITY_SERVICE_ID_BASE_RID,
	    SECURITY_TRUSTED_INSTALLER_RID1,
	    SECURITY_TRUSTED_INSTALLER_RID2,
	    SECURITY_TRUSTED_INSTALLER_RID3,
	    SECURITY_TRUSTED_INSTALLER_RID4,
	    SECURITY_TRUSTED_INSTALLER_RID5,
	    0, 0, &sid) )
	    ret->sid_ = sid;
    }
    return *ret.ptr();
}

SecurityID getFileSID( const char* fnm )
{
    // Get the handle of the file object.
    const DWORD dwFlagsAndAttributes = File::isFile(fnm)
	? FILE_ATTRIBUTE_NORMAL
	: (File::isDirectory(fnm) ? FILE_FLAG_BACKUP_SEMANTICS
	    : FILE_ATTRIBUTE_NORMAL);
    HANDLE hFile = CreateFile(
	TEXT(fnm),
	GENERIC_READ,
	FILE_SHARE_READ,
	NULL,
	OPEN_EXISTING,
	dwFlagsAndAttributes,
	NULL);

    SecurityID ret( false );
    if ( hFile == INVALID_HANDLE_VALUE )
	return ret;

    // Get the owner SID of the file.
    PSECURITY_DESCRIPTOR pSD = NULL;
    const DWORD dwRtnCode = GetSecurityInfo(
	hFile,
	SE_FILE_OBJECT,
	OWNER_SECURITY_INFORMATION,
	&ret.sid_,
	NULL,
	NULL,
	NULL,
	&pSD);

    if ( dwRtnCode != ERROR_SUCCESS )
	ret.sid_ = nullptr;

    CloseHandle(hFile);

    return ret;
}


bool pathContainsTrustedInstaller( const char* fnm )
{
    const FilePath fp( fnm );
    for ( int idx=0; idx<fp.nrLevels(); idx++ )
    {
	const BufferString filenm( fp.dirUpTo(idx) );
	if ( belongsToTrusterInstaller(filenm) )
	    return true;
    }

    return false;
}


bool belongsToStdUser( const char* fnm )
{
    const SecurityID retSid = getFileSID( fnm );
    if ( !retSid.isOK() ||
	 !getAdminSID().isOK() ||
	 !getTrustedInstallerSID().isOK() )
	return true;
    return retSid != getAdminSID() &&
	   retSid != getTrustedInstallerSID();
}


bool belongsToAdmin( const char* fnm )
{
    const SecurityID retSid = getFileSID( fnm );
    if ( !retSid.isOK() || !getAdminSID().isOK() )
	return false;
    return retSid == getAdminSID();
}


bool belongsToTrusterInstaller( const char* fnm )
{
    const SecurityID retSid = getFileSID( fnm );
    if ( !retSid.isOK() ||
	 !getTrustedInstallerSID().isOK() )
	return false;
    return retSid == getTrustedInstallerSID();
}


bool serviceIsRunning( const char* nm )
{
    return getServiceStatus(nm) == SERVICE_RUNNING;
}


int getServiceStatus( const char* nm )
{
    SC_HANDLE theService, scm;
    SERVICE_STATUS m_SERVICE_STATUS;
    SERVICE_STATUS_PROCESS ssStatus;
    DWORD dwBytesNeeded;

    scm = OpenSCManager(nullptr, nullptr, SC_MANAGER_ENUMERATE_SERVICE);
    if (!scm) {
            return 0;
    }

    theService = OpenService(scm, nm, SERVICE_QUERY_STATUS);
    if (!theService) {
        CloseServiceHandle(scm);
        return 0;
    }

    auto result = QueryServiceStatusEx(theService, SC_STATUS_PROCESS_INFO,
        reinterpret_cast<LPBYTE>(&ssStatus), sizeof(SERVICE_STATUS_PROCESS),
        &dwBytesNeeded);

    CloseServiceHandle(theService);
    CloseServiceHandle(scm);

    if (result == 0) {
        return 0;
    }

    return ssStatus.dwCurrentState;
}


bool NTUserBelongsToAdminGrp()
{
    mDefineStaticLocalObject(int, res, = -2);
    //-2: not tested, -1: cannot determine
    if ( res > -2 )
	return res == 1;

    if ( IsUserAnAdmin() )
    {
	res = 1;
	return true;
    }

    byte rawGroupList[4096];
    TOKEN_GROUPS& groupList = *((TOKEN_GROUPS*)rawGroupList);
    HANDLE hTok;
    if ( !OpenThreadToken(GetCurrentThread(),TOKEN_QUERY,false,&hTok) )
    {
	if ( !OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&hTok) )
	{
	    res = -1;
	    return false;
	}
    }

    // normally, I should get the size of the group list first, but ...
    DWORD lstsz = sizeof rawGroupList;
    if ( !GetTokenInformation(hTok,TokenGroups,&groupList,lstsz,&lstsz) )
    {
	res = -1;
	return false;
    }

    if ( !getAdminSID().isOK() )
	{ res = -1; return false; }

    // now, loop through groups in token and compare
    res = 0;
    for ( int idx=0; idx<groupList.GroupCount; idx++)
    {
	if (EqualSid(getAdminSID().sid_, groupList.Groups[idx].Sid))
	{
	    res = 1;
	    break;
	}
    }

    CloseHandle( hTok );
    return res == 1;
}


bool IsUserAnAdmin()
{
    mDefineStaticLocalObject(bool, isadmin, = ::IsUserAnAdmin() );
    return isadmin;
}

static bool isWindows11()
{
    if ( IsWindowsServer() )
	return false;

    static int res = -1;
    if ( res < 0 )
    {
	const StringView buildnrstr( getWinBuildNumber() );
	if ( buildnrstr.isNumber(true) )
	{
	    const int buildnr = buildnrstr.toInt();
	    res = buildnr >= 22000 ? 1 : 0;
	    // Will break down unless we known the upper limit as well (30000?)
	}
	else
	{
	    res = buildnrstr.contains( "Unknown" ) ? 0
		: (buildnrstr.startsWith("22") ? 1 : 0);
	    /* Will break down as soon as build numbers will reach 23000
	       Very likely already in 23H1 */
	}
    }

    return res == 1;
}

} // namespace WinUtils


const char* getWinVersion()
{
    mDeclStaticString( ret );
    if ( ret.isEmpty() )
    {
	if ( WinUtils::isWindows11() )
	    ret = 11;
	else
	{
	    DWORD dwFlagsRet = RRF_RT_REG_DWORD;
	    if ( !readKey(HKEY_LOCAL_MACHINE,
			"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
			"CurrentMajorVersionNumber",ret,&dwFlagsRet) )
	    {
		if ( readKey(HKEY_LOCAL_MACHINE,
		    "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
		    "CurrentVersion",ret,&dwFlagsRet) )
		{
		    const SeparString ss( ret.buf(), '.' );
		    if ( ss.size() > 0 )
			ret = ss[0];
		}

		if ( ret.isEmpty() )
		    ret.set( "Unknown major version" );
	    }
	}
    }

    return ret.buf();
}


const char* getWinMinorVersion()
{
    mDeclStaticString( ret );
    if ( ret.isEmpty() )
    {
	DWORD dwFlagsRet = RRF_RT_REG_DWORD;
	if ( !readKey(HKEY_LOCAL_MACHINE,
	    "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
	    "CurrentMinorVersionNumber",ret,&dwFlagsRet) )
	{
	    if ( readKey(HKEY_LOCAL_MACHINE,
		"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
		"CurrentVersion",ret,&dwFlagsRet) )
	    {
		const SeparString ss( ret.buf(), '.' );
		if ( ss.size() > 1 )
		    ret = ss[1];
	    }

	    if ( ret.isEmpty() )
		ret.set( "Unknown minor version" );
	}
    }

    return ret.buf();
}


const char* getWinBuildNumber()
{
    mDeclStaticString( ret );
    if ( ret.isEmpty() )
    {
	if ( !readKey(HKEY_LOCAL_MACHINE,
	    "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
	    "CurrentBuildNumber",ret) )
	    ret.set( "Unknown build number" );
    }

    return ret.buf();
}

const char* getFullWinVersion()
{
    mDeclStaticString( ret );
    if ( ret.isEmpty() )
    {
	const BufferString winverstr( getWinVersion() );
	const BufferString winminverstr( getWinMinorVersion() );
	if ( winverstr.contains("Unknown") && winminverstr.contains("Unknown"))
	{
	    ret.add( getWinProductName() );
	}
	else
	{
	    ret.add( getWinVersion() ).add( "." )
	       .add( getWinMinorVersion() );
	}
    }

    return ret.buf();

}


const char* getWinDisplayName()
{
    mDeclStaticString( ret );
    if ( ret.isEmpty() )
    {
	if ( !readKey(HKEY_LOCAL_MACHINE,
	    "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
	    "DisplayVersion",ret ) )
	    ret.set( "Unknown display name" );
    }

    return ret.buf();
}


const char* getWinEdition()
{
    mDeclStaticString( ret );
    if ( ret.isEmpty() )
    {
	if ( !readKey(HKEY_LOCAL_MACHINE,
		      "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
		      "EditionID",ret) )
	    ret.set( "Unknown edition" );
    }

    return ret;
}


const char* getWinProductName()
{
    mDeclStaticString( ret );
    if ( ret.isEmpty() )
    {
	if ( !readKey(HKEY_LOCAL_MACHINE,
		      "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
		      "ProductName",ret) )
	    ret.set( "Unknown product name" );

	if ( WinUtils::isWindows11() )
	    ret.replace( "10", "11" );
    }

    return ret;
}


bool IsWindowsServer()
{
    static int res = -1;
    if ( res < 0 )
    {
	res = 0;
	BufferString ret;
	if ( readKey(HKEY_LOCAL_MACHINE,
	     "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
	     "InstallationType",ret) && ret == "Server" )
	    res = 1;
    }

    return res == 1;
}


bool canHaveAppLocker()
{
    const BufferString editionnm( getWinEdition() );
    if ( !editionnm.matches("Enterprise") )
        return false;

    const FilePath applockercachefp(GetSpecialFolderLocation(
                        CSIDL_SYSTEM), "AppLocker" );

    const DirList dl( applockercachefp.fullPath(),
                      File::FilesInDir );
    return dl.isEmpty() ? WinUtils::serviceIsRunning("AppIDSvc")
                        : true;
}


bool hasAppLocker()
{
    mDefineStaticLocalObject(bool, forceapplockertest,
	    = GetEnvVarYN("OD_EMULATE_APPLOCKER", false));
    if ( forceapplockertest )
	return true;

    return canHaveAppLocker();
}


bool execShellCmd( const char* comm, const char* parm, const char* runin )
{
    const HINSTANCE res = ShellExecute( NULL, "runas",
				 comm,
				 parm,    // params
				 runin, // directory
				 SW_SHOW );
    return static_cast<int>(reinterpret_cast<uintptr_t>(res)) >
			    HINSTANCE_ERROR;;
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


bool executeWinProg( const char* comm, const char* parm, const char* runin )
{
mStartAllowDeprecatedSection
     if ( !comm || !*comm ) return false;

     const StringView winversionstr( getWinVersion() );
     const int winversion = winversionstr.isNumber( true )
			  ? winversionstr.toInt() : 0;
     if ( winversion < 6 )
     {
	 BufferString com( comm, " " );
	 com += parm;
	 return execProc( com, true, true, runin );
     }

     return execShellCmd( comm, parm, runin );
mStopAllowDeprecatedSection
}


bool getDefaultApplication( const char* filetype,
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
	errmsg =  "Cannot query registry for default application";
		  " with file type ";
	errmsg.add( filetype );
	return false;
    }

    cmd = value;
    RegCloseKey( handle );
    return true;
}


bool getDefaultBrowser( BufferString& cmd, BufferString& errmsg )
{
    BufferString appkey = "HTTP";
    HKEY handle;
    LONG res = 0;
    const BufferString subkey = "SOFTWARE\\Microsoft\\Windows\\Shell\\"
		"Associations\\UrlAssociations\\http\\UserChoice";
    res = RegOpenKeyEx( HKEY_CURRENT_USER, subkey.buf(), 0, KEY_READ, &handle );
    if ( res == ERROR_SUCCESS )
    {
	CHAR value[512];
	DWORD bufsz = sizeof( value );
	res = RegQueryValueEx( handle, "ProgId", NULL, NULL,
			       (LPBYTE)value, &bufsz );
	if ( res == ERROR_SUCCESS )
	    appkey = value;
	RegCloseKey( handle );
    }

    return getDefaultApplication( appkey, cmd, errmsg );
}


bool setRegKeyVal( const char* ky, const char* vanrnm, const char *val )
{
    QSettings regkey( ky, QSettings::NativeFormat );
    regkey.setValue("Default", "");
    regkey.setValue( vanrnm, val );
    regkey.sync();
    const bool ret = regkey.status() == QSettings::NoError;
    return ret;
}

bool removeRegKey( const char* ky )
{
    QSettings regkey( ky, QSettings::NativeFormat );
    regkey.clear();
    regkey.sync();
    return regkey.status() == QSettings::NoError;
}


bool readKey( const HKEY hkey, const char* path, const char* key,
			BufferString& ret,
			LPDWORD dwFlagsRet, LPDWORD dwTypeRet )
{
    const int sz = dwFlagsRet && *dwFlagsRet == RRF_RT_REG_DWORD
	? sizeof( DWORD )*2: 1024;
    mDeclareAndTryAlloc( BYTE*, Value_data, BYTE[sz] );
    DWORD Value_size = (DWORD)sz;
    const DWORD dwFlags = dwFlagsRet ? *dwFlagsRet : RRF_RT_ANY;
    DWORD dwType;
    const LSTATUS retcode = RegGetValueA( hkey, path, key, dwFlags,
	    &dwType, Value_data, &Value_size );
    if ( retcode != ERROR_SUCCESS )
    {
	delete [] Value_data;
	return false;
    }

    if ( dwFlagsRet && *dwFlagsRet == RRF_RT_REG_DWORD )
    {
	const DWORD lowval = LOWORD( *Value_data );
	const od_uint32 lowvali = (od_uint32)lowval;
	ret.set( lowvali );
    }
    else
	ret.set( (const char*)Value_data );

    if ( dwTypeRet )
	*dwTypeRet = dwType;

    delete [] Value_data;

    return true;
}


void disableAutoSleep()
{
    /*Prevents the machine from sleeping
    https://msdn.microsoft.com/en-us/library/windows/
    desktop/aa373208(v=vs.85).aspx*/
    SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED );
}

void enableAutoSleep()
{
   SetThreadExecutionState(ES_CONTINUOUS);
}

#endif // __win__
