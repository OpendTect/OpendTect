/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman Singh
 * DATE     : Jan 2016
-*/

static const char* rcsID mUsedVar = "$Id $";

#include "clusterjobdispatch.h"
#include "envvars.h"
#include "filepath.h"
#include "oddirs.h"
#include "sets.h"


static ObjectSet<Batch::ClusterProgDef> progdefs_;


void Batch::ClusterJobDispatcher::addDef( Batch::ClusterProgDef* pd )
{
    progdefs_ += pd;
}


Batch::ClusterJobDispatcher::ClusterJobDispatcher()
{
    jobspec_.execpars_.needmonitor( false );
}


uiString Batch::ClusterJobDispatcher::description() const
{
    return tr("The job will be split into parts that can each "
	      "run using an external cluster management tool like SLURM.");
}


bool Batch::ClusterJobDispatcher::isSuitedFor( const char* prognm ) const
{
    return defIdx( prognm ) >= 0;
}


int Batch::ClusterJobDispatcher::defIdx( const char* pnm ) const
{
    if ( !pnm )
	pnm = jobspec_.prognm_.buf();

    TypeSet<int> candidates;
    for ( int idx=0; idx<progdefs_.size(); idx++ )
    {
	const Batch::ClusterProgDef* def = progdefs_[idx];
	if ( def->isSuitedFor(pnm) )
	    candidates += idx;
    }

    if ( candidates.isEmpty() )
	return -1;
    else if ( candidates.size() > 1 )
    {
	for ( int idx=0; idx<candidates.size(); idx++ )
	{
	    const int pdidx = candidates[idx];
	    const Batch::ClusterProgDef* def = progdefs_[ pdidx ];
	    if ( def->canHandle(jobspec_) )
		return pdidx;
	}
    }

    return candidates[0];
}


bool Batch::ClusterJobDispatcher::canHandle( const Batch::JobSpec& js ) const
{
    for ( int idx=0; idx<progdefs_.size(); idx++ )
    {
	if ( progdefs_[idx]->canHandle(js) )
	    return true;
    }
    return false;
}


bool Batch::ClusterJobDispatcher::canResume( const Batch::JobSpec& js ) const
{
    for ( int idx=0; idx<progdefs_.size(); idx++ )
    {
	if ( progdefs_[idx]->canResume(js) )
	    return true;
    }
    return false;
}


bool Batch::ClusterJobDispatcher::init()
{
    if ( parfnm_.isEmpty() )
	setToDefParFileName();

    const int pdidx = defIdx();
    if ( pdidx < 0 || !progdefs_[pdidx]->canHandle(jobspec_) )
    {
	errmsg_ = tr( "The job cannot be handled using Cluster processing." );
	return false;
    }

    return true;
}


bool Batch::ClusterJobDispatcher::launch()
{
    const int pdidx = defIdx();
    if ( pdidx < 0 )
	{ errmsg_ = toUiString("Internal: init() not used"); return false; }

    if ( !writeParFile() )
	return false;

    const BufferString clusterprog =
		FilePath(GetExecPlfDir(),"od_clusterproc").fullPath();
    BufferString cmd( "\"", clusterprog, "\" " ); cmd.add( jobspec_.clargs_ );
    BufferString qtdparfnm( parfnm_ ); qtdparfnm.quote( '\"' );
    cmd.add( " " ).add( qtdparfnm );
    OS::MachineCommand mc( cmd );
    OS::CommandLauncher cl( mc );
    OS::CommandExecPars ep( jobspec_.execpars_ );
    ep.needmonitor( false ).launchtype( OS::RunInBG )
			   .isconsoleuiprog( GetEnvVarYN("DTECT_DEBUG") );
    return cl.execute( ep );
}
