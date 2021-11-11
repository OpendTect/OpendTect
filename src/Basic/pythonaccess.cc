/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A. Huck
 Date:		May 2019
________________________________________________________________________

-*/


#include "pythonaccess.h"

#include "ascstream.h"
#include "bufstringset.h"
#include "commanddefs.h"
#include "commandlineparser.h"
#include "dirlist.h"
#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "genc.h"
#include "oddirs.h"
#include "odplatform.h"
#include "oscommand.h"
#include "procdescdata.h"
#include "separstr.h"
#include "settingsaccess.h"
#include "string2.h"
#include "timefun.h"
#include "timer.h"
#include "uistrings.h"


namespace OD
{

    const char* PythonAccess::sKeyPythonSrc()	{ return "Python Source"; }
    const char* PythonAccess::sKeyEnviron()	{ return "Environment"; }
    const char* PythonAccess::sKeyPythonPath()	{ return "PythonPath"; }
    const char* PythonAccess::sKeyActivatePath() { return "activatefnm"; }
    static const char* sKeyPythonPathEnvStr()	{ return "PYTHONPATH"; }

} //namespace OD

BufferStringSet OD::PythonAccess::pystartpath_{0}; //From user environment

OD::PythonAccess& OD::PythA()
{
    mDefineStaticLocalObject( PtrMan<PythonAccess>, theinst,
							  = new PythonAccess );
    return *theinst;
}


const char* OD::PythonAccess::sPythonExecNm( bool v3, bool v2 )
{
#ifdef __win__
    return "python.exe";
#else
    if (v3)
	return "python3";
    else if (v2)
	return "python2";
    else
	return "python";
#endif
}


mDefineNameSpaceEnumUtils(OD,PythonSource,"Python Source")
{
    "Internal", "System", "Custom", 0
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
{
}


OD::PythonAccess::~PythonAccess()
{
    detachAllNotifiers();
    delete activatefp_;
}


const BufferStringSet& OD::PythonAccess::getBasePythonPath() const
{
    mDefineStaticLocalObject( PtrMan<BufferStringSet>, theinst,
						      = new BufferStringSet );
    return *theinst;
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
	    const_cast<BufferStringSet&>( getBasePythonPath() );
	bpythonpath.addIfNew( cleanfp.fullPath() );
    }

    updatePythonPath();
}


void OD::PythonAccess::updatePythonPath() const
{
    BufferStringSet pythonpaths = getBasePythonPath();

    BufferString pythonstr( sKey::Python() ); pythonstr.toLower();
    const IOPar& pythonsetts = Settings::fetch( pythonstr );
    PtrMan<IOPar> pathpar = pythonsetts.subselect( sKeyPythonPath() );
    if ( pathpar )
    {
	BufferStringSet settpaths, settpathsadd;
	settpaths.usePar( *pathpar );
	for ( auto settpathstr : settpaths )
	{
	    FilePath fp( settpathstr->buf() );
	    if ( !fp.exists() )
		continue;
	    fp.makeCanonical();
	    settpathsadd.addIfNew( fp.fullPath() );
	}

	pythonpaths.add( settpathsadd, false );
    }
}


void OD::PythonAccess::initClass()
{
    GetEnvVarDirList( sKeyPythonPathEnvStr(), pystartpath_, true );
    FilePath pythonmodsfp;
    if ( isDeveloperBuild() )
    {
	pythonmodsfp.set( __FILE__ );
	pythonmodsfp.set( pythonmodsfp.dirUpTo( pythonmodsfp.nrLevels()-4 ) )
		    .add( "external" ).add( "odpy" );
    }
    else
	pythonmodsfp.set( GetSoftwareDir(true) ).add( "bin" ).add( "python");
    if ( pythonmodsfp.exists() )
	PythA().addBasePath( pythonmodsfp );

    pythonmodsfp.add( "odpy" );
    if ( !pythonmodsfp.exists() )
    {
	pythonmodsfp = FilePath( GetSoftwareDir(true), "v7", "bin", "python" );
	if ( pythonmodsfp.exists() )
	    PythA().addBasePath( pythonmodsfp );
    }

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

	ePDD().add( *envnms[idx],
	 ::toUiString("Machine Learning Environment : <%1>").arg(*envnms[idx]),
	    ProcDesc::DataEntry::Python );
    }

