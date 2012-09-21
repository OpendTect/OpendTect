/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2007
-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "prestackeventsapi.h"

#include "binidvalset.h"
#include "cubesampling.h"
#include "executor.h"
#include "ioobj.h"
#include "ioman.h"
#include "offsetazimuth.h"
#include "survinfo.h"
#include "prestackeventsapimgr.h"
#include "prestackevents.h"
#include "prestackeventtransl.h"
#include "velocitypicks.h"



extern "C" int dGBPreStackEventsSetSurvey( const char* dataroot, const char* survey )
{
    return PreStack::EventsAPIMgr::getMgr().setSurvey( dataroot, survey );
}


extern "C" float dGBPreStackEventsGetInlDistance()
{
    return PreStack::EventsAPIMgr::getMgr().inlDistance();
}


extern "C" float dGBPreStackEventsGetCrlDistance()
{
    return PreStack::EventsAPIMgr::getMgr().crlDistance();
}


extern "C" int dGBPreStackEventsOpenReader( const char* reference )
{
    return PreStack::EventsAPIMgr::getMgr().openReader( reference );
}


extern "C" void dGBPreStackEventsCloseReader( int handle )
{
    PreStack::EventsAPIMgr::getMgr().closeReader( handle );
}


extern "C" int dGBPreStackEventsGetRanges( int handle,
	int& firstinl, int& lastinl, int& inlstep,
	int& firstcrl, int& lastcrl, int& crlstep )
{
    return PreStack::EventsAPIMgr::getMgr().getRanges( handle,
			    firstinl, lastinl, inlstep,
			    firstcrl, lastcrl, crlstep);
}


extern "C" int dGBPreStackEventsGetNextCDP(  int handle, int previnl, int prevcrl,
					int& nextinl, int& nextcrl )
{
    return PreStack::EventsAPIMgr::getMgr().getNextCDP( handle,
		    previnl, prevcrl, nextinl, nextcrl );
}


extern "C" int dGBPreStackEventsMoveReaderTo( int handle, int inl, int crl )
{
    return PreStack::EventsAPIMgr::getMgr().moveReaderTo( handle, inl, crl );

}


extern "C" int dGBPreStackEventsGetNrEvents( int handle )
{
    return PreStack::EventsAPIMgr::getMgr().getNrEvents( handle );
}


extern "C" int dGBPreStackEventsGetEventSize( int handle, int index )
{
    return PreStack::EventsAPIMgr::getMgr().getEventSize( handle, index );
}


extern "C" void dGBPreStackEventsGetEvent( int handle, int index, float* offsets,
				      float* azimuths, float* depths,
       				      float* quality )
{
    PreStack::EventsAPIMgr::getMgr().getEvent( handle, index, offsets, azimuths,
	    				     depths, quality );
}


extern "C" void dGBPreStackEventsGetDip( int handle, int index,
				    float& inldip, float& crldip )
{
    PreStack::EventsAPIMgr::getMgr().getDip( handle, index, inldip, crldip );
}


extern "C" int dGBPreStackEventsGetHorizonID( int handle, int index, int& horid )
{
    return PreStack::EventsAPIMgr::getMgr().getHorizonID( handle, index, horid );
}


extern "C" void dGBPreStackEventsGetEventWeight( int handle, int index,
					    float& weight )
{
    PreStack::EventsAPIMgr::getMgr().getQuality( handle, index, weight );
}


PreStack::EventsAPIMgr::EventsAPIMgr()
{
    velpicks_.allowNull( true );
    events_.allowNull( true );
    locations_.allowNull( true );
}


PreStack::EventsAPIMgr::~EventsAPIMgr()
{
    //Unref picks in events_
    deepUnRef( events_ );
    deepErase( locations_ );
}


int PreStack::EventsAPIMgr::setSurvey( const char* dataroot, const char* survey )
{
    if ( !IOM().setRootDir( dataroot ) )
	return -1;

    IOMan::setSurvey( survey );
    return 0;
}


float PreStack::EventsAPIMgr::inlDistance() const
{
    return SI().inlDistance();
}


float PreStack::EventsAPIMgr::crlDistance() const
{
    return SI().crlDistance();
}


