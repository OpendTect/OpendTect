/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Apr 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelltieeventstretch.cc,v 1.17 2009-09-03 09:41:40 cvsbruno Exp $";

#include "arrayndimpl.h"
#include "uiwelltieeventstretch.h"

#include "welld2tmodel.h"
#include "welltiedata.h"
#include "welltied2tmodelmanager.h"
#include "welltiegeocalculator.h"
#include "welltieunitfactors.h"
#include "welltiedata.h"
#include "welltiepickset.h"
#include "welltiesetup.h"
#include "uiwelltieview.h"

namespace WellTie
{

uiEventStretch::uiEventStretch( uiParent* p, WellTie::DataHolder* dh, 
				WellTie::uiTieView& v )
        : d2tmgr_(dh->d2TMGR())
  	, pmgr_(*dh->pickmgr())  
	, pickadded(this)
	, synthpickset_(*dh->pickmgr()->getSynthPickSet())
	, seispickset_(*dh->pickmgr()->getSeisPickSet())
	, WellTie::uiStretch(p,dh,v)
{
    synthpickset_.pickadded.notify(mCB(this,uiEventStretch,addSyntPick));
    seispickset_.pickadded.notify(mCB(this,uiEventStretch,addSeisPick));
} 


uiEventStretch::~uiEventStretch()
{
    synthpickset_.pickadded.remove(mCB(this,uiEventStretch,addSyntPick));
    seispickset_.pickadded.remove(mCB(this,uiEventStretch,addSeisPick));
}


void uiEventStretch::addSyntPick( CallBacker* )
{
    dataviewer_.drawUserPicks();
    pickadded.trigger();
}


void uiEventStretch::addSeisPick( CallBacker* )
{
    dataviewer_.drawUserPicks();
    pickadded.trigger();
}


void uiEventStretch::doWork( CallBacker* )
{
    pmgr_.sortByPos( seispickset_ ); 	
    pmgr_.sortByPos( synthpickset_ );
    doStretchWork();	
    timeChanged.trigger();
}


void uiEventStretch::doStretchWork()
{
    Well::D2TModel d2t = *wd_->d2TModel();
    const int d2tsz = d2t.size();
    Array1DImpl<float> d2tarr( d2tsz );
    Array1DImpl<float>* prvd2tarr = new Array1DImpl<float>( d2tsz );

    float timeshift =  seispickset_.getLastPos() - synthpickset_.getLastPos();

    for ( int idx=0; idx<d2tsz; idx++ )
    {
	prvd2tarr->set( idx, d2t.value(idx) );
	d2tarr.set( idx, d2t.value(idx) + timeshift );
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


void uiEventStretch::updatePicksPos( const Array1DImpl<float>& curtime,
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

}; //namespace WellTie
