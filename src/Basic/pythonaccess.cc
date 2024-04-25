/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "pythonaccess.h"

#include "applicationdata.h"
#include "ascstream.h"
#include "commandlaunchmgr.h"
#include "commandlineparser.h"
#include "dirlist.h"
#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "keystrs.h"
#include "oddirs.h"
#include "odplatform.h"
#include "procdescdata.h"
#include "separstr.h"
#include "settings.h"
#include "string2.h"
#include "survinfo.h"
#include "threadwork.h"
#include "uistrings.h"


namespace OD
{

    const char* PythonAccess::sKeyPythonSrc()	{ return "Python Source"; }
    const char* PythonAccess::sKeyEnviron()	{ return "Environment"; }
    const char* PythonAccess::sKeyPythonPath()	{ return "PythonPath"; }
    const char* PythonAccess::sKeyActivatePath() { return "activatefnm"; }
    static const char* sKeyPythonPathEnvStr()	{ return "PYTHONPATH"; }

} // namespace OD

BufferStringSet OD::PythonAccess::pystartpath_{0}; //From user environment

uiString OD::PythonAccess::firewallDesc()
{
    return od_static_tr( "firewallDesc","Machine Learning Environment : <%1>" );
}

OD::PythonAccess& OD::PythA()
{
    mDefineStaticLocalObject( PtrMan<PythonAccess>, theinst,
							  = new PythonAccess );
    return *theinst;
}


const char* OD::PythonAccess::sPythonExecNm( bool v3, bool v2 )
{
    if ( __iswin__ )
    {
	return "python.exe";
    }
    else
    {
	if ( v3 )
	    return "python3";
	else if ( v2 )
	    return "python2";
	else
	    return "python";
    }
}


mDefineNameSpaceEnumUtils(OD,PythonSource,"Python Source")
{
    "Internal", "System", "Custom", nullptr
};
template <>
void EnumDefImpl<OD::PythonSource>::init()
{
    uistrings_ += tr("Internal");
    uistrings_ += tr("System");
    uistrings_ += tr("Custom");
}



OD::PythonAccess::PythonAccess()
    : envChange(this)
    , envVerified(this)
    , lock_(Threads::Lock::SmallWork)
{
    mAttachCB( ApplicationData::applicationToBeStarted(),
	       PythonAccess::appToBeStartedCB );
    updatePythonPath();
}


OD::PythonAccess::~PythonAccess()
{
    detachAllNotifiers();
    delete activatefp_;
}


const BufferStringSet& OD::PythonAccess::getBasePythonPath_() const
{
    mDefineStaticLocalObject( PtrMan<BufferStringSet>, theinst,
						      = new BufferStringSet );
    return *theinst;
}


BufferStringSet OD::PythonAccess::getBasePythonPath() const
{
    return getBasePythonPath_();
}


BufferStringSet OD::PythonAccess::getUserPythonPath() const
{
    return pystartpath_;
}


void OD::PythonAccess::addBasePath( const FilePath& fp )
{
    if ( fp.exists() )
    {
	FilePath cleanfp( fp );
	cleanfp.makeCanonical();
	//Only place where it should be updated
	BufferStringSet& bpythonpath =
	    const_cast<BufferStringSet&>( getBasePythonPath_() );
	bpythonpath.addIfNew( cleanfp.fullPath() );
    }
}


void OD::PythonAccess::updatePythonPath() const
{
    BufferString pythonstr( sKey::Python() ); pythonstr.toLower();
    const IOPar& pythonsetts = Settings::fetch( pythonstr );
    PtrMan<IOPar> pathpar = pythonsetts.subselect( sKeyPythonPath() );
    if ( pathpar )
    {
	BufferStringSet settpaths;
	settpaths.usePar( *pathpar );
	for ( const auto* settpathstr : settpaths )
	{
	    const FilePath fp( settpathstr->buf() );
	    if ( !fp.exists() )
		continue;

	    mSelf().addBasePath( fp );
	}
    }
}


void OD::PythonAccess::initClass()
{
    GetEnvVarDirList( sKeyPythonPathEnvStr(), pystartpath_, true );
#ifdef __odpy_dir__
    FilePath pythonmodsfp;
    if ( isDeveloperBuild() )
	pythonmodsfp.set( __odpy_dir__ );
    else
	pythonmodsfp.set( GetSoftwareDir(true) ).add( "bin" ).add( "python" );
    if ( pythonmodsfp.exists() )
	PythA().addBasePath( pythonmodsfp );
#endif
#ifdef __safety_dir__
    if ( isDeveloperBuild() )
    {
	pythonmodsfp.set( __safety_dir__ );
	if ( pythonmodsfp.exists() )
	    PythA().addBasePath( pythonmodsfp );
    }
#endif

#ifdef __win__
    ManagedObjectSet<FilePath> fps;
    BufferStringSet envnms;

    BufferString pythonstr( sKey::Python() ); pythonstr.toLower();
    const IOPar& pythonsetts = Settings::fetch( pythonstr );
    PythonSource source = hasInternalEnvironment() ? Internal : System;
    PythonSourceDef().parse( pythonsetts, sKeyPythonSrc(), source );
    FilePath externalroot;
    CommandLineParser clp;
    const int totnrarg = GetArgC();
    bool useextparth = false;
    const char* pypathkey = ProcDesc::DataEntry::getTypeFlag(
					    ProcDesc::DataEntry::Python );
    BufferString rootstr;
    if ( clp.getVal(pypathkey,rootstr) )
    {
	if ( rootstr.isEmpty() || !File::isDirectory(rootstr) )
	    return;

	externalroot.setPath( rootstr );
	source = Custom;
    }

    if ( source == Custom )
    {
	BufferString virtenvloc;
	if ( !useextparth && pythonsetts.get(sKeyEnviron(),virtenvloc) )
	    externalroot = virtenvloc;

	if ( !getSortedVirtualEnvironmentLoc(fps, envnms, nullptr,
							    &externalroot) )
	    return;
    }
    else if ( source == Internal )
    {
	if ( !getSortedVirtualEnvironmentLoc(fps, envnms) )
	    return;
    }

    for ( int idx=0; idx<fps.size(); idx++ )
    {
	FilePath* fp = fps[idx];
	fp->add( sPythonExecNm() );
	if ( !File::exists(fp->fullPath()) )
	    continue;

	ePDD().add( *envnms[idx], firewallDesc().arg(*envnms[idx]),
						ProcDesc::DataEntry::Python );
    }

#endif
}


BufferString OD::PythonAccess::pyVersion() const
{
    if ( pythversion_.isEmpty() )
	mSelf().retrievePythonVersionStr();

    return pythversion_;
}


uiString OD::PythonAccess::pySummary() const
{
    BufferString pythonstr( sKey::Python() ); pythonstr.toLower();
    const IOPar& pythonsetts = Settings::fetch( pythonstr );
    PythonSource source;
    if (!PythonSourceDef().parse(pythonsetts,sKeyPythonSrc(),source))
	source = System;

    uiString result = tr("Using %1 %2").arg(source).arg(pyVersion());
    if ( source == Custom )
    {
	BufferString virtenvloc;
	if ( pythonsetts.get(sKeyEnviron(),virtenvloc) )
	{
	    if ( !virtenvnm_.isEmpty() )
		result.append( tr("environment %1").arg(virtenvnm_) );
	}
    }

    return result;
}


