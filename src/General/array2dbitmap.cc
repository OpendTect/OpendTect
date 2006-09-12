/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2006
-*/

static const char* rcsID = "$Id: array2dbitmap.cc,v 1.4 2006-09-12 15:44:36 cvsbert Exp $";

#include "array2dbitmapimpl.h"
#include "arraynd.h"
#include "sorting.h"
#include "interpol2d.h"
#include "stats.h"
#include <math.h>

const char A2DBitmapGenPars::cNoFill		= -127;
const char WVAA2DBitmapGenPars::cZeroLineFill	= -126;
const char WVAA2DBitmapGenPars::cWiggFill	= -125;
const char WVAA2DBitmapGenPars::cLeftFill	= -124;
const char WVAA2DBitmapGenPars::cRightFill	= -123;
const char VDA2DBitmapGenPars::cMinFill		= -120;
const char VDA2DBitmapGenPars::cMaxFill		= 120;

#define cNrFillSteps 241
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
    selectData();
    quickSort( statpts_.arr(), statpts_.size() );
}


void A2DBitMapInpData::selectData()
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
    dim0perpix_ = 1. / setup_.getPixPerDim(0);
    dim1perpix_ = 1. / setup_.getPixPerDim(1);

    scalerg_ = pars_.autoscale_ ? data_.scale( pars_.clipratio_ )
				: pars_.scale_;
    scalewidth_ = scalerg_.stop - scalerg_.start;

    doFill();
}

//---


WVAA2DBitmapGenerator::WVAA2DBitmapGenerator( const A2DBitMapInpData& d,
					      const A2DBitmapPosSetup& su )
    	: A2DBitmapGenerator(d,su,*new WVAA2DBitmapGenPars)
{
}


void WVAA2DBitmapGenerator::doFill()
{
    stripwidth_ = 2 * (0.5 + wvapars().overlap_) * setup_.avgDist(0);

    // Draw a mid line, if wanted, before anything else
    // In that way it will be overprinted by everything else
    if ( wvapars().drawmid_ )
    {
	for ( int idim0=0; idim0<szdim0_; idim0++ )
	{
	    const float middim0pos = dim0pos_[idim0];
	    if ( !setup_.isInside(0,middim0pos) )
		continue;

	    const int midpix = setup_.getPix( 0, middim0pos );
	    for ( int iy=0; iy<setup_.nrYPix(); iy++ )
		bitmap_->set( midpix, iy, WVAA2DBitmapGenPars::cZeroLineFill );
	}
    }

    for ( int idim0=0; idim0<szdim0_; idim0++ )
    {
	if ( setup_.isInside(0,dim0pos_[idim0]) )
	    drawTrace( idim0 );
    }
}


void WVAA2DBitmapGenerator::drawTrace( int idim0 )
{
    const float dim1eps = setup_.dimEps( 1 );
    int previdim1 = mUdf(int);
    float prevval = mUdf(float);
    Interpolate::PolyReg1DWithUdf<float> pr1d;
    const Array2D<float>& inpdata = data_.data();

    for ( int iy=0; iy<setup_.nrYPix(); iy++ )
    {
	const float dim1pos = dim1rg_.start + iy * dim1perpix_;
	if ( !setup_.isInside(1,dim1pos) )
	    continue;

	const int idim1 = (int)floor( dim1pos + dim1eps );
	const float dim1offs = dim1pos - idim1;

	const float v0 = inpdata.get( idim0, idim1 );
	const float v1 = idim1 < szdim1_-1
		       ? inpdata.get( idim0, idim1+1 ) : v0;
	if ( !pars_.nointerpol_ && idim1 != previdim1 )
	{
	    pr1d.set( idim1 > 0 ? inpdata.get( idim0, idim1-1 ) : v0,
		      v0,
		      v1,
		      idim1 < szdim1_-2 ? inpdata.get( idim0, idim1+2 ) : v1 );
	}

	float val = pars_.nointerpol_ ? (dim1offs < 0.5 ? v0 : v1)
	    			      : pr1d.apply( dim1offs );
	drawVal( idim0, iy, val, prevval );

	prevval = val;
	previdim1 = idim1;
    }
}


