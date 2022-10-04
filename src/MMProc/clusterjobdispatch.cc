/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "clusterjobdispatch.h"
#include "envvars.h"
#include "filepath.h"
#include "oddirs.h"
#include "sets.h"


// Batch::ClusterProgDef

Batch::ClusterProgDef::ClusterProgDef()
{
}


Batch::ClusterProgDef::~ClusterProgDef()
{
}


static ObjectSet<Batch::ClusterProgDef> progdefs_;


// Batch::ClusterJobDispatcher

void Batch::ClusterJobDispatcher::addDef( Batch::ClusterProgDef* pd )
{
    progdefs_ += pd;
}


Batch::ClusterJobDispatcher::ClusterJobDispatcher()
{
    jobspec_.execpars_.needmonitor( false );
}


Batch::ClusterJobDispatcher::~ClusterJobDispatcher()
{
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


bool Batch::ClusterJobDispatcher::launch( ID* batchid )
{
    pErrMsg("Do not call: Launching is handled in UI dispatcher itself");
    return false;
}
