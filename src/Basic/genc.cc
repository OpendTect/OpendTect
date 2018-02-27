/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : March 1994
 * FUNCTION : general utilities
-*/

#include "genc.h"

#include "debug.h"
#include "oddirs.h"
#include "oscommand.h"
#include "commondefs.h"
#include "buildinfo.h"
#include "bufstring.h"
#include "ptrman.h"
#include "file.h"
#include "filepath.h"
#include "staticstring.h"
#include "threadlock.h"
#include "survinfo.h"
#include "od_iostream.h"
#include "odmemory.h"
#include "convert.h"
#include "iopar.h"
#include <iostream>
#include <string.h>

#include <stdlib.h>
#ifdef __win__
# include <float.h>
# include <time.h>
# include <sys/timeb.h>
# include <shlobj.h>
# include <Psapi.h>
#else
# include <unistd.h>
# include <errno.h>
# include <signal.h>
#endif

mGlobal(Basic) void SetBaseDataDir(const char*);


#ifdef __lux__
static Threads::Atomic<int> canovercommit( 0 );
//0 = don't know
//1 = yes
//-1 = no
#endif

//In release mode, use own operator to allocate memory.
#ifndef __debug__

static bool memCanOverCommit()
{
# ifndef __lux__
    return false;
# else
    if ( canovercommit )
	return canovercommit == 1;

    // file should exist as of kernel 2.1.27
    FILE *file = fopen( "/proc/sys/vm/overcommit_memory", "r" );
    if ( !file )
	{ canovercommit = -1; return false; }

    fseek(file,0,SEEK_END);
    const long filesize = ftell(file);
    if ( filesize == 0 ) // same as if the file would contain '\0'
    {
	fclose( file );
	canovercommit = 1;
	return true;
    }

    char* contents = (char*) malloc( sizeof(char) * 1 );
    rewind(file);
    const size_t readres = fread(&contents[0],1,1,file);
    fclose( file );

    if ( readres>0 )
    {
	//https://www.win.tue.nl/~aeb/linux/lk/lk-9.html
	// values valid since kernel 2.5.30
	canovercommit = contents[0] == '\0' || contents[0] == '\1' ? 1 : -1;
    }
    else
    {
	canovercommit = -1;
    }

    free( contents );

    return canovercommit == 1;
# endif
}


# ifdef __win__
void* operator new( std::size_t sz )
# else
void* operator new( std::size_t sz ) throw(std::bad_alloc)
# endif
{
    void* p = malloc( sz );
    if ( !p )
	throw std::bad_alloc();

    //By writing into the mem, we force the os to allocate the memory
    if ( memCanOverCommit() )
	OD::sysMemZero( p, sz );

    return p;
}


mDefParallelCalc2Pars(MemPageMemorySetter,uiString::empty(),
		      unsigned char*,data,std::size_t,step)
mDefParallelCalcBody(
	unsigned char* ptr = data_ + start * step_;
    ,
	*ptr = '\0';
	ptr += step_;
    ,
)


# ifdef __win__
void* operator new[]( std::size_t sz )
# else
void* operator new[]( std::size_t sz ) throw(std::bad_alloc)
# endif
{
    void* p = malloc( sz );
    if ( !p )
	throw std::bad_alloc();

    //By writing into the mem, we force the os to allocate the memory
    if ( !memCanOverCommit() )
	return p;

#ifdef __lux__
    if ( sz > mODMemMinThreadSize )
    {
	/* Setting one or two values per memory page is sufficient
	   to ensure memory is allocated */
	const od_int64 step = sysconf( _SC_PAGESIZE ) - 2;
	const od_int64 nriterations = mCast(od_int64,mCast(float,sz)/step);
	MemPageMemorySetter setter( nriterations, (unsigned char*)p, step );
	setter.setReport( false );
	setter.execute();
    }
    else
# endif
	OD::sysMemZero( p, sz );

    return p;
}
#endif


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
int initWinSock()
{
    WSADATA wsaData;
    WORD wVersion = MAKEWORD( 2, 0 ) ;
    return !WSAStartup( wVersion, &wsaData );
}
#endif

