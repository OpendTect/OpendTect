/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bruno
 Date:		Apr 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltiepickset.cc,v 1.15 2009-07-10 16:11:17 cvsbruno Exp $";

#include "welltiepickset.h"

#include "welltiedata.h"

#include "arrayndimpl.h"
#include "valseriesevent.h"
#include "sorting.h"
#include "welldata.h"
#include "welld2tmodel.h"

WellTiePickSetMGR::WellTiePickSetMGR( const Well::Data* wd )
	: CallBacker(CallBacker::CallBacker())
    	, wd_(wd)
	, evtype_ (VSEvent::Extr)
{
}

WellTiePickSetMGR::~WellTiePickSetMGR()
{}


void WellTiePickSetMGR::setDataParams( const WellTieParams::DataParams* dpms )
{
    datapms_ = dpms;			    
}


void WellTiePickSetMGR::setData( const WellTieDataSet* data )
{
    dispdata_ = data;
}


void WellTiePickSetMGR::setEventType( int seltype )
{
    if ( seltype==1 )
	evtype_ = VSEvent::Max;
    else if ( seltype==2 )
	evtype_ = VSEvent::Min;
    else if ( seltype==3 )
	evtype_ = VSEvent::ZC;
    else 
	evtype_ = VSEvent::Extr;
}


void WellTiePickSetMGR::addPick( float vwrszstart, float vwrszstop, 
				 float xpos, float zpos )
{
    const int seissz = seispickset_.getSize();
    const int synthsz = synthpickset_.getSize();
    if ( abs(seissz-synthsz)<2 )
    {
	if ( xpos<(vwrszstop-vwrszstart)/2 )
	{
	    if ( abs(synthsz+1-seissz) > 1 || lastpicksynth_ == true )
		synthpickset_.clear( synthpickset_.getSize()-1 );
	    synthpickset_.add( 0, xpos, findEvent(zpos,true) );
	    lastpicksynth_ = true;
	}
	else
	{
	    if ( abs(seissz+1-synthsz) > 1 || lastpicksynth_ == false )
		seispickset_.clear( seispickset_.getSize()-1 );
	    seispickset_.add( 0, xpos, findEvent(zpos,false) );
	    lastpicksynth_ = false;
	}
    }
}


#define mTimeGate 0.04
float WellTiePickSetMGR::findEvent( float zpos, bool issynth )
{
    zpos *= 0.001;
    if ( evtype_ == VSEvent::None ) return zpos;
    const char* colnm = issynth ? datapms_->synthnm_ : datapms_->attrnm_; 
    const int maxidx = dispdata_->getLength()-1;
    Interval<float> intv ( zpos, mTimeGate );
    SamplingData<float> sd;
    ValueSeriesEvFinder<float,float> evf( *dispdata_->get(colnm), 
	    				  maxidx, sd );
    const float evpos = mNINT( evf.find( evtype_, intv ).pos );
    if ( evpos>maxidx || evpos<0 ) return zpos;

    return evpos;
}


void WellTiePickSetMGR::updateShift( int vwridx, float curpos )
{
    //TODO ???
    /*used only for log stretch and squueze...
    logpickset_.setMousePos( curpos );
    mousemoving.trigger();*/
}


void WellTiePickSetMGR::clearAllPicks()
{
    logpickset_.clearAll();
    seispickset_.clearAll();
    synthpickset_.clearAll();
}


void WellTiePickSetMGR::clearLastPicks()
{
    if ( seispickset_.getSize() == synthpickset_.getSize() )
    {
	if ( lastpicksynth_ )
	    synthpickset_.clear( synthpickset_.getSize()-1 );
	else
	    seispickset_.clear( seispickset_.getSize()-1 );
    }
    else if ( seispickset_.getSize() > synthpickset_.getSize() )
	seispickset_.clear( seispickset_.getSize()-1 );
    else if ( seispickset_.getSize() < synthpickset_.getSize() )
	synthpickset_.clear( synthpickset_.getSize()-1 );
}


bool WellTiePickSetMGR::isPick()
{
    return ( seispickset_.getSize() || synthpickset_.getSize() );
}


bool WellTiePickSetMGR::isSameSize()
{
    return ( seispickset_.getSize() == synthpickset_.getSize() );
}


void WellTiePickSetMGR::sortByPos( WellTiePickSet& pickset )
{
    const int sz = pickset.getSize();
    TypeSet<float> zvals;
    for ( int idx=0; idx<sz; idx++ )
	zvals += pickset.getPos(idx);

    mAllocVarLenArr( int, zidxs, sz );
    for ( int idx=0; idx<sz; idx++ )
	zidxs[idx] = idx;

    sort_coupled( zvals.arr(), mVarLenArr(zidxs), sz );

    for ( int idx=0; idx<sz; idx++ )
	pickset.setPos( idx, zvals[idx]  );
}




WellTiePickSet::WellTiePickSet()
	: mousepos_(0)
	, nrpickstotal_(0)
   	, pickadded(this)
{
}


WellTiePickSet::~WellTiePickSet()
{
    for ( int idx=pickset_.size()-1; idx>=0; idx-- )
	delete ( pickset_.remove(idx) );
}


void WellTiePickSet::add( int vwridx, float xpos, float zpos )
{
    UserPick* pick = new UserPick();
    pick->vidx_ = vwridx; pick->color_ = Color::DgbColor();
    pick->xpos_ = xpos;   pick->zpos_ = zpos;
    pickset_ += pick;
    nrpickstotal_++;
    pickadded.trigger();
}


void WellTiePickSet::clear( int idx )
{
    if ( pickset_.size() )
	delete ( pickset_.remove(idx) );
}


void WellTiePickSet::clearAll()
{
    for ( int idx=pickset_.size()-1; idx>=0; idx-- )
	delete ( pickset_.remove(idx) );
}