uiRetVal OD::PythonAccess::isUsable( bool force, BufferString* stdoutstr,
				     BufferString* stderrstr ) const
{
    if ( !force && istested_ && !isusable_ )
	return tr("Python environment is not usable");

    uiRetVal ret;
    BufferString tmpstderr;
    BufferString& stderrmsg = stderrstr ? *stderrstr : tmpstderr;
    if ( !mSelf().isUsable_(force,ret,stdoutstr,&stderrmsg) )
    {
	uiRetVal uirv = tr("Python environment is not usable");
	if ( !ret.isOK() )
	    uirv.add( ret );
	return uirv;
    }

    return uiRetVal::OK();
}


bool OD::PythonAccess::isUsable_( bool force, uiRetVal& ret,
				  BufferString* stdoutstr,
				  BufferString* stderrstr )
{
    Threads::Locker locker( lock_ );
    if ( !force && istested_ )
	return isusable_;

    istested_ = true;
    isusable_ = false;
    if ( force )
    {
	pythversion_.setEmpty();
	moduleinfos_.setEmpty();
    }

    BufferString pythonstr( sKey::Python() ); pythonstr.toLower();
    const IOPar& pythonsetts = Settings::fetch( pythonstr );
    PythonSource source = hasInternalEnvironment(false) ? Internal : System;
    PythonSourceDef().parse( pythonsetts, sKeyPythonSrc(), source );
    if ( source == System )
	return isEnvUsable( nullptr, nullptr, ret, stdoutstr, stderrstr );

    PtrMan<FilePath> externalroot = nullptr;
    PtrMan<BufferString> virtenvnm;
    if ( source == Custom )
    {
	BufferString virtenvloc;
	if ( pythonsetts.get(sKeyEnviron(),virtenvloc) )
	{
	    if ( !File::exists(virtenvloc) || !File::isDirectory(virtenvloc) )
	    {
		ret = tr("Selected custom python environment does "
			 "not exist:\n'%1'").arg( virtenvloc );
		return ret.isOK();
	    }

	    externalroot = new FilePath( virtenvloc );
	}

	virtenvnm = new BufferString();
	pythonsetts.get( sKey::Name(), *virtenvnm );
    }

    ManagedObjectSet<FilePath> pythonenvsfp;
    BufferStringSet envnms;
    if ( !getSortedVirtualEnvironmentLoc(pythonenvsfp,envnms,virtenvnm,
					 externalroot) )
    {
	const uiString msg = source == Custom
	     ? tr("Custom environment [%1] does not exist")
		    .arg( virtenvnm->isEmpty() ? "base" : virtenvnm->buf() )
	     : tr("Internal environments are not usable");
	ret = msg;
	return ret.isOK();
    }

    for ( int idx=0; idx<pythonenvsfp.size(); idx++ )
    {
	const FilePath* pythonenvfp = pythonenvsfp[idx];
	const BufferString& envnm = envnms.get( idx );
	if ( isEnvUsable(pythonenvfp,envnm.buf(),ret,stdoutstr,stderrstr) )
	    return true;
    }

    ret = tr("Internal environments are not usable");
    return false;
}


namespace OD {

BufferString& GetPythonActivatorExe()
{
    mDefineStaticLocalObject( PtrMan<BufferString>, ret, = new BufferString );
    return *ret.ptr();
}

} // namespace OD


void OD::PythonAccess::setPythonActivator( const char* fnm )
{
    if ( !fnm || File::exists(fnm) )
	GetPythonActivatorExe().set( fnm );
}


const char* OD::PythonAccess::getPythonActivatorPath()
{
    return GetPythonActivatorExe().buf();
}


bool OD::PythonAccess::needCheckRunScript()
{
    if ( !GetPythonActivatorExe().isEmpty() )
	return false;

    const FilePath pythonfp( GetSoftwareDir(true), "bin", "python" );
    return pythonfp.exists();
}


FilePath* OD::PythonAccess::getActivateScript( const FilePath& rootfp )
{
    FilePath ret( rootfp.fullPath(), "bin" );
    ret.add( "activate" );
    if ( __iswin__ )
	ret.setExtension( "bat" );

    if ( !ret.exists() )
    {
	ret.set( rootfp.fullPath() ).add( "condabin" );
	ret.add( "activate" );
	if ( __iswin__ )
	    ret.setExtension("bat");
    }

    return ret.exists() || !GetPythonActivatorExe().isEmpty()
	    ? new FilePath( ret ) : nullptr;
}


OD::PythonSource OD::PythonAccess::getPythonSource() const
{
    BufferString pythonstr( sKey::Python() ); pythonstr.toLower();
    const IOPar& pythonsetts = Settings::fetch( pythonstr );
    PythonSource source = hasInternalEnvironment(false) ? Internal : System;
    OD::PythonSourceDef().parse( pythonsetts, sKeyPythonSrc(), source );
    return source;
}


FilePath OD::PythonAccess::getPythonEnvFp() const
{
    if ( !activatefp_ )
	return FilePath();

    const FilePath envrootfp = getPythonEnvFp( *activatefp_ );
    return getPythonEnvFp( envrootfp );
}


OD::PythonSource OD::PythonAccess::getPythonSource( const FilePath* envrootfp )
{
    if ( !envrootfp || envrootfp->isEmpty() )
	return System;

    ManagedObjectSet<FilePath> fps;
    BufferStringSet envnms;
    getSortedVirtualEnvironmentLoc( fps, envnms );
    for ( const auto* fp : fps )
	if ( *fp == *envrootfp )
	    return Internal;

    return Custom;
}


FilePath OD::PythonAccess::getPythonEnvFp( const FilePath& activatefp )
{
    if ( activatefp.isEmpty() || activatefp.nrLevels() < 3 )
	return FilePath();

    FilePath rootfp( activatefp );
    rootfp.setFileName( nullptr ).setFileName( nullptr );
    return rootfp;
}


uiRetVal OD::PythonAccess::setEnvironment( const FilePath* pythonenvfp,
					   const char* envnm )
{
    Threads::Locker locker( lock_ );
    istested_ = true;
    isusable_ = false;
    pythversion_.setEmpty();
    moduleinfos_.setEmpty();
    uiRetVal ret;
    return isEnvUsable( pythonenvfp, envnm, ret ) ? uiRetVal::OK() : ret;
}