int PreStack::EventsAPIMgr::openReader( const char* reference )
{
    const MultiID mid( reference );
    PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( !ioobj ) return -1;

    int res = 0;
    while ( ids_.indexOf(res)!=-1 ) res++;

    if ( !strcmp(ioobj->group(),PSEventTranslatorGroup::sKeyword() ) )
    {
	PreStack::EventManager* picks = new PreStack::EventManager;
	picks->ref();
	PtrMan<Executor> exec = picks->setStorageID( mid, false );
	if ( exec && !exec->execute() )
	    return -1;

	events_ += picks;
	velpicks_ += 0;
    }
    else
    {
	RefMan<Vel::Picks> vp = Vel::VPM().get( mid, false, true, true );
	if ( !vp || vp->isEmpty() )
	    return -1;

	events_ += 0;
	velpicks_ += vp;
	vp->ref();
    }

    ids_ += res;
    curpos_ += BinID(-1,-1);
    locations_ += 0;
    return res;
}


void PreStack::EventsAPIMgr::closeReader( int handle )
{
    const int idx = ids_.indexOf( handle );
    if ( idx<0 ) return;

    if ( events_[idx] )
    {
	events_[idx]->getEvents( curpos_[idx], false, false )->unRef();

	events_[idx]->unRef();
	events_.remove( idx );
    }
    else if ( velpicks_[idx] )
    {
	velpicks_[idx]->unRef();
	velpicks_.remove( idx );
    }

    ids_.remove( idx );
    curpos_.remove( idx );
    delete locations_.remove( idx );
}


int PreStack::EventsAPIMgr::getRanges( int handle,
	int& firstinl, int& lastinl, int& inlstep,
	int& firstcrl, int& lastcrl, int& crlstep ) const
{
    const int idx = ids_.indexOf( handle );

    HorSampling hrg;
    if ( events_[idx] )
    {
	if ( !events_[idx]->getHorRanges( hrg ) )
	    return -1;
    }
    else
    {
	RowCol arrpos( -1, -1 );

	BinID bid;
	const MultiDimStorage<Vel::Pick>& picks = velpicks_[idx]->getAll();
	if ( !picks.next( arrpos ) || !picks.getPos( arrpos, bid ) )
	    return -1;

	hrg.start = hrg.stop = bid;
	while ( picks.next( arrpos ) && !picks.getPos( arrpos, bid ) )
	{
	    hrg.include( bid );
	}
    }

    firstinl = hrg.start.inl;
    lastinl = hrg.stop.inl;
    inlstep = hrg.step.inl;
    firstcrl = hrg.start.crl;
    lastcrl = hrg.stop.crl;
    crlstep = hrg.step.crl;

    return 0;
}


int PreStack::EventsAPIMgr::getNextCDP(int handle, int previnl, int prevcrl,
	                             int& nextinl, int& nextcrl )
{
    const int idx = ids_.indexOf( handle );

    if ( !locations_[idx] )
    {
	locations_.replace( 0,  new BinIDValueSet( 0, false ) );
	if ( events_[idx] )
	{
	    if ( !events_[idx]->getLocations( *locations_[idx] ) )
		return -1;
	}
	else
	{
	    RowCol arrpos( -1, -1 );

	    BinID bid;
	    const MultiDimStorage<Vel::Pick>& picks =
		velpicks_[idx]->getAll();
	    if ( !picks.next( arrpos ) || !picks.getPos( arrpos, bid ) )
		return -1;

	    locations_[idx]->add( bid );

	    while ( picks.next( arrpos ) && picks.getPos( arrpos, bid ) )
		locations_[idx]->add( bid );
	}
    }

    BinIDValueSet::Pos pos = previnl!=-1 && prevcrl!=-1
	?  locations_[idx]->findFirst( BinID(previnl, prevcrl ) )
	: BinIDValueSet::Pos( -1, -1 );

    if ( !locations_[idx]->next( pos, true ) )
	return 0;

    const BinID newbid = locations_[idx]->getBinID( pos );
    nextinl = newbid.inl;
    nextcrl = newbid.crl; 
    return 1;
}


int PreStack::EventsAPIMgr::moveReaderTo( int handle, int inl, int crl )
{
    const int idx = ids_.indexOf( handle );
    if ( idx<0 ) return -1;

    const BinID prevbid( curpos_[idx] );
    const BinID newbid( inl, crl );

    if ( events_[idx] )
    {
	if ( events_[idx]->getEvents( prevbid, false, false ) )
	    events_[idx]->getEvents( prevbid, false, false )->unRef();
	if ( events_[idx]->getEvents( newbid, true, false ) )
	    events_[idx]->getEvents( newbid, false, false )->ref();
    }

    curpos_[idx] = newbid;

    if ( events_[idx] )
    {
	events_[idx]->cleanUp( false );
	return events_[idx]->getEvents( newbid, false, false ) ? 1 : 0;
    }

    return  velpicks_[idx]->get( newbid, 0, 0, 0, 0, false )>0 ? 1 : 0;
}


