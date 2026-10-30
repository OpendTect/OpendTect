/*+
 * ________________________________________________________________________
 *
 * Copyright:	(C) 1995-2026 dGB Beheer B.V.
 * License:	https://dgbes.com/licensing
 * ________________________________________________________________________
 *
 * -*/

#include "pythonenv.h"

#include "bufstring.h"
#include "dirlist.h"
#include "envvars.h"
#include "file.h"
#include "keystrs.h"
#include "oddirs.h"
#include "odjson.h"
#include "odprocess.h"
#include "pythonaccess.h"
#include "uistrings.h"
#include <cerrno>
#ifdef __win__
# include <QProcess>
# include <Windows.h>
#endif

mDefineEnumUtils(OD::PythonEnv,EnvType,"Python Environment Type")
{
    "Venv",
    "Conda",
    "Unknown",
    nullptr
};

template <>
void EnumDefImpl<OD::PythonEnv::EnvType>::init()
{
    uistrings_ += tr("Venv");
    uistrings_ += tr("Conda");
    uistrings_ += tr("Unknown");
}


OD::PythonEnv::PythonEnv()
{}


OD::PythonEnv::~PythonEnv()
{}


void OD::PythonEnv::reset()
{
    envinfos_.erase();
    envroot_.setEmpty();
    selenv_ = -1;
    envtype_ = EnvType::Unknown;
    managernm_.setEmpty();
    manager_.setEmpty();
}


uiRetVal OD::PythonEnv::setEnvironmentRoot( const char* path )
{
    if ( !path || !File::isDirectory(path) )
    {
	reset();
	return uiRetVal( uiStrings::phrDoesntExist(::toUiString(path)) );
    }

    return resolveEnvironment( path );
}


int OD::PythonEnv::currentEnvironment() const
{
    return selenv_;
}


bool OD::PythonEnv::selectEnvironment( int idx )
{
    selenv_ = envinfos_.isEmpty() ? -1 : envinfos_.validIdx(idx) ? idx : 0;
    return selenv_!=-1;
}


bool OD::PythonEnv::selectEnvironment( const char* prefix )
{
    selenv_ = -1;
    if ( size()>0 )
    {
	for ( int idx=0; idx<size(); idx++ )
	{
	    const EnvInfo& envinfo = *envinfos_[idx];
	    if ( envinfo.prefix_==prefix )
	    {
		selenv_ = idx;
		break;
	    }
	}
    }

    return selenv_!=-1;
}


uiRetVal OD::PythonEnv::useJSON( const OD::JSON::Object& jspar, bool force )
{
    //	Legacy par format was to store the conda root in sKeyEnviron()
    //	and the environment name in sKey::Name()
    //	New par format saves the Conda/Pixi project root in sKeyEnviron()
    //	and the environment prefix in sKey::Name(). Stand-a-lone environments
    //	just have the environment prefix in sKeyEnviron() and no sKey::Name()
    uiRetVal uirv;
    FilePath envroot;
    if ( jspar.isPresent(OD::PythonAccess::sKeyEnviron()) )
	envroot = jspar.getFilePath( OD::PythonAccess::sKeyEnviron() );

    if ( envroot.isEmpty() )
    {
	//	Nothing to resolve: make sure a previously resolved environment
	//	is not silently kept.
	reset();
	return uiRetVal( tr("No Python environment root specified") );
    }

    const BufferString envnm = jspar.getStringValue( sKey::Name() );
    const BufferString prevroot = envRootPath();
    const int prevsel = currentEnvironment();
    const bool sameroot = prevroot==envroot.fullPath();
    const bool sameprefix = prefixPath()==envnm;
    if ( sameroot && sameprefix && !force )
	return uirv;

    uirv = setEnvironmentRoot( envroot.fullPath() );
    if ( uirv.isOK() )
    {
	if ( !envnm.isEmpty() && !selectEnvironment(envnm.buf()) )
	{
	    const FilePath legfp( envroot.fullPath(), "envs", envnm.buf() );
	    selectEnvironment( legfp.fullPath().buf() );
	}
	if ( selenv_<0 )
	    selectEnvironment( 0 );
    }
    else
    {
	setEnvironmentRoot( prevroot.buf() );
	selectEnvironment( prevsel );
    }

    return uirv;
}


