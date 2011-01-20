/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Apr 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelltieeventstretch.cc,v 1.21 2011-01-20 10:21:39 cvsbruno Exp $";

#include "uiwelltieeventstretch.h"

#include "arrayndimpl.h"
#include "welld2tmodel.h"
#include "welltiedata.h"
#include "welltiegeocalculator.h"
#include "welltiepickset.h"

namespace WellTie
{

EventStretch::EventStretch( PickSetMgr& pmgr ) 
	: timeChanged(this)
  	, pmgr_(pmgr) 
	, synthpickset_(pmgr_.synthPickSet())
	, seispickset_(pmgr_.seisPickSet())
	, d2t_(0)
	, timearr_(0)		 
{} 


EventStretch::~EventStretch()
{ delete timearr_; }


void EventStretch::doWork( CallBacker* )
{
    pmgr_.sortByPos(); 	
    doStretchWork();	
    timeChanged.trigger();
}


void EventStretch::doStretchWork()
{
    if ( !d2t_ ) return;
    const int d2tsz = d2t_->size();
    delete timearr_;
    timearr_ = new Array1DImpl<float>( d2tsz );
    Array1DImpl<float> prvtimearr( d2tsz );

    float timeshift = seispickset_.last().zpos_ - synthpickset_.last().zpos_;

    for ( int idx=0; idx<d2tsz; idx++ )
    {
	prvtimearr.set( idx, d2t_->value(idx) );
	timearr_->set( idx, d2t_->value(idx) + timeshift );
    }

    updatePicksPos( *timearr_, prvtimearr, true, 0 );
    infborderpos_ = 0;
    supborderpos_ = seispickset_.last().zpos_;
    const int seisz = seispickset_.size();

    for ( int idx=0; idx<seisz-1; idx++ )
    {
	if ( idx )
	    infborderpos_ = seispickset_[idx-1].zpos_;

	startpos_ = synthpickset_[idx].zpos_;
	stoppos_  = seispickset_[idx].zpos_;

	prvtimearr = Array1DImpl<float>( *timearr_ );
	doStretchData( prvtimearr, *timearr_ );
	//position of the following picks needs update if one of the pick moved
	updatePicksPos( *timearr_, prvtimearr, true, idx );
    }
}


void EventStretch::updatePicksPos( const Array1DImpl<float>& curtime,
				    const Array1DImpl<float>& prevtime,
				    bool issynth, int startidx )
{
    const TypeSet<Marker>& pickset = issynth ? synthpickset_ : seispickset_;
    for ( int pickidx=startidx; pickidx<pickset.size(); pickidx++ )
    {
	float curpos = pickset[pickidx].zpos_;
	const int newidx = geocalc_.getIdx( prevtime, curpos );
	curpos = curtime.get( newidx );
	pmgr_.setPickSetPos( issynth, pickidx, curpos );
    }
}


void EventStretch::doStretchData( const Array1DImpl<float>& prvt,
	                                Array1DImpl<float>& t )
{
    WellTie::GeoCalculator::StretchData sd;
    sd.start_ = geocalc_.getIdx( prvt, infborderpos_ );
    sd.pick1_ = geocalc_.getIdx( prvt, startpos_ );
    sd.pick2_ = geocalc_.getIdx( prvt, stoppos_ );
    sd.stop_  = geocalc_.getIdx( prvt, supborderpos_ );

    if ( sd.pick1_ < sd.start_ || sd.pick2_ > sd.stop_ )
	return;

    sd.inp_ = &prvt;    sd.outp_ = &t;
    geocalc_.stretch( sd );
}

}; //namespace WellTie
