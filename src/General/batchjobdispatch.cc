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
#include "ioman.h"


mImplFactory(Batch::JobDispatcher,Batch::JobDispatcher::factory)


Batch::JobSpec::JobSpec( Batch::JobSpec::ProcType pt )
    : execpars_(true)
{
    switch ( pt )
    {
#define mHandlePTCase(typ,pnm) \
	case typ: prognm_ = "od_process_" #pnm ; break
	mHandlePTCase(Attrib,attrib);
	mHandlePTCase(AttribEM,attrib_em);
	mHandlePTCase(Grid2D,2dgrid);
	mHandlePTCase(PreStack,prestack);
	mHandlePTCase(SEGY,segyio);
	mHandlePTCase(T2D,time2depth);
	mHandlePTCase(VelConv,velocityconv);
	mHandlePTCase(Vol,volume);
	default: break;
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


Batch::SingleJobDispatcher::SingleJobDispatcher()
{
}


const char* Batch::SingleJobDispatcher::description() const
{
    return "The job will be executed on one computer, in a single process.";
}


void Batch::SingleJobDispatcher::getDefParFilename( const char* prognm,
						    BufferString& fnm )
{
    if ( !prognm || !*prognm )
	prognm = "batchprog";
    FilePath parfp( GetProcFileName(prognm) );
    parfp.setExtension( ".par" );
    fnm.set( parfp.fullPath() );
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
    od_ostream parstrm( parfnm_ );
    ascostream astrm( parstrm );
    astrm.putHeader( "Parameters" );
    jobspec_.pars_.putTo( astrm );
    parstrm.close();

    BufferString cmd( "\"", jobspec_.prognm_, "\" " );
    cmd.add( jobspec_.clargs_ ).add( "\"" ).add( parfnm_ ).add( "\"" );
    OS::MachineCommand mc( cmd, remotehost_ );
    if ( !remoteexec_.isEmpty() )
	mc.setRemExec( remoteexec_ );
    OS::CommandLauncher cl( mc );
    return cl.execute( jobspec_.execpars_ );
}