OD::PythonEnv::EnvType OD::PythonEnv::envType() const
{
    return envtype_;
}


OD::PythonEnv::EnvInfo OD::PythonEnv::getEnvInfo( int idx ) const
{
    return envinfos_.validIdx(idx) ? *envinfos_[idx] : EnvInfo();
}


const ManagedObjectSet<OD::PythonEnv::EnvInfo>& OD::PythonEnv::getEnvInfos()
									const
{
    return envinfos_;
}


int OD::PythonEnv::size() const
{
    return envinfos_.size();
}


BufferString OD::PythonEnv::pythonPath() const
{
    return envinfos_.validIdx(selenv_) ? envinfos_[selenv_]->interp_ :
					 BufferString::empty();
}


BufferString OD::PythonEnv::pythonBinPath() const
{
    BufferString res;
    if ( envinfos_.validIdx(selenv_) )
    {
	const FilePath interpfp( envinfos_[selenv_]->interp_ );
	res = interpfp.pathOnly();
    }

    return res;
}


BufferString OD::PythonEnv::prefixPath() const
{
    return envinfos_.validIdx(selenv_) ? envinfos_[selenv_]->prefix_ :
					 BufferString::empty();
}


BufferString  OD::PythonEnv::envRootPath() const
{
    return envroot_;
}


uiString OD::PythonEnv::statusString() const
{
    const uiString env( tr("environment") );
    if ( envtype_==EnvType::Unknown )
	return uiStrings::phrInvalid( env );

    return uiStrings::phrJoinStrings( tr("Found"), toUiString(envtype_), env );
}


BufferString OD::PythonEnv::statusText() const
{
    if ( envtype_==EnvType::Unknown )
	return "Invalid environment";

    return BufferString( "Found ", toString(envtype_), " environment" );
}


// QProcess resolves a bare program name using the environment of the
// calling process, not the environment set for the child. Resolve tools
// from the target environment explicitly, so e.g. a venv's jupyter-lab is
// used instead of a system-wide installation.
static void resolveEnvProgram( BufferStringSet& runcmd,
			       const BufferString& binpath )
{
    if ( runcmd.isEmpty() || binpath.isEmpty() )
	return;

    const FilePath progfp( runcmd.get(0).buf() );
    if ( progfp.nrLevels()>1 )
	return;

    BufferStringSet paths;
    paths.add( binpath );
    const BufferString exefp =
	File::findExecutable( runcmd.get(0).buf(), paths, false );
    if ( !exefp.isEmpty() )
	runcmd.get(0).set( exefp );
}


BufferStringSet OD::PythonEnv::getRunCommand( const BufferStringSet& cmdline
									) const
{
    BufferStringSet runcmd;
    if ( cmdline.isEmpty() || !envinfos_.validIdx(selenv_) ||
	 envtype_==EnvType::Unknown )
	return runcmd;

    if ( envtype_==EnvType::Venv )
    {
	BufferStringSet venvcmd( cmdline );
	resolveEnvProgram( venvcmd, pythonBinPath() );
	return venvcmd;
    }

    if ( envtype_==EnvType::Conda )
    {
	const BufferString prefix = prefixPath();
	if ( prefix.isEmpty() )
	    return runcmd;

	if ( !manager_.isEmpty() )
	{
	    runcmd.add( manager_ );
	    if (__iswin__)
	    {
		runcmd.add("-r")
		      .add( FilePath::getTempFullPath("micromamba_tmp",
						       nullptr) );
	    }
	    runcmd.add( "run" )
		  .add( "-p" )
		  .add( prefix );
	    runcmd.append( cmdline );
	    return runcmd;
	}

//	No manager - run command directly
	BufferStringSet condacmd( cmdline );
	resolveEnvProgram( condacmd, pythonBinPath() );
	return condacmd;
    }

    return runcmd;
}


