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
#include "filepath.h"
#include "keystrs.h"
#include "oddirs.h"
#include "oscommand.h"
#include "settings.h"
#include "staticstring.h"
#include "string2.h"
#include "uistrings.h"

const char* OD::PythonAccess::sKeyPythonSrc() { return "Python Source"; }
const char* OD::PythonAccess::sKeyEnviron() { return "Environment"; }


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
{
}



OD::PythonAccess::~PythonAccess()
{
    delete activatefp_;
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
	isusable_ = isEnvUsable( nullptr, scriptstr, scriptexpectedout );
	return isusable_;
    }

    PtrMan<File::Path> externalroot = nullptr;
    BufferString virtenvnm;
    if ( source == Custom )
    {
	BufferString virtenvloc;
	if ( pythonsetts.get(sKeyEnviron(),virtenvloc) )
	    externalroot = new File::Path( virtenvloc );
	pythonsetts.get( sKey::Name(), virtenvnm );
    }

    ManagedObjectSet<File::Path> virtualenvsfp;
    if ( !getSortedVirtualEnvironmentLoc(virtualenvsfp,virtenvnm.buf(),
					 externalroot) )
	return false;

    for ( auto virtualenvfp : virtualenvsfp )
    {
	if ( isEnvUsable(virtualenvfp,scriptstr,scriptexpectedout) )
	{
	    isusable_ = true;
	    return true;
	}
    }

    return false;
}


File::Path* OD::PythonAccess::getActivateScript( const File::Path& rootfp )
{
    File::Path ret( rootfp.fullPath(), "bin" );
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

    return ret.exists() ? new File::Path( ret ) : nullptr;
}


