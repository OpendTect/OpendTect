/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu
 * DATE     : April 2007
-*/

static const char* rcsID mUnusedVar = "$Id: od_track_horizons.cc,v 1.1 2012-09-06 05:28:50 cvskris Exp $";

#include "batchprog.h"

#include "horizontracker.h"
#include "emmanager.h"
#include "threadwork.h"
#include "keystrs.h"
#include "moddepmgr.h"
#include "iopar.h"
#include "bufstring.h"
#include "convert.h"
#include "emhorizon.h"


#include "prog.h"


#define mError( msg, action ) \
{ \
    strm << msg << sKey::NewLine(); \
    action; \
}

bool BatchProgram::go( std::ostream& strm )
{
    OD::ModDeps().ensureLoaded("Seis");
    
    int nrhorizons;
    if ( !pars().get( MPE::HorizonAutoTracker::sNrHorizons(), nrhorizons ) )
	mError( "Cannot read nr of horizons", return false );
    
    ManagedObjectSet<MPE::HorizonAutoTracker> trackers( false );
    const int queueid =
	Threads::WorkManager::twm().addQueue(Threads::WorkManager::MultiThread);
    
    for ( int idx=0; idx<nrhorizons; idx++ )
    {
	PtrMan<IOPar> trackerpar = pars().subselect(Conv::to<const char*>(idx));
	if ( !trackerpar )
	    mError( BufferString( "Cannot find settings for tracker ", idx ),
		    continue );
	
	MultiID hormid;
	if ( !trackerpar->get( MPE::HorizonAutoTracker::sKeyHorizonID(),
			       hormid ) )
	    mError( BufferString( "Cannot find horizonid for tracker ", idx ),
		    continue );
	
	RefMan<EM::EMObject> emobj = EM::EMM().loadIfNotFullyLoaded( hormid );
	if ( !emobj )
	    mError( BufferString( "Cannot load ", hormid ), continue );

	mDynamicCastGet( EM::Horizon*, hor, emobj );
	if ( !hor )
	    mError( BufferString( hormid, " is not a horizon" ), continue );
	
	MPE::HorizonAutoTracker* tracker = new MPE::HorizonAutoTracker( *hor );
	if ( !tracker )
	    mError( BufferString( "Cannot allocate tracker for", hor->name() ),
		   continue );
	
	if ( tracker->usePar( *trackerpar ) )
	{
	    BufferString msg( "Cannot parse settings for ", hor->name(), ": " );
	    msg.add( tracker->errMsg() );
	    mError( msg.str(), delete tracker; continue );
	}
	
	if ( !tracker->init() )
	{
	    BufferString msg( "Cannot init tracker for ", hor->name(), ": " );
	    msg.add( tracker->errMsg() );
	    mError( msg.str(), delete tracker; continue );
	}
	
	trackers += tracker;
	
	Threads::WorkManager::twm().addWork( Threads::Work(*tracker, false),
					    0, queueid, false );
    }
    
    if ( !trackers.size() )
	mError("No valid trackers found.", return true; );
    
    Threads::WorkManager::twm().removeQueue(queueid, true );
    
    for ( int idx=0; idx<trackers.size(); idx++ )
    {
	if ( trackers[idx]->errMsg() )
	{
	    BufferString msg( "Error while tracking ",
			     trackers[idx]->horizon().name(), ": " );
	    msg.add( trackers[idx]->errMsg() );
	    mError( msg.str(), continue );
	}
    }
    
    return false;
}
