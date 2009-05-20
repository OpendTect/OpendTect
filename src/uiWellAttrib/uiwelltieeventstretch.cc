/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bruno
 Date:		Apr 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelltieeventstretch.cc,v 1.1 2009-05-20 14:27:29 cvsbruno Exp $";

#include "arrayndimpl.h"
#include "uiwelltieeventstretch.h"

#include "uiwelltieview.h"
#include "welltiedata.h"
#include "welltieunitfactors.h"
#include "welltiedata.h"
#include "welltiepickset.h"
#include "welltiesetup.h"
#include "welltiegeocalculator.h"

uiWellTieEventStretch::uiWellTieEventStretch( uiParent* p,
			    const WellTieParams* pms, const Well::Data* wd,
			    WellTieDataMGR& mgr, uiWellTieView& v,
			    WellTiePickSetMGR& pmgr )
        : uiWellTieStretch(p,pms,wd,mgr,v,pmgr)
	, synthpickset_(*pmgr.getSynthPickSet())
	, seispickset_(*pmgr.getSeisPickSet())
	, readyforwork(this)				
{
    pmgr.pickadded.notify(mCB(this,uiWellTieEventStretch,addUserPick));
} 


uiWellTieEventStretch::~uiWellTieEventStretch()
{
}


void uiWellTieEventStretch::addUserPick( CallBacker* cb )
{
    mCBCapsuleUnpack(int,vwridx,cb);
	
    if ( vwridx == 3 )
	dataviewer_.drawUserPicks( synthpickset_ );
    else if ( vwridx == 4)
	dataviewer_.drawUserPicks( seispickset_ );
    else return;

    if ( seispickset_.getSize() && synthpickset_.getSize() 
      		&& seispickset_.getSize() == synthpickset_.getSize() )
	readyforwork.trigger();
}


void uiWellTieEventStretch::doWork(CallBacker*)
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
	doStretchData(params_.dptnm_);
    }
    dispdataChanged.trigger();
}


void uiWellTieEventStretch::updatePos( float& pos )
{
    const int oldidx = prevdispdata_->getIdx( pos );
    const float oldtime = prevdispdata_->get( params_.timenm_, oldidx );
    const float olddah = prevdispdata_->get( params_.dptnm_, oldidx );
    const int newidx = dispdata_.findTimeIdx( olddah );
    pos = dispdata_.get( params_.timenm_, newidx  ); 	
    const float newtime = prevdispdata_->get( params_.timenm_, newidx );
}

