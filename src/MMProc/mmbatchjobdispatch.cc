/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Dec 2013
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "mmbatchjobdispatch.h"
#include "sets.h"


static ObjectSet<Batch::MMProgDef> progdefs_;


void Batch::MMJobDispatcher::addDef( Batch::MMProgDef* pd )
{
    progdefs_ += pd;
}


Batch::MMJobDispatcher::MMJobDispatcher()
{
    jobspec_.execpars_.needmonitor( false );
}


uiString Batch::MMJobDispatcher::description() const
{
    return "The job will be split into parts that can each "
	   "run on a separate computer.";
}


bool Batch::MMJobDispatcher::isSuitedFor( const char* prognm ) const
{
    return defIdx( prognm ) >= 0;
}


int Batch::MMJobDispatcher::defIdx( const char* pnm ) const
{
    if ( !pnm )
	pnm = jobspec_.prognm_.buf();

    TypeSet<int> candidates;
    for ( int idx=0; idx<progdefs_.size(); idx++ )
    {
	const Batch::MMProgDef* def = progdefs_[idx];
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
	    const Batch::MMProgDef* def = progdefs_[ pdidx ];
	    if ( def->canHandle(jobspec_) )
		return pdidx;
	}
    }

    return candidates[0];
}


bool Batch::MMJobDispatcher::canHandle( const Batch::JobSpec& js ) const
{
    for ( int idx=0; idx<progdefs_.size(); idx++ )
    {
	if ( progdefs_[idx]->canHandle(js) )
	    return true;
    }
    return false;
}


bool Batch::MMJobDispatcher::canResume( const Batch::JobSpec& js ) const
{
    for ( int idx=0; idx<progdefs_.size(); idx++ )
    {
	if ( progdefs_[idx]->canResume(js) )
	    return true;
    }
    return false;
}


bool Batch::MMJobDispatcher::init()
{
    if ( parfnm_.isEmpty() )
	setToDefParFileName();

    const int pdidx = defIdx();
    if ( pdidx < 0 || !progdefs_[pdidx]->canHandle(jobspec_) )
    {
	errmsg_.set( "The job cannot be handled Multi-Machine."
			"\nTry single-machine execution." );
	return false;
    }

    return true;
}


bool Batch::MMJobDispatcher::launch()
{
    const int pdidx = defIdx();
    if ( pdidx < 0 )
	{ errmsg_.set("Internal: init() not used"); return false; }

    if ( !writeParFile() )
	return false;

    BufferString cmd( progdefs_[pdidx]->mmprognm_, " ", jobspec_.clargs_ );
    BufferString qtdparfnm( parfnm_ ); qtdparfnm.quote( '\'' );
    cmd.add( " " ).add( qtdparfnm );
    OS::MachineCommand mc( cmd );
    OS::CommandLauncher cl( mc );
    OS::CommandExecPars ep( jobspec_.execpars_ );
    ep.needmonitor( false ).launchtype( OS::RunInBG ).isconsoleuiprog( false );
    return cl.execute( ep );
}
