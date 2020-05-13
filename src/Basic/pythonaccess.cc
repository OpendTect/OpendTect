/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A. Huck
 Date:		May 2019
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "pythonaccess.h"

#include "ascstream.h"
#include "bufstringset.h"
#include "dirlist.h"
#include "envvars.h"
#include "genc.h"
#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include "odplatform.h"
#include "oscommand.h"
#include "separstr.h"
#include "procdescdata.h"
#include "settingsaccess.h"
#include "string2.h"
#include "timefun.h"
#include "timer.h"
#include "uistrings.h"
#include "errmsg.h"

const char* OD::PythonAccess::sKeyPythonSrc() { return "Python Source"; }
const char* OD::PythonAccess::sKeyEnviron() { return "Environment"; }
const char* OD::PythonAccess::sKeyPythonPath() { return "PythonPath"; }


OD::PythonAccess& OD::PythA()
{
    mDefineStaticLocalObject( PtrMan<PythonAccess>, theinst, = nullptr );
    return *theinst.createIfNull();
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


BufferStringSet OD::PythonAccess::getBasePythonPath() const
{
    BufferStringSet pythonpaths = pystartpath_;
    const FilePath scriptbinfp( GetSoftwareDir(true), "v7", "bin" );
    const FilePath pythonmodsfp( scriptbinfp.fullPath(), "python" );

    if ( pythonmodsfp.exists() )
	pythonpaths.addIfNew( pythonmodsfp.fullPath() );

    return pythonpaths;
}


void OD::PythonAccess::updatePythonPath()
{
    BufferStringSet pythonpaths = getBasePythonPath();

    BufferString pythonstr( sKey::Python() ); pythonstr.toLower();
    const IOPar& pythonsetts = Settings::fetch( pythonstr );
    const PtrMan<IOPar> pathpar = pythonsetts.subselect( sKeyPythonPath() );
    if ( pathpar )
    {
	BufferStringSet settpaths;
	settpaths.usePar( *pathpar );
	pythonpaths.add( settpaths, false );
    }

    SetEnvVarDirList( "PYTHONPATH", pythonpaths, false );
}


void OD::PythonAccess::initClass()
{
    GetEnvVarDirList( "PYTHONPATH", pystartpath_, true );
    updatePythonPath();

#ifdef __win__
    ManagedObjectSet<FilePath> fps;
    BufferStringSet envnms;

    BufferString pythonstr( sKey::Python() ); pythonstr.toLower();
    const IOPar& pythonsetts = Settings::fetch( pythonstr );
    PythonSource source = hasInternalEnvironment() ? Internal : System;
    PythonSourceDef().parse( pythonsetts, sKeyPythonSrc(), source );
    FilePath externalroot;
    const int totnrarg = GetArgC();
    bool useextparth = false;
    if ( totnrarg > 4 )
    {
	BufferString str = GetArgV()[ totnrarg - 2 ];
	useextparth = str.isEqual( PythA().sKeyUseExtPyPath(),
							    CaseInsensitive );
	if ( useextparth )
	{
	    externalroot = GetArgV()[ totnrarg - 1 ];
	    if ( externalroot.isEmpty() ||
				!File::exists(externalroot.fullPath()) )
		return;

	    source = Custom;
	}
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

    uiStringSet result;
    result += tr("Using");
    result += uiString(tr("%1 %2").arg(source).arg(pyVersion()));

    if ( source == Custom )
    {
	BufferString virtenvloc;
	if ( pythonsetts.get(sKeyEnviron(),virtenvloc) )
	{
	    if ( virtenvnm_.isEmpty() )
		result += uiString(tr("from %1").arg(virtenvloc));
	    else
		result += uiString(tr("environment %1 in %2").arg(virtenvnm_)
							    .arg(virtenvloc));
	}
    }

    return result.cat( " " );
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
	    externalroot = new FilePath( virtenvloc );
	virtenvnm = new BufferString();
	pythonsetts.get( sKey::Name(), *virtenvnm );
    }

    ManagedObjectSet<FilePath> pythonenvsfp;
    BufferStringSet envnms;
    if ( !getSortedVirtualEnvironmentLoc(pythonenvsfp,envnms,virtenvnm,
					 externalroot) )
	return false;

    for ( int idx=0; idx<pythonenvsfp.size(); idx++ )
    {
	const FilePath* pythonenvfp = pythonenvsfp[idx];
	const BufferString& envnm = envnms.get( idx );
	if ( isEnvUsable(pythonenvfp,envnm.buf(),scriptstr,scriptexpectedout) )
	    return true;
    }

    return false;
}


FilePath* OD::PythonAccess::getActivateScript( const FilePath& rootfp )
{
    FilePath ret( rootfp.fullPath(), "bin" );
    if ( !ret.exists() )
    {
	ret.set( rootfp.fullPath() ).add( "condabin" );
	if ( !ret.exists() )
	    return nullptr;
    }
    ret.add( "activate" );
#ifdef __win__
    ret.setExtension( "bat" );
#endif

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

	activatefp = getActivateScript( FilePath(pythonenvfp->fullPath()) );
	if ( !activatefp )
	    return false;
    }

    BufferString comm( sPythonExecNm(true) );
    const bool doscript = scriptstr && *scriptstr;
    if ( doscript )
	comm.add( " -c " ).add( scriptstr );
    else
	comm.add( " --version" );

    const OS::MachineCommand cmd( comm );
    mDefineStaticLocalObject(bool, force_external,
				= GetEnvVarYN("OD_FORCE_PYTHON_ENV_OK") );
    bool res = force_external ? true
	     : doExecute( cmd, nullptr, nullptr, activatefp.ptr(), venvnm );
    if ( !res )
	return false;

    const bool testscriptout = scriptexpectedout && *scriptexpectedout;
    res = doscript ?  (testscriptout ? laststdout_ ==
				       FixedString(scriptexpectedout)
				     : res)
		   : !laststdout_.isEmpty() || !laststderr_.isEmpty();
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
    execpars.prioritylevel_ = 0.f;
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
    BufferString comm( sPythonExecNm(true) );
    comm.add( " -c \"import " ).add( nm ).add( "\"");
    const OS::MachineCommand cmd( comm );
    return execute( cmd ) && lastOutput(true,nullptr).isEmpty();
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
    if ( !activatefp || !envnm )
    {
	const OS::MachineCommand cmdret( cmd );
	cmd = cmdret;
	return nullptr;
    }

    FilePath* ret = new FilePath( FilePath::getTempName() );
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

#ifdef __win__
    strm.add( "@SETLOCAL" ).add( od_newline );
    strm.add( "@ECHO OFF" ).add( od_newline ).add( od_newline );
    if ( background )
    {
	strm.add( "SET procnm=%~n0" ).add( od_newline );
	strm.add( "SET proctitle=%procnm%_children" ).add( od_newline );
	strm.add( "SET pidfile=\"%~dpn0.pid\"" )
	    .add( od_newline ).add( od_newline);
    }
    strm.add( "@CALL \"" );
#else
    strm.add( "#!/bin/bash" ).add( od_newline ).add( od_newline );
    strm.add( "source " );
#endif
    strm.add( activatefp->fullPath() );
#ifdef __win__
    strm.add( "\"" );
#endif
    if ( envnm )
	strm.add( " " ).add( envnm ).add( od_newline );
#ifdef __win__
    if ( background )
	strm.add( "Start \"%proctitle%\" /MIN " );
#endif
    strm.add( cmd.command() );
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
	strm.add( "echo $! > " ).add( getPIDFilePathStr(*ret) );
#endif
    }
    strm.add( od_newline );

    strm.close();
