/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/

static const char* rcsID = "$Id: autotracker.cc,v 1.10 2006-08-22 12:51:22 cvsjaap Exp $";

#include "autotracker.h"

#include "emmanager.h"
#include "emobject.h"
#include "emtracker.h"
#include "mpeengine.h"
#include "sectionextender.h"
#include "sectionadjuster.h"
#include "sectiontracker.h"
#include "survinfo.h"

namespace MPE 
{

AutoTracker::AutoTracker( EMTracker& et, const EM::SectionID& sid )
    : Executor("Autotracker")
    , emtracker( et )
    , emobject( *EM::EMM().getObject(et.objectID()) )
    , sectionid( sid )
    , sectiontracker( et.getSectionTracker(sid,true) )
    , nrdone( 0 )
    , totalnr( 0 )
{
    extender = sectiontracker->extender();
    adjuster = sectiontracker->adjuster();

    PtrMan<EM::EMObjectIterator> iterator = emobject.createIterator(sectionid);
    totalnr = engine().activeVolume().hrg.totalNr();

    while ( true )
    {
	const EM::PosID pid = iterator->next();
	if ( pid.objectID()==-1 )
	    break;
	
	BinID bid = SI().transform( emobject.getPos(pid) );
	if ( engine().activeVolume().hrg.includes(bid) )
	{
	    totalnr--;
	    addSeed(pid);
	}
    }

    if ( !currentseeds.size() )
	totalnr = 0;
}


void AutoTracker::setNewSeeds( const TypeSet<EM::PosID>& seeds )
{
    currentseeds.erase();

    for( int idx=0; idx<seeds.size(); ++idx )
	addSeed(seeds[idx]);
}


int AutoTracker::nextStep()
{
    extender->reset();
    extender->setDirection( BinIDValue(BinID(0,0), mUdf(float)) );
    extender->setStartPositions(currentseeds);
    int res;
    while ( (res=extender->nextStep())>0 )
	;

    TypeSet<EM::SubID> addedpos = extender->getAddedPositions();
    TypeSet<EM::SubID> addedpossrc = extender->getAddedPositionsSource();

    //Remove nodes that have failed 8 times before
    for ( int idx=0; idx<addedpos.size(); idx++ )
    {
	const int blacklistidx = blacklist.indexOf(addedpos[idx]);
	if ( blacklistidx<0 ) continue;
	if ( blacklistscore[blacklistidx]>7 )
	{
	    const EM::PosID pid( emobject.id(), sectionid, addedpos[idx] );
	    emobject.unSetPos(pid,false);
	    addedpos.remove(idx);
	    addedpossrc.remove(idx);
	    idx--;
	}
    }

    adjuster->reset();    
    adjuster->setPositions(addedpos, &addedpossrc);
    while ( (res=adjuster->nextStep())>0 )
	;

    //Not needed anymore, so we avoid hazzles below if we simply empty it
    addedpossrc.erase();

    //Add positions that have failed to blacklist
    for ( int idx=0; idx<addedpos.size(); idx++ )
    {
	const EM::PosID pid( emobject.id(), sectionid, addedpos[idx] );
	if ( !emobject.isDefined(pid) )
	{
	    const int blacklistidx = blacklist.indexOf(addedpos[idx]);
	    if ( blacklistidx!=-1 ) blacklistscore[blacklistidx]++;
	    else
	    {
		blacklist += addedpos[idx];
		blacklistscore += 1;
	    }

	    addedpos.remove(idx);
	    idx--;
	}
    }

    //Some positions failed in the optimization, wich may lead to that
    //others are unsupported. Remove all unsupported nodes.
    sectiontracker->removeUnSupported( addedpos );

    // Make all new nodes seeds
    currentseeds = addedpos;
    nrdone += currentseeds.size();

    return currentseeds.size() ? MoreToDo : Finished;
}


void AutoTracker::setTrackBoundary( const CubeSampling& cs )
{ extender->setExtBoundary( cs ); }


void AutoTracker::unsetTrackBoundary()
{ extender->unsetExtBoundary(); }


bool AutoTracker::addSeed( const EM::PosID& pid )
{
    if ( pid.sectionID()!=sectionid )	return false;
    if ( !emobject.isAtEdge(pid) )	return false;

    const Coord3& pos = emobject.getPos(pid);
    if ( !engine().activeVolume().zrg.includes(pos.z) )	return false;
    const BinID bid = SI().transform(pos);
    if ( !engine().activeVolume().hrg.includes(bid) )	return false;

    currentseeds += pid.subID();
    return true;
}


}; // namespace MPE