BufferStringSet OD::PythonEnv::getShellRunCommand(
					const BufferStringSet& cmdline ) const
{
    BufferStringSet runcmd;
    if ( cmdline.isEmpty() || !envinfos_.validIdx(selenv_) ||
	 envtype_==EnvType::Unknown )
	return runcmd;

    if ( envtype_==EnvType::Conda && !manager_.isEmpty() )
	return getRunCommand( cmdline );

//	Venv, and conda without a manager: wrap in a shell that activates
//	the environment.
    const BufferString bindir = pythonBinPath();
    BufferStringSet quotedline( cmdline );
    AddQuotesIfNeeded( quotedline );
    BufferString inner( quotedline.cat( " " ) );
    BufferString shellcmd;
    if ( __iswin__ )
    {
	shellcmd.set( "call \"" )
		.add( FilePath(bindir, "activate.bat").fullPath() )
		.add( "\" & " )
		.add( inner );
	runcmd.add( "cmd.exe" )
	      .add( "/c" )
	      .add( shellcmd );
    }
    else
    {
	shellcmd.set( "source \"" )
		.add( FilePath(bindir, "activate").fullPath() )
		.add( "\" && exec " )
		.add( inner );
	runcmd.add( "/bin/bash" )
	      .add( "-c" )
	      .add( shellcmd );
    }

    return runcmd;
}


BufferStringSet OD::PythonEnv::getPythonRunCommand() const
{
    BufferStringSet cmdline;
    if ( !envinfos_.validIdx(selenv_) || envtype_==EnvType::Unknown )
	return cmdline;

    const BufferString interp = pythonPath();
    if ( interp.isEmpty() )
	return cmdline;

    cmdline.add( interp );
    BufferStringSet runcmd = getRunCommand( cmdline );
    return runcmd.isEmpty() ? cmdline : runcmd;
}


bool OD::PythonEnv::getModuleList( BufferStringSet& strs,
				   BufferString* errstr ) const
{
    strs.setEmpty();
    BufferStringSet runcmd = getPythonRunCommand();
    if ( runcmd.isEmpty() )
	return false;

    runcmd.add( "-c" )
	  .add( "import importlib.metadata;"
		"print("
		"*[f'{pkg.name} {pkg.version}' "
		"for pkg in importlib.metadata.distributions() "
		"if pkg.name not in ['pip','setuptools','wheel']], "
		"sep='\\n');" );
    const int res = runCommand( runcmd, &strs, errstr );
    strs.sort();
    return res==0;
}


bool OD::PythonEnv::getPythonVersion( BufferString& verstr,
				      BufferString* errstr ) const
{
    BufferStringSet runcmd = getPythonRunCommand();
    if ( runcmd.isEmpty() )
	return false;

    runcmd.add( "--version" );
    BufferStringSet outstrs;
    const int res = runCommand( runcmd, &outstrs, errstr );
    verstr = outstrs.cat();
    return res==0;
}

static int findEnvVar( const BufferStringSet& env, const char* key )
{
    const BufferString keyeq( key, "=" );
    for ( int idx=0; idx<env.size(); idx++ )
    {
	const BufferString& entry = env.get( idx );
	const bool ismatch = __iswin__
	    ? entry.startsWith( keyeq.buf(), OD::CaseInsensitive )
	    : entry.startsWith( keyeq.buf() );
	if ( ismatch )
	    return idx;
    }

    return -1;
}


static void setEnvVar( BufferStringSet& env, const char* key, const char* val )
{
    const int idx = findEnvVar( env, key );
    if ( idx < 0 )
    {
	if ( val )
	    env.add( BufferString(key, "=", val) );

	return;
    }

    if ( !val )
	env.removeSingle( idx );
    else
	env.replace( idx, BufferString(key, "=", val).buf() );
}


static BufferString getEnvVarValue( const BufferStringSet& env,
				    const char* key )
{
    const int idx = findEnvVar( env, key );
    if ( idx < 0 )
	return BufferString();

    const BufferString& entry = env.get( idx );
    const char* eqptr = entry.find( '=' );
    return eqptr ? BufferString( eqptr+1 ) : BufferString();
}


static void splitPathList( const char* value, char sep,
			   BufferStringSet& paths )
{
    paths.setEmpty();
    if ( !value || !*value )
	return;

    BufferString cur;
    for ( const char* ptr=value; ; ptr++ )
    {
	if ( *ptr==sep || !*ptr )
	{
	    if ( !cur.isEmpty() )
		paths.add( cur.buf() );
	    cur.setEmpty();
	    if ( !*ptr )
		break;
	}
	else
	    cur.add( *ptr );
    }
}


