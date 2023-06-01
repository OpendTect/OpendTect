/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "genc.h"

#include "applicationdata.h"
#include "commandlineparser.h"
#include "envvars.h"
#include "debug.h"
#include "oddirs.h"
#include "oscommand.h"
#include "commondefs.h"
#include "buildinfo.h"
#include "bufstring.h"
#include "ptrman.h"
#include "file.h"
#include "filepath.h"
#include "moddepmgr.h"
#include "perthreadrepos.h"
#include "threadlock.h"
#include "od_iostream.h"
#include "odmemory.h"
#include "odruncontext.h"
#include "plugins.h"
#include "separstr.h"
#include "convert.h"
#include "iopar.h"
#include "survinfo.h"
#include <iostream>
#include <string.h>

#include <math.h>
#include <stdlib.h>
#ifdef __win__
# include <float.h>
# include <time.h>
# include "winutils.h"
# include <sys/timeb.h>
# include <shlobj.h>
# include <Psapi.h>
# define mEnvVarDirSep ';'
#else
# include <unistd.h>
# include <errno.h>
# include <netdb.h>
# include <stdio.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <sys/types.h>
# include <signal.h>
# define mEnvVarDirSep ':'
#endif

#include <QString>
#include <QSysInfo>


static BufferString		initialdir_;
static int			argc_ = -1;
static char**			argv_ = nullptr;
static bool			needdataroot_ = true;
int& GetArgC()			{ return argc_; }
char** GetArgV()		{ return argv_; }
bool NeedDataBase()		{ return needdataroot_; }
bool AreProgramArgsSet()	{ return argc_ != -1; }


static OD::RunCtxt runctxt_ = OD::UnknownCtxt;
namespace OD
{
    RunCtxt GetRunContext()		{ return runctxt_; }
    void SetRunContext( RunCtxt rm )	{ runctxt_ = rm; }
}


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
    fread(&contents[0],1,1,file);
    fclose( file );

    //https://www.win.tue.nl/~aeb/linux/lk/lk-9.html
    // values valid since kernel 2.5.30
    canovercommit = contents[0] == '\0' || contents[0] == '\1' ? 1 : -1;
    free( contents );

    return canovercommit == 1;
# endif
}


void* operator new( std::size_t sz )
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


void* operator new[]( std::size_t sz )
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
	MemPageMemorySetter memsetter( nriterations, (unsigned char*)p, step );
	memsetter.setReport( false );
	memsetter.execute();
    }
    else
# endif
	OD::sysMemZero( p, sz );

    return p;
}
#endif


#define mConvDefFromStrToShortType(type,fn) \
namespace Conv { \
    template <> void set( type& _to, const char* const& s ) \
	{ _to = sCast(type,fn(s)); } \
    template <> void set( type& _to, const StringView& s ) \
	{ if ( !s.isEmpty() ) { _to = sCast(type,fn(s.str())); } } \
    template <> void set( type& _to, const BufferString& s ) \
	{ if ( !s.isEmpty() ) { _to = sCast(type,fn(s.str())); } } \
}

mConvDefFromStrToShortType( short, atoi )
mConvDefFromStrToShortType( unsigned short, atoi )
mConvDefFromStrToSimpleType( int, strtol(s,&endptr,0) )
mConvDefFromStrToSimpleType( od_uint32, strtoul(s,&endptr,0) )
mConvDefFromStrToSimpleType( od_int64, strtoll(s,&endptr,0) )
mConvDefFromStrToSimpleType( od_uint64, strtoull(s,&endptr,0) )
mConvDefFromStrToSimpleType( double, strtod(s,&endptr) )
mConvDefFromStrToSimpleType( float, strtof(s,&endptr) )


static Threads::Lock& getEnvVarLock()
{
    mDefineStaticLocalObject( Threads::Lock, lock, (false) )
    return lock;
}

static IOPar& sEnvVarEntries()
{
    static PtrMan<IOPar> envvar_entries = new IOPar;
    return *envvar_entries;
}

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