#endif
}


BufferString OD::PythonAccess::pyVersion() const
{
    if ( pythversion_.isEmpty() )
    {
	PythonAccess& acc = const_cast<PythonAccess&>( *this );
	acc.retrievePythonVersionStr();
    }

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


uiRetVal OD::PythonAccess::isUsable( bool force, const char* scriptstr,
				     const char* scriptexpectedout ) const
{
    if ( !force && istested_ )
    {
	uiRetVal ret;
	if ( !isusable_ && msg_.isEmpty() )
	    ret.add( tr("Cannot run Python") );
	return ret;
    }

    PythonAccess& pytha = const_cast<PythonAccess&>( *this );
    const bool isusable = pytha.isUsable_( force, scriptstr,
							scriptexpectedout );
    if ( isusable )
	return uiRetVal::OK();

    uiString clmsg;
    const BufferString stdoutstr( lastOutput(true,&clmsg) );
    const BufferString stderrstr( lastOutput(false,nullptr) );

    uiRetVal ret;
    if ( !clmsg.isEmpty() )
	ret.add( clmsg );
    if ( !stdoutstr.isEmpty() )
	ret.add( toUiString(stdoutstr) );
    if ( !stderrstr.isEmpty() )
	ret.add( toUiString(stderrstr) );
    return ret;
}


bool OD::PythonAccess::isUsable_( bool force, const char* scriptstr,
				 const char* scriptexpectedout )
{
    if ( !force && istested_ )
	return isusable_;

    istested_ = true;
    isusable_ = false;
    BufferString pythonstr( sKey::Python() ); pythonstr.toLower();
    const IOPar& pythonsetts = Settings::fetch( pythonstr );
    PythonSource source = hasInternalEnvironment(false) ? Internal : System;
    PythonSourceDef().parse( pythonsetts, sKeyPythonSrc(), source );
    if ( source == System )
	return isEnvUsable(nullptr,nullptr,scriptstr,scriptexpectedout);

    PtrMan<FilePath> externalroot = nullptr;
    PtrMan<BufferString> virtenvnm;
    if ( source == Custom )
    {
	BufferString virtenvloc;
	if ( pythonsetts.get(sKeyEnviron(),virtenvloc) )
	{
	    if ( !File::exists(virtenvloc) || !File::isDirectory(virtenvloc) )
	    {
		msg_ = tr("Selected custom python environment does "
			  "not exist:\n'%1'").arg( virtenvloc );
		return false;
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
	msg_ = source == Custom
	     ? tr("Custom environment [%1] does not exist")
		    .arg( virtenvnm->isEmpty() ? "base" : virtenvnm->buf() )
	     : tr("Internal environments are not usable");
	return false;
    }

    for ( int idx=0; idx<pythonenvsfp.size(); idx++ )
    {
	const FilePath* pythonenvfp = pythonenvsfp[idx];
	const BufferString& envnm = envnms.get( idx );
	if ( isEnvUsable(pythonenvfp,envnm.buf(),scriptstr,scriptexpectedout) )
	    return true;
    }

    msg_ = tr("Internal environments are not usable");

    return false;
}


namespace OD {

BufferString& GetPythonActivatorExe()
{
    mDefineStaticLocalObject( PtrMan<BufferString>, ret, = new BufferString );
    return *ret.ptr();
}

} //namespace OD


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
#ifdef __win__
	ret.setExtension( "bat" );
#endif
    if ( !ret.exists() )
    {
	ret.set( rootfp.fullPath() ).add( "condabin" );
	ret.add("activate");
#ifdef __win__
	ret.setExtension("bat");
#endif
	}
    return ret.exists() ? new FilePath( ret ) : nullptr;
}


bool OD::PythonAccess::isEnvUsable( const FilePath* pythonenvfp,
				    const char* envnm,
				    const char* scriptstr,
				    const char* scriptexpectedout )
{
    PtrMan<FilePath> activatefp;
    BufferString venvnm( envnm );
    if ( pythonenvfp )
    {
	if ( !pythonenvfp->exists() )
	    return false;

	activatefp = GetPythonActivatorExe().isEmpty()
		   ? getActivateScript( FilePath( pythonenvfp->fullPath() ) )
		   : new FilePath( *pythonenvfp );
	if ( !activatefp )
	    return false;
    }

    OS::MachineCommand cmd( sPythonExecNm(true) );
    const bool doscript = scriptstr && *scriptstr;
    if ( doscript )
	cmd.addArg( "-c" ).addArg( scriptstr );
    else
	cmd.addFlag( "version" );

    mDefineStaticLocalObject(bool, force_external,
				= GetEnvVarYN("OD_FORCE_PYTHON_ENV_OK") );
    bool res = force_external ? true
	     : doExecute( cmd, nullptr, nullptr, activatefp.ptr(), venvnm );
    if ( !res )
	return false;

#ifdef __debug__
    if ( res && laststderr_.contains("(PE) HiddenParam") )
	laststderr_.setEmpty();
#endif

    const bool testscriptout = scriptexpectedout && *scriptexpectedout;
    res = doscript ?  (testscriptout ? laststdout_ ==
				       FixedString(scriptexpectedout)
				     : res)
		   : laststderr_.isEmpty();
    if ( !res && !force_external )
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
	deleteAndZeroPtr( activatefp_ );
	virtenvnm_.setEmpty();
    }

    if ( pythversion_.isEmpty() && moduleinfos_.isEmpty() )
	notrigger = false;

    msg_.setEmpty();
    isusable_ = true;
    if ( !notrigger )
	envChangeCB(nullptr);

    return isusable_;
}


bool OD::PythonAccess::execute( const OS::MachineCommand& cmd,
				bool wait4finish ) const
{
    OS::CommandExecPars execpars( wait4finish ? OS::Wait4Finish : OS::RunInBG );
    execpars.createstreams_ = true;
    return execute( cmd, execpars );
}


bool OD::PythonAccess::execute( const OS::MachineCommand& cmd,
				BufferString& stdoutstr,
				BufferString* stderrstr,
				uiString* errmsg ) const
{
    if ( !const_cast<PythonAccess&>(*this).isUsable_(!istested_) )
	return false;

    const bool res = doExecute( cmd, nullptr, nullptr, activatefp_,
				virtenvnm_.buf() );
    if ( &stdoutstr != &laststdout_ )
	stdoutstr = laststdout_;
    if ( stderrstr && &laststderr_ != stderrstr )
	stderrstr->set( laststderr_ );
    if ( errmsg )
	*errmsg = msg_;

    return res;
}


bool OD::PythonAccess::execute( const OS::MachineCommand& cmd,
				const OS::CommandExecPars& pars,
				int* pid, uiString* errmsg ) const
{
    if ( !const_cast<PythonAccess&>(*this).isUsable_(!istested_) )
	return false;

    const bool res = doExecute( cmd, &pars, pid, activatefp_, virtenvnm_.buf());
    if ( errmsg )
	*errmsg = msg_;

    return res;
}


bool OD::PythonAccess::executeScript( const char* scriptstr,
				      bool wait4finish ) const
{
    OS::MachineCommand mc( sPythonExecNm(true), "-c", scriptstr );
    OS::CommandExecPars execpars( wait4finish ? OS::Wait4Finish
					      : OS::RunInBG );
    execpars.createstreams( true );
    return execute( mc, execpars );
}


bool OD::PythonAccess::executeScript( const BufferStringSet& scriptstrs,
				      bool wait4finish ) const
{
    return executeScript( BufferString(scriptstrs.cat(";")),
			  wait4finish );
}


BufferString OD::PythonAccess::lastOutput( bool stderrout, uiString* msg ) const
{
    if ( cl_.ptr() )
    {
	OS::CommandLauncher* cl = cl_.ptr();
	if ( msg && !cl->errorMsg().isEmpty() )
	    *msg = cl->errorMsg();

	BufferString ret;
	if ( stderrout )
	{
	    if ( cl->getStdError() )
	    {
		cl->getStdError()->getAll( ret );
		if ( ret.isEmpty() && !laststderr_.isEmpty() )
		    ret = laststderr_;
	    }
	    else
		ret = laststderr_;
	}
	else
	{
	    if ( cl->getStdOutput() )
	    {
		cl->getStdOutput()->getAll( ret );
		if ( ret.isEmpty() && !laststdout_.isEmpty() )
		    ret = laststdout_;
	    }
	    else
		ret = laststdout_;
	}

	return ret;
    }

    if ( msg )
	*msg = msg_;
    return stderrout ? laststderr_ : laststdout_;
}


bool OD::PythonAccess::isModuleUsable( const char* nm ) const
{
    const BufferString importscript( "import ", nm );
    return executeScript( importscript ) && lastOutput(true,nullptr).isEmpty();
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
    const FixedString typestr( str );
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
    if ( background )
	strm.add( "Start \"%proctitle%\" /MIN " );
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
	    const BufferString rootfnm = rootfp.dirUpTo( rootfp.nrLevels()-1 );
	    cmdret.addArg( rootfnm );
	    if ( envnm && *envnm )
		cmdret.addKeyedArg( "envnm", envnm );

	    const BufferString prognm( mc.program() );
	    if ( __iswin__ )
	    {
		if ( background )
		{
		    scriptfp = new FilePath(
			    FilePath::getTempFullPath("python", "txt" ) );

		    cmdret.addKeyedArg( "pidbasenm", scriptfp->fullPath() );
		}

		if ( prognm.startsWith("cmd",CaseInsensitive) ||
		     prognm.startsWith("powershell",CaseInsensitive) )
		    cmdret.addFlag( "consoleuiprog" );
	    }

	    cmdret.addFlag( "" ).addArg( prognm ).addArgs( mc.args() );
	    scriptcmd = OS::MachineCommand( cmdret, true );
	}
    }
    else
    {
	const OS::MachineCommand cmdret( scriptcmd, true );
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


bool OD::PythonAccess::doExecute( const OS::MachineCommand& cmd,
				  const OS::CommandExecPars* execpars, int* pid,
				  const FilePath* activatefp,
				  const char* envnm ) const
{
    laststdout_.setEmpty();
    laststderr_.setEmpty();
    msg_.setEmpty();

    FilePath scriptfp;
    const bool background = execpars && execpars->launchtype_ >= OS::RunInBG;
    cl_ = getLauncher( cmd, background, activatefp, envnm, scriptfp );
    if ( !cl_.ptr() )
    {
	msg_ = tr("Cannot create launcher for command '%1'")
		    .arg( cmd.toString(execpars) );
	return false;
    }

    BufferStringSet origpythonpathdirs;
    GetEnvVarDirList( sKeyPythonPathEnvStr(), origpythonpathdirs, false );
    BufferStringSet pythonpathdirs( getBasePythonPath() );
    pythonpathdirs.add( pystartpath_, false );
    SetEnvVarDirList( sKeyPythonPathEnvStr(), pythonpathdirs, false );

    const bool res = execpars ? cl_->execute( *execpars )
			      : cl_->execute( laststdout_, &laststderr_ );
    if ( pid )
	*pid = cl_->processID();

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

    if ( !res )
    {
	if ( cl_->errorMsg().isEmpty() )
	    msg_ = uiStrings::phrCannotStart( ::toUiString(cmd.program()) );
	else
	    msg_ = cl_->errorMsg();
    }

    return res;
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
	source = System;

    if ( source == Custom )
    {
	BufferString virtenvloc, virtenvnm;
	pythonsetts.get( sKeyEnviron(), virtenvloc );
	pythonsetts.get( sKey::Name(), virtenvnm );
#ifdef __win__
	fp = FilePath( virtenvloc, "envs", virtenvnm );
#else
	fp = FilePath( "/", virtenvloc, "envs", virtenvnm );
#endif
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
#ifdef __win__
    fp.add( "Scripts" );
#else
    fp.add( "bin" );
#endif
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
    if ( !isUsable_(!istested_) )
	return false;

    OS::MachineCommand cmd( sPythonExecNm(true), "--version" );
    BufferString laststdout, laststderr;
    const bool res = execute( cmd, laststdout, &laststderr );
    if ( res )
    {
	if ( !laststdout.isEmpty() )
	    pythversion_ = laststdout;
	else if ( !laststderr.isEmpty() )
	    pythversion_ = laststderr;
    }

    return !pythversion_.isEmpty();
}


void OD::PythonAccess::envChangeCB( CallBacker* )
{
    retrievePythonVersionStr();
    const uiRetVal uirv = updateModuleInfo( nullptr );
    if ( !uirv.isOK() )
	msg_.append( uirv );

    envChange.trigger();
}


uiRetVal OD::PythonAccess::verifyEnvironment( const char* piname )
{
    if ( !isUsable_(!istested_) )
    {
	uiRetVal ret = tr("Could not detect a valid Python installation:\n%1")
		.arg( lastOutput(true,nullptr) );
	return ret.add( tr("Python environment not usable") );
    }

    if ( !msg_.isEmpty() )
	return uiRetVal( msg_ );

    FilePath fp( mGetSWDirDataDir() );
    fp.add( "Python" );
    BufferString genericName( piname );
    genericName.add( "_requirements" );
    BufferString platSpecificName( piname );
    platSpecificName.add( "_requirements_" )
		    .add(Platform::local().shortName());

    fp.add( platSpecificName ).setExtension( "txt" );
    if ( !fp.exists() )
    {
	fp.setFileName( genericName );
	fp.setExtension( "txt" );
	if ( !fp.exists() )
	    return uiRetVal( uiStrings::phrFileDoesNotExist(fp.fullPath()) );
    }

    od_istream strm( fp.fullPath() );
    if ( !strm.isOK() )
	return uiRetVal( tr("Can't open requirements file: %1").arg(
			    fp.fullPath() ) );

    BufferString line;
    bool newlinefound = true;
    int lnum = 1;
    uiRetVal retval;
    while ( strm.isOK() )
    {
	strm.getLine( line, &newlinefound );
	if ( !newlinefound )
	    break;
	BufferStringSet modulestr;
	modulestr.unCat( line, "==" );
	BufferString modname = modulestr.get(0).trimBlanks().toLower();
	if ( modulestr.size() == 1 )
	    retval.add( hasModule( modname ) );
	else if (modulestr.size() >= 2 )
	{
	    BufferString ver = modulestr.get( 1 ).trimBlanks();
	    retval.add( hasModule( modname, ver ) );
	} else
	    retval.add( tr("Python requirements file: %1 error at line: %2"
			    ).arg( fp.fullPath() ).arg( lnum ) );
	lnum++;
    };

    return retval;
}


BufferString OD::PythonAccess::getPacmanExecNm() const
{
    if ( activatefp_ )
    {
	FilePath packmanexe( *activatefp_ );
#ifdef __win__
	if ( !GetPythonActivatorExe().isEmpty() )
	    packmanexe.add( "condabin" )
		      .add( "activate" );
#endif
	packmanexe.setFileName( "conda" );
#ifdef __win__
	packmanexe.setExtension( "exe" );
#endif
	if ( packmanexe.exists() )
	    return packmanexe.baseName();

#ifdef __win__
	packmanexe.setFileName( nullptr );
	packmanexe.setFileName( nullptr );
	packmanexe.add( "Scripts" );
#endif
	packmanexe.setFileName( "pip" );
#ifdef __win__
	packmanexe.setExtension( "exe" );
#endif
	if ( packmanexe.exists() )
	    return packmanexe.baseName();

	packmanexe.setFileName( "pip3" );
	if ( packmanexe.exists() )
	    return packmanexe.baseName();

	return BufferString( "pip" ); //Fallback
    }

#ifdef __win__
    return BufferString( "pip" );
#else
    return BufferString( "pip3" );
#endif
}


uiRetVal OD::PythonAccess::hasModule( const char* modname,
				      const char* minversion ) const
{
    uiString msg;
    if ( minversion )
	msg = tr("Package: %1 Version: %2 or higher required").arg( modname )
		.arg( minversion );
    else
	msg = tr("Package: %1 required").arg( modname );

    for ( auto module : moduleinfos_ )
    {
	if ( module->name() == modname )
	{
	    if ( minversion ) {
		const SeparString actverstr( module->versionstr_, '.' );
		const SeparString reqverstr( minversion, '.' );
		for ( int ver=0; ver<reqverstr.size(); ver++ )
		{
		    if ( actverstr.getUIValue(ver)<
			 reqverstr.getUIValue(ver) )
			return uiRetVal( tr("%1, but installed Version: %2")
					    .arg( msg )
			.arg( module->versionstr_ ) );
		    else if ( actverstr.getUIValue(ver)>reqverstr
				.getUIValue(ver))
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

    BufferString laststdout, laststderr;
    bool res = execute( mc, laststdout, &laststderr );
#ifdef __unix__
    if ( !res && FixedString(mc.program()) == FixedString("pip") )
    {
	mc.setProgram( "pip3" );
	res = execute( mc, laststdout, &laststderr );
    }
#endif
    BufferStringSet modstrs;
    modstrs.unCat( laststdout );
    if ( !res || modstrs.isEmpty() )
    {
	return uiRetVal( tr("Cannot generate a list of python modules:\n%1")
			    .arg(laststderr) );
    }

    for ( auto modstr : modstrs )
    {
	if ( modstr->startsWith("#") ||
	     modstr->startsWith("Package") || modstr->startsWith("----") )
	    continue;
	moduleinfos_.add( new ModuleInfo( modstr->trimBlanks().toLower() ) );
    }

    laststdout_.setEmpty();
    laststderr_.setEmpty();

    return uiRetVal::OK();
}


uiRetVal OD::PythonAccess::getModules( ManagedObjectSet<ModuleInfo>& mods )
{
    uiRetVal retval;
    if ( moduleinfos_.isEmpty() )
    {
	retval = updateModuleInfo( nullptr );
	if ( !retval.isOK() )
	    return retval;
    }

    mods.setEmpty();
    for ( auto module : moduleinfos_ )
    {
	ModuleInfo* minfo = new ModuleInfo( module->name() );
	minfo->versionstr_ = module->versionstr_;
	mods.add( minfo );
    }

    return retval;
}


bool OD::PythonAccess::openTerminal() const
{
    const CommandDefs& cmddefs =
			CommandDefs::getTerminalCommands( BufferStringSet() );
    if ( cmddefs.isEmpty() )
	return false;

    return openTerminal( cmddefs.first()->buf(), nullptr, nullptr );
}


bool OD::PythonAccess::openTerminal( const char* cmdstr,
				     const BufferStringSet* args,
				     const char* workingdirstr ) const
{
    if ( !cmdstr || !*cmdstr )
    {
	laststderr_ = "[Internal] No terminal name provided";
	return false;
    }

    OS::MachineCommand mc( cmdstr );
    OS::CommandExecPars pars( OS::RunInBG );
    if ( args )
	mc.addArgs( *args );

    BufferString workingdir( workingdirstr );
    if ( workingdir.isEmpty() )
	workingdir.set( GetPersonalDir() );

    pars.workingdir( workingdir );
    return execute( mc, pars );
}



OD::PythonAccess::ModuleInfo::ModuleInfo( const char* modulestr )
    : NamedObject("")
{
    char valbuf[1024];
    const char* nextword = getNextWord( modulestr, valbuf );
    BufferString namestr( valbuf ); namestr.clean( BufferString::NoSpaces );
    if ( !namestr.isEmpty() )
	setName( namestr );

    mSkipBlanks( nextword ); if ( !*nextword ) return;

    getNextWord( nextword, valbuf );
    versionstr_.set( valbuf ).clean( BufferString::NoSpaces );
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
    const FixedString openglstr( glstr );
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
    } while ( *nextword );

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
    uiRetVal retval;
    if ( !File::isDirectory(path) )
	{pFreeFnErrMsg("Not a directory"); }
    if ( !File::isWritable(path) )
    {
	retval.add( uiStrings::phrCannotRemove(::toUiString(path)) );
	return retval;
    }

    retval = PythA().isUsable();
    bool ret;
    if ( retval.isOK() )
    {
	retval.setEmpty();
	ret = PythA().executeScript( removeDirScript(path), waitforfin );

	uiString errmsg;
	const BufferString errstr = PythA().lastOutput( true, &errmsg );
	if ( !errmsg.isEmpty() )
	    retval.add( errmsg );
	if ( !errstr.isEmpty() )
	{
	    errmsg.setEmpty();
	    retval.add(  errmsg.append( errstr ) );
	}
    }
    else
	ret = File::removeDir( path );

    if ( !ret )
	retval.add( uiStrings::phrCannotRemove(uiStrings::sFolder()) );

    return retval;
}

};