static BufferString getCleanPathValue( const BufferString& value,
				       const BufferString& odinstdir,
				       const BufferString& windowsapps )
{
#ifdef __unix__
    (void)windowsapps;
#endif

    BufferStringSet paths;
    splitPathList( value.buf(), __iswin__ ? ';' : ':', paths );
    if ( paths.isEmpty() )
	return BufferString();

    FilePath odinstfp;
    if ( !odinstdir.isEmpty() )
    {
	odinstfp.set( odinstdir.buf() );
	odinstfp.makeCanonical();
    }

#ifdef __win__
    FilePath wafp;
    if ( !windowsapps.isEmpty() )
    {
	wafp.set( windowsapps.buf() );
	wafp.makeCanonical();
    }
#endif

    BufferStringSet kept;
    for ( const auto* path : paths )
    {
	FilePath pathfp( path->buf() );
	FilePath canonfp( pathfp );
	canonfp.makeCanonical();

	if ( !odinstfp.isEmpty() &&
	     ( canonfp==odinstfp || canonfp.isSubDirOf(odinstfp) ||
	       path->contains(odinstdir.buf()) ) )
	    continue;

#ifdef __win__
	if ( !wafp.isEmpty() &&
	     ( canonfp==wafp || canonfp.isSubDirOf(wafp) ||
	       path->contains(windowsapps.buf()) ) )
	    continue;
#endif

	kept.addIfNew( path->buf() );
    }

    return kept.cat( __iswin__ ? ";" : ":" );
}


static BufferString getODInstallDir()
{
    BufferString odinstdir( GetEnvVar("DTECT_APPL") );
    if ( odinstdir.isEmpty() )
	odinstdir.set( GetSoftwareDir(false) );

    return odinstdir;
}


#ifdef __win__
static BufferString getWindowsAppsDir( const BufferStringSet& env )
{
    BufferString localappdata( getEnvVarValue(env, "LOCALAPPDATA") );
    if ( localappdata.isEmpty() )
    {
	const BufferString homedrive( getEnvVarValue(env, "HOMEDRIVE") );
	const BufferString homepath( getEnvVarValue(env, "HOMEPATH") );
	if ( !homedrive.isEmpty() && !homepath.isEmpty() )
	    localappdata.set( homedrive ).add( homepath )
			.add( "\\AppData\\Local" );
    }

    if ( localappdata.isEmpty() )
	return BufferString();

    FilePath wafp( localappdata.buf() );
    wafp.add( "Microsoft" ).add( "WindowsApps" );
    return wafp.fullPath();
}
#endif


static void cleanPathEnv( BufferStringSet& env, const BufferString& odinstdir,
			  const BufferString& windowsapps )
{
    const BufferString value( getEnvVarValue(env, "PATH") );
    if ( value.isEmpty() )
	return;

    const BufferString cleanvalue =
	getCleanPathValue( value, odinstdir, windowsapps );
    setEnvVar( env, "PATH", cleanvalue.buf() );
}


#ifdef __unix__
static void cleanLDEnv( BufferStringSet& env, const BufferString& odinstdir )
{
    const char* ldlibkey = __ismac__ ? "DYLD_LIBRARY_PATH"
				     : "LD_LIBRARY_PATH";
    BufferString value( getEnvVarValue(env, ldlibkey) );
    const BufferString syslibpath(
			getEnvVarValue(env, "OD_SYSTEM_LIBRARY_PATH") );
    if ( !syslibpath.isEmpty() )
    {
	if ( !value.isEmpty() )
	    value.add( ":" ).add( syslibpath );
	else
	    value.set( syslibpath );
    }

    const BufferString cleanvalue =
	getCleanPathValue( value, odinstdir, BufferString() );
    if ( cleanvalue.isEmpty() )
	setEnvVar( env, ldlibkey, nullptr );
    else
	setEnvVar( env, ldlibkey, cleanvalue.buf() );
}
#endif


