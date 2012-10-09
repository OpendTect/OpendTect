/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Apr 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "arrayndimpl.h"
#include "sorting.h"
#include "seistrc.h"
#include "welltiepickset.h"
#include "welltiedata.h"

namespace WellTie
{

PickSetMgr::PickSetMgr( PickData& pd )
    : evtype_ (VSEvent::Extr)
    , pickadded(this)
    , synthpickset_(pd.synthpicks_)
    , seispickset_(pd.seispicks_)
{}


void PickSetMgr::setEventType( int seltype )
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

    
int PickSetMgr::getEventType() const
{
    switch ( evtype_ )
    {
        case VSEvent::Extr:
            return 1;
        case VSEvent::Max:
            return 2;
        case VSEvent::Min:
            return 3;
        case VSEvent::ZC:
            return 4;
            
        default:
            break;
    }
    
    return 0;
}
    
    
void PickSetMgr::addPick( float zpos, bool issynth, const SeisTrc* trc )
{
    const int seissz = seispickset_.size();
    const int synthsz = synthpickset_.size();
    if ( abs(seissz-synthsz)<2 )
    {
	TypeSet<Marker>& pickset = issynth ? synthpickset_ : seispickset_;
	const int sz = pickset.size();
	if ( (issynth && (abs(sz+1-seissz) > 1 || lastpicksynth_))
		|| (!issynth && (abs(sz+1-synthsz) > 1 || !lastpicksynth_)) )
	    pickset.remove( sz -1 );
	Marker m( trc ? findEvent( *trc, zpos ) : zpos );
	m.color_ = Color::DgbColor();
	pickset += m;
	lastpicksynth_ = issynth;
    }
    pickadded.trigger();
}



#define mTimeGate 0.02
float PickSetMgr::findEvent( const SeisTrc& trc, float zpos ) const
{
    zpos *= 0.001;
    if ( evtype_ == VSEvent::None ) return zpos;

    const int maxidx = trc.size();
    Interval<float> intvup ( zpos, zpos - mTimeGate );
    Interval<float> intvdown ( zpos, zpos + mTimeGate );
    SamplingData<float> sd = trc.info().sampling;
    Array1DImpl<float> vals( trc.size() );
    for ( int idx=0; idx<trc.size(); idx++ )
	vals.set( idx, trc.get( idx, 0 ) );

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

    return evpos;
}


void PickSetMgr::clearAllPicks()
{
    seispickset_.erase();
    synthpickset_.erase();
}


void PickSetMgr::clearLastPicks()
{
    if ( isSynthSeisSameSize() )
    {
	if ( lastpicksynth_ )
	    synthpickset_.remove( synthpickset_.size()-1 );
	else
	    seispickset_.remove( seispickset_.size()-1 );
    }
    else if ( seispickset_.size() > synthpickset_.size() )
	seispickset_.remove( seispickset_.size()-1 );
    else if ( seispickset_.size() < synthpickset_.size() )
	synthpickset_.remove( synthpickset_.size()-1 );
    lastpicksynth_ = !lastpicksynth_;
}


bool PickSetMgr::isPick() const
{
    return ( !seispickset_.isEmpty() || !synthpickset_.isEmpty() );
}


bool PickSetMgr::isSynthSeisSameSize() const
{
    return ( seispickset_.size() == synthpickset_.size() );
}


void PickSetMgr::setPickSetPos( bool issynth, int idx, float z )
{
    TypeSet<Marker>& pickset = issynth ? synthpickset_ : seispickset_;
    pickset[idx].zpos_ = z;
}


void PickSetMgr::sortByPos()
{
    sortByPos( synthpickset_ );
    sortByPos( seispickset_ );
}


void PickSetMgr::sortByPos( TypeSet<Marker>& pickset )
{
    const int sz = pickset.size();
    TypeSet<float> zvals;
    for ( int idx=0; idx<sz; idx++ )
	zvals += pickset[idx].zpos_; 

    mAllocVarLenArr( int, zidxs, sz );
    for ( int idx=0; idx<sz; idx++ )
	zidxs[idx] = idx;

    sort_coupled( zvals.arr(), mVarLenArr(zidxs), sz );

    for ( int idx=0; idx<sz; idx++ )
	pickset[idx].zpos_ = zvals[idx];
}

}; //namespace WellTie
