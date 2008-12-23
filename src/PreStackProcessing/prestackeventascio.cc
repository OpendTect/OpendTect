/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		November 2008
 RCS:		$Id: prestackeventascio.cc,v 1.2 2008-12-23 11:14:07 cvsdgb Exp $
________________________________________________________________________

-*/

#include "prestackeventascio.h"

namespace PreStack
{

EventExporter::EventExporter( std::ostream& strm, EventManager& events )
    : strm_( strm )
    , events_( events )
    , hrg_( true )
    , nrdone_( 0 )
    , fileidx_( 0 )
    , locations_( 0, false )
{
    events_.ref();
    events_.getLocations( locations_ );
}


EventExporter::~EventExporter()
{ events_.unRef(); }


void EventExporter::setHRange( const HorSampling& hrg )
{
    hrg_ = hrg;
}


const char* EventExporter::nrDoneText() const
{ return "CDP's processed"; }

#define mWrite( var ) strm_ << (var) << '\t';


int EventExporter::nextStep()
{
    if ( !locations_.next( pos_ ) )
	return Finished();

    nrdone_++;

    const BinID bid = locations_.getBinID(pos_);
    if ( !hrg_.includes( bid ) )
	return MoreToDo();

    RefMan<const EventSet> eventset = events_.getEvents( bid, true, false );
    if ( !eventset )
	return MoreToDo();

    for ( int eventidx=0; eventidx<eventset->events_.size(); eventidx++ )
    {
	const Event& event = *eventset->events_[eventidx];
	if ( event.sz_==0 )
	    continue;

	float inldip = 0, crldip = 0;
	events_.getDip( BinIDValue( bid, event.pick_[0]), event.horid_,
			inldip, crldip );

	for ( int pickidx=0; pickidx<event.sz_; pickidx++ )
	{
	    mWrite( bid.inl );
	    mWrite( bid.crl );
	    mWrite( fileidx_ );
	    mWrite( inldip );
	    mWrite( crldip );
	    mWrite( (int) event.quality_ );
	    mWrite( event.offsetazimuth_[pickidx].azimuth() );
	    mWrite( event.offsetazimuth_[pickidx].offset() );
	    mWrite( event.pick_[pickidx] );
	    mWrite( event.pickquality_ ? event.pickquality_[pickidx] : 255 );
	    strm_ << '\n';
	}

	fileidx_++;
    }

    return MoreToDo();
}



} // namespace PreStack