BufferStringSet OD::PythonEnv::getChildEnvironment( const ChildEnvPars& pars )
{
    BufferStringSet env;
    if ( pars.baselinenv_ && !pars.baselinenv_->isEmpty() )
	env = *pars.baselinenv_;
    else
	env = OD::Process::systemEnvironment();

    BufferString odinstdir;
    BufferString windowsapps;
    if ( pars.cleancustomenv_ )
    {
	odinstdir.set( getODInstallDir() );
#ifdef __win__
	const BufferString wa( getWindowsAppsDir(env) );
	if ( !wa.isEmpty() &&
	     (File::exists(FilePath(wa,"python.exe").fullPath()) ||
	      File::exists(FilePath(wa,"python3.exe").fullPath())) )
	    windowsapps.set( wa );
#endif
	cleanPathEnv( env, odinstdir, windowsapps );
    }

    BufferStringSet pythonpathdirs( pars.basepaths_ );
    pythonpathdirs.add( pars.userpaths_, false );
    if ( !pythonpathdirs.isEmpty() )
    {
	const BufferString pythonpath(
		pythonpathdirs.cat(__iswin__ ? ";" : ":") );
	setEnvVar( env, "PYTHONPATH", pythonpath.buf() );
    }

    if ( !pars.pathprepend_.isEmpty() )
    {
	BufferString newpath( pars.pathprepend_ );
	const BufferString oldpath( getEnvVarValue(env, "PATH") );
	if ( !oldpath.isEmpty() )
	    newpath.add( __iswin__ ? ";" : ":" ).add( oldpath );
	setEnvVar( env, "PATH", newpath.buf() );
    }

    if ( !pars.envprefix_.isEmpty() )
    {
	if ( pars.isvenv_ )
	{
	    setEnvVar( env, "CONDA_PREFIX", nullptr );
	    setEnvVar( env, "VIRTUAL_ENV", pars.envprefix_.buf() );
	}
	else
	{
	    setEnvVar( env, "VIRTUAL_ENV", nullptr );
	    setEnvVar( env, "CONDA_PREFIX", pars.envprefix_.buf() );
	}
    }

#ifdef __unix__
    if ( pars.cleancustomenv_ )
	cleanLDEnv( env, odinstdir );
#endif

    if ( pars.cleancustomenv_ )
    {
	setEnvVar( env, "LD_BIND_NOW", nullptr );
#ifdef __win__
	setEnvVar( env, "QT_QPA_PLATFORM", nullptr );
	setEnvVar( env, "QT_LOGGING_RULES", nullptr );
#else
	setEnvVar( env, "QT_XCB_GL_INTEGRATION", nullptr );
#endif

	if ( getEnvVarValue(env, "LC_ALL").isEmpty() )
	{
	    const BufferString lang( getEnvVarValue(env, "LANG") );
	    if ( !lang.isEmpty() )
		setEnvVar( env, "LC_ALL", lang.buf() );
	}

	if ( !odinstdir.isEmpty() )
	    setEnvVar( env, "DTECT_APPL", odinstdir.buf() );
	else
	    setEnvVar( env, "DTECT_APPL", nullptr );
    }

    return env;
}


#ifdef __unix__
/* Environment for a child process which must not load the OpendTect
   libraries, e.g. an interpreter from a venv or conda environment. */

static BufferStringSet childEnvironment()
{
    OD::PythonEnv::ChildEnvPars pars;
    pars.cleancustomenv_ = true;
    return OD::PythonEnv::getChildEnvironment( pars );
}
#endif


/* Hands the caller-composed environment to the child process, or else an
   environment which does not expose the OpendTect libraries to it. The
   environment of the current process is never modified. */

static void setChildEnvironment( OD::Process& proc,
				 const BufferStringSet* environment )
{
    if ( environment && !environment->isEmpty() )
	proc.setEnvironment( *environment );
#ifdef __unix__
    else
	proc.setEnvironment( childEnvironment() );
#endif
}


static bool startProcess( RefMan<OD::Process> proc, PID_Type* pid=nullptr )
{
    if ( pid )
	return proc->startDetached( pid );

    proc->start();
    return true;
}