bool OD::PythonAccess::isEnvUsable( const FilePath* pythonenvfp,
				    const char* envnm, uiRetVal& ret,
				    BufferString* stdoutstr,
				    BufferString* stderrstr )
{
    PtrMan<FilePath> activatefp;
    BufferString venvnm( envnm );
    if ( pythonenvfp )
    {
	if ( !pythonenvfp->exists() )
	    return false;

	activatefp = getActivateScript( *pythonenvfp );
	if ( !activatefp )
	    return false;
    }

    OS::MachineCommand cmd( sPythonExecNm(true) );
    cmd.addFlag( "version" );

    BufferString tmpstdout, tmpstderr;
    BufferString& stdoutmsg = stdoutstr ? *stdoutstr : tmpstdout;
    BufferString& stderrmsg = stderrstr ? *stderrstr : tmpstderr;
    mDefineStaticLocalObject(bool, force_external,
				= GetEnvVarYN("OD_FORCE_PYTHON_ENV_OK") );
    if ( !force_external )
    {
	ret = doExecute( cmd, nullptr, nullptr, activatefp.ptr(), venvnm,
			 &stdoutmsg, &stderrmsg );
    }

    if ( !ret.isOK() )
	return false;

    bool notrigger;
    if ( pythonenvfp )
    {
	notrigger = activatefp_ &&
		    activatefp_->fullPath() == activatefp->fullPath() &&
		    virtenvnm_ == venvnm;
	delete activatefp_;
	activatefp_ = new FilePath( *activatefp );
	virtenvnm_.set( venvnm );
    }
    else
    {
	notrigger = !activatefp_ && virtenvnm_.isEmpty() && venvnm.isEmpty();
	deleteAndNullPtr( activatefp_ );
	virtenvnm_.setEmpty();
    }

    const bool firstinit = pythversion_.isEmpty() && moduleinfos_.isEmpty();
    if ( firstinit )
	notrigger = false;

    isusable_ = true;
    if ( !notrigger )
    {
	pythversion_ = stdoutmsg;
	updatePythonPath();
	Threads::Locker locker( lock_, Threads::Locker::DontWaitForLock );
	locker.unlockNow();
	envChange.trigger();
    }

    return isusable_;
}


bool OD::PythonAccess::execute( const OS::MachineCommand& cmd, uiRetVal& ret,
				bool wait4finish, BufferString* stdoutstr,
				BufferString* stderrstr ) const
{
    OS::CommandExecPars execpars( wait4finish ? OS::Wait4Finish : OS::RunInBG );
    execpars.createstreams( true );
    return execute( cmd, execpars, ret, nullptr, stdoutstr, stderrstr );
}


bool OD::PythonAccess::execute( const OS::MachineCommand& cmd,
				BufferString& stdoutstr, uiRetVal& ret,
				BufferString* stderrstr ) const
{
    if ( !mSelf().isUsable_(!istested_,ret,&stdoutstr,stderrstr) )
	return false;

    ret = doExecute( cmd, nullptr, nullptr, activatefp_, virtenvnm_.buf(),
		     &stdoutstr, stderrstr );
    return ret.isOK();
}


bool OD::PythonAccess::execute( const OS::MachineCommand& cmd,
				const OS::CommandExecPars& pars,
				uiRetVal& ret, int* pid,
				BufferString* stdoutstr,
				BufferString* stderrstr ) const
{
    if ( !mSelf().isUsable_(!istested_,ret,stdoutstr,stderrstr) )
	return false;

    ret = doExecute( cmd, &pars, pid, activatefp_, virtenvnm_.buf(),
		     stdoutstr, stderrstr );
    return ret.isOK();
}


bool OD::PythonAccess::executeScript( const char* scriptstr,
				      BufferString& stdoutstr, uiRetVal& ret,
				      BufferString* stderrstr ) const
{
    const OS::MachineCommand mc( sPythonExecNm(true), "-c", scriptstr );
    return execute( mc, stdoutstr, ret, stderrstr );
}


bool OD::PythonAccess::executeScript( const char* scriptstr, uiRetVal& ret,
				      bool wait4finish,
				      BufferString* stdoutstr,
				      BufferString* stderrstr ) const
{
    if ( wait4finish && stdoutstr )
	return executeScript( scriptstr, *stdoutstr, ret, stderrstr );

    const OS::MachineCommand mc( sPythonExecNm(true), "-c", scriptstr );
    return execute( mc, ret, wait4finish, stdoutstr, stderrstr );
}


bool OD::PythonAccess::executeScript( const BufferStringSet& scriptstrs,
				      BufferString& stdoutstr, uiRetVal& ret,
				      BufferString* stderrstr ) const
{
    return executeScript( BufferString(scriptstrs.cat(";")), stdoutstr, ret,
			  stderrstr );
}


bool OD::PythonAccess::executeScript( const BufferStringSet& scriptstrs,
				      uiRetVal& ret, bool wait4finish,
				      BufferString* stdoutstr,
				      BufferString* stderrstr ) const
{
    return executeScript( BufferString(scriptstrs.cat(";")), ret, wait4finish,
			  stdoutstr, stderrstr );
}


bool OD::PythonAccess::isModuleUsable( const char* nm, uiRetVal& ret ) const
{
   BufferStringSet importscript;
    if ( BufferString("pptx")==nm )
    {
	importscript.add( "import collections" );
	importscript.add( "import collections.abc" );
    }

    importscript.add( BufferString("import ", nm) );
    return executeScript( importscript, ret );
}


BufferString OD::PythonAccess::getDataTypeStr( OD::DataRepType typ )
{
    BufferString ret;
    if ( typ == F32 )
	ret.set( "float32" );
    else if ( typ == F64 )
	ret.set( "float64" );
    else if ( typ == SI8 )
	ret.set( "int8" );
    else if ( typ == UI8 )
	ret.set( "uint8" );
    else if ( typ == SI16 )
	ret.set( "int16" );
    else if ( typ == UI16 )
	ret.set( "uint16" );
    else if ( typ == SI32 )
	ret.set( "int32" );
    else if ( typ == UI32 )
	ret.set( "uint32" );
    else if ( typ == SI64 )
	ret.set( "int64" );
/*    else if ( typ == UI64 )
	ret.set( "uint64" );*/

    return ret;
}


OD::DataRepType OD::PythonAccess::getDataType( const char* str )
{
    DataRepType ret = AutoDataRep;
    const StringView typestr( str );
    if ( typestr == "float32" )
	ret = F32;
    else if ( typestr == "float64" )
	ret = F64;
    else if ( typestr == "int8" )
	ret = SI8;
    else if ( typestr == "uint8" )
	ret = UI8;
    else if ( typestr == "int16" )
	ret = SI16;
    else if ( typestr == "uint16" )
	ret = UI16;
    else if ( typestr == "int32" )
	ret = SI32;
    else if ( typestr == "uint32" )
	ret = UI32;
    else if ( typestr == "int64" )
	ret = SI64;
    else if ( typestr == "uint64" )
	ret = SI64;

    return ret;
}


static BufferString getPIDFilePathStr( const FilePath& scriptfp )
{
    FilePath ret( scriptfp );
    ret.setExtension( "pid" );
    return ret.fullPath();
}


