/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Apr 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltiepickset.cc,v 1.30 2010-06-01 13:31:45 cvsbruno Exp $";

#include "welltiepickset.h"

#include "welltiedata.h"
#include "sorting.h"
#include "valseriesevent.h"

namespace WellTie
{

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


void PickSetMGR::addPick( float zpos, bool issynth )
{
    const int seissz = seispickset_.getSize();
    const int synthsz = synthpickset_.getSize();
    if ( abs(seissz-synthsz)<2 )
    {
	if ( issynth )
	{
	    if ( abs(synthsz+1-seissz) > 1 || lastpicksynth_ == true )
		synthpickset_.clear( synthpickset_.getSize()-1 );
	    synthpickset_.add( true, findEvent(zpos,true) );
	    lastpicksynth_ = true;
	}
	else
	{
	    if ( abs(seissz+1-synthsz) > 1 || lastpicksynth_ == false )
		seispickset_.clear( seispickset_.getSize()-1 );
	    seispickset_.add( false, findEvent(zpos,false) );
	    lastpicksynth_ = false;
	}
    }
    pickadded.trigger();
}


#define mTimeGate 0.02
float PickSetMGR::findEvent( float zpos, bool issynth )
{
    zpos *= 0.001;
    if ( evtype_ == VSEvent::None ) return zpos;

    const WellTie::Params::DataParams& dpms = *dholder_.dpms();
    const char* colnm = issynth ? dpms.synthnm_ : dpms.seisnm_; 
    const int maxidx = dpms.timeintvs_[1].nrSteps()-1;
    Interval<float> intvup ( zpos, zpos - mTimeGate );
    Interval<float> intvdown ( zpos, zpos + mTimeGate );
    SamplingData<float> sd( dpms.timeintvs_[1].start, dpms.timeintvs_[1].step );
    const Array1DImpl<float>& vals = *dholder_.getLogVal( colnm );
    ValueSeriesEvFinder<float,float> evf( vals, maxidx, sd );
    const float evposup =  evf.find( evtype_, intvup ).pos;
    const float evposdown =  evf.find( evtype_, intvdown ).pos;
    float evpos = evposdown;
    if ( (mIsUdf(evposup) || evposup<0) && (mIsUdf(evposdown) || evposdown<0) )
	evpos = zpos;
    else if ( mIsUdf(evposdown) || evposdown<0 )
	evpos = evposup;
    else if ( fabs(zpos-evposup)<fabs(zpos-evposdown) )
	evpos = evposup;

    return evpos + dpms.timeintvs_[1].step/2;
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


void PickSet::add( bool issynth, float zpos )
{
    UserPick* pick = new UserPick();
    pick->color_ = Color::DgbColor();
    pick->issynthetic_ = issynth;
    pick->zpos_ = zpos;
    pickset_ += pick;
    nrpickstotal_++;
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