int PreStack::EventsAPIMgr::getNrEvents( int handle ) const
{
    const int idx = ids_.indexOf( handle );
    if ( idx<0 ) return 0;

    if ( events_[idx] )
    {
	const PreStack::EventSet* ge =  events_[idx]->getEvents( curpos_[idx] );
	return ge ? ge->events_.size() : 0; 
    }

    return velpicks_[idx]->get( curpos_[idx], 0, 0, 0, 0, false );
}


int PreStack::EventsAPIMgr::getEventSize( int handle, int index ) const
{
    const int idx = ids_.indexOf( handle );
    if ( idx<0 ) return 0;

    if ( events_[idx] )
    {
	const PreStack::EventSet* ge =  events_[idx]->getEvents( curpos_[idx] );
	return ge ? ge->events_[index]->sz_ : 0;
    }

    return 2;
}


void PreStack::EventsAPIMgr::getEvent( int handle, int index, float* offsets,
			float* azimuths, float* depths, float* quality) const
{
    const int idx = ids_.indexOf( handle );
    if ( idx<0 ) return;

    if ( events_[idx] )
    {
	const PreStack::EventSet* ge =  events_[idx]->getEvents( curpos_[idx] );
	const PreStack::Event* pick = ge->events_[index];

	for ( int idy=pick->sz_-1; idy>=0; idy-- )
	{
	    if ( offsets ) offsets[idy] = pick->offsetazimuth_[idy].offset();
	    if ( azimuths ) azimuths[idy] = pick->offsetazimuth_[idy].azimuth();
	    if ( depths ) depths[idy] = pick->pick_[idy];
	    if ( quality ) quality[idy] = pick->pickquality_
					    ? pick->pickquality_[idy] : 1.f;
	}
    }
    else
    {
	TypeSet<Vel::Pick> picks;
	velpicks_[idx]->get(curpos_[idx],picks,false,false);
	const bool fail = picks.size()<=index;

	if ( offsets )
	{
	    offsets[0] = 0;
	    offsets[1] = picks[index].offset_;
	}

	if ( azimuths )
	{
	    azimuths[0] = 0;
	    azimuths[1] = 0;
	}

	if ( depths )
	{
	    depths[0] = fail ? mUdf(float) : picks[index].depth_;
	    depths[1] =
		fail ? mUdf(float) : picks[index].depth_+picks[index].vel_;
	}

	if ( quality )
	{
	    quality[0] = 1;
	    quality[1] = 1;
	}
    }
}


void PreStack::EventsAPIMgr::getDip( int handle, int index,
				   float& inldip, float& crldip)
{
    inldip = crldip = 0;

    const int idx = ids_.indexOf( handle );
    if ( idx<0 ) return;

    if ( !events_[idx] )
	return;

    const PreStack::EventSet* ge =  events_[idx]->getEvents( curpos_[idx] );
    const PreStack::Event* pick = ge->events_[index];

    if ( !pick->sz_ )
	return;

    events_[idx]->getDip( BinIDValue(curpos_[idx],pick->pick_[0]),
	    		  pick->horid_, inldip, crldip );
}


void PreStack::EventsAPIMgr::getQuality( int handle, int index,
					float& res ) const
{
    const int idx = ids_.indexOf( handle );
    if ( idx<0 ) return;

    if ( !events_[idx] )
    {
	res = 1;
	return;
    }

    const PreStack::EventSet* ge =  events_[idx]->getEvents( curpos_[idx] );
    const PreStack::Event* pick = ge->events_[index];
    if ( pick ) res = pick->quality_;
}


int PreStack::EventsAPIMgr::getHorizonID( int handle, int index,
					int& horid ) const
{
    const int idx = ids_.indexOf( handle );
    if ( idx<0 ) return 0;

    if ( !events_[idx] )
    {
	return 0;
    }

    const PreStack::EventSet* ge =  events_[idx]->getEvents( curpos_[idx] );
    const PreStack::Event* pick = ge->events_[index];
    if ( !pick )
	return 0;

    horid = pick->horid_;

    return 1;
}


PreStack::EventsAPIMgr& PreStack::EventsAPIMgr::getMgr()
{
    static PreStack::EventsAPIMgr mgr;
    return mgr;
}
