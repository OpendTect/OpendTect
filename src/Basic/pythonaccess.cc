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

OD::PythonAccess& OD::PythA()
{
    static PtrMan<OD::PythonAccess> theinst = nullptr;
    return *theinst.createIfNull();
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

};


OD::PythonAccess::~PythonAccess()
{
    delete activatefp_;
}


bool OD::PythonAccess::isUsable( bool force )
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
	isusable_ = isEnvUsable( nullptr );
	return isusable_;
    }

    PtrMan<File::Path> externalroot = nullptr;
    if ( source == Custom )
    {
	BufferString virtenvloc;
	if ( pythonsetts.get(sKeyEnviron(),virtenvloc) )
	    externalroot = new File::Path( virtenvloc );
    }

    ManagedObjectSet<File::Path> virtualenvsfp;
    if ( !getSortedVirtualEnvironmentLoc(virtualenvsfp,nullptr,externalroot) )
	return false;

    BufferString glversion, maxcudaversion;
    const bool hasnv = usesNvidiaCard( &glversion );
    const bool candocuda = hasnv && cudaCapable( glversion, &maxcudaversion );
    for ( auto virtualenvfp : virtualenvsfp )
    {
	if ( isEnvUsable(virtualenvfp,candocuda) )
	{
	    isusable_ = true;
	    return true;
	}
    }

    return false;
}


bool OD::PythonAccess::isEnvUsable( const File::Path* virtualenvfp,
				    bool tensorflowtest )
{
    BufferString venvnm, stdoutstr, stderrstr;
    PtrMan<File::Path> activatefp;
    if ( virtualenvfp )
    {
	if ( !virtualenvfp->exists() )
	    return false;

	venvnm.set( virtualenvfp->baseName() );
	const BufferString rootfpstr(
			    virtualenvfp->dirUpTo(virtualenvfp->nrLevels()-3) );
	activatefp = new File::Path( rootfpstr, "bin", "activate" );
	if ( !activatefp->exists() )
	    return false;
    }

    OS::MachineCommand cmd( sPythonExecNm(true) );
    if ( tensorflowtest )
    {
	cmd.addArg( "-c" );
	BufferStringSet scriptlines;
	scriptlines.add( "import tensorflow as tf" );
	scriptlines.add( "hello = tf.constant('hello TensorFlow!')" );
	scriptlines.add( "sess = tf.Session()" );
	scriptlines.add( "print( sess.run( hello ) )" );
	const BufferString scripttext( "\"", scriptlines.cat(";"), "\"" );
	cmd.addArg( scripttext );
    }
    else
    {
	cmd.addFlag( "version" );
    }

    bool res = doExecute( cmd, nullptr, nullptr, &stdoutstr, &stderrstr,
			  activatefp.ptr(), venvnm, nullptr );
    if ( !res )
	return false;

    res = tensorflowtest ? stdoutstr == "b'hello TensorFlow!'"
			 : !stdoutstr.isEmpty();
    if ( res )
    {
	if ( virtualenvfp )
	{
	    delete activatefp_;
	    activatefp_ = new File::Path( *activatefp );
	    virtenvnm_.set( venvnm );
	}
	else
	{
	    deleteAndZeroPtr( activatefp_ );
	    virtenvnm_.setEmpty();
	}
    }

    return res;
}


bool OD::PythonAccess::execute( const OS::MachineCommand& cmd,
				BufferString& stdoutstr,
				BufferString* stderrstr,
				uiString* errmsg )
{
    if ( !isUsable(!istested_) )
	return false;

    return doExecute( cmd, nullptr, nullptr,
		      &stdoutstr, stderrstr,
		      activatefp_, virtenvnm_.buf(), errmsg );
}


bool OD::PythonAccess::execute( const OS::MachineCommand& cmd,
				const OS::CommandExecPars& pars,
				int* pid, uiString* errmsg )
{
    if ( !isUsable(!istested_) )
	return false;

    return doExecute( cmd, &pars, pid,
		      nullptr, nullptr,
		      activatefp_, virtenvnm_.buf(), errmsg );
}


OS::CommandLauncher* OD::PythonAccess::getLauncher(
						const OS::MachineCommand& mc,
						File::Path& scriptfp )
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

#ifdef __unix__
    strm.add( "#!/bin/bash" ).add( "\n\n" );
    strm.add( "source " );
#endif
    strm.add( activatefp->fullPath() );
    if ( envnm )
	strm.add( " " ).add( envnm );
    strm.add( "\n" );
    strm.add( cmd.program() ).add( " " )
	.add( cmd.args().cat(" ") ).add( "\n" );
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
				  BufferString* stdoutstr,
				  BufferString* stderrstr,
				  const File::Path* activatefp,
				  const char* envnm, uiString* errmsg )
{
    File::Path scriptfp;
    PtrMan<OS::CommandLauncher> cl = getLauncher( cmd, activatefp, envnm,
						  scriptfp );
    if ( !cl )
    {
	if ( errmsg )
	    *errmsg = tr("Cannot create launcher for command '%1'")
			.arg( cmd.getSingleStringRep() );
	return false;
    }

    const bool res = stdoutstr ? cl->execute( *stdoutstr, stderrstr )
			       : ( execpars ? cl->execute( *execpars )
					    : cl->execute() );
    if ( pid )
	*pid = cl->processID();

    if ( !scriptfp.isEmpty() )
    {
	if ( execpars && (execpars->launchtype_ == OS::RunInBG) )
	    Threads::sleep( 0.5 );
	File::remove( scriptfp.fullPath() );
    }

    if ( !res && errmsg )
    {
	if ( cl->errorMsg().isEmpty() )
	    errmsg->set( uiStrings::phrCannotStart(cmd.program()) );
	else
	    errmsg->set( cl->errorMsg() );
    }

    return res;
}


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
    if ( envnm )
    {
	for ( int idx=0; idx<dl.size(); idx++ )
	{
	    const File::Path fp( dl.fullPath(idx) );
	    if ( fp.baseName() != FixedString(envnm) )
		continue;

	    virtualenvfp.add( new File::Path(fp) );
	    return true;
	}
    }
    else
    {
	BufferStringSet prioritydirs;
	TypeSet<int> prioritylist;
	for ( int idx=0; idx<dl.size(); idx++ )
	{
	    const BufferString envpath( dl.fullPath(idx) );
	    const DirList priorityfiles( envpath,File::FilesInDir,"Priority.*");
	    for ( int idy=0; idy<priorityfiles.size(); idy++ )
	    {
		const File::Path priofp( priorityfiles.fullPath(idy) );
		prioritydirs.add( envpath );
		prioritylist += toInt( priofp.extension() );
		break;
	    }
	}
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

    return !virtualenvfp.isEmpty();
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

const char* OD::PythonAccess::sKeyPythonSrc() { return "Python Source"; }
const char* OD::PythonAccess::sKeyEnviron() { return "Environment"; }


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



bool OD::PythonAccess::validInternalEnvironment( const File::Path& fp )
{
    if ( !fp.exists() )
	return false;

    const BufferString relinfostr( "relinfo" );
    const File::Path relinfofp( fp, relinfostr );
    if ( !relinfofp.exists() )
	return false;

    const File::Path activatefp( fp, "bin", "activate" );
    if ( !activatefp.exists() )
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