int OD::PythonEnv::runCommand( const BufferStringSet& runcmd,
			       BufferStringSet* outstrs,
			       BufferString* errstr,
			       int msecs, const char* workdir,
			       const BufferStringSet* environment ) const
{
    if ( runcmd.isEmpty() )
	return -1;

    const BufferString prog = runcmd.get(0);
    BufferStringSet cmdargs( runcmd );
    cmdargs.removeSingle( 0 );
    RefMan<OD::Process> proc = new OD::Process;
    if ( outstrs )
	proc->setStandardOutputStream( false, true, nullptr );

    if ( errstr )
	proc->setStandardErrorStream( false, true, nullptr );

    proc->setProgram( prog.buf() );
    proc->setArguments( cmdargs );
    if ( workdir && *workdir )
	proc->setWorkingDirectory( workdir );

    setChildEnvironment( *proc, environment );

    startProcess( proc );
    if ( !proc->waitForFinished( msecs ) )
	proc->close();

    if ( outstrs )
	proc->getAll( *outstrs );

    if ( errstr )
	proc->getAll( *errstr, false );

    return proc->exitCode();
}


RefMan<OD::Process> OD::PythonEnv::startCommand(
					    const BufferStringSet& runcmd,
					    BufferString* errstr,
					    const char* workdir,
					    const BufferStringSet* environment
					) const
{
    if ( runcmd.isEmpty() )
	return nullptr;

    const BufferString prog = runcmd.get(0);
    BufferStringSet cmdargs( runcmd );
    cmdargs.removeSingle( 0 );
    RefMan<OD::Process> proc = new OD::Process;
    proc->setStandardOutputStream( false, true, nullptr );
    proc->setStandardErrorStream( false, true, nullptr );

    proc->setProgram( prog.buf() );
    proc->setArguments( cmdargs );
    if ( workdir && *workdir )
	proc->setWorkingDirectory( workdir );

    setChildEnvironment( *proc, environment );

    startProcess( proc );
    if ( !proc->isRunning() && !proc->waitForStarted(10000) )
    {
	if ( errstr )
	    proc->getAll( *errstr, false );

	return nullptr;
    }

    return proc;
}


PID_Type OD::PythonEnv::runCommandBG( const BufferStringSet& runcmd,
				      BufferString* errstr,
				      const char* workdir,
				      bool createconsole,
				      const BufferStringSet* environment ) const
{
    PID_Type pid = -1;
    if ( runcmd.isEmpty() )
	return pid;

    const BufferString prog = runcmd.get(0);
    BufferStringSet cmdargs( runcmd );
    cmdargs.removeSingle( 0 );
    RefMan<OD::Process> proc = new OD::Process;
    if ( errstr )
	proc->setStandardErrorStream( false, true, nullptr );

#ifdef __win__
    if ( createconsole )
    {
	proc->process()->setCreateProcessArgumentsModifier(
	    [] (QProcess::CreateProcessArguments* args)
	{
	    args->flags |= CREATE_NEW_CONSOLE;
	    args->startupInfo->dwFlags &= ~STARTF_USESTDHANDLES;
	    args->startupInfo->dwFlags |= STARTF_USESHOWWINDOW;
	    args->startupInfo->hStdInput = GetStdHandle(STD_INPUT_HANDLE);
	    args->startupInfo->wShowWindow = SW_SHOW;
	});
    }
#endif

    proc->setProgram( prog.buf() );
#ifdef __win__
    const bool isshellcmd = prog.startsWith( "cmd", OD::CaseInsensitive ) &&
			    !cmdargs.isEmpty() && cmdargs.get(0) == "/c";
    if ( isshellcmd )
    {
	BufferStringSet shellargs( cmdargs );
	shellargs.removeSingle( 0 );
	const BufferString nativeargs( "/c ", shellargs.cat(" ") );
	proc->process()->setNativeArguments( nativeargs.str() );
    }
    else
#endif
	proc->setArguments( cmdargs );

    if ( workdir && *workdir )
	proc->setWorkingDirectory( workdir );

    setChildEnvironment( *proc, environment );

    const bool started = startProcess( proc, &pid );
    if ( !started )
    {
	if ( errstr )
	    proc->getAll( *errstr, false );

	return -1;
    }

    return pid;
}


bool OD::PythonEnv::hasManager() const
{
    return !manager_.isEmpty();
}


bool OD::PythonEnv::isCloneable() const
{
    return envtype_==EnvType::Conda && hasManager();
}