bool OD::PythonAccess::isEnvUsable( const File::Path* virtualenvfp,
				    const char* scriptstr,
				    const char* scriptexpectedout )
{
    BufferString venvnm;
    PtrMan<File::Path> activatefp;
    if ( virtualenvfp )
    {
	if ( !virtualenvfp->exists() )
	    return false;

	venvnm.set( virtualenvfp->baseName() );
	const BufferString rootfpstr(
			    virtualenvfp->dirUpTo(virtualenvfp->nrLevels()-3) );
	activatefp = getActivateScript( File::Path(rootfpstr) );
	if ( !activatefp )
	    return false;
    }

    OS::MachineCommand cmd( sPythonExecNm(true) );
    const bool doscript = scriptstr && *scriptstr;
    if ( doscript )
	cmd.addArg( "-c" ).addArg( scriptstr );
    else
	cmd.addFlag( "version" );

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
    if ( virtualenvfp )
    {
	notrigger = activatefp_ &&
		    activatefp_->fullPath() == activatefp->fullPath() &&
		    virtenvnm_ == venvnm;
	delete activatefp_;
	activatefp_ = new File::Path( *activatefp );
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


bool OD::PythonAccess::execute( const OS::MachineCommand& cmd ) const
{
    return execute( cmd, laststdout_, &laststderr_, &msg_ );
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
	errmsg->set( msg_ );

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
	errmsg->set( msg_ );

    return res;
}


BufferString OD::PythonAccess::lastOutput( bool stderrout, uiString* msg ) const
{
    if ( msg )
	msg->set( msg_ );
    return stderrout ? laststderr_ : laststdout_;
}


bool OD::PythonAccess::isModuleUsable( const char* nm ) const
{
    OS::MachineCommand cmd( sPythonExecNm(true) );
    cmd.addArg( "-c" ).addArg( BufferString("\"import ",nm,"\"") );
    return execute( cmd ) && lastOutput(true,nullptr).isEmpty();
}


OS::CommandLauncher* OD::PythonAccess::getLauncher(
						const OS::MachineCommand& mc,
						File::Path& scriptfp ) const
{
    if ( !isUsable(!istested_) )
	return nullptr;

    return getLauncher( mc, activatefp_, virtenvnm_.buf(), scriptfp );
}


File::Path* OD::PythonAccess::getCommand( OS::MachineCommand& cmd,
					  const File::Path* activatefp,
					  const char* envnm )
{
    if ( !activatefp || !envnm )
	return nullptr;

    File::Path* ret = new File::Path(
			  File::Path::getTempFullPath("runpython",nullptr) );
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
    strm.add( cmd.program() ).add( " " )
	.add( cmd.args().cat(" ") ).add( od_newline );
#ifdef __win__
    strm.add( "conda deactivate" ).add( od_newline );
#endif
    strm.close();
#ifdef __unix__
    File::makeExecutable( ret->fullPath(), true );
#endif
    cmd = OS::MachineCommand( ret->fullPath() );
    return ret;
}


OS::CommandLauncher* OD::PythonAccess::getLauncher(
						const OS::MachineCommand& mc,
						const File::Path* activatefp,
						const char* envnm,
						File::Path& scriptfpret )
{
    OS::MachineCommand scriptcmd( mc );
    PtrMan<File::Path> scriptfp = getCommand( scriptcmd, activatefp, envnm );
    if ( scriptfp )
	scriptfpret = *scriptfp;
    else
	scriptfpret.set( nullptr );

    OS::CommandLauncher* cl = new OS::CommandLauncher( scriptcmd );
    return cl;
}


bool OD::PythonAccess::doExecute( const OS::MachineCommand& cmd,
				  const OS::CommandExecPars* execpars, int* pid,
				  const File::Path* activatefp,
				  const char* envnm ) const
{
    laststdout_.setEmpty();
    laststderr_.setEmpty();
    msg_.setEmpty();

    File::Path scriptfp;
    PtrMan<OS::CommandLauncher> cl = getLauncher( cmd, activatefp, envnm,
						  scriptfp );
    if ( !cl )
    {
	msg_ = tr("Cannot create launcher for command '%1'")
		    .arg( cmd.getSingleStringRep() );
	return false;
    }

    const bool res = execpars ? cl->execute( *execpars )
			      : cl->execute( laststdout_, &laststderr_ );
    if ( pid )
	*pid = cl->processID();

    if ( !scriptfp.isEmpty() )
    {
	if ( execpars && (execpars->launchtype_ == OS::RunInBG) )
	    Threads::sleep( 0.5 );
	File::remove( scriptfp.fullPath() );
    }

    if ( !res )
    {
	if ( cl->errorMsg().isEmpty() )
	    msg_.set( uiStrings::phrCannotStart(cmd.program()) );
	else
	    msg_.set( cl->errorMsg() );
    }

    if ( execpars && execpars->createstreams_ )
    {
	if ( cl->getStdOutput() )
	    cl->getStdOutput()->getAll( laststdout_ );
	if ( cl->getStdError() )
	    cl->getStdError()->getAll( laststderr_ );
    }

    return res;
}


namespace OD
{
    static const char* sKeyPriorityGlobExpr()	{ return "Priority.*"; }
} // namespace OD


bool OD::PythonAccess::getSortedVirtualEnvironmentLoc(
					ObjectSet<File::Path>& virtualenvfp,
					const char* envnm,
					const File::Path* externalroot )
{
    File::Path envsfp;
    if ( externalroot )
	envsfp = *externalroot;
    else
    {
	if ( !hasInternalEnvironment() ||
	     (hasInternalEnvironment() &&
	      !getInternalEnvironmentLocation(envsfp,true)) )
	    return false;
    }

    const DirList dl( File::Path(envsfp,"envs").fullPath().str(),
		      File::DirsInDir );
    if ( envnm && *envnm )
    {
	if ( !dl.isPresent(envnm) )
	    return false;

	const File::Path fp( dl.fullPath( dl.indexOf(envnm) ) );
	virtualenvfp.add( new File::Path(fp) );
	return true;
    }
    else
    {
	BufferStringSet prioritydirs;
	TypeSet<int> prioritylist;
	for ( int idx=0; idx<dl.size(); idx++ )
	{
	    const BufferString envpath( dl.fullPath(idx) );
	    const DirList priorityfiles( envpath, File::FilesInDir,
					 sKeyPriorityGlobExpr() );
	    if ( !priorityfiles.isEmpty() )
	    {
		const File::Path priofp( priorityfiles.fullPath(0) );
		prioritydirs.add( envpath );
		prioritylist += toInt( priofp.extension() );
	    }
	}
	if ( prioritylist.isEmpty() )
	{
	    for ( int idx=0; idx<dl.size(); idx++ )
		virtualenvfp.add( new File::Path(dl.fullPath(idx)) );
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

		virtualenvfp.add( new File::Path(prioritydirs.get(prioidx)) );
		prioritydirs.removeSingle( prioidx );
		prioritylist.removeSingle( prioidx );
	    }
	}
    }

    return !virtualenvfp.isEmpty();
}


bool OD::PythonAccess::validInternalEnvironment( const File::Path& fp )
{
    if ( !fp.exists() )
	return false;

    const BufferString relinfostr( "relinfo" );
    const File::Path relinfofp( fp, relinfostr );
    if ( !relinfofp.exists() )
	return false;

    PtrMan<File::Path> activatefp = getActivateScript( fp );
    if ( !activatefp )
	return false;

    const DirList dl( File::Path(fp,"envs").fullPath().str(), File::DirsInDir );
    for ( int idx=0; idx<dl.size(); idx++)
    {
	File::Path envfp( dl.fullPath(idx) );
#ifdef __unix__
	envfp.add( "bin" );
#endif
	envfp.add( sPythonExecNm() );
	if ( envfp.exists() )
	    return true;
    }

    return false;
}


bool OD::PythonAccess::getInternalEnvironmentLocation( File::Path& fp,
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
    File::Path fp;
    if ( !getInternalEnvironmentLocation(fp,userdef) )
	return false;

    return true;
}


uiRetVal OD::PythonAccess::getModules( ObjectSet<ModuleInfo>& mods,
				       const char* cmd )
{
    const OS::MachineCommand mc( cmd );
    const bool res = execute( mc );
    if ( !res )
    {
	return uiRetVal( tr("Cannot detect list of python modules:\n%1")
				.arg(laststderr_) );
    }
    BufferStringSet modstrs;
    modstrs.unCat( laststdout_ );
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
    OS::MachineCommand cmd( "od_glxinfo" );
    BufferString stdoutstr;
    if ( !cmd.execute(stdoutstr) || stdoutstr.isEmpty() )
	return false;

    BufferStringSet glxinfostrs;
    glxinfostrs.unCat( stdoutstr.str() );
    for ( auto line : glxinfostrs )
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
    } while( *nextword );

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

