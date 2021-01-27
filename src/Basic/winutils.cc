/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Lammertink
 * DATE     : November 2004
 * FUNCTION : Utilities for win32, amongst others path conversion
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
#include "ptrman.h"
#include "staticstring.h"
#include "string2.h"

#ifdef __win__
// These get rid of warnings about casting HINSTANCE to int
#pragma warning( disable : 4302 )
#pragma warning( disable : 4311 )

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

bool WinUtils::fileInUse( const char* fnm )
{
    HANDLE handle = CreateFileA(fnm,
        GENERIC_READ | GENERIC_WRITE,
        0,
        0,
        OPEN_EXISTING,
        0,
        0);
    const bool ret = handle == INVALID_HANDLE_VALUE;
    CloseHandle(handle);
    return ret;
}


namespace WinUtils
{
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
}


bool WinUtils::pathContainsTrustedInstaller( const char* fnm )
{
    const File::Path fp( fnm );
    for ( int idx=0; idx<fp.nrLevels(); idx++ )
    {
	const BufferString filenm( fp.dirUpTo(idx) );
	if ( belongsToTrusterInstaller(filenm) )
	    return true;
    }

    return false;
}


bool WinUtils::belongsToStdUser( const char* fnm )
{
    const SecurityID retSid = getFileSID( fnm );
    if ( !retSid.isOK() ||
	 !getAdminSID().isOK() ||
	 !getTrustedInstallerSID().isOK() )
	return true;
    return retSid != getAdminSID() &&
	   retSid != getTrustedInstallerSID();
}


bool WinUtils::belongsToAdmin( const char* fnm )
{
    const SecurityID retSid = getFileSID( fnm );
    if ( !retSid.isOK() || !getAdminSID().isOK() )
	return false;
    return retSid == getAdminSID();
}


bool WinUtils::belongsToTrusterInstaller( const char* fnm )
{
    const SecurityID retSid = getFileSID( fnm );
    if ( !retSid.isOK() ||
	 !getTrustedInstallerSID().isOK() )
	return false;
    return retSid == getTrustedInstallerSID();
}


bool WinUtils::serviceIsRunning( const char* nm )
{
    return getServiceStatus(nm) == SERVICE_RUNNING;
}


int WinUtils::getServiceStatus( const char* nm )
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





unsigned int WinUtils::getWinVersion()
{
    mDeclStaticString( ret );
    if ( ret.isEmpty() )
    {
	DWORD dwFlagsRet = RRF_RT_REG_DWORD;
	if ( !readKey( HKEY_LOCAL_MACHINE,
	    "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
	    "CurrentMajorVersionNumber", ret, &dwFlagsRet) )
	    ret.set( "Unknown major version" );
    }

    return ret.toInt();
}


unsigned int WinUtils::getWinMinorVersion()
{
    mDeclStaticString( ret );
    if ( ret.isEmpty() )
    {
	DWORD dwFlagsRet = RRF_RT_REG_DWORD;
	if ( !readKey( HKEY_LOCAL_MACHINE,
	    "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
	    "CurrentMinorVersionNumber", ret, &dwFlagsRet ) )
	    ret.set( "Unknown minor version" );
    }

    return ret.toInt();
}


const char* WinUtils::getWinBuildNumber()
{
    mDeclStaticString( ret );
    if ( ret.isEmpty() )
    {
	if ( !readKey( HKEY_LOCAL_MACHINE,
	    "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
	    "CurrentBuildNumber", ret ) )
	    ret.set( "Unknown build number" );
    }

    return ret.buf();
}

const char* WinUtils::getFullWinVersion()
{
    mDeclStaticString( ret );
    if ( ret.isEmpty() )
    {
	ret.add( getWinVersion() ).add( "." )
	   .add( getWinMinorVersion() );
    }

    return ret.buf();

}


const char* WinUtils::getWinDisplayName()
{
    mDeclStaticString( ret );
    if ( ret.isEmpty() )
    {
	if ( !readKey( HKEY_LOCAL_MACHINE,
	    "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
	    "DisplayVersion", ret ) )
	    ret.set( "Unknown display name" );
    }

    return ret.buf();
}


const char* WinUtils::getWinEdition()
{
    mDeclStaticString(ret);
    if ( ret.isEmpty() )
    {
	if ( !readKey(HKEY_LOCAL_MACHINE,
		      "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
		      "EditionID", ret) )
	    ret.set( "Unknown edition" );
    }

    return ret;
}


const char* WinUtils::getWinProductName()
{
    mDeclStaticString(ret);
    if ( ret.isEmpty() )
    {
	if ( !readKey(HKEY_LOCAL_MACHINE,
		      "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
		      "ProductName", ret) )
	    ret.set( "Unknown product name" );
    }

    return ret;
}


bool WinUtils::canHaveAppLocker()
{
    const BufferString editionnm( getWinEdition() );
    if ( !editionnm.matches("Enterprise") )
        return false;

    const File::Path applockercachefp(getSpecialFolderLocation(
                        CSIDL_SYSTEM), "AppLocker" );

    const DirList dl( applockercachefp.fullPath(),
                      File::FilesInDir );
    return dl.isEmpty() ? serviceIsRunning("AppIDSvc")
                        : true;
}


bool WinUtils::hasAppLocker()
{
    mDefineStaticLocalObject(bool, forceapplockertest,
        = GetEnvVarYN("OD_EMULATE_APPLOCKER", false));
    if ( forceapplockertest )
        return true;

    return canHaveAppLocker();
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


bool WinUtils::NTUserBelongsToAdminGrp()
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


bool WinUtils::IsUserAnAdmin()
{
    mDefineStaticLocalObject(bool, isadmin, = ::IsUserAnAdmin() );
    return isadmin;
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


bool WinUtils::readKey( const HKEY hkey, const char* path, const char* key,
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

#endif // __win__