static PtrMan<BufferString> osident = 0;

const char* GetOSIdentifier()
{
    if ( osident )
	return osident->buf();

    BufferString* tmp = new BufferString;

#ifdef __win__

    DWORD dwVersion = 0;
    DWORD dwMajorVersion = 0;
    DWORD dwMinorVersion = 0;
    DWORD dwBuild = 0;

#pragma warning( push )
#pragma warning( disable:4996 )
    dwVersion = GetVersion();
#pragma warning( pop )

    // Get the Windows version.
    dwMajorVersion = (DWORD)(LOBYTE(LOWORD(dwVersion)));
    dwMinorVersion = (DWORD)(HIBYTE(LOWORD(dwVersion)));

    // Get the build number.
    if (dwVersion < 0x80000000)
	dwBuild = (DWORD)(HIWORD(dwVersion));

    const char* dot = ".";
    tmp->add( "Windows ").add( (od_uint64) dwMajorVersion )
	.add( dot ).add((od_uint64) dwMinorVersion )
	.add( dot ).add( (od_uint64) dwBuild );
#endif

#ifdef __lux__
    if ( !OS::ExecCommand( "lsb_release -d", OS::Wait4Finish, tmp ) )
	tmp->set( "Unknown Linux");
#endif

    osident.setIfNull( tmp, true );

    return osident->buf();
}


#ifdef __win__

typedef BOOL(WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);


bool is64BitWindows()
{
#if _WIN64
    return true;
#elif _WIN32

    LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS)
	GetProcAddress(GetModuleHandle(TEXT("kernel32")), "IsWow64Process");

    if ( fnIsWow64Process )
    {
	BOOL res;
	if ( !fnIsWow64Process( GetCurrentProcess(), &res ) )
	    return false;

	return res;
    }

    return false;
#endif
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

static bool is_exiting_ = false;


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


bool IsExiting() { return is_exiting_; }


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


bool isProcessAlive( int pid )
{
#ifdef __win__
    HANDLE hProcess = OpenProcess( SYNCHRONIZE, FALSE, pid );
    DWORD ret = WaitForSingleObject( hProcess, 0 );
    CloseHandle( hProcess );
    return ret == WAIT_TIMEOUT;
#else
    const int res = kill( pid, 0 );
    return res == 0;
#endif
}


const char* GetProcessNameForPID( int pid )
{
    mDeclStaticString( ret );
    BufferString procname;
#ifdef __win__
    HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
				   PROCESS_VM_READ, FALSE, pid );
    char procnamebuff[MAX_PATH];
    ret.setEmpty();
    if ( hProcess )
    {
	GetModuleFileNameEx( hProcess, 0, procnamebuff, MAX_PATH );
	procname = procnamebuff;
    }
#else
    const BufferString cmd( "ps -p ", pid, " -o command=" );
    BufferString stdoutput, stderror;
    OS::ExecCommand( cmd, OS::Wait4Finish, &stdoutput, &stderror );
    procname = !stdoutput.isEmpty() ? stdoutput : stderror.isEmpty()
						? "" : stderror;
    char* ptrfirstspace = procname.find(' ');
    if ( ptrfirstspace ) *ptrfirstspace = '\0';
#endif
    const File::Path procpath( procname );
    ret = procpath.fileName();
    return ret.isEmpty() ? 0 : ret.buf();
}