const char* GetOSIdentifier()
{
    mDefineStaticLocalObject( PtrMan<BufferString>, ret, )
    if ( ret )
	return ret->buf();

    auto* tmp = new BufferString;

#ifdef __win__
    const char* dot = ".";
    tmp->add( "Windows " ).add( getFullWinVersion() )
	.add( dot ).add( getWinBuildNumber() )
	.add( " (" ).add( getWinEdition() ).add( ": ")
	.add( getWinDisplayName() ).add( ")" );
#elif __lux__
    OS::MachineCommand machcomm( "lsb_release", "-d" );
    if ( !machcomm.execute(*tmp) )
	tmp->set( "Unknown Linux");
#endif

    ret.setIfNull( tmp, true );
    return ret->buf();
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


static void mUnusedVar GetLocalHostNameNoQt( char* ret )
{
#ifdef __win__
    initWinSock();
#endif
    gethostname( ret, 256 );
}


static BufferString localhostnameoverrule;
extern "C" { mGlobal(Basic) void SetLocalHostNameOverrule(const char*); }
mExternC(Basic) void SetLocalHostNameOverrule(const char* hostnm)
{
    mDefineStaticLocalObject(Threads::Mutex, mutex, )
    Threads::MutexLocker lock(mutex);
    localhostnameoverrule = hostnm;
}


const char* GetLocalHostName()
{
    mDefineStaticLocalObject( PtrMan<BufferString>, ret, = new BufferString() )
    if ( !ret->isEmpty() )
	return ret->str();

    if ( !localhostnameoverrule.isEmpty() )
	ret->set( localhostnameoverrule );

#ifndef OD_NO_QT
# if QT_VERSION >= QT_VERSION_CHECK(5,6,0)
    BufferString hostnm( QSysInfo::machineHostName() );
    char* dot = hostnm.find( '.' );
    if ( dot ) { *dot ='\0'; }
    ret->set( hostnm );
# else
    GetLocalHostNameNoQt( ret );
# endif
#else
    GetLocalHostNameNoQt( ret );
#endif
    return ret->str();
}

static const char* noAddrFn( bool )	{ return nullptr; }
using constcharFromBoolFn = const char*(*)(bool);
static constcharFromBoolFn localaddrfn_ = noAddrFn;

mGlobal(Basic) void setGlobal_Basic_Fns(constcharFromBoolFn);
void setGlobal_Basic_Fns( constcharFromBoolFn addrfn )
{
    localaddrfn_ = addrfn;
}


extern "C" { mGlobal(Basic) const char* GetLocalAddress( bool ipv4only ); }
mExternC(Basic) const char* GetLocalAddress( bool ipv4only )
{
    return (*localaddrfn_)( ipv4only );
}


void SwapBytes( void* p, int n )
{
    int nl = 0;
    unsigned char* ptr = sCast(unsigned char*,p);
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
    mDefineStaticLocalObject( Threads::Atomic<int>, nrfns, (0) )
    mDefineStaticLocalObject( PtrAllVoidFn, fns, [100] )
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


const char* getProcessNameForPID( int pid )
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
    OS::MachineCommand machcomm( "ps" );
    machcomm.addKeyedArg( "p", toString(pid), OS::OldStyle )
	    .addKeyedArg( "o", "command=", OS::OldStyle );
    BufferString stdoutput,stderror;
    if ( machcomm.execute(stdoutput,&stderror) )
    {
	const bool hasstdout = !stdoutput.isEmpty();
	BufferString& retstr = hasstdout ? stdoutput : stderror;
	if ( !hasstdout )
	    retstr.embed( '<', '>' );
	procname = retstr;
	char* ptrfirstspace = procname.find(' ');
	if ( ptrfirstspace ) *ptrfirstspace = '\0';
    }
    else
	procname.setEmpty().embed( '<', '>' );

#endif
    const FilePath procpath( procname );
    ret = procpath.fileName();
    return ret.isEmpty() ? nullptr : ret.buf();
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
	std::cerr << "\nExitProgram (PID: " << GetPID() << ")" << std::endl;
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

    OS::MachineCommand machcomm( argv_[0] );
    for ( int idx=1; idx<argc_; idx++ )
	machcomm.addArg( argv_[idx] );
    return machcomm.execute( OS::RunInBG );
}


static void basicProgramRestarter()
{
    if ( StartProgramCopy() )
	ApplicationData::exit( 0 );
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
#ifdef __win__
    BufferString res( 4096, false );
    size_t written;
    if ( getenv_s(&written,res.getCStr(),res.bufSize(),env) != 0 ||
	 written == 0 )
	return nullptr;
#else
    const char* res = getenv( env );
#endif

    mDeclStaticString( resbuf );
    resbuf = res;
    return resbuf.str();
}


mExtern(Basic) void UnsetOSEnvVar( const char* env )
{
#ifdef __msvc__
    _putenv_s( env, "" );
#else
    unsetenv( env );
#endif
}


static void loadEntries( const char* fnm, IOPar* iop=nullptr )
{
    if ( !fnm || !*fnm )
	return;

    od_istream strm( fnm );
    if ( !strm.isOK() )
	return;

    if ( !iop )
	iop = &sEnvVarEntries();

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
    {
	pFreeFnErrMsg( "Asked for empty env var" );
	return nullptr;
    }

    if ( insysadmmode_ )
	return GetOSEnvVar( env );

    mDefineStaticLocalObject( bool, filesread, = false )
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

    BufferString res = sEnvVarEntries().find( env );
    if ( res.isEmpty() )
	res = GetOSEnvVar( env );

    mDeclStaticString( retbuf );

    retbuf = res;
    return retbuf.str();
}


mExtern(Basic) bool GetEnvVarDirList( const char* env, BufferStringSet& ret,
				      bool checkdirs )
{
    if ( !env || !*env )
	return false;

    Threads::Locker lock( getEnvVarLock() );
    ret.setEmpty();
    const BufferString allpaths( GetEnvVar(env) );
    if ( allpaths.isEmpty() )
	return false;

    const SeparString allpathssep( allpaths, mEnvVarDirSep );
    for ( int idx=0; idx<allpathssep.size(); idx++ )
    {
	const StringView curpath( allpathssep[idx] );
	if ( !checkdirs ||
	     (File::exists(curpath) && File::isDirectory(curpath)) )
	{
	    FilePath curfp( curpath );
	    curfp.makeCanonical();
	    ret.addIfNew( curfp.fullPath() );
	}
    }

    return !ret.isEmpty();
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


mExtern(Basic) void SetEnvVarDirList( const char* env,
				      const BufferStringSet& dirs,
				      bool appendnoerase )
{
    if ( !env || !*env || dirs.isEmpty() )
	return;

    Threads::Locker lock( getEnvVarLock() );
    BufferStringSet dirsedit;
    if ( appendnoerase )
    {
	BufferStringSet existingdirs;
	if ( GetEnvVarDirList(env,existingdirs,true) )
	    dirsedit.add( existingdirs, false );
    }
    dirsedit.add( dirs, false );

    const BufferString ret( dirsedit.cat( BufferString().add(mEnvVarDirSep) ) );
    SetEnvVar( env, ret );
}


mExtern(Basic) const char* GetEnvVarDirListWoOD( const char* ky,
						 const char* filter )
{
    mDeclStaticString( ret );
    ret.setEmpty();

    BufferStringSet pathdirs;
    if ( !GetEnvVarDirList(ky,pathdirs,true) )
	return ret;

    BufferString instdir( GetSoftwareDir(false) );
    if ( instdir.isEmpty() )
	ret.set( GetEnvVar(ky) );
    else
    {
	FilePath odinstfp( instdir );
	odinstfp.makeCanonical();
	BufferStringSet accepteddirs;
	for ( const auto pathdir : pathdirs )
	{
	    FilePath pathdirfp( pathdir->buf() );
	    pathdirfp.makeCanonical();
	    if ( pathdirfp == odinstfp || pathdirfp.isSubDirOf(odinstfp) ||
		 pathdir->contains(instdir.buf()) )
		continue;
	    if ( filter && pathdir->contains(filter) )
		continue;
	    accepteddirs.add( pathdir->buf() );
	}
	ret.set( accepteddirs.cat( BufferString().add(mEnvVarDirSep) ) );
    }

    return ret;
}


static bool writeEntries( const char* fnm, const IOPar& iop )
{
    od_ostream strm( fnm );
    if ( !strm.isOK() )
	return false;

    IOParIterator iter( iop );
    BufferString key, val;
    while ( iter.next(key,val) )
	strm << key << od_tab << val << od_endl;

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


mExtern(Basic) bool SetProgramArgs( int argc, char** argv, bool ddrequired )
{
    char* getcwdres = getcwd( initialdir_.getCStr(), initialdir_.minBufSize() );
    if ( !getcwdres )
	{ pFreeFnErrMsg("Cannot read current directory"); }

    argc_ = argc;
    argv_ = new char* [argc_];
    for ( int idx=0; idx<argc_; idx++ )
	argv_[idx] = argv[idx];

    od_putProgInfo( argc_, argv_ );
    needdataroot_ = ddrequired;

#ifdef __unix__
    FilePath fp( GetFullExecutablePath() );
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
    BufferString datfp( FilePath(GetSoftwareDir(0), "Resources").fullPath());
    SetEnvVar( "DTECT_APPL", datfp );
#else
    SetEnvVar( "DTECT_APPL", GetSoftwareDir(true) );
#endif
    return true;
}

static BufferString execnmoverrule;
extern "C" { mGlobal(Basic) void SetExecutableNameOverrule(const char*); }
mExternC(Basic) void SetExecutableNameOverrule( const char* execnm )
{
    mDefineStaticLocalObject(Threads::Mutex, mutex, )
    Threads::MutexLocker lock(mutex);
    execnmoverrule = execnm;
}

static BufferString executablepathoverrule;
extern "C" { mGlobal(Basic) void SetExecutablePathOverrule(const char*); }
mExternC(Basic) void SetExecutablePathOverrule( const char* dirnm )
{
    mDefineStaticLocalObject(Threads::Mutex, mutex, )
    Threads::MutexLocker lock(mutex);
    executablepathoverrule = dirnm;
}


static const char* getShortPathName( const char* path )
{
#ifdef __unix__
    return path;
#else
    mDeclStaticString( shortpath );
    //Extract the shortname by removing spaces
    shortpath.setMinBufSize( 1025 );
    GetShortPathName( path, shortpath.getCStr(), shortpath.minBufSize()-1 );

    return shortpath;
#endif
}


mExternC(Basic) const char* GetFullExecutablePath( void )
{
    mDefineStaticLocalObject( BufferString, res, )
    mDefineStaticLocalObject( Threads::Lock, lock, )

    Threads::Locker locker( lock );

    if ( res.isEmpty() )
    {
	if ( !executablepathoverrule.isEmpty() )
	    res = executablepathoverrule;
	else
	{
	    FilePath executable;
#ifdef __win__
	    char fullpath[1024];

	    // get the fullpath to the executabe including the extension.
	    // Necessary because we cannot use argv[0] on Windows
	    GetModuleFileName(NULL, fullpath, (sizeof(fullpath)/sizeof(char)));
	    executable = fullpath;
#else
	    executable = GetArgV()[0];
#endif
	    if ( !executable.isAbsolute() )
	    {
		FilePath filepath = initialdir_.buf();
		filepath.add( GetArgV()[0] );
		executable = filepath;
	    }

	    res = getShortPathName( executable.fullPath() );
	}
    }

    return res;
}


mExternC(Basic) const char* GetExecutableName( void )
{
    mDefineStaticLocalObject( BufferString, res, )
    mDefineStaticLocalObject( Threads::Lock, lock, )

    Threads::Locker locker( lock );

    if ( res.isEmpty() )
    {
	if ( !execnmoverrule.isEmpty() )
	    return execnmoverrule.buf();

	FilePath fpargv0 = argv_[0];
	if ( !fpargv0.isAbsolute() )
	    fpargv0 = FilePath( initialdir_, argv_[0] );

	fpargv0.setExtension( nullptr );
	res = fpargv0.fileName();
    }

    return res;
}


mExternC(Basic) void sleepSeconds( double secs )
{
    if ( secs > 0 )
	Threads::sleep( secs );
}


mExternC(Basic) bool SetBindings( const char* odbindir, int argc, char** argv,
				  bool needdatabase, const char* bindinglibnm )
{
    if ( AreProgramArgsSet() )
	return true;

    if ( !File::exists(odbindir) && File::isDirectory(odbindir) )
    {
	std::cerr << "Err: Cannot set bindings without a valid path";
	std::cerr << std::endl;
	return false;
    }

    BufferString executablepath;
    if ( bindinglibnm && *bindinglibnm )
    {
	const StringView bindingfp( bindinglibnm );
	const char* lastsep = bindingfp.findLast(
					FilePath::dirSep(FilePath::Local) );
	BufferString bindingfnm( lastsep+1 );
	const BufferString bindingexecutablepath( odbindir,
					    FilePath::dirSep(FilePath::Local),
					    bindingfnm.buf() );
	if ( File::exists(bindingexecutablepath.buf()) )
	    executablepath = bindingexecutablepath;

	bindingfnm.replace( '.', '\0' ); //Shortest baseName
	if ( !__iswin__ )
	    bindingfnm.replace( "lib", "" );

	SetExecutableNameOverrule( bindingfnm.buf() );
    }

    if ( executablepath.isEmpty() )
    {
	BufferString libnm( 256, false );
	SharedLibAccess::getLibName( "Basic", libnm.getCStr(), libnm.bufSize());
	executablepath.set( odbindir ).add( FilePath::dirSep(FilePath::Local) )
		      .add( libnm );
	if ( !File::exists(executablepath.buf()) )
	{
	    std::cerr << "Err: Cannot find Basic library in: '";
	    std::cerr << odbindir << "'" << std::endl;
	    return false;
	}
    }

    executablepathoverrule.set( executablepath.str() );
#ifdef __win__
    SetDllDirectoryA( odbindir );
#endif
    const int newargc = argc+1;
    char** newargv = new char*[newargc];
    newargv[0] = (char*)(executablepathoverrule.str());
    for ( int idx=0; idx<argc; idx++ )
	newargv[idx+1] = argv[idx];

    SetRunContext( OD::BatchProgCtxt );
    const bool ret = SetProgramArgs( newargc, newargv, needdatabase );
    delete [] newargv;

    OD::ModDeps().ensureLoaded( "Network" );

    return ret;
}


mExternC(Basic) bool InitBindings( const char** moddeps, bool forgui )
{
    if ( !AreProgramArgsSet() )
    {
	std::cerr << "Err: Cannot initialize bindings before they are set";
	std::cerr << std::endl;
	return false;
    }

    PIM().loadAuto( false );
    while ( moddeps )
    {
	const char* moddep = *moddeps++;
	if ( !moddep || !*moddep )
	    break;

	OD::ModDeps().ensureLoaded( moddep );
    }

    if ( forgui )
	PIM().loadAuto( true );

    return true;
}


mExternC(Basic) void CloseBindings()
{
    NotifyExitProgram( (PtrAllVoidFn)(-1) );
    PIM().unLoadAll();
    const_cast<OD::ModDepMgr&>( OD::ModDeps() ).closeAll();
}
