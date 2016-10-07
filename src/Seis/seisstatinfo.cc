/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Mahant Mothey
 Date:		September 2016
________________________________________________________________________

-*/

#include "seisstatinfo.h"

#include "arrayndimpl.h"
#include "seistrc.h"
#include "statrand.h"

#define mMaxArrSize 1000
#define mTotalNrSamples mMaxArrSize*mMaxArrSize


SeisStatInfo::SeisStatInfo()
	: rc_(*new Stats::ParallelCalc<float>(Stats::CalcSetup(false)
						.require(Stats::Min)
						.require(Stats::Max)
						.require(Stats::Average)
						.require(Stats::Median)
						.require(Stats::StdDev)
						.require(Stats::RMS)) )
	, valrange_(mUdf(float),-mUdf(float))
	, nrvals_(0)
{
    mTryAlloc( trcvals_, Array2DImpl<float>(mMaxArrSize,mMaxArrSize) );
}


SeisStatInfo::~SeisStatInfo()
{
    delete &rc_;
}


void SeisStatInfo::setEmpty()
{
    if ( !trcvals_ ) return;

    rc_.setEmpty();
    trcvals_->setAll( mUdf(float) );
    nrvals_ = 0;
}


void SeisStatInfo::useTrace( const SeisTrc& trc )
{
    if ( !trcvals_ ) return;

    for ( int icomp=0; icomp<trc.nrComponents(); icomp++ )
    {
	for ( int idx=0; idx<trc.size(); idx++ )
	{
	    const float val = trc.get( idx, icomp );
	    if ( mIsUdf(val) ) continue;

	    if ( ++nrvals_ > mTotalNrSamples )
	    {
		const int randidx = Stats::randGen().getIndex( nrvals_ );
		if ( randidx >= mTotalNrSamples )
		    continue;

		const int rowidx = randidx / mMaxArrSize;
		const int colidx = randidx % mMaxArrSize;
		trcvals_->set( rowidx, colidx, val );
	    }
	    else
	    {
		const int validx = nrvals_ - 1;
		trcvals_->set( validx/mMaxArrSize, validx%mMaxArrSize, val );
	    }

	    valrange_.include( val, false );
	}
    }
}


static int getNrIntervals( int nrpts )
{
    int res = nrpts / 25;
    if ( res < 10 ) res = 10;
    else if ( res < 20 ) res = 20;
    else if ( res < 50 ) res = 50;
    else res = 100;
    return res;
}


bool SeisStatInfo::fillPar( IOPar& iop ) const
{
    if ( !trcvals_ || !nrvals_ )
	return false;

    rc_.setValues( trcvals_->getData(), mMIN(mTotalNrSamples,nrvals_) );
    if ( !rc_.execute() )
	{ errmsg_ = rc_.errMsg(); return false; }

    const int nrpts = rc_.count();
    const int nrclasses = getNrIntervals( nrpts );
    TypeSet<float> histdata( nrclasses, 0 );
    const float min = valrange_.start, max = valrange_.stop;
    const float step = (max - min) / nrclasses;
    if ( mIsZero(step,1e-6) )
    {
	histdata[nrclasses/2] = mCast( float, nrpts );
	fillStats( iop, histdata, Interval<float>(min-1,max+1) );
	return true;
    }

    for ( int idx=0; idx<nrpts; idx++ )
    {
	int seg = mCast(int,(rc_.medvals_[idx] - min) / step);
	if ( seg < -1 || seg > nrclasses )
	    { pErrMsg("Huh"); continue; }

	if ( seg < 0 )		seg = 0;
	if ( seg == nrclasses ) seg = nrclasses - 1;

	histdata[seg] += 1;
    }

    const int scalefac = nrvals_/nrpts;
    for ( int idx=0; idx<histdata.size(); idx++ )
	histdata[idx] *= scalefac;

    fillStats( iop, histdata, Interval<float>(min-0.5f*step,max+0.5f*step) );
    return true;
}


void SeisStatInfo::fillStats( IOPar& iop, const TypeSet<float>& histdata,
			      const Interval<float>& xrg ) const
{
    iop.set( sKey::Data(), histdata );
    iop.set( sKey::ValueRange(), xrg );
    iop.set( sKey::Average(), rc_.average() );
    iop.set( sKey::StdDev(), rc_.stdDev() );
    iop.set( sKey::Median(), rc_.median() );
    iop.set( sKey::RMS(), rc_.rms() );
    iop.set( sKey::NrValues(), nrvals_ );
}
