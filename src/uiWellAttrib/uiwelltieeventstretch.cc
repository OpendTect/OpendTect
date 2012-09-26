/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Apr 2009
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiwelltieeventstretch.h"

#include "arrayndimpl.h"
#include "interpol1d.h"
#include "idxable.h"
#include "survinfo.h"
#include "welld2tmodel.h"
#include "welltiedata.h"
#include "welltied2tmodelmanager.h"
#include "welltiepickset.h"

namespace WellTie
{

EventStretch::EventStretch( PickSetMgr& pmgr, D2TModelMgr& d2tmgr ) 
  	: pmgr_(pmgr) 
	, synthpickset_(pmgr_.synthPickSet())
	, seispickset_(pmgr_.seisPickSet())
	, d2tmgr_(d2tmgr)					   
	, d2t_(0)
{} 


void EventStretch::doWork( CallBacker* )
{
    pmgr_.sortByPos(); 	
    doStretchWork();	
}


void EventStretch::doStretchWork()
{
    if ( !d2t_ && d2t_->size() < 2 ) 
	return;

    if ( synthpickset_.size() == 1 && seispickset_.size() == 1 )
	doStaticShift();
    else
	doStretchSqueeze();
}


void EventStretch::doStaticShift()
{
    const float shift = seispickset_.last().zpos_ - synthpickset_.last().zpos_;
    d2tmgr_.shiftModel( shift );
}


#define mGapSize SI().zStep()
void EventStretch::doStretchSqueeze()
{
    int d2tsz = d2t_->size();
    //we need to interpolate the model for efficient stretch/squeeze
    TypeSet<float> d2tarr, daharr;
    for ( int idx=0; idx<d2tsz-1; idx++ )
    {
	const float timeval1 = d2t_->value(idx);
	const float timeval2 = d2t_->value(idx+1);
	d2tarr += timeval1;
	daharr += d2t_->dah( idx );
	if ( fabs( timeval2 - timeval1 ) > mGapSize )
	{
	    float time = timeval1;
	    while ( time < timeval2 )
	    {
		time += mGapSize;
		d2tarr += time;
		daharr += d2t_->getDah( time );
	    }
	}
    }

    d2tarr += d2t_->value( d2tsz-1 );
    daharr += d2t_->dah( d2tsz-1 );

    const float lasttime = d2tmgr_.getData().timeintv_.stop;
    float lastd2ttime = d2t_->value( d2tsz-1 );
    while ( lastd2ttime < lasttime  )
    {
	lastd2ttime += mGapSize;
	d2tarr += lastd2ttime;
	daharr += d2t_->getDah( lastd2ttime );
    }

    d2tsz = d2tarr.size();

    Array1DImpl<float> calibratedarr( d2tsz );
    TypeSet<int> ctrlidxs; TypeSet<float> ctrlvals;
    for ( int idx=0; idx<seispickset_.size(); idx++ )
    {
	const float pos = synthpickset_[idx].zpos_;
	int idx1 = -1;
	IdxAble::findFPPos( d2tarr.arr(), d2tsz, pos, -1, idx1 );
	ctrlidxs += idx1;
	ctrlvals += seispickset_[idx].zpos_;
    }
    IdxAble::callibrateArray( d2tarr.arr(), d2tsz,
				ctrlvals.arr(), ctrlidxs.arr(),
				ctrlvals.size(), false, calibratedarr.arr() );

    d2tmgr_.setFromData( daharr.arr(), calibratedarr.arr(), d2tsz );
}

}; //namespace WellTie
