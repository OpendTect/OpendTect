/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : March 1994
 * FUNCTION : general utilities
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "genc.h"
#include "envvars.h"
#include "debug.h"
#include "oddirs.h"
#include "svnversion.h"
#include "bufstring.h"
#include "ptrman.h"
#include "filepath.h"
#include "staticstring.h"
#include "threadlock.h"
#include "od_iostream.h"
#include "iopar.h"
#include <iostream>
#include <string.h>

#include <math.h>
#include <stdlib.h>
#ifndef __win__
# include <unistd.h>
# include <errno.h>
# include <sys/types.h>
# include <signal.h>
#else
# include <float.h>
# include <time.h>
# include <sys/timeb.h>
# include <shlobj.h>
#endif



#define mConvDefFromStrToShortType(type,fn) \
void set( type& _to, const char* const& s ) { _to = (type)fn(s); }

mConvDefFromStrToShortType( short, atoi )
mConvDefFromStrToShortType( unsigned short, atoi )
mConvDefFromStrToSimpleType( int, (int)strtol(s,&endptr,0) )
mConvDefFromStrToSimpleType( od_uint32, (od_uint32)strtoul(s,&endptr,0) )
mConvDefFromStrToSimpleType( od_int64, strtoll(s,&endptr,0) )
mConvDefFromStrToSimpleType( od_uint64, strtoull(s,&endptr,0) )
mConvDefFromStrToSimpleType( double, strtod(s,&endptr) )
mConvDefFromStrToSimpleType( float, strtof(s,&endptr) )



static Threads::Lock& getEnvVarLock()
{
    mDefineStaticLocalObject( Threads::Lock, lock, (false) );
    return lock;
}

static IOPar envvar_entries;
static int insysadmmode_ = 0;
mExternC( Basic ) int InSysAdmMode(void) { return insysadmmode_; }
mExternC( Basic ) void SetInSysAdmMode(void) { insysadmmode_ = 1; }

#ifdef __win__
const char* GetLocalIP(void)
{
    mDefineStaticLocalObject( char, ret, [16] );

    struct in_addr addr;
    struct hostent* remotehost = gethostbyname( GetLocalHostName() );
    addr.s_addr = *(u_long *)remotehost->h_addr_list[0];
    strcpy( ret, inet_ntoa(addr) );
    return ret;
}


int initWinSock()
{
    WSADATA wsaData;
    WORD wVersion = MAKEWORD( 2, 0 ) ;
    return !WSAStartup( wVersion, &wsaData );
}
#endif


const char* GetLocalHostName()
{
    mDefineStaticLocalObject( char, ret, [256] );
#ifdef __win__
    initWinSock();
#endif
    gethostname( ret, 256 );
    return ret;
}


void SwapBytes( void* p, int n )
{
    int nl = 0;
    unsigned char* ptr = (unsigned char*)p;
    unsigned char c;

    if ( n < 2 ) return;
    n--;
    while ( nl < n )
    {
	c = ptr[nl]; ptr[nl] = ptr[n]; ptr[n] = c;
	nl++; n--;
    }
}


void PutIsLittleEndian( unsigned char* ptr )
{
#ifdef __little__
    *ptr = 1;
#else
    *ptr = 0;
#endif
}

#ifdef __msvc__
#include <process.h>
#include <direct.h>
#define getpid	_getpid
#define getcwd  _getcwd
#endif

int GetPID()
{
    return getpid();
}


void NotifyExitProgram( PtrAllVoidFn fn )
{
    mDefineStaticLocalObject( Threads::Atomic<int>, nrfns, (0) );
    mDefineStaticLocalObject( PtrAllVoidFn, fns, [100] );
    int idx;

    if ( fn == ((PtrAllVoidFn)(-1)) )
    {
	for ( idx=0; idx<nrfns; idx++ )
	    (*(fns[idx]))();
    }
    else
    {
        const int myfnidx = nrfns++;
	fns[myfnidx] = fn;
    }
}


