/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Dec 2013
-*/


#include "batchjobdispatch.h"
#include "keystrs.h"
#include "filepath.h"
#include "oddirs.h"
#include "ascstream.h"
#include "dirlist.h"
#include "dbman.h"
#include "iopar.h"
#include "pythonaccess.h"

static const char* sKeyProgramName = "Program.Name";
static const char* sKeyClArgs = "Program.Args";


mImplClassFactory( Batch::JobDispatcher, factory )


Batch::JobSpec::JobSpec( Batch::JobSpec::ProcType pt, OS::LaunchType lt )
    : execpars_(lt)
    , prognm_(progNameFor(pt))
{
    execpars_.needmonitor_ = true;
}


Batch::JobSpec::JobSpec( const char* pnm, OS::LaunchType lt )
    : execpars_(lt)
    , prognm_(pnm)
{
    execpars_.needmonitor_ = true;
}


Batch::JobSpec::JobSpec( const IOPar& iop, OS::LaunchType lt )
    : execpars_(lt)
{
    usePar( iop );
    pars_.removeWithKey( sKey::Survey() );
    pars_.removeWithKey( sKey::DataRoot() );
}


void Batch::JobSpec::usePar( const IOPar& iop )
{
    pars_ = iop;
    pars_.removeWithKey( sKeyProgramName );
    pars_.removeWithKey( sKeyClArgs );
    execpars_.removeFromPar( pars_ );

    prognm_ = iop.find( sKeyProgramName );
    iop.get( sKeyClArgs, clargs_ );
    execpars_.usePar( iop );
}


void Batch::JobSpec::fillPar( IOPar& iop ) const
{
    iop = pars_;
    iop.set( sKeyProgramName, prognm_ );
    iop.set( sKeyClArgs, clargs_ );
    execpars_.fillPar( iop );
}


void Batch::JobSpec::setDefaultPythonArgs()
{
    OS::MachineCommand mc;
    const BufferString isoseachky( "--",
		    OS::MachineCommand::sKeyIsolationScript() );
    if ( !clargs_.isPresent(isoseachky.str()) )
    {
	const char* isolatefnm = OS::MachineCommand::getIsolationScriptFnm();
	if ( File::exists(isolatefnm) )
	    mc.addKeyedArg( OS::MachineCommand::sKeyIsolationScript(),
			    isolatefnm );
    }

    const BufferString pythseachky( "--",
			OD::PythonAccess::sKeyActivatePath() );
    if ( !clargs_.isPresent(pythseachky.str()) )
    {
	const char* activatefpath = OD::PythonAccess::getPythonActivatorPath();
	if ( File::exists(activatefpath) )
	    mc.addKeyedArg( OD::PythonAccess::sKeyActivatePath(),
			    activatefpath );
    }

    const BufferStringSet& mcargs = mc.args();
    if ( !mcargs.isEmpty() )
	clargs_.append( mcargs );
}


const char* Batch::JobSpec::progNameFor( ProcType pt )
{
    switch ( pt )
    {
#	define mHandlePTCase(typ,pnm) \
	case typ: return "od_process_" # pnm
	mHandlePTCase(Attrib,attrib);
	mHandlePTCase(AttribEM,attrib_em);
	mHandlePTCase(Grid2D,2dgrid);
	mHandlePTCase(PreStack,prestack);
	mHandlePTCase(SEGY,segyio);
	mHandlePTCase(T2D,time2depth);
	mHandlePTCase(TwoDto3D,2dto3d);
	mHandlePTCase(VelConv,velocityconv);
	mHandlePTCase(Vol,volume);
	case NonODBase:
	    return "";
	default:
	    pFreeFnErrMsg("switch case not added");
	    return 0;
    }
}


