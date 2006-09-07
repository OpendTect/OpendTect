/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2006
-*/

static const char* rcsID = "$Id: array2dbitmap.cc,v 1.2 2006-09-07 19:05:17 cvsbert Exp $";

#include "array2dbitmap.h"
#include "arrayndimpl.h"
#include "sorting.h"
#include "stats.h"
#include <math.h>


const char A2DBitmapGenPars::cNoFill = -127;
#define mMaxNrStatPts 2000
#define mMinNrStatPts 100


Interval<float> A2DBitMapInpData::scale( float clipratio ) const
{
    const int nrstatpts = nrPts();
    int nroffs = (int)(clipratio * nrstatpts + .5);
    if ( nroffs > nrstatpts / 2 - 1 )
	nroffs = nrstatpts / 2 - 1;
    if ( nroffs < 0 )
	nroffs = 0;

    return Interval<float>( statpts_[nroffs], statpts_[nrstatpts-nroffs-1] );
}


void A2DBitMapInpData::collectData()
{
    const int szdim0 = data_.info().getSize( 0 );
    const int szdim1 = data_.info().getSize( 1 );
    const int totnrsamps = szdim0 * szdim1;

    if ( totnrsamps > mMaxNrStatPts )
    {
	const int maxtries = mMaxNrStatPts * 1000;
	Stat_initRandom( 0 );
	for ( int itry=0; itry<maxtries; itry++ )
	{
	    const int totidx = Stat_getIndex( totnrsamps );
	    const float val = data_.get( totidx / szdim1, totidx % szdim1 );
	    if ( !mIsUdf(val) )
	    {
		statpts_ += val;
		if ( statpts_.size() >= mMaxNrStatPts )
		    return;
	    }
	}
    }

    if ( statpts_.size() >= mMinNrStatPts ) return;

    for ( int idim0=0; idim0<szdim0; idim0++ )
    {
	for ( int idim1=0; idim1<szdim1; idim1++ )
	{
	    const float val = data_.get( idim0, idim1 );
	    if ( !mIsUdf(val) )
	    {
		statpts_ += val;
		if ( statpts_.size() >= mMaxNrStatPts )
		    return;
	    }
	}
    }

    if ( statpts_.size() < 1 )
	{ statpts_ += 0; statpts_ += 0; }
    else if ( statpts_.size() < 2 )
	statpts_ += statpts_[0];
}


A2DBitmapPosSetup::A2DBitmapPosSetup( const Array2DInfo& i, float* p )
    	: szdim0_(i.getSize(0))
    	, szdim1_(i.getSize(1))
	, nrxpix_(0)
	, nrypix_(0)
	, dim0pos_(0)
{
    dim1rg_.start = -0.5; dim1rg_.stop = szdim1_-0.5;
    setPositions( p );
}


A2DBitmapPosSetup::~A2DBitmapPosSetup()
{
    delete [] dim0pos_;
}


void A2DBitmapPosSetup::setPositions( float* p )
{
    if ( szdim0_ < 1 ) return;

    delete [] dim0pos_; dim0pos_ = p;

    Interval<float> posbounds;
    dim0avgdist_ = 1;
    if ( !dim0pos_ )
    {
	dim0pos_ = new float [szdim0_];
	for ( int idx=0; idx<szdim0_; idx++ )
	    dim0pos_[idx] = idx;
	posbounds.start = 0; posbounds.stop = szdim0_-1;
    }
    else
    {
	posbounds.start = dim0pos_[0]; posbounds.stop = dim0pos_[szdim0_-1];
	if ( szdim0_ > 1 )
	{
	    float totdist = 0;
	    for ( int idx=1; idx<szdim0_; idx++ )
		totdist += fabs(dim0pos_[idx] - dim0pos_[idx-1]);
	    dim0avgdist_ = totdist / (szdim0_ - 1);
	}
    }

    posbounds.sort();
    dim0rg_.start = posbounds.start - dim0avgdist_ * 0.5;
    dim0rg_.stop = posbounds.stop + dim0avgdist_ * 0.5;
}


void A2DBitmapPosSetup::setBitmapSizes( int n0, int n1 ) const
{
    A2DBitmapPosSetup& self = *(const_cast<A2DBitmapPosSetup*>(this));
    // Don't like to declare all mutable as they shld be const everyhwere else
    self.nrxpix_ = n0; self.nrypix_ = n1;
    self.dim0rg_.sort(); self.dim1rg_.sort();
    self.pixperdim0_ = nrxpix_ / dim0rg_.width();
    self.pixperdim1_ = nrypix_ / dim1rg_.width();
}


int A2DBitmapPosSetup::getPix( int dim, float pos ) const
{
    const float fpix = getPixOffs( dim, pos );

    int pix = mNINT(fpix);
    if ( pix < 0 )
	pix = 0;
    else if ( pix >= (dim?nrypix_:nrxpix_) )
	pix = (dim?nrypix_:nrxpix_) - 1;

    return pix;
}


bool A2DBitmapPosSetup::isInside( int dim, float pos ) const
{
    const float fpix = getPixOffs( dim, pos );

    return fpix + 1e-6 > 0 && fpix - 1e-6 < (dim?nrypix_:nrxpix_) - 1;
}


A2DBitmapGenerator::A2DBitmapGenerator( const A2DBitMapInpData& dat,
					const A2DBitmapPosSetup& setp,
					A2DBitmapGenPars& gp )
	: data_(dat)
    	, setup_(setp)
    	, pars_(gp)
    	, bitmap_(0)
{
}


void A2DBitmapGenerator::initBitmap()
{
    const int totsz = bitmapSize(0) * bitmapSize(1);
    if ( totsz > 0 )
	memset( bitmap_->getData(), A2DBitmapGenPars::cNoFill,
		totsz*sizeof(char) );
}


int A2DBitmapGenerator::bitmapSize( int dim ) const
{
    return bitmap_ ? bitmap_->info().getSize( dim ? 1 : 0 ) : 0;
}


void A2DBitmapGenerator::setBitmap( A2DBitMap& bm )
{
    bitmap_ = &bm;
    setup_.setBitmapSizes( bitmapSize(0), bitmapSize(1) );
}


void A2DBitmapGenerator::fill()
{
    if ( !bitmap_ ) return;
    setBitmap( *bitmap_ );
    if ( !setup_.nrXPix() || !setup_.nrYPix() )
	return;

    szdim0_ = setup_.dimSize( 0 );
    szdim1_ = setup_.dimSize( 1 );
    dim0pos_ = setup_.dim0Positions();
    dim0rg_ = setup_.dimRange( 0 );
    dim1rg_ = setup_.dimRange( 1 );

    scalerg_ = pars_.autoscale_ ? data_.scale( pars_.clipratio_ )
				: pars_.scale_;
    scalewidth_ = scalerg_.stop - scalerg_.start;

    doFill();
}
