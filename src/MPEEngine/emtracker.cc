/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/

static const char* rcsID = "$Id: emtracker.cc,v 1.1 2005-01-06 09:25:21 kristofer Exp $";

#include "emtracker.h"

#include "sectiontracker.h"

namespace MPE 
{

EMTracker::EMTracker()
    : isenabled ( true )
{}


EMTracker::~EMTracker()
{
    deepErase( sectiontrackers );
}


bool EMTracker::trackSections( const TrackPlane& ) { return true; }


bool EMTracker::trackIntersections( const TrackPlane& ) { return true; }


const char* EMTracker::errMsg() const
{ return errmsg[0] ? (const char*) errmsg : 0; }



TrackerFactory::TrackerFactory( const char* emtype, EMTrackerCreationFunc func )
    : type( emtype )
    , createfunc( func )
{}


const char* TrackerFactory::emObjectType() const { return type; } 


EMTracker* TrackerFactory::create( EM::EMObject* emobj ) const
{ return createfunc( emobj ); }


};  //namespace
