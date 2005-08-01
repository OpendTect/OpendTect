/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/

static const char* rcsID = "$Id: emtracker.cc,v 1.12 2005-08-01 07:09:28 cvsnanne Exp $";

#include "emtracker.h"

#include "emhistory.h"
#include "emmanager.h"
#include "emobject.h"
#include "sectionextender.h"
#include "sectionselector.h"
#include "sectionadjuster.h"
#include "sectiontracker.h"
#include "consistencychecker.h"
#include "trackplane.h"
#include "iopar.h"

#include "ioman.h"
#include "ioobj.h"
#include "mpesetup.h"


namespace MPE 
{

EMTracker::EMTracker( EM::EMObject* emo )
    : isenabled (true)
    , emobject(emo)
{}


EMTracker::~EMTracker()
{
    deepErase( sectiontrackers );
}


BufferString EMTracker::objectName() const
{ return emobject ? emobject->name() : 0; }



EM::ObjectID EMTracker::objectID() const
{ return emobject ? emobject->id() : -1; }


bool EMTracker::trackSections( const TrackPlane& plane )
{
    if ( !emobject || !isenabled ) return true;

    const int initialhistnr = EM::EMM().history().currentEventNr();
    ConsistencyChecker* consistencychecker = getConsistencyChecker();
    if ( consistencychecker ) consistencychecker->reset();

    for ( int idx=0; idx<emobject->nrSections(); idx++ )
    {
	const EM::SectionID sectionid = emobject->sectionID(idx);
	SectionTracker* sectiontracker = getSectionTracker(sectionid,true);
	if ( !sectiontracker )
	    continue;

	sectiontracker->reset();
	sectiontracker->selector()->setTrackPlane( plane );

	if ( plane.getTrackMode() == TrackPlane::Erase )
	{
	    erasePositions( sectionid,
		    	    sectiontracker->selector()->selectedPositions() );
	    continue;
	}
	
	sectiontracker->extender()->setDirection( plane.motion() );
	sectiontracker->select();
	const bool hasextended = sectiontracker->extend();
	bool hasadjusted = true;
	if ( hasextended && sectiontracker->adjusterUsed() )
	    hasadjusted = sectiontracker->adjust();

	if ( !hasextended || !hasadjusted )
	{
	    while ( EM::EMM().history().canUnDo() &&
		    EM::EMM().history().currentEventNr()!=initialhistnr )
	    {
		bool res = EM::EMM().history().unDo(1);
		if ( !res ) break;
	    }

	    EM::EMM().history().setCurrentEventAsLast();

	    errmsg = sectiontracker->errMsg();
	    return false;;
	}

	EM::PosID posid( emobject->id(), sectionid );
	if ( consistencychecker )
	{
	    const TypeSet<EM::SubID>& addedpos = 
		sectiontracker->extender()->getAddedPositions();
	    for ( int posidx=0; posidx<addedpos.size(); posidx++ )
	    {
		posid.setSubID( addedpos[posidx] );
		consistencychecker->addNodeToCheck( posid );
	    }
	}
    }

    if ( consistencychecker )
	consistencychecker->nextStep();

    return true;
}


bool EMTracker::trackIntersections( const TrackPlane& )
{ return true; }


bool EMTracker::snapPositions( const TypeSet<EM::PosID>& pids ) 
{
    if ( !emobject ) return false;

    for ( int idx=0; idx<emobject->nrSections(); idx++ )
    {
	const EM::SectionID sid = emobject->sectionID(idx);

	TypeSet<EM::SubID> subids;
	for ( int idy=0; idy<pids.size(); idy++ )
	{
	    const EM::PosID& pid = pids[idy];
	    if ( pid.objectID()!= emobject->id() )
		continue;
	    if ( pid.sectionID()!=sid )
		continue;

	    subids += pid.subID();
	}

	SectionTracker* sectiontracker = getSectionTracker(sid,true);
	if ( !sectiontracker )
	    continue;

	SectionAdjuster* adjuster = sectiontracker->adjuster();
	if ( !adjuster ) continue;

	adjuster->reset();
	adjuster->setPositions( subids );

	while ( int res=adjuster->nextStep() )
	{
	    if ( res==-1 )
	    {
		errmsg = adjuster->errMsg();
		return false;
	    }
	}
    }

    return true;
}


const char* EMTracker::errMsg() const
{ return errmsg[0] ? (const char*) errmsg : 0; }


SectionTracker* EMTracker::getSectionTracker( EM::SectionID sid, bool create )
{
    for ( int idx=0; idx<sectiontrackers.size(); idx++ )
    {
	if ( sectiontrackers[idx]->sectionID()==sid )
	    return sectiontrackers[idx];
    }

    if ( !create ) return 0;

    SectionTracker* sectiontracker = createSectionTracker( sid );
    if ( !sectiontracker || !sectiontracker->init() )
    {
	delete sectiontracker;
	return 0;
    }

    if ( sectiontrackers.size() )
    {
	IOPar par;
	sectiontrackers[0]->fillPar( par );
	sectiontracker->usePar( par );
    }

    sectiontrackers += sectiontracker;
    return sectiontracker;
}


void EMTracker::erasePositions( EM::SectionID sectionid,
				const TypeSet<EM::SubID>& pos )
{
    EM::PosID posid( emobject->id(), sectionid );
    for ( int idx=0; idx<pos.size(); idx++ )
    {
	posid.setSubID( pos[idx] );
	emobject->unSetPos( posid, true );
    }
}


void EMTracker::fillPar( IOPar& iopar ) const
{
    for ( int idx=0; idx<sectiontrackers.size(); idx++ )
    {
	SectionTracker* st = sectiontrackers[idx];
	IOPar localpar;
	localpar.set( sectionidStr(), st->sectionID() );
	localpar.set( setupidStr(), st->setupID() );

	BufferString key( "Section" ); key += idx;
	iopar.mergeComp( localpar, key );
    }
}


bool EMTracker::usePar( const IOPar& iopar )
{
    int idx=0;
    while ( true )
    {
	BufferString key( "Section" ); key += idx;
	PtrMan<IOPar> localpar = iopar.subselect( key );
	if ( !localpar ) return true;

	int sid;
	if ( !localpar->get(sectionidStr(),sid) ) continue;
	SectionTracker* st = getSectionTracker( (EM::SectionID)sid, true );
	if ( !st ) continue;
	MultiID setupid;
	if ( localpar->get(setupidStr(),setupid) )
	    st->setSetupID( setupid );

	PtrMan<IOObj> ioobj = IOM().get( setupid );
	if ( !ioobj ) continue;
	MPE::Setup setup;
	BufferString bs;
	if ( !MPESetupTranslator::retrieve( setup, ioobj, bs ) ) continue;
	IOPar setuppar;
	setup.fillPar( setuppar );
	st->usePar( setuppar );
	idx++;
    }

    return true;
}


// TrackerFactory +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TrackerFactory::TrackerFactory( const char* emtype, EMTrackerCreationFunc func )
    : type( emtype )
    , createfunc( func )
{}


const char* TrackerFactory::emObjectType() const
{ return type; } 


EMTracker* TrackerFactory::create( EM::EMObject* emobj ) const
{ return createfunc( emobj ); }


}; // namespace MPE