BufferStringSet OD::PythonEnv::getCloneCommand( const char* destdir ) const
{
    BufferStringSet runcmd;
    const BufferString prefix = prefixPath();
    if ( !destdir || !envinfos_.validIdx(selenv_) ||prefix.isEmpty() )
	return runcmd;

    if ( envtype_==EnvType::Conda &&
	 (managernm_=="micromamba" || managernm_=="conda") )
    {
	runcmd.add( manager_ )
	      .add( "create" )
	      .add( "--clone" )
	      .add( prefix )
	      .add( "-p" )
	      .add( destdir );
    }

    AddQuotesIfNeeded( runcmd );
    return runcmd;
}


BufferString OD::PythonEnv::GetPythonPath()
{
    BufferString pypath;
    BufferStringSet pypathset = OD::PythA().getBasePythonPath();
    BufferString dnm = GetEnvVar("OD_APPL_PLUGIN_DIR");
    if (!dnm.isEmpty())
    {
	FilePath fp(dnm, "bin", "python");
	if (fp.exists())
	    pypathset.insertAt(new BufferString(fp.fullPath()), 0);
    }

    dnm = GetEnvVar("OD_USER_PLUGIN_DIR");
    if (!dnm.isEmpty())
    {
	FilePath fp(dnm, "bin", "python");
	if (fp.exists())
	    pypathset.insertAt(new BufferString(fp.fullPath()), 0);
    }

    if ( !pypathset.isEmpty() )
	pypath.add("PYTHONPATH=")
	      .add(__iswin__ ? pypathset.cat(";") : pypathset.cat(":"));

    return pypath;
}


void OD::PythonEnv::AddQuotesIfNeeded( BufferStringSet& args )
{
    const char* quote = "\"";
    for ( int idx = 0; idx < args.size(); idx++ )
    {
	BufferString& str = args.get(idx);
	if (!str.find(' '))
	    continue;

	if ((str.first()=='"' && str.last()=='"'))
	    continue;

	str.insertAt(0, quote);
	str.add(quote);
    }
}


BufferString OD::PythonEnv::getEnvBinFolderName() const
{
    if ( __iswin__ )
	return "Scripts";
    else
	return "bin";
}


static void addEnv( ObjectSet<OD::PythonEnv::EnvInfo>& envinfos,
		    const BufferString& envpath,
		    const BufferString& interppath,
		    const BufferString& display,
		    bool isdefault )
{
    auto* info = new OD::PythonEnv::EnvInfo;
    info->prefix_.set( envpath );
    info->interp_.set( interppath );
    info->display_.set( display );
    info->isdefault_ = isdefault;
    envinfos.add( info );
}