Batch::JobSpec::ProcType Batch::JobSpec::procTypeFor( const char* odpnm )
{
    if ( !odpnm || !*odpnm || !FixedString(odpnm).startsWith("od_process_") )
	return NonODBase;

    const FixedString pnm( odpnm + 11 );
#   define mRetForName(typ,str) \
    if ( pnm == #str ) return typ

    mRetForName(Attrib,attrib);
    mRetForName(AttribEM,attrib_em);
    mRetForName(Grid2D,2dgrid);
    mRetForName(PreStack,prestack);
    mRetForName(SEGY,segyio);
    mRetForName(T2D,time2depth);
    mRetForName(TwoDto3D,2dto3d);
    mRetForName(VelConv,velocityconv);
    mRetForName(Vol,volume);

    return NonODBase;
}


bool Batch::JobDispatcher::canHandle( const JobSpec& js ) const
{
    return isSuitedFor( js.prognm_ );
}

namespace Batch
{

static Threads::Atomic<Batch::ID> curjobid( Batch::JobDispatcher::getInvalid());

};

bool Batch::JobDispatcher::go( const Batch::JobSpec& js, Batch::ID* jobid )
{
    if ( !canHandle(js) )
    {
	errmsg_ = tr("Batch job is not suited for %1 execution")
		.arg( factoryDisplayName() );
	return getInvalid();
    }

    jobspec_ = js;
    if ( !init() )
	return getInvalid();

    ++curjobid;
    if ( jobid )
	*jobid = curjobid;

    return launch( jobid );
}


void Batch::JobDispatcher::getDefParFilename( const char* prognm,
						BufferString& fnm )
{
    if ( !prognm || !*prognm )
	prognm = "batchjob";
    File::Path parfp( GetProcFileName(prognm) );
    parfp.setExtension( ".par", false );
    const BufferString filename = parfp.fileName();
    if ( filename.startsWith("od_") )
    {
	BufferString newfnm = filename.buf() + 3;
	parfp.setFileName( newfnm );
    }
    fnm.set( parfp.fullPath() );
}


void Batch::JobDispatcher::getJobNames( BufferStringSet& nms )
{
    DirList dl( GetProcFileName(0), File::FilesInDir, "*.par" );
    for ( int idx=0; idx<dl.size(); idx++ )
	nms.add( getJobName(dl.get(idx)) );
}


BufferString Batch::JobDispatcher::getJobName( const char* inp )
{
    File::Path parfp( inp );
    parfp.setExtension( 0 );
    BufferString ret( parfp.fileName() );
    ret.replace( '_', ' ' );
    return ret;
}


void Batch::JobDispatcher::setJobName( const char* inp )
{
    BufferString nm( inp );
    nm.clean( BufferString::AllowDots );
    File::Path parfp( GetProcFileName(nm) );
    parfp.setExtension( ".par", false );
    parfnm_.set( parfp.fullPath() );
}


bool Batch::JobDispatcher::writeParFile() const
{
    IOPar wrpar( jobspec_.pars_ );
    jobspec_.fillPar( wrpar );
    wrpar.set( sKey::Survey(), DBM().surveyDirectoryName() );
    wrpar.set( sKey::DataRoot(), GetBaseDataDir() );

    od_ostream parstrm( parfnm_ ); ascostream astrm( parstrm );
    astrm.putHeader( "Parameters" );
    wrpar.putTo( astrm, true );
    const bool ret = parstrm.isOK();
    parstrm.close();
    if ( !ret )
    {
	errmsg_ = tr("Cannot write parameter file:\n %1").arg( parfnm_ );
	parstrm.addErrMsgTo( errmsg_ );
    }
    return ret;
}


static const char* sKeyResumeProcessing = "Resume Processing";

void Batch::JobDispatcher::setUserWantsResume( IOPar& iop, bool yn )
{
    iop.setYN( sKeyResumeProcessing, yn );
}


bool Batch::JobDispatcher::userWantsResume( const IOPar& iop )
{
    return iop.isTrue( sKeyResumeProcessing );
}


void Batch::JobDispatcher::addIDTo( Batch::ID batchid, OS::MachineCommand& mc )
{
    if ( batchid > getInvalid() )
	mc.addKeyedArg( OS::MachineCommand::sKeyJobID(), batchid );
}
