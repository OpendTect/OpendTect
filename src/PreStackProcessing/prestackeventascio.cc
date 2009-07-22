/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		November 2008
 RCS:		$Id: prestackeventascio.cc,v 1.4 2009-07-22 16:01:34 cvsbert Exp $
________________________________________________________________________

-*/

#include "prestackeventascio.h"
#include "executor.h"

namespace PreStack
{

EventExporter::EventExporter( std::ostream& strm, EventManager& events )
    : strm_( strm )
    , events_( events )
    , hrg_( true )
    , nrdone_( 0 )
    , fileidx_( 0 )
    , locations_( 0, false )
    , message_( "" )
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
{ return "CDP's exported"; }

#define mWrite( var ) strm_ << (var) << '\t';
#define mBatchSize    10000



int EventExporter::nextStep()
{
    bool isatend = false;
    message_ = "Reading";

    BinIDValueSet currentbatch( 0, false );
    for ( int idx=0; idx<mBatchSize; idx++ )
    {
	if ( !locations_.next( pos_ ) )
	{
	    isatend = true;
	    break;
	}

	const BinID bid = locations_.getBinID(pos_);
	if ( !hrg_.includes( bid ) )
	    continue;

	currentbatch.add( bid );
    }


    PtrMan<Executor> exec = events_.load( currentbatch, false );
    if ( !exec || !exec->execute() )
	return ErrorOccurred();

    message_ = "Writing";
    BinIDValueSet::Pos pos;

    while ( currentbatch.next( pos ) )
    {
	const BinID bid = currentbatch.getBinID( pos );
	RefMan<const EventSet> eventset = events_.getEvents( bid, false,false );
	if ( !eventset )
	    return ErrorOccurred();

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

	nrdone_++;
    }

    return isatend ? Finished() : MoreToDo();
}



} // namespace PreStack