FilePath* OD::PythonAccess::getCommand( OS::MachineCommand& cmd,
					bool background,
					const FilePath* activatefp,
					const char* envnm )
{
    auto* ret = new FilePath( FilePath::getTempFullPath("runpython",nullptr) );
    if ( !ret )
	return nullptr;
#ifdef __win__
    ret->setExtension( "bat" );
#else
    ret->setExtension( "sh" );
#endif
    od_ostream strm( *ret );
    if ( !strm.isOK() )
    {
	delete ret;
	return nullptr;
    }

    BufferString temppath( File::getTempPath() );
    if ( temppath.find(' ') )
	temppath.quote();

#ifdef __win__
    strm.add( "@SETLOCAL" ).add( od_newline );
    strm.add( "@ECHO OFF" ).add( od_newline ).add( od_newline );

    strm.add( "SET TMPDIR=" ).add( temppath ).add( od_newline );
    if ( background )
    {
	strm.add( "SET procnm=%~n0" ).add( od_newline );
	strm.add( "SET proctitle=%procnm%_children" ).add( od_newline );
	strm.add( "SET pidfile=\"%~dpn0.pid\"" )
	    .add( od_newline ).add( od_newline);
    }
    strm.add( "@CALL \"" );
#else
    strm.add( "#!/bin/bash" ).add( od_newline ).add( od_newline )
	.add( "export TMPDIR=" ).add( temppath ).add( od_newline )
	.add( "source " );
#endif
    strm.add( activatefp->fullPath() );
#ifdef __win__
    strm.add( "\"" );
#endif
    if ( envnm )
    {
	BufferString venvnm( envnm );
	if ( venvnm.find(' ') )
	    venvnm.quote( '\"' );
	strm.add( " " ).add( venvnm ).add( od_newline );
    }
#ifdef __win__
    if (background)
    {
	const BufferString prognm( cmd.program() );
	if ( prognm.startsWith("cmd", OD::CaseInsensitive) ||
	     prognm.startsWith("powershell", OD::CaseInsensitive) ||
	     prognm.startsWith("wt", OD::CaseInsensitive))
	    strm.add("Start \"%proctitle%\" ");
	else
	    strm.add("Start \"%proctitle%\" /MIN ");
    }
#endif
    BufferStringSet args( cmd.args() );
#ifdef __unix__
    const bool isscript = args.size() > 1 && args.get(0) == "-c";
#endif
    args.insertAt( new BufferString( cmd.program() ), 0 );
    for ( int idx=0; idx<args.size(); idx++ )
    {
	auto* arg = args[idx];
	if ( arg->find(' ') && arg->firstChar() != '\'' &&
	     arg->firstChar() != '\"' )
#ifdef __win__
	    arg->quote('\"');
#else
	{
	    if ( isscript && idx > 0 )
		arg->quote('\"');
	    else
		arg->quote();
	}
#endif
    }
    strm.add( args.cat(" ") );
    if ( background )
    {
#ifdef __win__
	strm.add( od_newline );
	strm.add( "timeout 2 > nul" ).add( od_newline );
	strm.add( "FOR /F \"tokens=* USEBACKQ\" %%g IN (`tasklist /FI" );
	strm.add( " \"WINDOWTITLE eq %proctitle%\" /FO CSV /NH`) DO " );
	strm.add( "(SET \"PROCRET=%%g\")" ).add( od_newline );
	strm.add( "FOR /F \"tokens=2 delims=,\" %%g IN (\"%PROCRET%\") DO (" );
	strm.add( "SET \"PIDRET=%%g\")" ).add( od_newline );
	strm.add( "ECHO %PIDRET:\"=% > %PIDFILE%" );
#else
	strm.add( " 2>/dev/null" );
	strm.add( " &" ).add( od_newline );
	BufferString pidfile( getPIDFilePathStr(*ret) );
	strm.add( "echo $! > " ).add( pidfile.quote() );
#endif
    }
    strm.add( od_newline );

    strm.close();
#ifdef __unix__
    File::makeExecutable( ret->fullPath(), true );
#endif
    cmd = OS::MachineCommand( ret->fullPath(), true );

    return ret;
}


OS::CommandLauncher* OD::PythonAccess::getLauncher(
						const OS::MachineCommand& mc,
						bool background,
						const FilePath* activatefp,
						const char* envnm,
						FilePath& scriptfpret )
{
    OS::MachineCommand scriptcmd( mc );
    PtrMan<FilePath> scriptfp;
    if ( activatefp )
    {
	if ( GetPythonActivatorExe().isEmpty() )
	{
	    scriptfp = getCommand( scriptcmd, background,
				   activatefp, envnm );
	}
	else
	{
	    OS::MachineCommand cmdret( GetPythonActivatorExe() );
	    const FilePath rootfp( *activatefp );
	    const BufferString rootfnm = rootfp.dirUpTo( rootfp.nrLevels()-3 );
	    cmdret.addArg( rootfnm );
	    if ( envnm && *envnm )
		cmdret.addKeyedArg( "envnm", envnm );

	    const BufferString prognm( mc.program() );
	    if ( __iswin__ )
	    {
		const SurveyDiskLocation& sdl = SI().diskLocation();
		const uiRetVal uirv =
			    SurveyInfo::isValidDataRoot( sdl.basePath() );
		if ( uirv.isOK() && sdl.exists() )
		{
		    cmdret.addKeyedArg( CommandLineParser::sDataRootArg(),
				        sdl.basePath() );
		    cmdret.addKeyedArg( CommandLineParser::sSurveyArg(),
				        sdl.dirName() );
		}

		if ( background )
		{
		    scriptfp = new FilePath(
			    FilePath::getTempFullPath("python", "txt" ) );

		    cmdret.addKeyedArg( "pidbasenm", scriptfp->fullPath() );
		}

		if ( prognm.startsWith("cmd",OD::CaseInsensitive) ||
		     prognm.startsWith("powershell",OD::CaseInsensitive) )
		    cmdret.addFlag( "consoleuiprog" );
	    }

	    cmdret.addFlag( "" ).addArg( prognm ).addArgs( mc.args() );
	    scriptcmd = OS::MachineCommand( cmdret, true );
	}
    }
    else
    {
	OS::MachineCommand cmdret( scriptcmd, true );
	const StringView prognm( scriptcmd.program() );
	if ( !GetPythonActivatorExe().isEmpty() &&
	     (prognm.startsWith("cmd", OD::CaseInsensitive) ||
	      prognm.startsWith("powershell",OD::CaseInsensitive)) )
	    cmdret.addFlag( "consoleuiprog" );

	scriptcmd = cmdret;
    }

    if ( scriptfp )
	scriptfpret = *scriptfp;
    else
	scriptfpret.set( nullptr );

    auto* cl = new OS::CommandLauncher( scriptcmd );
    return cl;
}


void OD::PythonAccess::getPIDFromFile( const char* pidfnm, int* pid )
{
    double waittm = 10., waitstep = 0.1;
    while ( waittm > 0. )
    {
	waittm -= waitstep;
	if ( !File::exists(pidfnm) )
	    { Threads::sleep( waitstep ); continue; }

	BufferString pidstr;
	if ( !File::getContent(pidfnm,pidstr) || pidstr.isEmpty() )
	    { Threads::sleep( waitstep ); continue; }

	const int localpid = pidstr.toInt();
	if ( !isProcessAlive(localpid) )
	    { Threads::sleep( waitstep ); continue; }

	if ( pid )
	    *pid = localpid;
	File::remove( pidfnm );
	return;
    }
}


