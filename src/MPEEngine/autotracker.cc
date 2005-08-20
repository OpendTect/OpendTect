/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/

static const char* rcsID = "$Id: autotracker.cc,v 1.2 2005-08-20 19:16:24 cvskris Exp $";

#include "autotracker.h"

#include "emmanager.h"
#include "emobject.h"
#include "emtracker.h"
#include "mpeengine.h"
#include "parametricsurface.h"
#include "sectionextender.h"
#include "sectionadjuster.h"
#include "sectiontracker.h"
#include "survinfo.h"

namespace MPE 
{

AutoTracker::AutoTracker( EMTracker& et, const EM::SectionID& sectionid )
    : Executor("Autotracker")
    , emtracker( et )
    , emobject( *EM::EMM().getObject(et.objectID()) )
    , sid( sectionid )
    , sectiontracker( et.getSectionTracker(sectionid,true) )
    , nrdone( 0 )
    , totalnr( 0 )
{
    extender = sectiontracker->extender();
    adjuster = sectiontracker->adjuster();

    PtrMan<EM::EMObjectIterator> iterator = emobject.createIterator(sid);
    totalnr = iterator->maximumSize();

    while ( true )
    {
	const EM::PosID pid = iterator->next();
	if ( pid.objectID()==-1 )
	    break;

	if ( totalnr>0 ) totalnr--;

	if ( emobject.isAtEdge(pid) )
	    currentseeds += pid.subID();
    }

    totalnr = currentseeds.size();
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
	    const EM::PosID pid( emobject.id(), sid, addedpos[idx] );
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

    //Add positions that have failed to blacklist
    for ( int idx=0; idx<addedpos.size(); idx++ )
    {
	const EM::PosID pid( emobject.id(), sid, addedpos[idx] );
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
	    addedpossrc.remove(idx);
	}
    }

    //Some positions failed in the optimization, wich may lead to that
    //others are unsupported. Remove all unsupported nodes.
    mDynamicCastGet(const Geometry::ParametricSurface*, gesurf,
	    	    const_cast<const EM::EMObject&>(emobject).getElement(sid) );
    bool change = true;
    while ( gesurf && change )
    {
	change = false;
	for ( int idx=0; idx<addedpos.size(); idx++ )
	{
	    if ( !gesurf->hasSupport(RowCol(addedpos[idx])) )
	    {
		const EM::PosID pid( emobject.id(), sid, addedpos[idx] );
		emobject.unSetPos(pid,false);
		addedpos.remove(idx);
		addedpossrc.remove(idx);
		idx--;
		change = true;
	    }
	}
    }

    nrdone += currentseeds.size();

    //Make all new nodes seeds, apart from them outside the activearea
    currentseeds.erase();
    const CubeSampling activevolume = engine().activeVolume();
    for ( int idx=0; idx<addedpos.size(); idx++ )
    {
	const EM::PosID pid( emobject.id(), sid, addedpos[idx] );
	const Coord3 pos = emobject.getPos(pid);
	const BinID bid = SI().transform(pos);
	if ( activevolume.hrg.includes(bid) &&
	     activevolume.zrg.includes(pos.z))
	    currentseeds += addedpos[idx];
    }

    totalnr += currentseeds.size();
    return currentseeds.size() ? 1 : 0;
}

}; // namespace MPE
