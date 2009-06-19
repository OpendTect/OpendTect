/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bruno
 Date:		Apr 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelltieeventstretch.cc,v 1.8 2009-06-19 08:30:13 cvsbruno Exp $";

#include "arrayndimpl.h"
#include "uiwelltieeventstretch.h"

#include "welltiedata.h"
#include "welltied2tmodelmanager.h"
#include "welltiegeocalculator.h"
#include "welltieunitfactors.h"
#include "welltiedata.h"
#include "welltiepickset.h"
#include "welltiesetup.h"
#include "uiwelltieview.h"

uiWellTieEventStretch::uiWellTieEventStretch( uiParent* p, 
					      WellTieDataHolder* dh, 
			 		      uiWellTieView& v )
        : d2tmgr_(dh->d2TMGR())
  	, pmgr_(*dh->pickmgr())  
	, readyforwork(this)
	, pickadded(this)
	, synthpickset_(*dh->pickmgr()->getSynthPickSet())
	, seispickset_(*dh->pickmgr()->getSeisPickSet())
	, uiWellTieStretch(p,dh,v)
{
    synthpickset_.pickadded.notify(mCB(this,uiWellTieEventStretch,addSyntPick));
    seispickset_.pickadded.notify(mCB(this,uiWellTieEventStretch,addSeisPick));
} 


uiWellTieEventStretch::~uiWellTieEventStretch()
{
}


void uiWellTieEventStretch::addSyntPick( CallBacker* )
{
    dataviewer_.drawUserPicks();
    checkReadyForWork();
    pickadded.trigger();
}


void uiWellTieEventStretch::addSeisPick( CallBacker* )
{
    dataviewer_.drawUserPicks();
    checkReadyForWork();
    pickadded.trigger();
}


void uiWellTieEventStretch::checkReadyForWork()
{
    if ( seispickset_.getSize() && synthpickset_.getSize() )
	readyforwork.trigger();
}


void uiWellTieEventStretch::doWork(CallBacker*)
{
    float seistime = time( seispickset_.getLastDah() );
    float synthtime = time( synthpickset_.getLastDah() );
    d2tmgr_->shiftModel( seistime - synthtime );
    seispickset_.setDah( 0, dah(seistime) );
    synthpickset_.setDah( 0, dah(seistime) );
    
    if ( synthpickset_.getSize() > 1 )
    {
	//pmgr_.sortByDah( seispickset_ );
	//pmgr_.sortByDah( synthpickset_ );
	doStretchWork();	
    }
}


void uiWellTieEventStretch::doStretchWork()
{
    for ( int idx=0; idx<seispickset_.getSize(); idx++ )
    {
	//position of the following picks needs update if one of the pick moved
	if ( idx )
	{
	    infborderpos_ = time( seispickset_.getDah(idx-1) );
	    for ( int pickidx=idx; pickidx<synthpickset_.getSize(); pickidx++ )
	    {
		float pos = time( synthpickset_.getDah(pickidx) );
		updateTime( pos );
		synthpickset_.setDah( pickidx, dah(pos) );
	    }
	}
	startpos_ = time( synthpickset_.getDah(idx) );
	stoppos_  = time( seispickset_.getDah(idx) );

	delete prevdispdata_;
	prevdispdata_ = new WellTieDataSet ( dispdata_ );
	doStretchData( params_.dpms_.dptnm_ );

	updateTime( startpos_ );
	synthpickset_.setDah( idx, dah(startpos_) );
    }
    dispdataChanged.trigger();
}


void uiWellTieEventStretch::updateTime( float& pos )
{
    WellTieParams::DataParams dpms = params_.dpms_;
    const int oldidx = prevdispdata_->getIdx( pos );
    const float oldtime = prevdispdata_->get( dpms.timenm_, oldidx );
    const float olddah = prevdispdata_->get( dpms.dptnm_, oldidx );
    const int newidx = dispdata_.getIdxFromDah( olddah );
    pos = dispdata_.get( dpms.timenm_, newidx  ); 	
    const float newtime = prevdispdata_->get( dpms.timenm_, newidx );
}