uiRetVal OD::PythonAccess::doExecute( const OS::MachineCommand& cmd,
				  const OS::CommandExecPars* execpars, int* pid,
				  const FilePath* activatefp, const char* envnm,
				  BufferString* stdoutstr,
				  BufferString* stderrstr ) const
{
    FilePath scriptfp;
    const bool background = execpars && execpars->launchtype_ >= OS::RunInBG;
    PtrMan<OS::CommandLauncher> cl =
		    getLauncher( cmd, background, activatefp, envnm, scriptfp );
    if ( !cl.ptr() )
    {
	return tr("Cannot create launcher for command '%1'")
		    .arg( cmd.toString(execpars) );
    }

    BufferStringSet origpythonpathdirs;
    GetEnvVarDirList( sKeyPythonPathEnvStr(), origpythonpathdirs, false );
    BufferStringSet pythonpathdirs( getBasePythonPath() );
    pythonpathdirs.add( pystartpath_, false );
    SetEnvVarDirList( sKeyPythonPathEnvStr(), pythonpathdirs, false );

    bool res;
    if ( execpars )
	res = cl->execute( *execpars );
    else if ( stdoutstr )
	res = cl->execute( *stdoutstr, stderrstr );
    else
    {
	BufferString tmpstdout;
	res = cl->execute( tmpstdout, stderrstr );
    }

    if ( pid )
	*pid = cl->processID();

    if ( origpythonpathdirs.isEmpty() )
	UnsetOSEnvVar( sKeyPythonPathEnvStr() );
    else
	SetEnvVarDirList( sKeyPythonPathEnvStr(), origpythonpathdirs, false );

    if ( !scriptfp.isEmpty() )
    {
	const BufferString pidfnm( getPIDFilePathStr(scriptfp) );
	if ( res && background )
	    getPIDFromFile( pidfnm, pid );
	if ( scriptfp.exists() )
	    File::remove( scriptfp.fullPath() );
    }

    uiRetVal ret;
    if ( res && execpars && stdoutstr && cl->getStdOutput() )
	cl->getStdOutput()->getAll( *stdoutstr );

    if ( !res && !cl->errorMsg().isEmpty() )
	ret = tr("Cannot execute '%1'").arg( cmd.toString(execpars) );

    if ( cl->getStdError() )
    {
	BufferString stderrorret;
	if ( stderrstr )
	    stderrorret.set( stderrstr->buf() );
	else
	    cl->getStdError()->getAll( stderrorret );

#ifdef __debug__
    if ( stderrorret.contains("(PE) HiddenParam") )
	stderrorret.setEmpty();
#endif
	if ( !stderrorret.isEmpty() )
	    ret.add( ::toUiString(stderrorret.str()) );
    }

    return ret;
}


namespace OD
{
    static const char* sKeyPriorityGlobExpr()	{ return "Priority.*"; }
} // namespace OD


bool OD::PythonAccess::getSortedVirtualEnvironmentLoc(
					ObjectSet<FilePath>& pythonenvfp,
					BufferStringSet& envnms,
					const BufferString* envnm,
					const FilePath* externalroot )
{
    FilePath envsfp;
    if ( externalroot )
	envsfp = *externalroot;
    else
    {
	if ( !hasInternalEnvironment() ||
	     (hasInternalEnvironment() &&
	      !getInternalEnvironmentLocation(envsfp,true)) )
	    return false;
    }

    if ( envnm )
    {
	if ( envnm->isEmpty() )
	{
	    pythonenvfp.add( new FilePath(envsfp) );
	    envnms.add( *envnm );
	    return true;
	}
	else
	{
	    const DirList dl( FilePath(envsfp,"envs").fullPath().str(),
			      File::DirsInDir );
	    if ( dl.isPresent(envnm->str()) )
	    {
		pythonenvfp.add( new FilePath(envsfp) );
		envnms.add( *envnm );
		return true;
	    }

	    BufferStringSet txtenvnms;
	    getCondaEnvsFromTxt( txtenvnms );
	    if ( txtenvnms.isPresent(envnm->str()) )
	    {
		pythonenvfp.add( new FilePath( envsfp ) );
		envnms.add( *envnm );
		return true;
	    }
	}

	return false;
    }

    const DirList dl( FilePath(envsfp,"envs").fullPath().str(),
		      File::DirsInDir );
    BufferStringSet prioritydirs;
    TypeSet<int> prioritylist;

    if ( !externalroot )
    {
	for ( int idx=0; idx<dl.size(); idx++ )
	{
	    const BufferString envpath( dl.fullPath( idx ) );
	    const DirList priorityfiles( envpath, File::FilesInDir,
					 sKeyPriorityGlobExpr() );
	    if ( !priorityfiles.isEmpty() )
	    {
		const FilePath priofp( priorityfiles.fullPath(0) );
		prioritydirs.add( envpath );
		prioritylist += toInt( priofp.extension() );
	    }
	}
    }

    if ( prioritylist.isEmpty() )
    {
	for ( int idx=0; idx<dl.size(); idx++ )
	{
	    pythonenvfp.add( new FilePath(envsfp) );
	    const FilePath virtenvpath( dl.fullPath(idx) );
	    envnms.add( virtenvpath.baseName() );
	}

	BufferStringSet txtenvnms;
	getCondaEnvsFromTxt( txtenvnms );
	for ( const auto* txtenvnm : txtenvnms )
	{
	    pythonenvfp.add( new FilePath(envsfp) );
	    envnms.add( txtenvnm->buf() );
	}

	pythonenvfp.add( new FilePath( envsfp ) );
	envnms.add( BufferString::empty() ); //Add base environment
    }
    else
    {
	while( !prioritylist.isEmpty() )
	{
	    int highestprio = -1, prioidx = -1;
	    for ( int idx=0; idx<prioritylist.size(); idx++ )
	    {
		if ( prioritylist[idx] > highestprio )
		{
		    highestprio = prioritylist[idx];
		    prioidx = idx;
		}
	    }
	    if ( prioidx == -1 )
		break;

	    pythonenvfp.add( new FilePath(envsfp) );
	    const FilePath virtenvpath( prioritydirs.get(prioidx) );
	    envnms.add( virtenvpath.baseName() );
	    prioritydirs.removeSingle( prioidx );
	    prioritylist.removeSingle( prioidx );
	}
    }

    return !pythonenvfp.isEmpty();
}


bool OD::PythonAccess::getCondaEnvsFromTxt( BufferStringSet& envnms )
{
    ManagedObjectSet<FilePath> fps;
    getCondaEnvFromTxtPath( fps );
    for ( const auto* fp : fps )
	envnms.add( fp->fullPath() );

    return !envnms.isEmpty();
}


bool OD::PythonAccess::getCondaEnvFromTxtPath( ObjectSet<FilePath>& fp )
{
    const FilePath envstxtfp( GetPersonalDir(), ".conda", "environments.txt" );
    if ( envstxtfp.exists() )
    {
	od_istream envstrm( envstxtfp.fullPath() );
	while ( !envstrm.atEOF() )
	{
	    BufferString line;
	    envstrm.getLine( line );
	    FilePath envfp( line );
	    if ( envfp.exists() )
		fp.add( new FilePath(envfp) );
	}
    }

    return !fp.isEmpty();
}


bool OD::PythonAccess::validInternalEnvironment( const FilePath& fp )
{
    if ( !fp.exists() )
	return false;

    const BufferString fpstr( fp.fullPath() );
    FilePath pythfp( fp );
    const bool islink = File::isLink( fpstr );
    if ( islink )
    {
	pythfp.set( File::linkEnd( fpstr ) );
	if ( !pythfp.exists() || !File::isDirectory(pythfp.fullPath()) )
	    return false;
    }

    const BufferString relinfostr( "relinfo" );
    const FilePath relinfofp( pythfp, relinfostr );
    if ( !relinfofp.exists() )
	return false;

    PtrMan<FilePath> activatefp = getActivateScript( pythfp );
    if ( !activatefp )
	return false;

    const DirList dl( FilePath(pythfp,"envs").fullPath().str(),
		      File::DirsInDir );
    for ( int idx=0; idx<dl.size(); idx++)
    {
	FilePath envfp( dl.fullPath(idx) );
#ifdef __unix__
	envfp.add( "bin" );
#endif
	envfp.add( sPythonExecNm() );
	if ( envfp.exists() )
	{
	    if ( islink )
		const_cast<FilePath&>( fp ) = pythfp;
	    return true;
	}
    }

    FilePath baseenvfp( pythfp );
#ifdef __unix__
    baseenvfp.add( "bin" );
#endif
    baseenvfp.add( sPythonExecNm() );

    return baseenvfp.exists();
}