static int doExitProgram( int ret )
{
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


int ExitProgram( int ret )
{
    is_exiting_ = true;
    if ( AreProgramArgsSet() && od_debug_isOn(DBG_PROGSTART) )
    {
	std::cerr << "\nExitProgram (PID: " << GetPID() << std::endl;
#ifndef __win__
	int dateres mUnusedVar = system( "date" );
#endif
    }

    NotifyExitProgram( (PtrAllVoidFn)(-1) );

    return doExitProgram( ret );
}


bool StartProgramCopy()
{
    if ( AreProgramArgsSet() && od_debug_isOn(DBG_PROGSTART) )
    {
	std::cerr << "\nCreating Program copy (PID: " << GetPID() << std::endl;
#ifndef __win__
	int dateres mUnusedVar = system( "date" );
#endif
    }

    const BufferString fullcmdline( GetFullCommandLine() );
    return OS::ExecCommand( fullcmdline, OS::RunInBG );
}


static void basicProgramRestarter()
{
    if ( StartProgramCopy() )
	ExitProgram( 0 );
}


ProgramRestartFn GetBasicProgramRestarter()
{
    return basicProgramRestarter;
}

ProgramRestartFn program_restarter_ = basicProgramRestarter;

void SetProgramRestarter( ProgramRestartFn fn )
{
    program_restarter_ = fn;
}

void RestartProgram()
{
    program_restarter_();
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


mExtern(Basic) void UnsetOSEnvVar( const char* env )
{
#ifdef __msvc__
    _putenv_s( env, "" );
#else
    unsetenv( env );
#endif
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
	char* nmptr = line.getCStr();
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
	{ pFreeFnErrMsg( "Asked for empty env var" ); return 0; }
    if ( insysadmmode_ )
	return GetOSEnvVar( env );

    mDefineStaticLocalObject( bool, filesread, = false );
    if ( !filesread )
    {
	if ( !AreProgramArgsSet() )
	{
	    //We should not be here before SetProgramInfo() is called.
	    pFreeFnErrMsg( "Use SetProgramArgs()" );
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


mExtern(Basic) bool GetEnvVarYN( const char* env, bool defaultval )
{
    const char* s = GetEnvVar( env );
    if ( !s )
	return defaultval;

    return *s == '0' || *s == 'n' || *s == 'N' ||
	   *s == 'f' || *s == 'F' ? false : true;
}


mExtern(Basic) int GetEnvVarIVal( const char* env, int defltval )
{
    const char* s = GetEnvVar( env );
    return toInt( s, defltval );
}


mExtern(Basic) double GetEnvVarDVal( const char* env, double defltval )
{
    const char* s = GetEnvVar( env );
    return toDouble( s, defltval );
}


mExtern(Basic) float GetEnvVarFVal( const char* env, float defltval )
{
    const char* s = GetEnvVar( env );
    return toFloat( s, defltval );
}


mExtern(Basic) void SetEnvVar( const char* env, const char* val )
{
    if ( !env || !*env || !val )
	return;

    Threads::Locker lock( getEnvVarLock() );
#ifdef __msvc__
    _putenv_s( env, val );
#else
    setenv( env, val, 1 );
#endif
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


mExtern(Basic) bool WriteEnvVar( const char* env, const char* val )
{
    if ( !env || !*env )
	return false;

    Threads::Locker lock( getEnvVarLock() );

    BufferString fnm( insysadmmode_
	    ? GetSetupDataFileName(ODSetupLoc_SWDirOnly,"EnvVars",1)
	    : GetSettingsFileName("envvars") );
    IOPar iop;
    loadEntries( fnm, &iop );
    iop.set( env, val );
    return writeEntries( fnm, iop ) ? 1 : 0;
}


mExternC(Basic) const char* GetVCSVersion(void)
{ return mVCS_VERSION; }


static int argc_ = -1;
static BufferString initialdir_;
static char** argv_ = 0;


mExtern(Basic) char** GetArgV(void)
{ return argv_; }


mExtern(Basic) int& GetArgC(void)
{ return argc_; }


mExtern(Basic) bool AreProgramArgsSet(void)
{ return GetArgC()!=-1; }


#ifndef __win__
static void insertInPath( const char* envkey, const char* dir, const char* sep )
{
    BufferString pathval( GetEnvVar(envkey) );
    if ( pathval.isEmpty() )
	pathval.set( dir );
    else
	pathval.insertAt( 0, BufferString(dir,sep) );
    SetEnvVar( envkey, pathval.buf() );
}
#endif


//Implemented in src/Basic/qpaths.cc
void setQtPaths();


static void getDataRoot( bool isrequired )
{
    BufferString dataroot = GetBaseDataDir();

    bool drootinargs = false;
    if ( argc_ > 2 && FixedString(argv_[1]) == "--dataroot" )
    {
	dataroot = File::linkEnd( argv_[2] );
	drootinargs = true;
	// These args are of no use to anyone else. Let's remove them ...
	for ( int idx=3; idx<argc_; idx++ )
	    argv_[idx-2] = argv_[idx];
	argc_ -= 2;
    }

    const uiRetVal uirv = SurveyInfo::isValidDataRoot( dataroot );
    if ( !uirv.isOK() && isrequired )
    {
	ErrMsg( BufferString( argv_[0], ": ", uirv.getText() ) );
	ExitProgram( 1 );
    }

    if ( drootinargs )
	SetBaseDataDir( dataroot );
}


mExtern(Basic) void SetProgramArgs( int argc, char** argv, bool drrequired )
{
    char* getcwdres = getcwd( initialdir_.getCStr(), initialdir_.minBufSize() );
    if ( !getcwdres )
	{ pFreeFnErrMsg("Cannot read current directory"); }

    argc_ = argc;
    argv_ = new char* [argc_];
    for ( int idx=0; idx<argc_; idx++ )
	argv_[idx] = argv[idx];

    od_putProgInfo( argc_, argv_ );
    getDataRoot( drrequired );

#ifndef __win__
    File::Path fp( GetFullExecutablePath() );
    const BufferString execdir( fp.pathOnly() );
    insertInPath( "PATH", execdir.buf(), ":" );
    insertInPath( "path", execdir.buf(), " " );
# ifdef __mac__
    insertInPath( "DYLD_LIBRARY_PATH", execdir.buf(), ":" );
# else
    insertInPath( "LD_LIBRARY_PATH", execdir.buf(), ":" );
# endif
#endif

    // Set this so that scripts run from the program have it available
# ifdef __mac__
    BufferString datfp( File::Path(GetSoftwareDir(0), "Resources").fullPath());
    SetEnvVar( "DTECT_APPL", datfp );
#else
    SetEnvVar( "DTECT_APPL", GetSoftwareDir(true) );
#endif

    setQtPaths();
}


static const char* getShortPathName( const char* path )
{
#ifndef __win__
    return path;
#else
    char fullpath[1024];

    // get the fullpath to the exectuabe including the extension.
    // Necessary because we cannot use argv[0] on Windows
    GetModuleFileName( NULL, fullpath, (sizeof(fullpath)/sizeof(char)) );

	//Extract the shortname by removing spaces
    mDeclStaticString( shortpath );
    shortpath.setMinBufSize( 1025 );
    GetShortPathName( fullpath, shortpath.getCStr(), shortpath.minBufSize()-1 );

    return shortpath;
#endif
}


mExternC(Basic) const char* GetFullCommandLine( void )
{
    mDefineStaticLocalObject( BufferString, res, );
    mDefineStaticLocalObject( Threads::Lock, lock, );

    Threads::Locker locker( lock );

    if ( res.isEmpty() )
    {
	for ( int idx=0; idx<argc_; idx++ )
	    res.add( argv_[idx] ).add( " " );
    }

    return res;
}


mExternC(Basic) const char* GetFullExecutablePath( void )
{
    mDefineStaticLocalObject( BufferString, res, );
    mDefineStaticLocalObject( Threads::Lock, lock, );

    Threads::Locker locker( lock );

    if ( res.isEmpty() )
    {
	File::Path fpargv0 = argv_[0];
	if ( !fpargv0.isAbsolute() )
	    fpargv0 = File::Path( initialdir_, argv_[0] );

	res = getShortPathName( fpargv0.fullPath() );
    }

    return res;
}


mExternC(Basic) const char* GetExecutableName( void )
{
    mDefineStaticLocalObject( BufferString, res, );
    mDefineStaticLocalObject( Threads::Lock, lock, );

    Threads::Locker locker( lock );

    if ( res.isEmpty() )
    {
	File::Path fpargv0 = argv_[0];
	if ( !fpargv0.isAbsolute() )
	    fpargv0 = File::Path( initialdir_, argv_[0] );

	fpargv0.setExtension( 0 );
	res = fpargv0.fileName();
    }

    return res;
}

mExternC(Basic) void sleepSeconds( double secs )
{
    if ( secs > 0 )
	Threads::sleep( secs );
}
