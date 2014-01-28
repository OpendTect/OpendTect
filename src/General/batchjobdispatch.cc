/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Dec 2013
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "batchjobdispatch.h"
#include "keystrs.h"
#include "filepath.h"
#include "strmprov.h"
#include "oscommand.h"
#include "oddirs.h"
#include "ascstream.h"
#include "dirlist.h"
#include "ioman.h"


mImplFactory(Batch::JobDispatcher,Batch::JobDispatcher::factory)


Batch::JobSpec::JobSpec( Batch::JobSpec::ProcType pt )
    : execpars_(true)
    , prognm_(progNameFor(pt))
{
    execpars_.needmonitor_ = true;
}


const char* Batch::JobSpec::progNameFor( ProcType pt )
{
    switch ( pt )
    {
#define mHandlePTCase(typ,pnm) \
	case typ: return "od_process_" # pnm
	mHandlePTCase(Attrib,attrib);
	mHandlePTCase(AttribEM,attrib_em);
	mHandlePTCase(Grid2D,2dgrid);
	mHandlePTCase(PreStack,prestack);
	mHandlePTCase(SEGY,segyio);
	mHandlePTCase(T2D,time2depth);
	mHandlePTCase(VelConv,velocityconv);
	mHandlePTCase(Vol,volume);
	default: pFreeFnErrMsg("progNameFor","Switch case not added"); return 0;
    }
}


bool Batch::JobDispatcher::canHandle( const JobSpec& js ) const
{
    return isSuitedFor( js.prognm_ );
}


bool Batch::JobDispatcher::go( const Batch::JobSpec& js )
{
    if ( !canHandle(js) )
    {
	errmsg_.set( "Batch job is not suited for " )
		.add( factoryDisplayName() ).add( " execution" );
	return false;
    }

    jobspec_ = js;
    if ( !init() )
	return false;

    jobspec_.pars_.set( sKey::Survey(), IOM().surveyName() );
    jobspec_.pars_.set( sKey::DataRoot(), GetBaseDataDir() );

    return launch();
}


void Batch::JobDispatcher::getDefParFilename( const char* prognm,
						BufferString& fnm )
{
    if ( !prognm || !*prognm )
	prognm = "batchjob";
    FilePath parfp( GetProcFileName(prognm) );
    parfp.setExtension( ".par" );
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
    DirList dl( GetProcFileName(0), DirList::FilesOnly, "*.par" );
    for ( int idx=0; idx<dl.size(); idx++ )
	nms.add( getJobName(dl.get(idx)) );
}


BufferString Batch::JobDispatcher::getJobName( const char* inp )
{
    FilePath parfp( inp );
    parfp.setExtension( 0 );
    BufferString ret( parfp.fileName() );
    ret.replace( '_', ' ' );
    return ret;
}


void Batch::JobDispatcher::setJobName( const char* inp )
{
    BufferString nm( inp );
    nm.clean( BufferString::AllowDots );
    FilePath parfp( GetProcFileName(nm) );
    parfp.setExtension( ".par" );
    parfnm_.set( parfp.fullPath() );
}


bool Batch::JobDispatcher::writeParFile() const
{
    od_ostream parstrm( parfnm_ );
    ascostream astrm( parstrm );
    astrm.putHeader( "Parameters" );
    jobspec_.pars_.putTo( astrm );
    const bool ret = parstrm.isOK();
    parstrm.close();
    if ( !ret )
    {
	errmsg_.set( "Cannot write parameter file:\n" ).add( parfnm_ );
	parstrm.addErrMsgTo( errmsg_ );
    }
    return ret;
}


Batch::SingleJobDispatcher::SingleJobDispatcher()
{
}


const char* Batch::SingleJobDispatcher::description() const
{
    return "The job will be executed on one computer, in a single process.";
}


bool Batch::SingleJobDispatcher::init()
{
    if ( parfnm_.isEmpty() )
	getDefParFilename( jobspec_.prognm_, parfnm_ );

    FilePath fp( parfnm_ );
    fp.setExtension( 0 );
    BufferString logfnm( fp.fullPath() );
    logfnm.add( "_log.txt" );
    jobspec_.pars_.update( sKey::LogFile(), logfnm );

    return true;
}


bool Batch::SingleJobDispatcher::launch()
{
    if ( !writeParFile() )
	return false;

    BufferString cmd( jobspec_.prognm_, " ", jobspec_.clargs_ );
    BufferString qtdparfnm( parfnm_ ); qtdparfnm.quote( '\"' );
    cmd.add( qtdparfnm );
    OS::MachineCommand mc( cmd, remotehost_ );
    if ( !remoteexec_.isEmpty() )
	mc.setRemExec( remoteexec_ );
    OS::CommandLauncher cl( mc );
    BufferString logfile;
    jobspec_.pars_.get( sKey::LogFile(), logfile );
    jobspec_.execpars_.monitorfnm( logfile );
    return cl.execute( jobspec_.execpars_ );
}
