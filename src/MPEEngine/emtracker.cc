/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/

static const char* rcsID = "$Id: emtracker.cc,v 1.24 2005-11-03 23:27:26 cvskris Exp $";

#include "emtracker.h"

#include "attribsel.h"
#include "autotracker.h"
#include "consistencychecker.h"
#include "emhistory.h"
#include "emmanager.h"
#include "emobject.h"
#include "executor.h"
#include "mpeengine.h"
#include "sectionextender.h"
#include "sectionselector.h"
#include "sectionadjuster.h"
#include "sectiontracker.h"
#include "survinfo.h"
#include "trackplane.h"
#include "trackstattbl.h"
#include "iopar.h"

#include "ioman.h"
#include "ioobj.h"
#include "mpesetup.h"


namespace MPE 
{

EMTracker::EMTracker( EM::EMObject* emo )
    : isenabled (true)
    , emobject(0)
{
    setEMObject(emo);
}


EMTracker::~EMTracker()
{
    deepErase( sectiontrackers );
    setEMObject(0);
}


BufferString EMTracker::objectName() const
{ return emobject ? emobject->name() : 0; }



EM::ObjectID EMTracker::objectID() const
{ return emobject ? emobject->id() : -1; }


bool EMTracker::trackSections( const TrackPlane& plane )
{
    if ( !emobject || !isenabled || plane.getTrackMode()==TrackPlane::Move ||
	    plane.getTrackMode()==TrackPlane::None )
	return true;


    const int initialhistnr = EM::EMM().history().currentEventNr();
    ConsistencyChecker* consistencychecker = getConsistencyChecker();
    if ( consistencychecker ) consistencychecker->reset();

    bool success = true;
    for ( int idx=0; idx<emobject->nrSections(); idx++ )
    {
	const EM::SectionID sectionid = emobject->sectionID(idx);
	SectionTracker* sectiontracker = getSectionTracker(sectionid,true);
	if ( !sectiontracker )
	    continue;

	EM::PosID posid( emobject->id(), sectionid );
	if ( !sectiontracker->trackWithPlane(plane) )
	{
	    errmsg = sectiontracker->errMsg();
	    success = false;
	}
	else if ( consistencychecker )
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

    if ( !success )
	return false;

    if ( consistencychecker )
	consistencychecker->nextStep();

    return true;
}


bool EMTracker::trackIntersections( const TrackPlane& )
{ return true; }


Executor* EMTracker::trackInVolume()
{
    ExecutorGroup* res = 0;
    for ( int idx=0; idx<emobject->nrSections(); idx++ )
    {
	const EM::SectionID sid = emobject->sectionID(idx);
	if ( !res )
	{
	    res = new ExecutorGroup("Autotracker", true );
	    res->setNrDoneText("seeds processed");
	}

	res->add( new AutoTracker( *this, sid ) );
    }

    return res;
}


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

	const bool didremoveonfailure = adjuster->removeOnFailure(false);

	while ( int res=adjuster->nextStep() )
	{
	    if ( res==-1 )
	    {
		errmsg = adjuster->errMsg();
		adjuster->removeOnFailure(didremoveonfailure);
		return false;
	    }
	}

	adjuster->removeOnFailure(didremoveonfailure);
    }

    return true;
}


CubeSampling EMTracker::getAttribCube( const Attrib::SelSpec& spec ) const
{
    CubeSampling res( engine().activeVolume() );

    for ( int sectidx=0; sectidx<sectiontrackers.size(); sectidx++ )
    {
	CubeSampling cs = sectiontrackers[sectidx]->getAttribCube( spec );
	res.include( cs );
    }


    return res;
}


void EMTracker::getNeededAttribs( ObjectSet<const Attrib::SelSpec>& res )
{
    for ( int sectidx=0; sectidx<sectiontrackers.size(); sectidx++ )
    {
	ObjectSet<const Attrib::SelSpec> specs;
	sectiontrackers[sectidx]->getNeededAttribs( specs );

	for ( int idx=0; idx<specs.size(); idx++ )
	{
	    const Attrib::SelSpec* as = specs[idx];
	    if ( indexOf(res,*as) < 0 )
		res += as;
	}
    }
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


void EMTracker::setEMObject( EM::EMObject* no ) 
{
    if ( emobject ) emobject->unRef();
    emobject = no;
    if ( emobject ) emobject->ref();
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