void OD::PythonAccess::GetPythonEnvPath( FilePath& fp )
{
    BufferString pythonstr( sKey::Python() ); pythonstr.toLower();
    const IOPar& pythonsetts = Settings::fetch( pythonstr );
    PythonSource source;
    if ( !PythonSourceDef().parse(pythonsetts,sKeyPythonSrc(),source) )
	source = hasInternalEnvironment() ? Internal : System;

    if ( source == Custom )
    {
	BufferString virtenvloc, virtenvnm;
	pythonsetts.get( sKeyEnviron(), virtenvloc );
	pythonsetts.get( sKey::Name(), virtenvnm );
	fp.set( virtenvloc ).add( "envs" ).add( virtenvnm );
	if ( !fp.exists() )
	{
	    BufferStringSet txtenvnms;
	    getCondaEnvsFromTxt( txtenvnms );
	    if ( txtenvnms.isPresent(virtenvnm.str()) )
		fp.set( virtenvnm );
	}
    }
    else if ( source == Internal )
    {
	ManagedObjectSet<FilePath> fps;
	BufferStringSet envnms;
	getSortedVirtualEnvironmentLoc( fps, envnms );
	if ( !fps.isEmpty() )
	{
	    fp = *fps.first();
	    fp.add( "envs" ).add( envnms.first()->buf() );
	}
    }
}


void OD::PythonAccess::GetPythonEnvBinPath( FilePath& fp )
{
    GetPythonEnvPath( fp );
    fp.add( __iswin__ ? "Scripts" : "bin" );
}


void OD::PythonAccess::getPathToInternalEnv( FilePath& fp, bool userdef )
{
    fp = getInternalEnvPath( userdef );
}


FilePath OD::PythonAccess::getInternalEnvPath( bool userdef )
{
    FilePath fp;
    if ( getInternalEnvironmentLocation(fp,userdef) )
	return fp;

    return FilePath();
}


bool OD::PythonAccess::getInternalEnvironmentLocation( FilePath& fp,
						       bool userdef )
{
    const BufferString envloc( GetEnvVar("DTECT_PYTHON_INTERNAL") );
    if ( !envloc.isEmpty() )
    {
	fp.set( envloc );
	if ( validInternalEnvironment(fp) )
	    return true;
    }

    fp.set( GetSetupDataFileName(ODSetupLoc_ApplSetupPref,"Python_envs.txt",
				 false) );
    BufferString pythonloc;
    if ( fp.exists() )
    {
	od_istream strm( fp.fullPath() );
	ascistream astrm( strm );
	IOPar par;
	par.getFrom( astrm );
	if ( par.get(sKey::FileName(),pythonloc) && !pythonloc.isEmpty() )
	{
	    fp.set( pythonloc );
	    if ( validInternalEnvironment(fp) )
		return true;
	}
    }

    fp.set( GetSoftwareDir(false) );
    DirList dl( fp.pathOnly(), File::DirsInDir );
    const BufferStringSet::idx_type defidx = dl.nearestMatch( "Python" );
    if ( dl.validIdx(defidx) )
	dl.swap( 0, defidx );
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	const FilePath pythfp( dl.fullPath(idx) );
	if ( !validInternalEnvironment(pythfp) )
	    continue;

	fp = pythfp;
	return true;
    }

    if ( !userdef )
	return false;

    BufferString pythonstr( sKey::Python() ); pythonstr.toLower();
    const IOPar& pythonsetts = Settings::fetch( pythonstr );
    PythonSource source;
    if ( !PythonSourceDef().parse(pythonsetts,sKeyPythonSrc(),source) ||
	 source != Internal || !pythonsetts.get(sKeyEnviron(),pythonloc) )
	return false;

    fp.set( pythonloc );
    return validInternalEnvironment( fp );
}


bool OD::PythonAccess::hasInternalEnvironment( bool userdef )
{
    const FilePath fp( getInternalEnvPath( userdef ) );
    return fp.exists();
}


bool OD::PythonAccess::retrievePythonVersionStr()
{
    const bool nottested = !istested_;
    uiRetVal ret;
    if ( !isUsable_(nottested,ret) )
	return false;

    if ( nottested )
	return !pythversion_.isEmpty();

    OS::MachineCommand cmd( sPythonExecNm(true) );
    cmd.addFlag( "version" );
    BufferString stdoutstr, stderrstr;
    const bool res = execute( cmd, stdoutstr, ret, &stderrstr );
    if ( res )
    {
	if ( !stdoutstr.isEmpty() )
	    pythversion_ = stdoutstr;
	else if ( !stderrstr.isEmpty() )
	    pythversion_ = stderrstr;
    }

    return !pythversion_.isEmpty();
}


void OD::PythonAccess::appToBeStartedCB( CallBacker* )
{
    File::initTempDir();
}


namespace OD {
class PyEnvWork : public Task
{
public:

PyEnvWork( const char* piname, const CallBack& cb )
    : piname_(piname)
    , obj_(const_cast<CallBacker*>(cb.cbObj()))
    , cbf_(cb.cbFn())
{}

bool execute() override
{
    (obj_->*cbf_)( this );
    return true;
}

const char* getPIName() const
{
    return piname_.buf();
}

private:
    BufferString piname_;
    CallBacker* obj_;
    CallBackFunction cbf_;
};

} // namespace OD


uiRetVal OD::PythonAccess::verifyEnvironment( const char* piname )
{
    const CallBack cb = mCB(this,PythonAccess,verifyEnvironmentCB);
    auto* pywk = new PyEnvWork( piname, cb );
    Threads::WorkManager::twm().addWork( Threads::Work(*pywk,true), nullptr,
	Threads::CommandLaunchMgr::getMgr().wmQueueID(), false, false, true );

    return uiRetVal::OK();
}