mExternC(Basic) const char* GetLastSystemErrorMessage()
{
    mDeclStaticString( ret );
#ifndef __win__
    char buf[1024];
    ret = strerror_r( errno, buf, 1024 );
#endif
    return ret.buf();
}


mExternC(Basic) void ForkProcess(void)
{
#ifndef __win__
    switch ( fork() )
    {
    case 0:
	break;
    case -1:
	std::cerr << "Cannot fork: " << GetLastSystemErrorMessage() <<std::endl;
    default:
	ExitProgram( 0 );
    }
#endif
}


#define isBadHandle(h) ( (h) == NULL || (h) == INVALID_HANDLE_VALUE )

int isProcessAlive( int pid )
{
#ifdef __win__
    HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION, FALSE,
				   GetPID() );
    return isBadHandle(hProcess) ? 0 : 1;
#else
    const int res = kill( pid, 0 );
    return res == 0 ? 1 : 0;
#endif
}


int ExitProgram( int ret )
{
    if ( AreProgramArgsSet() && od_debug_isOn(DBG_PROGSTART) )
    {
	std::cerr << "\nExitProgram (PID: " << GetPID() << std::endl;
#ifndef __win__
	system( "date" );
#endif
    }

    NotifyExitProgram( (PtrAllVoidFn)(-1) );

// On Mac OpendTect crashes when calling the usual exit and shows error message
// dyld: odmain bad address of lazy symbol pointer passed to
// stub_binding_helper
// _Exit does not call registered exit functions and prevents crash
#ifdef __mac__
    _Exit( ret );
#endif

#ifdef __msvc__
    exit( ret ? EXIT_FAILURE : EXIT_SUCCESS );
#else
    exit(ret);
    return ret; // to satisfy (some) compilers
#endif
}


/*-> envvar.h */

mExtern(Basic) const char* GetOSEnvVar( const char* env )
{
    Threads::Locker lock( getEnvVarLock() );
    const char* res = getenv( env );
    if ( !res ) return 0;

    mDeclStaticString( resbuf );
    resbuf = res;
    return resbuf.buf();
}


static void loadEntries( const char* fnm, IOPar* iop=0 )
{
    if ( !fnm || !*fnm )
	return;

    od_istream strm( fnm );
    if ( !strm.isOK() )
	return;

    if ( !iop ) iop = &envvar_entries;
    BufferString line;
    while ( strm.getLine(line) )
    {
	char* nmptr = line.buf();
	mSkipBlanks(nmptr);
	if ( !*nmptr || *nmptr == '#' )
	    continue;

	char* valptr = nmptr;
	mSkipNonBlanks( valptr );
	if ( !*valptr ) continue;
	*valptr++ = '\0';
	mTrimBlanks(valptr);
	if ( !*valptr ) continue;

	iop->set( nmptr, valptr );
    }
}


mExtern(Basic) const char* GetEnvVar( const char* env )
{
    Threads::Locker lock( getEnvVarLock() );
    if ( !env || !*env )
	{ pFreeFnErrMsg( "Asked for empty env var", "GetEnvVar" ); return 0; }
    if ( insysadmmode_ )
	return GetOSEnvVar( env );

    mDefineStaticLocalObject( bool, filesread, = false );
    if ( !filesread )
    {
	if ( !AreProgramArgsSet() )
	{
	    //We should not be here before SetProgramInfo() is called.
	    pFreeFnErrMsg( "Use SetProgramArgs()", "GetEnvVar" );
	    return GetOSEnvVar( env );
	}

	filesread = true;
	loadEntries(GetSettingsFileName("envvars") );
	loadEntries(GetSetupDataFileName(ODSetupLoc_ApplSetupOnly,"EnvVars",1));
	loadEntries(GetSetupDataFileName(ODSetupLoc_SWDirOnly,"EnvVars",1) );
    }

    const char* res = envvar_entries.find( env );
    if ( !res ) res = GetOSEnvVar( env );

    mDeclStaticString( retbuf );

    retbuf = res;
    return retbuf.str();
}