void WVAA2DBitmapGenerator::drawVal( int idim0, int iy, float val,
				     float prevval )
{
    if ( mIsUdf(val) )
	return;

    const float middim0pos = dim0pos_[idim0];
    const float stripstart = middim0pos - stripwidth_ / 2;
    const float valratio = (val - scalerg_.start) / scalewidth_;
    const float valdim0pos = stripstart + valratio * stripwidth_;

    const bool isleft = val < (wvapars().medismid_ ? data_.midVal() : 0);
    const int midpix = setup_.getPix( 0, middim0pos );
    const int valpix = setup_.getPix( 0, valdim0pos );

    if ( isleft && wvapars().fillleft_ )
    {
	for ( int ix=valpix; ix<=midpix; ix++ )
	    bitmap_->set( ix, iy, WVAA2DBitmapGenPars::cLeftFill );
    }

    if ( !isleft && wvapars().fillright_ )
    {
	for ( int ix=midpix; ix<=valpix; ix++ )
	    bitmap_->set( ix, iy, WVAA2DBitmapGenPars::cRightFill );
    }

    if ( wvapars().drawwiggles_ && setup_.isInside(0,valdim0pos) )
    {
	if ( mIsUdf(prevval) ) prevval = val;
	const float prevratio = (prevval - scalerg_.start) / scalewidth_;
	const float prevvaldim0pos = stripstart + prevratio * stripwidth_;
	const int prevvalpix = setup_.getPix( 0, prevvaldim0pos );

	int from, to;
	if ( prevvalpix < valpix )
	{
	    from = prevvalpix; to = valpix;
	    if ( from != to ) from++;
	}
	else
	{
	    from = valpix; to = prevvalpix;
	    if ( from != to ) to--;
	}

	for ( int ix=from; ix<=to; ix++ )
	    bitmap_->set( ix, iy, WVAA2DBitmapGenPars::cWiggFill );
    }
}


bool WVAA2DBitmapGenerator::dump( std::ostream& strm ) const
{
    const int nrxpix = setup_.nrXPix(); const int nrypix = setup_.nrYPix();
    if ( !bitmap_ || nrxpix == 0 || nrypix == 0 )
	return false;

    for ( int iy=0; iy<nrypix; iy++ )
    {
	for ( int ix=0; ix<nrxpix; ix++ )
	{
	    char c = bitmap_->get( ix, iy );
	    switch ( c )
	    {
	    case WVAA2DBitmapGenPars::cWiggFill:	strm << '.';	break;
	    case WVAA2DBitmapGenPars::cZeroLineFill:	strm << '|';	break;
	    case WVAA2DBitmapGenPars::cLeftFill:	strm << '(';	break;
	    case WVAA2DBitmapGenPars::cRightFill:	strm << ')';	break;
	    default:					strm << ' ';	break;
	    }
	}
	strm << '\n';
    }

    return true;
}


//---



float VDA2DBitmapGenPars::offset( char c )
{
    return (c - cMinFill) / ((float)(cNrFillSteps - 1));
}


VDA2DBitmapGenerator::VDA2DBitmapGenerator( const A2DBitMapInpData& d,
					    const A2DBitmapPosSetup& su )
    	: A2DBitmapGenerator(d,su,*new VDA2DBitmapGenPars)
{
}


void VDA2DBitmapGenerator::doFill()
{
    strippixs_ = setup_.avgDist(0) * setup_.getPixPerDim(0);

    for ( int idim0=0; idim0<szdim0_; idim0++ )
    {
	if ( setup_.isInside(0,dim0pos_[idim0]) )
	    drawStrip( idim0 );
    }
}


void VDA2DBitmapGenerator::drawStrip( int idim0 )
{
    const float curpos = dim0pos_[idim0];
    float stripmidpix = setup_.getPixOffs( 0, curpos );
    float halfstrippixs = strippixs_ / 2;
    Interval<int> pixs2do( (int)floor(stripmidpix-halfstrippixs+1e-6),
	    		   (int)ceil( stripmidpix+halfstrippixs-1e-6) );
    if ( pixs2do.start < 0 ) pixs2do.start = 0;
    if ( pixs2do.stop >= setup_.nrXPix() ) pixs2do.stop = setup_.nrXPix() - 1;

    // Check whether left neighbour has already done some pixels
    if ( idim0 > 0 )
    {
	const float prevpos = dim0pos_[idim0-1];
	while ( true )
	{
	    float startpixpos = dim0rg_.start + dim0perpix_ * pixs2do.start;
	    if ( startpixpos - prevpos < curpos - startpixpos )
		pixs2do.start++;
	    else
		break;
	}
    }
    // Check whether right neighbour should do some pixels
    if ( idim0 < szdim0_-1 )
    {
	const float nextpos = dim0pos_[idim0+1];
	while ( true )
	{
	    float stoppixpos = dim0rg_.start + dim0perpix_ * pixs2do.stop;
	    if ( stoppixpos - curpos > nextpos - stoppixpos )
		pixs2do.stop--;
	    else
		break;
	}
    }
}


#define mV00Val \
    inpdata.get( idim0, idim1 )
#define mV10Val \
    idim0 < szdim0_-1 ? inpdata.get( idim0+1, idim1 ) : v00
#define mV01Val \
    idim1 < szdim1_-1 ? inpdata.get( idim0, idim1+1 ) : v00