void OD::PythonAccess::verifyEnvironmentCB( CallBacker* cb )
{
    mDynamicCastGet(PyEnvWork*,pyenvwk,cb);
    if ( !pyenvwk )
    {
	envVerified.trigger( uiStrings::sInvalid() );
	return;
    }

    uiRetVal ret;
    if ( !isUsable_(!istested_,ret) )
    {
	uiRetVal uirv = tr("Could not detect a valid Python installation:");
	uirv.add( ret );
	envVerified.trigger( uirv );
	return;
    }

    Threads::Locker locker( lock_ );
    ManagedObjectSet<ModuleInfo> moduleinfos;
    if ( moduleinfos_.isEmpty() )
    {
	ret = updateModuleInfo( nullptr );
	if ( !ret.isOK() )
	{
	    envVerified.trigger( ret );
	    return;
	}
    }

    moduleinfos = moduleinfos_;
    locker.unlockNow();

    FilePath fp( mGetSWDirDataDir() );
    fp.add( "Python" );
    const char* piname = pyenvwk->getPIName();
    BufferString genericName( piname );
    genericName.add( "_requirements" );
    BufferString platSpecificName( piname );
    platSpecificName.add( "_requirements_" )
		    .add( Platform::local().shortName() );

    fp.add( platSpecificName ).setExtension( "txt" );
    if ( !fp.exists() )
    {
	fp.setFileName( genericName );
	fp.setExtension( "txt" );
	if ( !fp.exists() )
	{
	    ret = uiStrings::phrFileDoesNotExist( fp.fullPath() );
	    envVerified.trigger( ret );
	    return;
	}
    }

    od_istream strm( fp.fullPath() );
    if ( !strm.isOK() )
    {
	ret = tr("Can't open requirements file: %1").arg( fp.fullPath() );
	envVerified.trigger( ret );
	return;
    }

    BufferString line;
    bool newlinefound = true;
    int lnum = 1;
    while ( strm.isOK() )
    {
	bool isminver = true;
	strm.getLine( line, &newlinefound );
	if ( !newlinefound )
	    break;

	line.trimBlanks();
	if ( line.isEmpty() )
	    continue;

	BufferStringSet modulestr;
	if ( line.contains("==") )
	    modulestr.unCat( line, "==" );
	else if ( line.contains(">=") )
	    modulestr.unCat( line, ">=" );
	else if ( line.contains("<") )
	{
	    modulestr.unCat( line, "<" );
	    isminver = false;
	}
	else
	    modulestr.add( line );

	const BufferString modname = modulestr.get(0).trimBlanks().toLower();
	if ( modulestr.size() == 1 )
	    ret.add( hasModule( moduleinfos, modname ) );
	else if ( modulestr.size() >= 2 )
	{
	    const BufferString ver = modulestr.get( 1 ).trimBlanks();
	    ret.add( hasModule( moduleinfos, modname, isminver, ver ) );
	}
	else
	{
	    ret.add( tr("Python requirements file: %1 error at line: %2"
			    ).arg( fp.fullPath() ).arg( lnum ) );
	}

	lnum++;
    };

    envVerified.trigger( ret );
}


BufferString OD::PythonAccess::getPacmanExecNm() const
{
    if ( activatefp_ )
    {
	FilePath packmanexe( *activatefp_ );
	packmanexe.setFileName( "conda" );
	if ( __iswin__ )
	    packmanexe.setExtension( "bat" );

	if ( packmanexe.exists() )
	    return packmanexe.fileName();

	if ( __iswin__ )
	{
	    packmanexe.setFileName( nullptr ).setFileName( nullptr )
		      .add( "Scripts" );
	}

	packmanexe.setFileName( "pip" );
	if ( __iswin__ )
	    packmanexe.setExtension( "exe" );

	if ( packmanexe.exists() )
	    return packmanexe.baseName();

	packmanexe.setFileName( "pip3" );
	if ( packmanexe.exists() )
	    return packmanexe.baseName();

	return BufferString( "pip" ); //Fallback
    }

    return BufferString( __iswin__ ? "pip" : "pip3" );
}


uiRetVal OD::PythonAccess::hasModule( const char* modname, bool isminver,
				      const char* version ) const
{
    return hasModule( moduleinfos_, modname, isminver, version );
}


uiRetVal OD::PythonAccess::hasModule( const ObjectSet<ModuleInfo>& moduleinfos,
				      const char* modname, bool isminver,
				      const char* version )
{
    uiString msg;
    if ( version )
	msg = tr("Package: %1 Version: %2 %3 required").arg( modname )
		.arg( isminver ? ">=" : "<" ).arg( version );
    else
	msg = tr("Package: %1 required").arg( modname );

    BufferStringSet modnames;
    modnames.unCat( modname, "|" );
    for ( const auto* reqmod : moduleinfos )
    {
	if ( modnames.isPresent(reqmod->name().buf()) )
	{
	    if ( version ) {
		const SeparString actverstr( reqmod->versionstr_, '.' );
		const SeparString reqverstr( version, '.' );
		for ( int ver=0; ver<reqverstr.size(); ver++ )
		{
		    const int actver = actverstr.getUIValue(ver);
		    const int reqver = reqverstr.getUIValue(ver);
		    if ( (isminver && (actver<reqver)) ||
					      (!isminver && (actver>reqver))  )
			return uiRetVal( tr("%1, but installed Version: %2")
				    .arg( msg ).arg( reqmod->versionstr_ ) );
		    else if ( (isminver && (actver>reqver)) ||
						(!isminver && (actver<reqver)) )
			break;
		}
	    }
	    return uiRetVal::OK();
	}
    }

    return uiRetVal( tr("%1, but module not found").arg( msg ) );
}


uiRetVal OD::PythonAccess::updateModuleInfo( const char* defprog,
					     const char* defarg )
{
    OS::MachineCommand mc( defprog );
    if ( mc.isBad() )
    {
	mc.setProgram( getPacmanExecNm() );
	mc.addArg( "list" );
    }
    else
	mc.addArg( defarg );

    moduleinfos_.setEmpty();
    if ( mc.args().isEmpty() )
	return uiRetVal( tr("Invalid command: %1").arg(mc.program()) );

    uiRetVal ret;
    BufferString stdoutstr;
    bool res = execute( mc, stdoutstr, ret );
    if ( !ApplicationData::hasInstance() )
	return uiStrings::sClose();

#ifdef __unix__
    if ( !res && StringView(mc.program()) == StringView("pip") )
    {
	mc.setProgram( "pip3" );
	res = execute( mc, stdoutstr, ret );
	if ( !ApplicationData::hasInstance() )
	    return uiStrings::sClose();
    }
#endif
    BufferStringSet modstrs;
    modstrs.unCat( stdoutstr );
    if ( !res || modstrs.isEmpty() )
    {
	return uiRetVal( tr("Cannot generate a list of python modules:\n%1")
			    .arg(ret) );
    }

    for ( auto* modstr : modstrs )
    {
	if ( modstr->startsWith("#") ||
	     modstr->startsWith("Package") || modstr->startsWith("----") )
	    continue;
	moduleinfos_.add( new ModuleInfo( modstr->trimBlanks().toLower() ) );
    }

    return uiRetVal::OK();
}


uiRetVal OD::PythonAccess::getModules( ManagedObjectSet<ModuleInfo>& mods )
{
    uiRetVal retval;
    Threads::Locker locker( lock_ );
    if ( moduleinfos_.isEmpty() )
    {
	retval = updateModuleInfo( nullptr );
	if ( !retval.isOK() )
	    return retval;
    }
    locker.unlockNow();

    mods.setEmpty();
    for ( const auto* mod : moduleinfos_ )
    {
	auto* minfo = new ModuleInfo( mod->name() );
	minfo->versionstr_ = mod->versionstr_;
	mods.add( minfo );
    }

    return retval;
}


void OD::PythonAccess::setForScript( const char* scriptnm,
				     OS::MachineCommand& mc ) const
{
    FilePath scriptfp;
    GetPythonEnvPath( scriptfp );
    if ( scriptfp.exists() && scriptfp.isAbsolute() )
	scriptfp.add( __iswin__ ? "Scripts" : "bin" );

    scriptfp.add( scriptnm );
    if ( __iswin__ )
	scriptfp.setExtension( "exe" );

    if ( scriptfp.exists() && scriptfp.isAbsolute() )
    {
	if ( __iswin__ )
	{
	    FilePath pyscriptfp( scriptfp );
	    const BufferString pyscriptfnm( scriptnm, "-script" );
	    pyscriptfp.setFileName( pyscriptfnm ).setExtension( "py" );
	    /* Such script exists only for a conda installation, where
	       executing using python.exe is not required */
	    if ( pyscriptfp.exists() )
	    {
		mc.setProgram( scriptfp.fileName() );
		return;
	    }
	}

	mc.setProgram( sPythonExecNm() ).addArg( scriptfp.fullPath() );
    }
    else
	mc.setProgram( scriptfp.fileName() );
}


