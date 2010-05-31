/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Apr 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelltieeventstretch.cc,v 1.19 2010-05-31 14:14:04 cvsbruno Exp $";

#include "uiwelltieeventstretch.h"
#include "arrayndimpl.h"

#include "welld2tmodel.h"
#include "welltiedata.h"
#include "welltied2tmodelmanager.h"
#include "welltiegeocalculator.h"
#include "welltieunitfactors.h"
#include "welltiedata.h"
#include "welltiepickset.h"
#include "welltiesetup.h"

namespace WellTie
{

EventStretch::EventStretch( WellTie::DataHolder& dh ) 
	: CallBacker(CallBacker::CallBacker())
	, timeChanged(this)
	, params_(*dh.params())
	, wtsetup_(dh.setup())
	, geocalc_(dh.geoCalc())
        , d2tmgr_(dh.d2TMGR())
  	, pmgr_(*dh.pickmgr())  
	, synthpickset_(*dh.pickmgr()->getSynthPickSet())
	, seispickset_(*dh.pickmgr()->getSeisPickSet())
	, d2t_(0)			    
{
} 


void EventStretch::doWork( CallBacker* )
{
    pmgr_.sortByPos( seispickset_ ); 	
    pmgr_.sortByPos( synthpickset_ );
    doStretchWork();	
    timeChanged.trigger();
}


void EventStretch::setD2TModel( const Well::D2TModel* dtm )
{
    d2t_ = dtm;
}


void EventStretch::doStretchWork()
{
    if ( !d2t_ ) return;
    const int d2tsz = d2t_->size();
    Array1DImpl<float> d2tarr( d2tsz );
    Array1DImpl<float>* prvd2tarr = new Array1DImpl<float>( d2tsz );

    float timeshift =  seispickset_.getLastPos() - synthpickset_.getLastPos();

    for ( int idx=0; idx<d2tsz; idx++ )
    {
	prvd2tarr->set( idx, d2t_->value(idx) );
	d2tarr.set( idx, d2t_->value(idx) + timeshift );
    }

    updatePicksPos( d2tarr, *prvd2tarr, synthpickset_, 0 );
    infborderpos_ = 0;
    supborderpos_ = seispickset_.getLastPos();

    for ( int idx=0; idx<seispickset_.getSize()-1; idx++ )
    {
	if ( idx && idx<seispickset_.getSize()-1 )
	    infborderpos_ = seispickset_.getPos(idx-1);

	startpos_ = synthpickset_.getPos(idx);
	stoppos_  = seispickset_.getPos(idx);

	delete prvd2tarr; prvd2tarr = 0;
	prvd2tarr = new Array1DImpl<float>( d2tarr );

	doStretchData( *prvd2tarr, d2tarr );
	//position of the following picks needs update if one of the pick moved
	updatePicksPos( d2tarr, *prvd2tarr, synthpickset_, idx );
    }
    delete prvd2tarr;
    d2tmgr_->replaceTime( d2tarr );
}


void EventStretch::updatePicksPos( const Array1DImpl<float>& curtime,
				    const Array1DImpl<float>& prevtime,
				    WellTie::PickSet& pickset, 
				    int startidx )
{
    for ( int pickidx=startidx; pickidx<pickset.getSize(); pickidx++ )
    {
	float curpos = pickset.getPos( pickidx );
	const int newidx = geocalc_->getIdx( prevtime, curpos );
	curpos = curtime.get( newidx ); 	
	pickset.setPos( pickidx, curpos );
    }
}


void EventStretch::doStretchData( const Array1DImpl<float>& prvt,
	                                Array1DImpl<float>& t )
{
    WellTie::GeoCalculator::StretchData sd;
    sd.start_ = geocalc_->getIdx( prvt, infborderpos_ );
    sd.pick1_ = geocalc_->getIdx( prvt, startpos_ );
    sd.pick2_ = geocalc_->getIdx( prvt, stoppos_ );
    sd.stop_  = geocalc_->getIdx( prvt, supborderpos_ );

    if ( sd.pick1_ < sd.start_ || sd.pick2_ > sd.stop_ )
	return;

    sd.inp_ = &prvt;    sd.outp_ = &t;

    geocalc_->stretch( sd );
}

}; //namespace WellTie