#define mV11Val \
    idim0 < szdim0_-1 ? (idim1 < szdim1_-1 \
		      ? inpdata.get( idim0+1, idim1+1 ) : v10) : v01

void VDA2DBitmapGenerator::drawPixLines( const Interval<int>& xpixs2do )
{
    const float dim0eps = setup_.dimEps( 0 );
    const float dim1eps = setup_.dimEps( 1 );
    int previdim1 = mUdf(int);
    Interpolate::PolyReg2DWithUdf<float> pr2d;
    const Array2D<float>& inpdata = data_.data();

    for ( int ix=xpixs2do.start; ix<=xpixs2do.stop; ix++ )
    {
	const float dim0pos = dim0rg_.start + ix * dim0perpix_;
	const int idim0 = (int)floor( dim0pos + dim0eps );
	const float dim0offs = dim0pos - idim0;

	for ( int iy=0; iy<setup_.nrYPix(); iy++ )
	{
	    const float dim1pos = dim1rg_.start + iy * dim1perpix_;
	    if ( !setup_.isInside(1,dim1pos) )
		continue;

	    const int idim1 = (int)floor( dim1pos + dim1eps );
	    const float dim1offs = dim1pos - idim1;

	    if ( !pars_.nointerpol_ && idim1 != previdim1 )
		fillInterpPars( pr2d, idim0, idim1 );

	    float val;
	    if ( !pars_.nointerpol_ )
		val = pr2d.apply( dim0offs, dim1offs );
	    else
	    {
		const float v00 = mV00Val;
		val = v00;
		if ( dim0offs > 0.5 || dim1offs > 0.5 )
		{
		    const float v10 = mV10Val;
		    const float v01 = mV01Val;
		    if ( dim0offs > 0.5 && dim1offs > 0.5 )
			val = mV11Val;
		    else
			val = dim0offs > 0.5 ? v10 : v01;
		}
	    }
	    drawVal( ix, iy, val );

	    previdim1 = idim1;
	}
    }
}


void VDA2DBitmapGenerator::fillInterpPars(
	Interpolate::PolyReg2DWithUdf<float>& pr2d, int idim0, int idim1 )
{
    const Array2D<float>& inpdata = data_.data();

    const float v00 = mV00Val;
    const float v01 = mV01Val;
    const float v10 = mV10Val;
    const float v11 = mV11Val;

    const float vm10 = idim0 > 0
		     ? inpdata.get(idim0-1,idim1) : v00;
    const float vm11 = idim0 > 0 ? (idim1 < szdim1_-1
	    	     ? inpdata.get(idim0-1,idim1+1) : vm10) : v01;
    const float v02  = idim1 < szdim1_-2
		     ? inpdata.get(idim0,idim1+2) : v01;
    const float v0m1 = idim1 > 0
		     ? inpdata.get(idim0,idim1-1) : v00;
    const float v1m1 = idim0 < szdim0_-1 ? (idim1 > 0
	    	     ? inpdata.get(idim0+1,idim1-1) : v0m1) : v10;
    const float v12  = idim0 < szdim0_-1 ? (idim1 < szdim1_-2
	    	     ? inpdata.get(idim0+1,idim1+2) : v11) : v02;
    const float v20  = idim0 < szdim0_-2
		     ? inpdata.get(idim0+2,idim1) : v10;
    const float v21  = idim0 < szdim0_-2 ? (idim1 < szdim1_-1
	    	     ? inpdata.get(idim0+2,idim1+1) : v10) : v20;

    pr2d.set( vm10, vm11, v0m1, v00, v01, v02, v1m1, v10, v11, v12, v20, v21 );
}


void VDA2DBitmapGenerator::drawVal( int ix, int iy, float val )
{
    if ( mIsUdf(val) )
	return;

    const float valratio = (val - scalerg_.start) / scalewidth_;
    const char bmval = (char)(VDA2DBitmapGenPars::cMinFill
	    		      + valratio * cNrFillSteps - .5);
    bitmap_->set( ix, iy, bmval );
}


bool VDA2DBitmapGenerator::dump( std::ostream& strm ) const
{
    const int nrxpix = setup_.nrXPix(); const int nrypix = setup_.nrYPix();
    if ( !bitmap_ || nrxpix == 0 || nrypix == 0 )
	return false;

    for ( int iy=0; iy<nrypix; iy++ )
    {
	for ( int ix=0; ix<nrxpix; ix++ )
	{
	    int c = ((int)bitmap_->get(ix,iy)) - VDA2DBitmapGenPars::cMinFill;
	    if ( c < 0 || c >= cNrFillSteps )
		strm << ' ';
	    else
	    {
		char out = (char)(33 + 90 * VDA2DBitmapGenPars::offset(c) + .5);
		strm << out;
	    }
	}
	strm << '\n';
    }

    return true;
}
