/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "volprochorinterfiller.h"

#include "arrayndimpl.h"
#include "emhorizon2d.h"
#include "emmanager.h"
#include "mousecursor.h"
#include "multiid.h"
#include "seisdatapack.h"
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
    RegularSeisDataPack* output = getOutput( getOutputSlotID(0) );
    if ( !output || output->isEmpty() || !isOK() )
	return false;

    const TrcKeySampling& hs = output->sampling().hsamp_;
    const StepInterval<int> outputinlrg( hs.inlRange() );

    if ( !outputinlrg.includes( bid.inl(), false ) ||
         (bid.inl()-outputinlrg.start)%outputinlrg.step )
	return false;

    const StepInterval<int> outputcrlrg( hs.crlRange() );

    if ( !outputcrlrg.includes( bid.crl(), false ) ||
         (bid.crl()-outputcrlrg.start)%outputcrlrg.step )
	return false;

    //TODO: Rework EM::Horizon2D to avoid the manipulation below.
    EM::SubID topsubid = bid.toInt64(), botsubid = bid.toInt64();
    mDynamicCastGet(const EM::Horizon2D*,tophor2d,tophorizon_)
    mDynamicCastGet(const EM::Horizon2D*,bothor2d,bottomhorizon_)
    if ( tophor2d )
    {
	const int lineidx =
		tophor2d->geometry().lineIndex( Pos::GeomID(bid.inl()) );
	topsubid = BinID( lineidx, bid.crl() ).toInt64();
    }

    if ( bothor2d )
    {
	const int lineidx =
		bothor2d->geometry().lineIndex( Pos::GeomID(bid.inl()) );
	botsubid = BinID( lineidx, bid.crl() ).toInt64();
    }

    const double topdepth = tophorizon_
	? tophorizon_->getPos( topsubid ).z : SI().zRange(true).start;

    const double bottomdepth = bottomhorizon_
	? bottomhorizon_->getPos( botsubid ).z : SI().zRange(true).stop;

    const SamplingData<double>
	zsampling( output->sampling().zsamp_ );

    const int topsample = mIsUdf(topdepth)
	? mUdf(int)
	: zsampling.nearestIndex( topdepth );

    const int bottomsample = mIsUdf(bottomdepth)
	? mUdf(int)
	: zsampling.nearestIndex( bottomdepth );

    SamplingData<double> cursampling;
    if ( usegradient_ )
	cursampling.step = gradient_ * output->sampling().zsamp_.step;
    else if ( topsample==bottomsample )
	cursampling.step = 0;
    else
	cursampling.step = (topvalue_-bottomvalue_)/(topsample-bottomsample);

    cursampling.start = topvalue_-topsample*cursampling.step;

    const RegularSeisDataPack* input = getInput( getInputSlotID(0) );
    const Array3D<float>* inputarr = input && input->nrComponents()
	? &input->data( 0 ) : 0;

    StepInterval<int> inputinlrg;
    if ( inputarr )
    {
	inputinlrg = input->sampling().hsamp_.inlRange();
	if ( !inputinlrg.includes( bid.inl(), false ) ||
	      (bid.inl()-inputinlrg.start)%inputinlrg.step )
	    inputarr = 0;
    }

    StepInterval<int> inputcrlrg;
    if ( inputarr )
    {
	inputcrlrg = input->sampling().hsamp_.crlRange();
	if ( !inputcrlrg.includes( bid.crl(), false ) ||
	      (bid.crl()-inputcrlrg.start)%inputcrlrg.step )
	    inputarr = 0;
    }

    const int inputinlidx = inputinlrg.nearestIndex( bid.inl() );
    const int inputcrlidx = inputcrlrg.nearestIndex( bid.crl() );
    const int outputinlidx = outputinlrg.nearestIndex( bid.inl() );
    const int outputcrlidx = outputcrlrg.nearestIndex( bid.crl() );

    Array3D<float>& outputarray = output->data(0);
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
	    const int cursample = idx;
	    value = (float) cursampling.atIndex( cursample );
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


od_int64 HorInterFiller::extraMemoryUsage( OutputSlotID,
	const TrcKeySampling&, const StepInterval<int>& ) const
{
    return 0;
}


} // namespace VolProc