mExtern(Basic) int GetEnvVarYN( const char* env, int defaultval )
{
    const char* s = GetEnvVar( env );
    if ( !s )
	return defaultval;

    return *s == '0' || *s == 'n' || *s == 'N' ? 0 : 1;
}


mExtern(Basic) int GetEnvVarIVal( const char* env, int defltval )
{
    const char* s = GetEnvVar( env );
    return s ? atoi(s) : defltval;
}


mExtern(Basic) double GetEnvVarDVal( const char* env, double defltval )
{
    const char* s = GetEnvVar( env );
    return s ? atof(s) : defltval;
}


mExtern(Basic) float GetEnvVarFVal( const char* env, float defltval )
{
    const char* s = GetEnvVar( env );
    return s ? atof(s) : defltval;
}


mExtern(Basic) int SetEnvVar( const char* env, const char* val )
{
    if ( !env || !*env )
	return mC_False;

    Threads::Locker lock( getEnvVarLock() );
#ifdef __msvc__
    _putenv_s( env, val );
#else
    setenv( env, val, 1 );
#endif

    return mC_True;
}


static bool writeEntries( const char* fnm, const IOPar& iop )
{
    od_ostream strm( fnm );
    if ( !strm.isOK() )
	return false;

    for ( int idx=0; idx<iop.size(); idx++ )
	strm << iop.getKey(idx) << od_tab << iop.getValue(idx) << od_endl;

    return strm.isOK();
}


mExtern(Basic) int WriteEnvVar( const char* env, const char* val )
{
    if ( !env || !*env )
	return 0;

    Threads::Locker lock( getEnvVarLock() );

    BufferString fnm( insysadmmode_
	    ? GetSetupDataFileName(ODSetupLoc_SWDirOnly,"EnvVars",1)
	    : GetSettingsFileName("envvars") );
    IOPar iop;
    loadEntries( fnm, &iop );
    iop.set( env, val );
    return writeEntries( fnm, iop ) ? 1 : 0;
}


mExternC(Basic) int GetSubversionRevision(void)
{ return mSVN_VERSION; }


mExternC(Basic) const char* GetSubversionUrl(void)
{ return mSVN_URL; }



int argc = -1;
BufferString initialdir;
char** argv = 0;


mExternC(Basic) char** GetArgV(void)
{ return argv; }


mExternC(Basic) int GetArgC(void)
{ return argc; }


mExternC(Basic) int AreProgramArgsSet(void)
{ return GetArgC()!=-1; }


mExternC(Basic) void SetProgramArgs( int newargc, char** newargv )
{
    getcwd( initialdir.buf(), initialdir.minBufSize() );

    argc = newargc;
    argv = newargv;

    od_putProgInfo( argc, argv );
}


static const char* getShortPathName( const char* path )
{
#ifndef __win__
    return path;
#else
    char fullpath[1024];
    GetModuleFileName( NULL, fullpath, (sizeof(fullpath)/sizeof(char)) );
    // get the fullpath to the exectuabe including the extension.
    // Do not use argv[0] on Windows
    mDeclStaticString( shortpath );
    shortpath.setMinBufSize( 1025 );
    GetShortPathName( fullpath, shortpath.buf(), shortpath.minBufSize()-1 );
    //Extract the shortname by removing spaces
    return shortpath;
#endif
}


mExternC(Basic) const char* GetFullExecutablePath( void )
{
    mDefineStaticLocalObject( BufferString, res, );
    mDefineStaticLocalObject( Threads::Lock, lock, );

    Threads::Locker locker( lock );

    if ( res.isEmpty() )
    {
	FilePath executable = GetArgV()[0];
	if ( !executable.isAbsolute() )
	{
	    FilePath filepath = initialdir.buf();
	    filepath.add( GetArgV()[0] );
	    executable = filepath;
	}

	res = getShortPathName( executable.fullPath() );
    }

    return res;
}