bool OD::PythonAccess::openTerminal( const char* cmdstr, uiRetVal& ret,
				     const BufferStringSet* args,
				     const char* workingdirstr ) const
{
    if ( !cmdstr || !*cmdstr )
    {
	ret = tr("No terminal name provided");
	return false;
    }

    if ( !mSelf().isUsable_(!istested_,ret) )
	return false;

    BufferString prognm( cmdstr );
    bool iswindowsterminal = false;
    if ( prognm == "wt.exe" )
    {
	iswindowsterminal = true;
	const BufferStringSet paths;
	const BufferString progfp = File::findExecutable( prognm.buf(), paths );
	if ( File::exists(progfp.buf()) )
	    prognm = progfp;
    }

    OS::MachineCommand mc( prognm.buf() );
    if ( args )
	mc.addArgs( *args );

    BufferString workingdir( workingdirstr );
    if ( workingdir.isEmpty() )
	workingdir.set( GetPersonalDir() );

    OS::CommandExecPars pars( OS::RunInBG );
    if ( __iswin__ )
    {
	const StringView activatescript( getPythonActivatorPath() );
	pars.isconsoleuiprog( activatescript.isEmpty() && !iswindowsterminal );
    }

    pars.workingdir( workingdir );
    return execute( mc, pars, ret );
}



OD::PythonAccess::ModuleInfo::ModuleInfo( const char* modulestr )
    : NamedObject("")
{
    char valbuf[1024];
    const char* nextword = getNextWord( modulestr, valbuf );
    BufferString namestr( valbuf ); namestr.clean( BufferString::NoSpaces );
    if ( !namestr.isEmpty() )
	setName( namestr );

    mSkipBlanks( nextword ); if ( !nextword || !*nextword ) return;

    getNextWord( nextword, valbuf );
    versionstr_.set( valbuf ).clean( BufferString::NoSpaces );
}


bool OD::PythonAccess::ModuleInfo::operator ==( const ModuleInfo& oth ) const
{
    return name() == oth.name() && versionstr_ == oth.versionstr_;
}


bool OD::PythonAccess::ModuleInfo::operator !=( const ModuleInfo& oth ) const
{
    return !(*this == oth);
}


BufferString OD::PythonAccess::ModuleInfo::displayStr( bool withver ) const
{
    BufferString ret( name() );
    if ( withver )
	ret.add( " " ).add( versionstr_ );
    return ret;
}


namespace OD
{

static const char* sKeyNvidia() { return "NVIDIA"; }

static bool usesNvidiaCard( BufferString* glversionstr )
{
    bool ret = false;
    OS::MachineCommand cmd( "od_glxinfo" );
    BufferString stdoutstr;
    if ( !cmd.execute(stdoutstr) || stdoutstr.isEmpty() )
	return false;

    BufferStringSet glxinfostrs;
    glxinfostrs.unCat( stdoutstr.str() );
    for ( const auto line : glxinfostrs )
    {
	if ( !line->startsWith("OpenGL") )
	    continue;

	if ( line->contains("vendor string:") )
	    ret = line->contains( sKeyNvidia() );
	else if ( line->contains("version string:") && glversionstr )
	{
	    glversionstr->set( line->find( ':' )+1 );
	    glversionstr->trimBlanks();
	}
    }

    return ret;
}


//from https://docs.nvidia.com/cuda/cuda-toolkit-release-notes/index.html
static const char* cudastrs[] = { "11.4 Update 3", "11.3.1 Update 1",
    "11.2.2 Update 2", "11.1.1 Update 1", "11.0.3 Update 1",
    "10.2.89", "10.1.105", "10.0.130",
    "9.2.148 update 1", "9.2.88", "9.1.85", "9.0.76", "8.0.61 GA2", "8.0.44",
    "7.5.16", "7.0.28", 0 };
static const float nvidiavers[] = {
#ifdef __win__
    472.50f, 465.89f, 461.33f, 456.81f, 451.82f,
    441.22f, 418.96f, 411.31f, 398.26f,
    397.44f, 391.29f, 385.54f, 376.51f, 369.30f, 353.66f, 347.62f };
#else
    470.8201f, 465.1901f, 460.3203f, 455.32f,
    450.5106f, 440.33f, 418.39f, 410.48f, 396.37f,
    396.26f, 390.46f, 384.81f, 375.26f, 367.48f, 352.31f, 346.46f };
#endif

static bool cudaCapable( const char* glstr, BufferString* maxcudaversionstr )
{
    const StringView openglstr( glstr );
    if ( !openglstr.contains(sKeyNvidia()) )
	return false;

    char valbuf[1024];
    const char* nextword = getNextWord( openglstr, valbuf );
    mSkipBlanks( nextword );
    const char* lastword = nullptr;
    do
    {
	lastword = nextword;
	nextword = getNextWord( nextword, valbuf );
	mSkipBlanks( nextword );
    } while ( nextword && *nextword );

    const float version = toFloat(lastword);
    int idx = 0;
    BufferString tmpcudastr;
    BufferString& maxcudaversion = maxcudaversionstr ? *maxcudaversionstr
						     : tmpcudastr;
    while( true )
    {
	const float nvidiaver = nvidiavers[idx];
	const char* cudastr = cudastrs[idx++];
	if ( !cudastr )
	    break;

	if ( version >= nvidiaver )
	{
	    maxcudaversion.set( cudastr );
	    break;
	}
    }

    return !maxcudaversion.isEmpty();
}


bool canDoCUDA( BufferString& maxverstr )
{
    BufferString glversion;
    const bool hasnv = usesNvidiaCard( &glversion );
    return hasnv && cudaCapable( glversion, &maxverstr );
}


static BufferStringSet removeDirScript( const BufferString& path )
{
    BufferStringSet script;
    return script.add( "import shutil" )
		 .add( BufferString("shutil.rmtree(r'", path, "')") );
}


uiRetVal pythonRemoveDir( const char* path, bool waitforfin )
{
    if ( !File::isDirectory(path) )
	{ pFreeFnErrMsg("Not a directory"); }

    if ( !File::isWritable(path) )
	return uiStrings::phrCannotRemove( ::toUiString(path) );

    bool res; uiRetVal ret;
    if ( PythA().isUsable().isOK() )
	res = PythA().executeScript( removeDirScript(path), ret, waitforfin );
    else
	res = File::removeDir( path );

    if ( !res )
	ret.add( uiStrings::phrCannotRemove(uiStrings::sFolder()) );

    return ret;
}


void OD::PythonAccess::reReadFWRules( const BufferString& pypath )
{
    ObjectSet<FilePath> fps;
    BufferStringSet envnms;
    FilePath fp( pypath );
    OD::PythonAccess::getSortedVirtualEnvironmentLoc( fps, envnms, nullptr,
									&fp );
    for ( const BufferString* envnm : envnms )
    {
	if ( envnm->isEmpty() )
	    continue;

	ePDD().add( envnm->buf(),
		    OD::PythonAccess::firewallDesc().arg(envnm->buf()),
						ProcDesc::DataEntry::Python );
    }
}

} // namespace OD
