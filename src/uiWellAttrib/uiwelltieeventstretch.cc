/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bruno
 Date:		Apr 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelltieeventstretch.cc,v 1.10 2009-06-22 07:45:57 cvsbruno Exp $";

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
    if ( synthpickset_.getSize() == 1 && seispickset_.getSize() == 1 )
	shiftModel();
    else if ( synthpickset_.getSize()>1 && seispickset_.getSize()>1 )
    {
	//pmgr_.sortByDah( seispickset_ );
	//pmgr_.sortByDah( synthpickset_ );
	shiftDahData(); 
	updatePicksPos( synthpickset_, 0 );
	doStretchWork();	
	dispdataChanged.trigger();
    }
}


void uiWellTieEventStretch::shiftModel()
{
    float seistime = time( seispickset_.getLastDah() );
    float synthtime = time( synthpickset_.getLastDah() );
    d2tmgr_->shiftModel( seistime - synthtime );
}


void uiWellTieEventStretch::doStretchWork()
{
    for ( int idx=0; idx<seispickset_.getSize()-1; idx++ )
    {
	if ( idx )
	{
	    infborderpos_ = time( seispickset_.getDah(idx-1) );
	    updatePicksPos( synthpickset_, idx );
	}
	//position of the following picks needs update if one of the pick moved
	startpos_ = time( synthpickset_.getDah(idx) );
	stoppos_  = time( seispickset_.getDah(idx) );

	delete prevdispdata_;
	prevdispdata_ = new WellTieDataSet ( dispdata_ );

	doStretchData( params_.dpms_.dptnm_ );
	updateTime( startpos_ );
    }
}


void uiWellTieEventStretch::updatePicksPos( WellTiePickSet& pickset, 
					    int startidx )
{
    for ( int pickidx=startidx; pickidx<pickset.getSize(); pickidx++ )
    {
	float pos = time( pickset.getDah( pickidx ) );
	updateTime( pos );
	pickset.setDah( pickidx, dah(pos) );
    }
}


void uiWellTieEventStretch::shiftDahData()
{
    const float dahshift = seispickset_.getLastDah()-synthpickset_.getLastDah();
    for ( int idx=0; idx<dispdata_.getLength(); idx++ )
	dispdata_.set( params_.dpms_.dptnm_, idx, 
		dispdata_.get(params_.dpms_.dptnm_,idx)-dahshift );
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

