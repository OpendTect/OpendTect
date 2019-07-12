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
#include "dirlist.h"
#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include "oscommand.h"
#include "settings.h"
#include "string2.h"
#include "timefun.h"
#include "timer.h"
#include "thread.h"
#include "uistrings.h"

const char* OD::PythonAccess::sKeyPythonSrc() { return "Python Source"; }
const char* OD::PythonAccess::sKeyEnviron() { return "Environment"; }

#define mFileRetentionTimeInMilliSec 60000
#define mDelCycleTym                mFileRetentionTimeInMilliSec*5

OD::PythonAccess& OD::PythA()
{
    static PtrMan<OD::PythonAccess> theinst = nullptr;
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
    , filedeltimer_(*new Timer( "Delete Files" ))
{
    mAttachCB( filedeltimer_.tick, PythonAccess::handleFilesCB );
}



OD::PythonAccess::~PythonAccess()
{
    detachAllNotifiers();
    delete activatefp_;
    delete &filedeltimer_;
    for ( int idx=fptodelset_.size()-1; idx>=0; idx-- )
	File::remove( fptodelset_[idx]->fullPath() );
}


bool OD::PythonAccess::isUsable( bool force, const char* scriptstr,
				 const char* scriptexpectedout ) const
{
    if ( !force )
	return isusable_;

    OD::PythonAccess& pytha = const_cast<OD::PythonAccess&>( *this );
    return pytha.isUsable( force, scriptstr, scriptexpectedout );
}


bool OD::PythonAccess::isUsable( bool force, const char* scriptstr,
				 const char* scriptexpectedout )
{
    if ( !force )
	return isusable_;

    istested_ = true;
    isusable_ = false;
    BufferString pythonstr( sKey::Python() ); pythonstr.toLower();
    const IOPar& pythonsetts = Settings::fetch( pythonstr );
    PythonSource source;
    const bool usesystem =
	!PythonSourceDef().parse(pythonsetts,sKeyPythonSrc(),source) ||
	source == System;

    if ( usesystem )
    {
	isusable_ = isEnvUsable(nullptr,nullptr,scriptstr,scriptexpectedout);
	return isusable_;
    }

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
	{
	    isusable_ = true;
	    return true;
	}
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
    bool res = doExecute( cmd, nullptr, nullptr, activatefp.ptr(), venvnm );
    if ( !res )
	return false;

    const bool testscriptout = scriptexpectedout && *scriptexpectedout;
    res = doscript ?  (testscriptout ? laststdout_ ==
				       FixedString(scriptexpectedout)
				     : res)
		   : !laststdout_.isEmpty() || !laststderr_.isEmpty();
    if ( !res )
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

    msg_.setEmpty();
    if ( !notrigger )
	envChange.trigger();

    return true;
}


bool OD::PythonAccess::execute( const OS::MachineCommand& cmd,
				bool wait4finish ) const
{
    const OS::CommandExecPars execpars( wait4finish ? OS::Wait4Finish
						    : OS::RunInBG );
    return execute( cmd, execpars );
}


bool OD::PythonAccess::execute( const OS::MachineCommand& cmd,
				BufferString& stdoutstr,
				BufferString* stderrstr,
				uiString* errmsg ) const
{
    if ( !isUsable(!istested_) )
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
    if ( !isUsable(!istested_) )
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
		cl->getStdError()->getAll( ret );
	    else
		ret = laststderr_;
	}
	else
	{
	    if ( cl->getStdOutput() )
		cl->getStdOutput()->getAll( ret );
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


FilePath* OD::PythonAccess::getCommand( OS::MachineCommand& cmd,
					  const FilePath* activatefp,
					  const char* envnm )
{
    if ( !activatefp || !envnm )
	return nullptr;

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
    strm.add( "@ECHO OFF" ).add( od_newline );
    strm.add( "@CALL \"" );
#else
    strm.add( "#!/bin/bash" ).add( "\n\n" );
    strm.add( "source " );
#endif
    strm.add( activatefp->fullPath() );
#ifdef __win__
    strm.add( "\"" );
#endif
    if ( envnm )
	strm.add( " " ).add( envnm ).add( od_newline );
    strm.add( cmd.command() ).add( od_newline );
#ifdef __win__
    strm.add( "conda deactivate" ).add( od_newline );
#endif
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
						const FilePath* activatefp,
						const char* envnm,
						FilePath& scriptfpret )
{
    OS::MachineCommand scriptcmd( mc );
    PtrMan<FilePath> scriptfp = getCommand( scriptcmd, activatefp, envnm );
    if ( scriptfp )
	scriptfpret = *scriptfp;
    else
	scriptfpret.set( nullptr );

    OS::CommandLauncher* cl = new OS::CommandLauncher( scriptcmd, true );
    return cl;
}


void OD::PythonAccess::handleFilesCB( CallBacker* )
{
    filedeltimer_.stop();
    for ( int idx=fptodelset_.size(); idx>=0; idx-- )
    {
	const FilePath& fp = *fptodelset_[idx];
	if ( !fp.exists() )
	{
	    fptodelset_.removeSingle( idx );
	    continue;
	}

	const BufferString scriptfnm( fp.fullPath() );
	const od_int64 creationtym = File::getTimeInMilliSeconds( scriptfnm );
	const od_int64 currtym = Time::getMilliSeconds();
	const double timediff = creationtym - currtym;
	if ( timediff < mFileRetentionTimeInMilliSec )
	    continue;

	File::remove( scriptfnm );
	fptodelset_.removeSingle( idx );
    }

    if ( !fptodelset_.isEmpty() && !filedeltimer_.isActive() )
	filedeltimer_.start( mDelCycleTym );
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
    cl_ = getLauncher( cmd, activatefp, envnm, scriptfp );
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
	if ( res && execpars && (execpars->launchtype_ == OS::RunInBG) )
	{
	    fptodelset_.add( new FilePath(scriptfp) );
	    if ( !filedeltimer_.isActive() )
		filedeltimer_.start( mDelCycleTym );
	}
	else
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


uiRetVal OD::PythonAccess::getModules( ObjectSet<ModuleInfo>& mods,
				       const char* cmd )
{
    OS::MachineCommand mc( cmd );
    BufferString laststdout, laststderr;
    bool res = execute( mc, laststdout, &laststderr );
#ifdef __unix__
    if ( !res )
    {
	BufferStringSet cmdstrs;
	cmdstrs.unCat( cmd, " " );
	if ( !cmdstrs.isEmpty() && cmdstrs.get(0) == "pip" )
	{
	    cmdstrs.get(0) = "pip3";
	    mc.setCommand( cmdstrs.cat( " " ) );
	    res = execute( mc, laststdout, &laststderr );
	}
    }
#endif
    if ( !res )
    {
	return uiRetVal( tr("Cannot detect list of python modules:\n%1")
				.arg(laststderr) );
    }

    BufferStringSet modstrs;
    modstrs.unCat( laststdout );
    for ( int idx=2; idx<modstrs.size(); idx++ )
	mods.add( new ModuleInfo( modstrs[idx]->buf() ) );

    return uiRetVal::OK();
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
static const char* cudastrs[] = { "10.1.105", "10.0.130", "9.2.148 update 1",
    "9.2.88", "9.1.85", "9.0.76", "8.0.61 GA2", "8.0.44", "7.5.16", "7.0.28",
0 };
#ifdef __win__
static const float nvidiavers[] = { 418.96f, 411.31f, 398.26f, 397.44f, 391.29f,
    385.54f, 376.51f, 369.30f, 353.66f, 347.62f };
#else
static const float nvidiavers[] = { 418.39f, 410.48f, 396.37f, 396.26f, 390.46f,
    384.81f, 375.26f, 367.48f, 352.31f, 346.46f };
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

};

