/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Apr 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltiepickset.cc,v 1.22 2009-09-24 15:29:09 cvsbruno Exp $";

#include "welltiepickset.h"

#include "welltiedata.h"

#include "arrayndimpl.h"
#include "sorting.h"
#include "survinfo.h"
#include "valseriesevent.h"
#include "welldata.h"
#include "welld2tmodel.h"

namespace WellTie
{

void PickSetMGR::setDataParams( const WellTie::Params::DataParams* dpms )
{
    datapms_ = dpms;			    
}


void PickSetMGR::setData( const WellTie::LogSet* data )
{
    logsset_ = data;
}


void PickSetMGR::setEventType( int seltype )
{
    if ( seltype==1 )
	evtype_ = VSEvent::Extr;
    else if ( seltype==2 )
	evtype_ = VSEvent::Max;
    else if ( seltype==3 )
	evtype_ = VSEvent::Min;
    else if ( seltype==4 )
	evtype_ = VSEvent::ZC;
    else 
	evtype_ = VSEvent::None;
}


void PickSetMGR::addPick( float vwrszstart, float vwrszstop, 
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


#define mTimeGate 0.02
float PickSetMGR::findEvent( float zpos, bool issynth )
{
    zpos *= 0.001;
    if ( evtype_ == VSEvent::None ) return zpos;

    const char* colnm = issynth ? datapms_->synthnm_ : datapms_->attrnm_; 
    const int maxidx = datapms_->timeintvs_[0].nrSteps();
    Interval<float> intvup ( zpos, zpos - mTimeGate );
    Interval<float> intvdown ( zpos, zpos + mTimeGate );
    SamplingData<float> sd; sd.start = 0; sd.step = SI().zStep();
    ValueSeriesEvFinder<float,float> evf(*logsset_->getVal(colnm), maxidx, sd );
    const float evposup =  evf.find( evtype_, intvup ).pos;
    const float evposdown =  evf.find( evtype_, intvdown ).pos;
    float evpos = evposdown;
    if ( (mIsUdf(evposup) || evposup<0) && (mIsUdf(evposdown) || evposdown<0) )
	evpos = zpos;
    else if ( mIsUdf(evposdown) || evposdown<0 )
	evpos = evposup;
    else if ( fabs(zpos-evposup)<fabs(zpos-evposdown) )
	evpos = evposup;

    return evpos;
}


void PickSetMGR::updateShift( int vwridx, float curpos )
{
    //TODO ???
    /*used only for log stretch and squueze...
    logpickset_.setMousePos( curpos );
    mousemoving.trigger();*/
}


void PickSetMGR::clearAllPicks()
{
    seispickset_.clearAll();
    synthpickset_.clearAll();
}


void PickSetMGR::clearLastPicks()
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


bool PickSetMGR::isPick()
{
    return ( seispickset_.getSize() || synthpickset_.getSize() );
}


bool PickSetMGR::isSameSize()
{
    return ( seispickset_.getSize() == synthpickset_.getSize() );
}


void PickSetMGR::sortByPos( PickSet& pickset )
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



PickSet::~PickSet()
{
    for ( int idx=pickset_.size()-1; idx>=0; idx-- )
	delete ( pickset_.remove(idx) );
}


void PickSet::add( int vwridx, float xpos, float zpos )
{
    UserPick* pick = new UserPick();
    pick->vidx_ = vwridx; pick->color_ = Color::DgbColor();
    pick->xpos_ = xpos;   pick->zpos_ = zpos;
    pickset_ += pick;
    nrpickstotal_++;
    pickadded.trigger();
}


void PickSet::clear( int idx )
{
    if ( pickset_.size() )
	delete ( pickset_.remove(idx) );
}


void PickSet::clearAll()
{
    for ( int idx=pickset_.size()-1; idx>=0; idx-- )
	delete ( pickset_.remove(idx) );
}

}; //namespace WellTie
