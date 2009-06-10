/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bruno
 Date:		Apr 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelltieeventstretch.cc,v 1.4 2009-06-10 08:30:04 cvsbruno Exp $";

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
	: uiWellTieStretch(p,dh,v)
	, synthpickset_(*dh->pickmgr_->getSynthPickSet())
	, seispickset_(*dh->pickmgr_->getSeisPickSet())
        , d2tmgr_(dh->d2tmgr_)	
	, readyforwork(this)				
{
    dh->pickmgr_->pickadded.notify(mCB(this,uiWellTieEventStretch,addUserPick));
} 


uiWellTieEventStretch::~uiWellTieEventStretch()
{
}


void uiWellTieEventStretch::addUserPick( CallBacker* cb )
{
    mCBCapsuleUnpack(int,vwridx,cb);
	
    if ( vwridx == 4 )
	dataviewer_.drawUserPicks( &synthpickset_ );
    else if ( vwridx == 5 )
	dataviewer_.drawUserPicks( &seispickset_ );
    else return;

    if ( seispickset_.getSize() && synthpickset_.getSize() )
	readyforwork.trigger();
}


void uiWellTieEventStretch::doWork(CallBacker*)
{
    if ( synthpickset_.getSize() == 1 )
	d2tmgr_->shiftModel( seispickset_.getLastPos()
			    	- synthpickset_.getLastPos() );
    else
	doStretchWork();	
}


void uiWellTieEventStretch::doStretchWork()
{
    for (int idx=0; idx<seispickset_.getSize(); idx++)
    {
	if ( idx )
	{
	    infborderpos_ = seispickset_.getPos(idx-1);
	    for (int pickidx=idx; pickidx<synthpickset_.getSize(); pickidx++)
	    {
		float pos = synthpickset_.getPos(pickidx);
		updatePos(pos);
		synthpickset_.setPos(pickidx,pos);
	    }
	}
	startpos_ = synthpickset_.getPos(idx);
	stoppos_  = seispickset_.getPos(idx);
	
	delete prevdispdata_;
	prevdispdata_ = new WellTieDataSet ( dispdata_ );
	doStretchData( params_.dptnm_ );
    }
    dispdataChanged.trigger();
}


void uiWellTieEventStretch::updatePos( float& pos )
{
    const int oldidx = prevdispdata_->getIdx( pos );
    const float oldtime = prevdispdata_->get( params_.timenm_, oldidx );
    const float olddah = prevdispdata_->get( params_.dptnm_, oldidx );
    const int newidx = dispdata_.getIdxFromDah( olddah );
    pos = dispdata_.get( params_.timenm_, newidx  ); 	
    const float newtime = prevdispdata_->get( params_.timenm_, newidx );
}

