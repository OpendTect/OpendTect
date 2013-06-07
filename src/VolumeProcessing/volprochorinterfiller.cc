/*+
 *(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 *Author:	Y.C. Liu
 *Date:		April 2007
-*/

static const char* rcsID = "$Id$";

#include "volprochorinterfiller.h"

#include "arraynd.h"
#include "emhorizon.h"
#include "emmanager.h"
#include "mousecursor.h"
#include "survinfo.h"

namespace VolProc
{

HorInterFiller::HorInterFiller()
    : topvalue_( mUdf(float) )
    , bottomvalue_( mUdf(float) )
    , tophorizon_( 0 )
    , bottomhorizon_( 0 )
    , gradient_( mUdf(float) )
    , usegradient_( true )
{}


HorInterFiller::~HorInterFiller()
{
    releaseData();
}


void HorInterFiller::releaseData()
{
    Step::releaseData();
    if ( tophorizon_ ) tophorizon_->unRef();
    if ( bottomhorizon_ ) bottomhorizon_->unRef(); 
    tophorizon_ = 0;
    bottomhorizon_ = 0;
}


bool HorInterFiller::setTopHorizon( const MultiID* tmid )
{
    if ( !tmid )
    {
	tophorizon_ = 0;
	return true;
    }

    RefMan<EM::Horizon> newhor = tmid ? loadHorizon( *tmid ) : 0;
    if ( !newhor ) return false;
   
    tophorizon_ = newhor;
    tophorizon_->ref();

    return true;
}    


const MultiID* HorInterFiller::getTopHorizonID() const
{ return tophorizon_ ? &tophorizon_->multiID() : 0; }    


float HorInterFiller::getTopValue() const
{ return topvalue_; }


const MultiID* HorInterFiller::getBottomHorizonID() const
{ return bottomhorizon_ ? &bottomhorizon_->multiID() : 0; }


float HorInterFiller::getBottomValue() const
{ return bottomvalue_; }


void HorInterFiller::setBottomValue( float bv )
{ bottomvalue_ = bv; }


void HorInterFiller::setTopValue( float tv )
{ topvalue_ = tv; }


bool HorInterFiller::usesGradient() const
{ return usegradient_; }


void HorInterFiller::useGradient( bool yn )
{ usegradient_ = yn; }


float HorInterFiller::getGradient() const
{ return gradient_; }


void HorInterFiller::setGradient( float g )
{ gradient_ = g; }


bool HorInterFiller::setBottomHorizon( const MultiID* bmid )
{
    if ( !bmid )
    {
	bottomhorizon_ = 0;
	return true;
    }

    RefMan<EM::Horizon> newhor = bmid ? loadHorizon( *bmid ) : 0;
    if ( !newhor ) return false;
 
    bottomhorizon_ = newhor;
    bottomhorizon_->ref();

    return true;
}


EM::Horizon* HorInterFiller::loadHorizon( const MultiID& mid ) const
{
    RefMan<EM::EMObject> emobj = EM::EMM().loadIfNotFullyLoaded( mid );
    mDynamicCastGet( EM::Horizon*, newhor, emobj.ptr() );
    if ( !newhor ) return 0;

    newhor->ref();
    emobj = 0;
    newhor->unRefNoDelete();
    return newhor;
}


bool HorInterFiller::computeBinID( const BinID& bid, int )
{
    if ( !output_ || !output_->nrCubes() || !isOK() )
	return false;

    const StepInterval<int> outputinlrg( output_->inlsampling_.start,
   			 output_->inlsampling_.atIndex( output_->getInlSz()-1 ),
			 output_->inlsampling_.step );

    if ( !outputinlrg.includes( bid.inl, false ) ||
         (bid.inl-outputinlrg.start)%outputinlrg.step )
	return false;

    const StepInterval<int> outputcrlrg( output_->crlsampling_.start,
			output_->crlsampling_.atIndex( output_->getCrlSz()-1 ),
			output_->crlsampling_.step );

    if ( !outputcrlrg.includes( bid.crl, false ) ||
         (bid.crl-outputcrlrg.start)%outputcrlrg.step )
	return false;

    const double topdepth = tophorizon_
	? tophorizon_->getPos(tophorizon_->sectionID(0), bid.toInt64()).z
	: SI().zRange(true).start;

    const double bottomdepth = bottomhorizon_
	? bottomhorizon_->getPos(
		bottomhorizon_->sectionID(0), bid.toInt64() ).z
	: SI().zRange(true).stop;

    const SamplingData<double>
	zsampling( output_->z0_*output_->zstep_, output_->zstep_ );

    const int topsample = mIsUdf(topdepth)
	? mUdf(int)
	: zsampling.nearestIndex( topdepth );

    const int bottomsample = mIsUdf(bottomdepth)
	? mUdf(int)
	: zsampling.nearestIndex( bottomdepth );

    SamplingData<double> cursampling;
    if ( usegradient_ )
	cursampling.step = gradient_ * output_->zstep_;
    else if ( topsample==bottomsample )
	cursampling.step = 0;
    else
	cursampling.step = (topvalue_-bottomvalue_)/(topsample-bottomsample);

    cursampling.start = topvalue_-topsample*cursampling.step;

    const Array3D<float>* inputarr = input_ && input_->nrCubes()
       	? &input_->getCube( 0 ) : 0;

    StepInterval<int> inputinlrg;
    if ( inputarr )
    {
	inputinlrg = input_->inlsampling_.interval( input_->getInlSz() );
	if ( !inputinlrg.includes( bid.inl, false ) ||
	      (bid.inl-inputinlrg.start)%inputinlrg.step )
	    inputarr = 0;
    }

    StepInterval<int> inputcrlrg;
    if ( inputarr )
    {
	inputcrlrg = input_->crlsampling_.interval( input_->getCrlSz() );
	if ( !inputcrlrg.includes( bid.crl, false ) ||
	      (bid.crl-inputcrlrg.start)%inputcrlrg.step )
	    inputarr = 0;
    }

    const int inputinlidx = inputinlrg.nearestIndex( bid.inl );
    const int inputcrlidx = inputcrlrg.nearestIndex( bid.crl );
    const int outputinlidx = outputinlrg.nearestIndex( bid.inl );
    const int outputcrlidx = outputcrlrg.nearestIndex( bid.crl );

    Array3D<float>& outputarray = output_->getCube(0);
    for ( int idx=outputarray.info().getSize(2)-1; idx>=0; idx-- )
    {
	bool dobg = false;
	if ( mIsUdf(topsample) && mIsUdf(bottomsample) )
	    dobg = true;
	else if ( !mIsUdf(topsample) && !mIsUdf(bottomsample) )
	{
	    const Interval<int> rg(topsample,bottomsample);
	    dobg = !rg.includes( idx, false );
	}
	else if ( tophorizon_ && bottomhorizon_ )
	    dobg = true;
	else if ( !mIsUdf(topsample) )
	    dobg = idx<topsample;
	else 
	    dobg = true;

    	float value;
	if ( dobg )
	{
	    value = inputarr ? inputarr->get(inputinlidx,inputcrlidx,idx)
			     : mUdf( float );
	}   
	else
	{
	    const int cursample = output_->z0_+idx;
	    value = cursampling.atIndex( cursample );
	}

        outputarray.set( outputinlidx, outputcrlidx, idx, value );
    } 
     
    return true;
}


void HorInterFiller::fillPar( IOPar& pars ) const
{
    Step::fillPar( pars );

    if ( tophorizon_ ) pars.set( sKeyTopHorID(), tophorizon_->multiID() );
    if ( bottomhorizon_ ) pars.set( sKeyBotHorID(), bottomhorizon_->multiID() );

    pars.set( sKeyTopValue(), topvalue_ );
    pars.set( sKeyBotValue(), bottomvalue_ );
    pars.set( sKeyGradient(), gradient_ );
    pars.setYN( sKeyUseGradient(), usegradient_ );
}


bool HorInterFiller::usePar( const IOPar& pars )
{
    if ( !Step::usePar( pars ) )
	return false;

    MouseCursorChanger cursorlock( MouseCursor::Wait );

    topvalue_ = mUdf(float);
    bottomvalue_ = mUdf(float);
    gradient_ = mUdf(float);
    usegradient_ = false;

    pars.getYN( sKeyUseGradient(), usegradient_ );
    pars.get( sKeyGradient(), gradient_ );
    pars.get( sKeyBotValue(), bottomvalue_ );
    pars.get( sKeyTopValue(), topvalue_ );

    MultiID tophorid;
    if ( pars.get( sKeyTopHorID(), tophorid) && !setTopHorizon( &tophorid ) )
	return false;

    MultiID bottomhorid;
    if ( pars.get( sKeyBotHorID(), bottomhorid ) &&
	 !setBottomHorizon( &bottomhorid ) )
	return false;

    return isOK();
}


bool HorInterFiller::isOK() const
{
    if ( mIsUdf(topvalue_) )
	return false;

    if ( usegradient_ && mIsUdf(gradient_) )
	return false;

    if ( !usegradient_ && mIsUdf(bottomvalue_) )
	return false;

    return true;
}

}; //namespace