uiRetVal OD::PythonEnv::resolveEnvironment( const char* path )
{
//  Start from scratch: a failed resolution must not leave a partially
//  updated environment that still looks valid.
    reset();

    const auto fail = [this]( const uiString& msg )
		      {
			  reset();
			  return uiRetVal( msg );
		      };

    FilePath envrootfp( path );
    EnvType envtype = EnvType::Unknown;
    envrootfp.makeCanonical();
    BufferStringSet managerpaths;

//  Pixi project
    if ( FilePath(envrootfp, "pixi.toml").exists() )
    {
	const FilePath envsfp( envrootfp, ".pixi", "envs" );
	if ( envsfp.exists() )
	{
	    const DirList dl( envsfp.fullPath(), File::DirListType::DirsInDir );
	    for ( int idx=0; idx<dl.size(); idx++ )
	    {
		const FilePath envfp( dl.fullPath(idx) );
		FilePath interpfp( envfp );
		if ( !__iswin__ )
		    interpfp.add( getEnvBinFolderName() );

		interpfp.add( OD::PythonAccess::sPythonExecNm() );
		if ( !File::isExecutable(interpfp.fullPath()) )
		    return fail(
			    tr("No python at: %1").arg(interpfp.fullPath()) );

		BufferString display( envfp.baseName() );
		const bool isdefault = display=="default";
		display.add( " (pixi)" );
		addEnv( envinfos_, envfp.fullPath(), interpfp.fullPath(),
			display, isdefault );
	    }
	    envtype = EnvType::Conda;
	}
	else
	    return fail(
		    tr("Folder: %1 does not exist").arg(envsfp.fullPath()) );
    }
// Conda project
    else if ( FilePath(envrootfp, "conda-meta").exists() &&
	      FilePath(envrootfp, "envs").exists() )
    {
	envtype = EnvType::Conda;
	FilePath interpfp( envrootfp );
	if ( !__iswin__ )
	    interpfp.add( getEnvBinFolderName() );

	interpfp.add( OD::PythonAccess::sPythonExecNm() );
	if ( !File::isExecutable(interpfp.fullPath()) )
	    return fail( tr("No python at: %1").arg(interpfp.fullPath()) );

	addEnv( envinfos_, envrootfp.fullPath(), interpfp.fullPath(),
		BufferString("base (conda)"), true );
	envtype = EnvType::Conda;
	managerpaths.add( interpfp.pathOnly() );
	const FilePath envsfp( envrootfp, "envs" );
	if ( envsfp.exists() )
	{
	    const DirList dl( envsfp.fullPath(), File::DirListType::DirsInDir );
	    for ( int idx=0; idx<dl.size(); idx++ )
	    {
		const FilePath envfp( dl.fullPath(idx) );
		interpfp =  envfp;
		if ( !__iswin__ )
		    interpfp.add( getEnvBinFolderName() );

		interpfp.add( OD::PythonAccess::sPythonExecNm() );
		if ( !File::isExecutable(interpfp.fullPath()) )
		    return fail(
			    tr("No python at: %1").arg(interpfp.fullPath()) );

		BufferString display( envfp.baseName() );
		display.add( " (conda)" );
		addEnv( envinfos_, envfp.fullPath(), interpfp.fullPath(),
			display, false );
	    }
	}
    }
//  Standard Venv environment folder
    else if ( FilePath(envrootfp, "pyvenv.cfg").exists() )
    {
	const FilePath interpfp( envrootfp, getEnvBinFolderName(),
				 OD::PythonAccess::sPythonExecNm() );
	if ( !File::isExecutable(interpfp.fullPath()) )
	    return fail( tr("No python at: %1").arg(interpfp.fullPath()) );

	envtype = EnvType::Venv;
	addEnv( envinfos_, envrootfp.fullPath(), interpfp.fullPath(),
		BufferString(), true );
    }
//  UV project
    else if ( FilePath(envrootfp, ".venv", "pyvenv.cfg").exists() )
    {
	const FilePath envfp( envrootfp, ".venv" );
	const FilePath interpfp( envfp, getEnvBinFolderName(),
				 OD::PythonAccess::sPythonExecNm() );
	if ( !File::isExecutable(interpfp.fullPath()) )
	    return fail( tr("No python at: %1").arg(interpfp.fullPath()) );

	envtype = EnvType::Venv;
	addEnv( envinfos_, envfp.fullPath(), interpfp.fullPath(),
		BufferString(), true );
    }
//  Stand-a-lone Conda environment
    else if ( FilePath(envrootfp, "conda-meta").exists() )
    {
	FilePath interpfp( envrootfp );
	if ( !__iswin__ )
	    interpfp.add( getEnvBinFolderName() );

	interpfp.add( OD::PythonAccess::sPythonExecNm() );
	if ( !File::isExecutable(interpfp.fullPath()) )
	    return fail( tr("No python at: %1").arg(interpfp.fullPath()) );

	envtype = EnvType::Conda;
	addEnv( envinfos_, envrootfp.fullPath(), interpfp.fullPath(),
		BufferString(), true );
    }
    else
	return fail( uiStrings::phrInvalid(tr("Python Environment")) );

	BufferString manager, managernm;
    if ( envtype==EnvType::Conda )
    {
	manager = File::findExecutable( "micromamba", managerpaths );
	managernm = "micromamba";
	if (manager.isEmpty())
	{
	    manager = File::findExecutable("conda", managerpaths);
	    managernm = "conda";
	}

	if (manager.isEmpty())
	{
	    managernm.setEmpty();
	    return fail(tr("Cannot use Conda environments without "
		"micromamba or conda and neither was found."));
	}
    }

    manager_ = manager;
    managernm_ = managernm;
    envtype_ = envtype;
    envroot_ = envrootfp.fullPath();
    selenv_ = envinfos_.isEmpty() ? -1 : 0;
    return uiRetVal::OK();
}