#ifdef __unix__
    File::makeExecutable( ret->fullPath(), true );
#endif
    BufferString cmdstr( ret->fullPath() );
    OS::CommandLauncher::addQuotesIfNeeded( cmdstr );
    cmd = OS::MachineCommand( cmdstr );

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
    PtrMan<FilePath> scriptfp = getCommand( scriptcmd, background,
					    activatefp, envnm );
    if ( scriptfp )
	scriptfpret = *scriptfp;
    else
	scriptfpret.set( nullptr );

    OS::CommandLauncher* cl = new OS::CommandLauncher( scriptcmd, true );
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
    const bool background = execpars && execpars->launchtype_ == OS::RunInBG;
    cl_ = getLauncher( cmd, background, activatefp, envnm, scriptfp );
    if ( !cl_.ptr() )
    {
	msg_ = tr("Cannot create launcher for command '%1'")
		    .arg( cmd.getSingleStringRep() );
	return false;
    }

    const bool res = execpars ? cl_->execute( *execpars )
			      : cl_->execute( laststdout_, &laststderr_ );
    if ( pid )
	*pid = cl_->processID();

    if ( !scriptfp.isEmpty() )
    {
	const BufferString pidfnm( getPIDFilePathStr(scriptfp) );
	if ( res && background )
	    getPIDFromFile( pidfnm, pid );
	File::remove( scriptfp.fullPath() );
    }

    if ( !res )
    {
	if ( cl_->errorMsg().isEmpty() )
	{
	    BufferStringSet commandline;
	    commandline.unCat( cmd.command(), " " );
	    msg_ = uiStrings::phrCannotStart( commandline.isEmpty()
					      ? uiStrings::sBatchProgram()
					      : toUiString(commandline.get(0)));
	}
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
	if ( !envnm->isEmpty() )
	{
	    const DirList dl( FilePath(envsfp,"envs").fullPath().str(),
			      DirList::DirsOnly );
	    if ( !dl.isPresent(envnm->str()) )
		return false;
	}

	pythonenvfp.add( new FilePath(envsfp) );
	envnms.add( *envnm );
	return true;
    }

    const DirList dl( FilePath(envsfp,"envs").fullPath().str(),
		      DirList::DirsOnly );
    BufferStringSet prioritydirs;
    TypeSet<int> prioritylist;
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	const BufferString envpath( dl.fullPath(idx) );
	const DirList priorityfiles( envpath, DirList::FilesOnly,
				     sKeyPriorityGlobExpr() );
	if ( !priorityfiles.isEmpty() )
	{
	    const FilePath priofp( priorityfiles.fullPath(0) );
	    prioritydirs.add( envpath );
	    prioritylist += toInt( priofp.extension() );
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


bool OD::PythonAccess::validInternalEnvironment( const FilePath& fp )
{
    if ( !fp.exists() )
	return false;

    const BufferString relinfostr( "relinfo" );
    const FilePath relinfofp( fp, relinfostr );
    if ( !relinfofp.exists() )
	return false;

    PtrMan<FilePath> activatefp = getActivateScript( fp );
    if ( !activatefp )
	return false;

    const DirList dl( FilePath(fp,"envs").fullPath().str(), DirList::DirsOnly );
    for ( int idx=0; idx<dl.size(); idx++)
    {
	FilePath envfp( dl.fullPath(idx) );
#ifdef __unix__
	envfp.add( "bin" );
#endif
	envfp.add( sPythonExecNm() );
	if ( envfp.exists() )
	    return true;
    }

    return false;
}


void OD::PythonAccess::GetPythonEnvPath( FilePath& fp )
{
    BufferString pythonstr( sKey::Python() ); pythonstr.toLower();
    const IOPar& pythonsetts = Settings::fetch( pythonstr );
    PythonSource source;
    if (!PythonSourceDef().parse(pythonsetts,sKeyPythonSrc(),source) )
	source = System;

    if ( source == Custom ) {
	BufferString virtenvloc, virtenvnm;
	pythonsetts.get(sKeyEnviron(),virtenvloc);
	pythonsetts.get(sKey::Name(),virtenvnm);
	#ifdef __win__
	fp = FilePath( virtenvloc, "envs", virtenvnm );
	#else
	fp = FilePath( "/", virtenvloc, "envs", virtenvnm );
	#endif
    }
    else if (source == Internal) {
	ManagedObjectSet<FilePath> fps;
	BufferStringSet envnms;
	getSortedVirtualEnvironmentLoc( fps, envnms );
	if ( fps.size()<1 )
	    return;
	fp = *fps[0];
	fp.add("envs").add(envnms.get(0));
    }
}


void OD::PythonAccess::GetPythonEnvBinPath( FilePath& fp )
{
    GetPythonEnvPath( fp );
#ifdef __win__
    fp.add("Scripts");
#else
    fp.add("bin");
#endif
}


void OD::PythonAccess::getPathToInternalEnv( FilePath& fp, bool userdef )
{
    fp = getInternalEnvPath( userdef );
}


FilePath OD::PythonAccess::getInternalEnvPath( bool userdef )
{
    FilePath fp;
    getInternalEnvironmentLocation( fp, userdef );
    return fp;
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

    const BufferString comm( sPythonExecNm(true), " --version" );
    const OS::MachineCommand cmd( comm );
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
    updatePythonPath();
    const uiRetVal uirv = updateModuleInfo( nullptr );
    if ( !uirv.isOK() )
	msg_.append( uirv );

    envChange.trigger();
}


uiRetVal OD::PythonAccess::verifyEnvironment( const char* piname )
{
    if ( !isUsable_(!istested_) )
	return uiRetVal( tr("Could not detect a valid Python installation.") );

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
	    return uiRetVal( uiStrings::sFileDoesntExist() );
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
	packmanexe.setFileName( "conda" );
#ifdef __win__
	packmanexe.setExtension( "bat" );
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

    for ( int idx=0; idx<moduleinfos_.size(); idx++ )
    {
	const ModuleInfo* module = moduleinfos_[idx];
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


uiRetVal OD::PythonAccess::updateModuleInfo( const char* cmd )
{
    BufferString modulecmd( cmd );
    if ( modulecmd.isEmpty() )
	modulecmd.set( getPacmanExecNm() ).addSpace().add( "list" );

    moduleinfos_.setEmpty();

    BufferStringSet cmdstrs;
    cmdstrs.unCat( modulecmd, " " );
    if ( cmdstrs.isEmpty() )
	return uiRetVal( tr("Invalid command: %1").arg(modulecmd) );

    const BufferString prognm( cmdstrs.first()->str() );
    cmdstrs.removeSingle(0);
    BufferString comm( prognm );
    comm.addSpace().add( cmdstrs.cat( " " ) );
    OS::MachineCommand mc( comm );
    BufferString laststdout, laststderr;
    bool res = execute( mc, laststdout, &laststderr );
#ifdef __unix__
    if ( !res && prognm == FixedString("pip") )
    {
	comm.set( "pip3" ).addSpace().add( cmdstrs.cat( " " ) );
	mc.setCommand( comm );
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

    for ( int idx=0; idx<modstrs.size(); idx++ )
    {
	BufferString& modstr = modstrs.get( idx );
	if ( modstr.startsWith("#") ||
	     modstr.startsWith("Package") || modstr.startsWith("----") )
	    continue;
	moduleinfos_.add( new ModuleInfo( modstr.trimBlanks().toLower() ) );
    }

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
    for ( int idx=0; idx<moduleinfos_.size(); idx++ )
    {
	const ModuleInfo* module = moduleinfos_[idx];
	ModuleInfo* minfo = new ModuleInfo( module->name() );
	minfo->versionstr_ = module->versionstr_;
	mods.add( minfo );
    }

    return retval;
}


bool OD::PythonAccess::openTerminal() const
{
    const BufferString termem = SettingsAccess().getTerminalEmulator();
    BufferString cmd;
    bool immediate = false;
#ifdef __win__
    cmd.set( "start " ).add( termem );
    immediate = true;
#else
    cmd.set( termem );
#endif
    return execute( OS::MachineCommand(cmd), immediate );
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
    const OS::MachineCommand cmd( "od_glxinfo" );
    OS::CommandLauncher cl( cmd );
    BufferString stdoutstr;
    if ( !cl.execute(stdoutstr) || stdoutstr.isEmpty() )
	return false;

    BufferStringSet glxinfostrs;
    glxinfostrs.unCat( stdoutstr.str() );
    for ( int idx=0; idx<glxinfostrs.size(); idx++ )
    {
	const BufferString& line = *glxinfostrs[idx];
	if ( !line.startsWith("OpenGL") )
	    continue;

	if ( line.contains("vendor string:") )
	    ret = line.contains( sKeyNvidia() );
	else if ( line.contains("version string:") && glversionstr )
	{
	    glversionstr->set( line.find( ':' )+1 );
	    glversionstr->trimBlanks();
	}
    }

    return ret;
}


//from https://docs.nvidia.com/cuda/cuda-toolkit-release-notes/index.html
static const char* cudastrs[] = { "10.2.89", "10.1.105", "10.0.130",
    "9.2.148 update 1", "9.2.88", "9.1.85", "9.0.76", "8.0.61 GA2", "8.0.44",
    "7.5.16", "7.0.28", 0 };
#ifdef __win__
static const float nvidiavers[] = { 441.22f, 418.96f, 411.31f, 398.26f,
    397.44f, 391.29f, 385.54f, 376.51f, 369.30f, 353.66f, 347.62f };
#else
static const float nvidiavers[] = { 440.33f, 418.39f, 410.48f, 396.37f,
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

BufferString removeDirScript(const BufferString& path)
{
    BufferStringSet script;
    script.add( "import shutil" );
    const BufferString remcmd( "shutil.rmtree(r'", path, "')" );
    script.add( remcmd );
    return BufferString( "\"", script.cat(";"), "\"" );
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
	BufferString cmdstr( PythA().sPythonExecNm() );
	BufferString pathstr( removeDirScript(path) );
	OS::CommandLauncher::addQuotesIfNeeded( pathstr );
	cmdstr.addSpace().add( "-c" ).addSpace().add( pathstr );
	const OS::MachineCommand cmd( cmdstr );

	ret = PythA().execute( cmd, waitforfin );

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
    {
	retval.setEmpty();
	ret = File::removeDir( path );
    }

    if ( !ret )
	retval.add( uiStrings::phrCannotRemove(uiStrings::sFolder()) );

    return retval;
}

};

