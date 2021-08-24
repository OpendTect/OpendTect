/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2006
-*/


#include "array2dbitmapimpl.h"
#include "interpol2d.h"
#include "statruncalc.h"

char A2DBitMapGenPars::cNoFill()		{ return -127; }
char WVAA2DBitMapGenPars::cRefLineFill()	{ return -126; }
char WVAA2DBitMapGenPars::cWiggFill()		{ return -125; }
char WVAA2DBitMapGenPars::cLowFill()		{ return -124; }
char WVAA2DBitMapGenPars::cHighFill()		{ return -123; }
char VDA2DBitMapGenPars::cMinFill()		{ return -120; }
char VDA2DBitMapGenPars::cMaxFill()		{ return 120; }

#define cNrFillSteps 241
#define mMaxNrStatPts 5000
#define mXPMStartLn '"'
#define mXPMEndLn "\",\n"


Interval<float> A2DBitMapInpData::scale( const Interval<float>& clipratio,
					 float midval ) const
{
    Interval<float> res;
    if ( mIsUdf(midval) )
    {
	if ( mIsUdf(clipratio.stop) )
	    clipper_.getRange( clipratio.start, res );
	else
	    clipper_.getRange( clipratio.start, clipratio.stop, res );
    }
    else
	clipper_.getSymmetricRange( clipratio.start, midval, res );

    return res;
}


float A2DBitMapInpData::midVal() const
{
    const LargeValVec<float>& statpts = clipper_.statPts();
    return statpts.size() ? statpts[statpts.size()/2] : mUdf(float);
}


void A2DBitMapInpData::collectData()
{
    clipper_.reset();
    clipper_.setApproxNrValues( data_.info().getTotalSz(), mMaxNrStatPts );
    clipper_.putData( data_ );
    clipper_.fullSort();
}


A2DBitMapPosSetup::A2DBitMapPosSetup( const Array2DInfo& i, float* p )
	: szdim0_(i.getSize(0))
	, szdim1_(i.getSize(1))
	, nrxpix_(0)
	, nrypix_(0)
	, dim0rg_(0,0)
	, dim1rg_(0,0)
	, dim0pos_(0)
	, dim0mediandist_(1.0f)
{
    setDim0Positions( p );
    setDim1Positions( 0.f, (float)szdim1_-1 );
}


A2DBitMapPosSetup::~A2DBitMapPosSetup()
{
    delete [] dim0pos_;
}


void A2DBitMapPosSetup::setDim0Positions( float* p )
{
    if ( szdim0_ < 1 ) return;

    delete [] dim0pos_; dim0pos_ = p;

    if ( !dim0pos_ )
    {
	dim0pos_ = new float [szdim0_];
	for ( int idx=0; idx<szdim0_; idx++ )
	    dim0pos_[idx] = (float)idx;
    }


    Interval<float> posbounds( dim0pos_[0], dim0pos_[0] );
    for ( int idx=1; idx<szdim0_; idx++ )
	posbounds.include( dim0pos_[idx] );

    if ( dim0pos_[0] != dim0pos_[szdim0_-1] )
    {
	TypeSet<float> dists;
	for ( int idx=1; idx<szdim0_; idx++ )
	    dists += fabs(dim0pos_[idx] - dim0pos_[idx-1]);

	if ( !dists.isEmpty() )
	{
	    Stats::RunCalc<float> runcalc(
		    Stats::CalcSetup().require(Stats::Median) );
	    runcalc.addValues( dists.size(), dists.arr() );
	    dim0mediandist_ = runcalc.median();
	}
    }

    posbounds.sort();
    dim0rg_.start = posbounds.start - dim0mediandist_ * 0.5f;
    dim0rg_.stop = posbounds.stop + dim0mediandist_ * 0.5f;
}


void A2DBitMapPosSetup::setDim1Positions( float start, float stop )
{
    dim1pos_.start = start; dim1pos_.stop = stop;
    dim1pos_.sort();
    const float dim1avgdist = szdim1_>1 ? dim1pos_.width() / (szdim1_ - 1) : 1;
    dim1rg_.start = dim1pos_.start - dim1avgdist * 0.5f;
    dim1rg_.stop = dim1pos_.stop + dim1avgdist * 0.5f;
}


void A2DBitMapPosSetup::setPixSizes( int xpix, int ypix )
{
    availablenrxpix_ = xpix;
    availablenrypix_ = ypix;
}


void A2DBitMapPosSetup::setBitMapSizes( int n0, int n1 ) const
{
    A2DBitMapPosSetup& self = *(const_cast<A2DBitMapPosSetup*>(this));
    // Don't like to declare all mutable as they shld be const everyhwere else
    self.nrxpix_ = n0; self.nrypix_ = n1;
    self.dim0rg_.sort(); self.dim1rg_.sort();
    self.pixperdim0_ = nrxpix_ / dim0rg_.width();
    self.pixperdim1_ = nrypix_ / dim1rg_.width();
}


int A2DBitMapPosSetup::getPix( int dim, float pos ) const
{
    const float fpix = getPixOffs( dim, pos );

    int pix = mNINT32(fpix);
    if ( pix < 0 )
	pix = 0;
    else if ( pix >= (dim?nrypix_:nrxpix_) )
	pix = (dim?nrypix_:nrxpix_) - 1;

    return pix;
}


bool A2DBitMapPosSetup::isInside( int dim, float pos ) const
{
    const float fpix = getPixOffs( dim, pos );

    return fpix + 1e-6 > 0 && fpix - 1e-6 < (dim?nrypix_:nrxpix_) - 1;
}


A2DBitMapGenerator::A2DBitMapGenerator( const A2DBitMapInpData& dat,
					const A2DBitMapPosSetup& setp,
					A2DBitMapGenPars& gp )
	: data_(dat)
	, setup_(setp)
	, pars_(gp)
	, bitmap_(0)
{
}


void A2DBitMapGenerator::initBitMap( A2DBitMap& bm )
{
    const od_uint64 totsz = bm.info().getTotalSz();
    if ( totsz > 0 )
	OD::memSet( bm.getData(), A2DBitMapGenPars::cNoFill(),
			totsz*sizeof(char) );
}


int A2DBitMapGenerator::bitmapSize( int dim ) const
{
    return bitmap_ ? bitmap_->info().getSize( dim ? 1 : 0 ) : 0;
}


void A2DBitMapGenerator::setBitMap( A2DBitMap& bm )
{
    bitmap_ = &bm;
    setup_.setBitMapSizes( bitmapSize(0), bitmapSize(1) );
}


void A2DBitMapGenerator::setPixSizes( int xpix, int ypix )
{
    A2DBitMapPosSetup& su = const_cast<A2DBitMapPosSetup&>(setup_);
    su.setPixSizes( xpix, ypix );
}


void A2DBitMapGenerator::fill()
{
    if ( !bitmap_ ) return;
    setBitMap( *bitmap_ );
    if ( !setup_.nrXPix() || !setup_.nrYPix() )
	return;

    szdim0_ = setup_.dimSize( 0 );
    szdim1_ = setup_.dimSize( 1 );
    dim0pos_ = setup_.dim0Positions();
    dim1pos_ = setup_.dim1Positions();
    dim0rg_ = setup_.dimRange( 0 );
    dim1rg_ = setup_.dimRange( 1 );
    dim0perpix_ = 1.f / setup_.getPixPerDim(0);
    dim1perpix_ = 1.f / setup_.getPixPerDim(1);

    scalerg_ = pars_.autoscale_ && pars_.scale_.isUdf()
		    ? data_.scale( pars_.clipratio_, pars_.midvalue_ )
		    : pars_.scale_;
    pars_.scale_ = scalerg_;
    scalewidth_ = scalerg_.stop - scalerg_.start;
    if ( mIsZero(scalewidth_,1e-8) )
    {
	scalewidth_ = 1e-8;
	scalerg_.start -= scalewidth_ / 2;
	scalerg_.stop += scalewidth_ / 2;
    }

    doFill();
}


//---


WVAA2DBitMapGenerator::WVAA2DBitMapGenerator( const A2DBitMapInpData& d,
					      const A2DBitMapPosSetup& su )
	: A2DBitMapGenerator(d,su,*new WVAA2DBitMapGenPars)
{
}


Interval<int> WVAA2DBitMapGenerator::getDispTrcIdxs() const
{
    Interval<float> dim0rg = setup_.dimRange( 0 );
    dim0rg.widen( mNINT32(1 + wvapars().overlap_) * setup_.dim0MedianDist() );

    Interval<int> disptrcs( mUdf(int), -mUdf(int) );
    for ( int idim0=0; idim0<szdim0_; idim0++ )
	if ( dim0rg.includes(dim0pos_[idim0],true) )
	    disptrcs.include( idim0, false );
    return disptrcs;
}


float WVAA2DBitMapGenerator::getDim0Offset( float val ) const
{
    const float valratio = (val - scalerg_.start) / scalewidth_;
    return (valratio - 0.5f) * stripwidth_ * (wvapars().x1reversed_ ? -1 : 1);
}


int WVAA2DBitMapGenerator::dim0SubSampling( int nrdisptrcs ) const
{
    const float nrpixperdim0 = setup_.availableXPix() / mCast(float,nrdisptrcs);
    const int minpixperdim0 = gtPars().minpixperdim0_;
    const float fret = minpixperdim0 / nrpixperdim0;
    const int ret = mNINT32( Math::Ceil(fret) );
    return ret < 2 ? 1 : ret;
}


void WVAA2DBitMapGenerator::doFill()
{
    const Interval<int> trcidxs = getDispTrcIdxs();
    const int nrtrcs = trcidxs.width()+1;
    const int dispeach = dim0SubSampling( nrtrcs );
    stripwidth_ = (1 + wvapars().overlap_) * dispeach * setup_.dim0MedianDist();

    for ( int idx=0; idx<=trcidxs.width(); idx++ )
    {
	const int idim0 = trcidxs.atIndex( idx, dispeach );
	if ( idim0 > trcidxs.stop )
	    break;
	drawTrace( idim0 );
    }
}


void WVAA2DBitMapGenerator::drawTrace( int idim0 )
{
    int previdim1 = mUdf(int);
    float prevval = mUdf(float);
    Interpolate::PolyReg1DWithUdf<float> pr1d;
    const Array2D<float>& inpdata = data_.data();

    float midval = wvapars().midvalue_;
    if ( mIsUdf(midval) ) midval = data_.midVal();
    const float middim0pos = dim0pos_[idim0] + getDim0Offset(midval);
    const float dim1wdth = dim1pos_.width();
    const float dim1fac = (szdim1_ - 1) / (dim1wdth ? dim1wdth : 1);

    for ( int iy=0; iy<setup_.nrYPix(); iy++ )
    {
	const float dim1pos = dim1rg_.start + iy * dim1perpix_;
	if ( !setup_.isInside(1,dim1pos) )
	    continue;

	const float fdim1 = (dim1pos - dim1pos_.start) * dim1fac;
	const int idim1 = (int)Math::Floor( fdim1 + 1e-6 );
	const float dim1offs = fdim1 - idim1;

	if ( idim1<0 || idim1>=szdim1_ )
	    continue;

	const float v0 = inpdata.get( idim0, idim1 );
	const float v1 = idim1 < szdim1_-1
		       ? inpdata.get( idim0, idim1+1 ) : v0;
	float val = dim1offs < 0.5 ? v0 : v1;
	if ( !pars_.nointerpol_ )
	{
	    if ( idim1 != previdim1 )
		pr1d.set( idim1 > 0 ? inpdata.get( idim0, idim1-1 ) : v0,
			  v0, v1,
		      idim1 < szdim1_-2 ? inpdata.get( idim0, idim1+2 ) : v1 );
	    val = pr1d.apply( dim1offs );
	}

	drawVal( idim0, iy, val, prevval, midval, middim0pos );

	prevval = val;
	previdim1 = idim1;
    }
}

#define mPrepVal() \
    if ( mIsUdf(val) ) return; \
    val = scalerg_.limitValue( val )


void WVAA2DBitMapGenerator::drawVal( int idim0, int iy, float val,
				     float prevval,
				     float midval, float middim0pos )
{
    mPrepVal();

    const float centerdim0pos = dim0pos_[idim0];
    const float valdim0pos = centerdim0pos + getDim0Offset(val);

    const bool& x1reversed = wvapars().x1reversed_;
    const bool isleft = x1reversed ? val>midval : val<midval;
    const int midpix = setup_.getPix( 0, middim0pos );
    const int valpix = setup_.getPix( 0, valdim0pos );

    if ( isleft && (x1reversed ? wvapars().fillhigh_ : wvapars().filllow_) )
    {
	const char leftfill = x1reversed ? WVAA2DBitMapGenPars::cHighFill()
					 : WVAA2DBitMapGenPars::cLowFill();
	for ( int ix=valpix; ix<=midpix; ix++ )
	    bitmap_->set( ix, iy, leftfill );
    }

    if ( !isleft && (x1reversed ? wvapars().filllow_ : wvapars().fillhigh_) )
    {
	const char rightfill = x1reversed ? WVAA2DBitMapGenPars::cLowFill()
					  : WVAA2DBitMapGenPars::cHighFill();
	for ( int ix=midpix; ix<=valpix; ix++ )
	    bitmap_->set( ix, iy, rightfill );
    }

    if ( wvapars().drawrefline_ )
    {
	int refpix = midpix;
	const float reflineval = wvapars().reflinevalue_;
	if ( !mIsUdf(reflineval) )
	{
	    const float refdim0pos = centerdim0pos + getDim0Offset(reflineval);
	    refpix = setup_.getPix( 0, refdim0pos );
	}

	if ( refpix>=0 && refpix<setup_.nrXPix() )
	    bitmap_->set( refpix, iy, WVAA2DBitMapGenPars::cRefLineFill() );
    }

    if ( wvapars().drawwiggles_ && setup_.isInside(0,valdim0pos) )
    {
	if ( mIsUdf(prevval) ) prevval = val;
	prevval = scalerg_.limitValue( prevval );

	const float prevvaldim0pos = centerdim0pos + getDim0Offset(prevval);
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
	    bitmap_->set( ix, iy, WVAA2DBitMapGenPars::cWiggFill() );
    }
}

//---



float VDA2DBitMapGenPars::offset( char c )
{
    return (c - cMinFill()) / ((float)(cNrFillSteps - 1));
}


VDA2DBitMapGenerator::VDA2DBitMapGenerator( const A2DBitMapInpData& d,
					    const A2DBitMapPosSetup& su )
	: A2DBitMapGenerator(d,su,*new VDA2DBitMapGenPars)
{
}


void VDA2DBitMapGenerator::doFill()
{
    const float mediandim0dist = setup_.dim0MedianDist();
    strippixs_ = mediandim0dist * setup_.getPixPerDim(0);

    stripstodraw_.erase();

    for ( int idim0=0; idim0<szdim0_; idim0++ )
    {
	float pos2chk = dim0pos_[idim0];
	if ( pos2chk < dim0rg_.start ) pos2chk += mediandim0dist*.5f;
	if ( pos2chk > dim0rg_.stop ) pos2chk -= mediandim0dist*.5f;

	if ( setup_.isInside(0,pos2chk) )
	    stripstodraw_ += idim0;
    }

    execute();
}


bool VDA2DBitMapGenerator::doWork( od_int64 start, od_int64 stop, int )
{
    for ( int idx=mCast(int,start); idx<=stop; idx++ )
    {
	drawStrip( stripstodraw_[idx] );
    }

    return true;
}


od_int64 VDA2DBitMapGenerator::nrIterations() const
{ return stripstodraw_.size(); }



void VDA2DBitMapGenerator::drawStrip( int idim0 )
{
    const float curpos = dim0pos_[idim0];
    float stripmidpix = setup_.getPixOffs( 0, curpos );
    float halfstrippixs = strippixs_ / 2;
    Interval<int> pixs2do( (int)Math::Floor(stripmidpix-halfstrippixs+1e-6),
			   (int)Math::Ceil( stripmidpix+halfstrippixs-1e-6) );
    if ( pixs2do.start < 0 ) pixs2do.start = 0;
    if ( pixs2do.stop >= setup_.nrXPix() ) pixs2do.stop = setup_.nrXPix() - 1;

    // The problem is: some of the 'to do' pixels may in fact be nearer a
    // neighbouring position. That doesn't only cost performance, it
    // will also introduce a directional overprinting.
    // Therefore, the pixs2do we have now are the maximum pixs2do

    if ( idim0 > 0 )
    {
	const float prevpos = dim0pos_[idim0-1];
	float prevstripmidpix = setup_.getPixOffs( 0, prevpos );
	float halfwaypix = (prevstripmidpix + stripmidpix) * .5f;
	if ( halfwaypix > pixs2do.start )
	    pixs2do.start = (int)Math::Ceil(halfwaypix-1e-6);
    }
    if ( idim0 < szdim0_-1 )
    {
	const float nextpos = dim0pos_[idim0+1];
	float nextstripmidpix = setup_.getPixOffs( 0, nextpos );
	float halfwaypix = (nextstripmidpix + stripmidpix) * .5f;
	if ( halfwaypix < pixs2do.stop )
	    pixs2do.stop = (int)Math::Floor(halfwaypix-1e-6);
    }

    drawPixLines( idim0, pixs2do );
}


#define mV00Val \
    inpdata.info().validPos( idim0, idim1 ) \
	? inpdata.get( idim0, idim1 ) \
	: mUdf(float);
#define mV10Val \
    inpdata.info().validPos( idim0+1, idim1 ) \
	? inpdata.get( idim0+1, idim1 ) \
	: v[0]
#define mV01Val \
    inpdata.info().validPos( idim0, idim1+1 ) \
	? inpdata.get( idim0, idim1+1 ) \
	: v[0]
#define mV11Val \
    inpdata.info().validDimPos( 0, idim0+1 ) \
	? (inpdata.info().validDimPos( 1, idim1+1 ) \
		? inpdata.get( idim0+1, idim1+1 ) \
		: v[2]) \
	: v[1]


void VDA2DBitMapGenerator::drawPixLines( int stripdim0,
					 const Interval<int>& xpixs2do )
{
    Interpolate::Applier2D<float>* interp = 0;
    if ( !pars_.nointerpol_ )
    {
	if ( vdpars().lininterp_ )
	    interp = new Interpolate::LinearReg2DWithUdf<float>;
	else
	{
	    const float pixperval0 = setup_.nrXPix() / ((float)szdim0_);
	    const float pixperval1 = setup_.nrYPix() / ((float)szdim1_);
	    const float xstretch = pixperval0 / pixperval1;
	    interp = new Interpolate::PolyReg2DWithUdf<float>( xstretch );
	}
    }

    const Array2D<float>& inpdata = data_.data();
    const float dim1wdth = dim1pos_.width();
    const float dim1fac = (szdim1_ - 1) / (dim1wdth ? dim1wdth : 1);
    int previdim1 = mUdf(int);

    for ( int ix=xpixs2do.start; ix<=xpixs2do.stop; ix++ )
    {
	const float dim0pos = dim0rg_.start + ix * dim0perpix_;
	int idim0 = stripdim0;
	if ( dim0pos_[stripdim0] > dim0pos && stripdim0 > 0 )
	    idim0--;
	const float v0dim0pos = dim0pos_[idim0];
	const float v1dim0pos = idim0<szdim0_-1 ? dim0pos_[idim0+1] : dim0pos+1;
	float denom = v1dim0pos-v0dim0pos;
	if ( mIsZero(denom,mDefEpsF) )
	    denom = 1;
	const float dim0offs = (dim0pos-v0dim0pos) / denom;

	for ( int iy=0; iy<setup_.nrYPix(); iy++ )
	{
	    const float dim1pos = dim1rg_.start + iy*dim1perpix_;
	    if ( !setup_.isInside(1,dim1pos) )
		continue;

	    const float fdim1 = (dim1pos - dim1pos_.start) * dim1fac;
	    int idim1 = (int)Math::Floor( fdim1 + 1e-6 );
	    const float dim1offs = fdim1 - idim1;

	    if ( interp && idim1 != previdim1 )
		fillInterpPars( *interp, idim0, idim1 );

	    float val;
	    if ( interp )
		val = interp->apply( dim0offs, dim1offs );
	    else
	    {
		if ( idim0 < 0 ) idim0 = 0;
		if ( idim1 < 0 ) idim1 = 0;
		if ( idim0 >= inpdata.info().getSize(0) )
		    idim0 = inpdata.info().getSize(0)-1;
		if ( idim1 >= inpdata.info().getSize(1) )
		    idim1 = inpdata.info().getSize(1)-1;
		if ( dim0offs <= 0.5 && dim1offs <= 0.5 )
		    { val = mV00Val; }
		else
		{
		    float v[4];
		    v[0] = mV00Val; v[1] = mV01Val; v[2] = mV10Val;
		    if ( dim0offs > 0.5 && dim1offs > 0.5 )
			val = mV11Val;
		    else
			val = dim0offs > 0.5 ? v[2] : v[1];
		}
	    }

	    drawVal( ix, iy, val );

	    previdim1 = idim1;
	}
    }
    delete interp;
}


#define mGetAll \
{ \
    v[0] = mGet( 0, 0 ); \
    v[1] = mGet( 0, 1 ); \
    v[2] = mGet( 1, 0 ); \
    v[3] = mGet( 1, 1 ); \
 \
    if ( !vdpars().lininterp_ ) \
    { \
	v[4] = mGet( -1, 0 ); \
	v[5] = mGet( -1, 1 ); \
	v[6] = mGet( 0, -1 ); \
	v[7] = mGet( 0, 2 ); \
	v[8] = mGet( 1, -1 ); \
	v[9] = mGet( 1, 2 ); \
	v[10] = mGet( 2, 0 ); \
	v[11] = mGet( 2, 1 ); \
    } \
}


void VDA2DBitMapGenerator::fillInterpPars(
	Interpolate::Applier2D<float>& interp, int idim0, int idim1 )
{
    float v[12];
    const Array2D<float>& inpdata = data_.data();
    const ValueSeries<float>* storage = inpdata.getStorage();
    const float* storageptr = storage ? storage->arr() : 0;

    if ( storageptr )
    {
	storageptr += inpdata.info().getOffset( idim0, idim1 );
#define mGet( i0, i1 ) \
    (idim0+i0)<szdim0_ && (idim0+i0)>=0 && (idim1+i1)<szdim1_ && (idim1+i1)>=0 \
	? storageptr[i0*szdim1_+i1] : mUdf(float);

	mGetAll;
#undef mGet
    }
    else if ( storage )
    {
	const od_int64 offset =
	    mCast(od_int64,inpdata.info().getOffset(idim0,idim1));
#define mGet( i0, i1 ) \
    (idim0+i0)<szdim0_ && (idim0+i0)>=0 && (idim1+i1)<szdim1_ && (idim1+i1)>=0 \
	? storage->value(offset+i0*szdim1_+i1) : mUdf(float);
	mGetAll;
#undef mGet
    }
    else
    {
#define mGet( i0, i1 ) \
    (idim0+i0)<szdim0_ && (idim0+i0)>=0 && (idim1+i1)<szdim1_ && (idim1+i1)>=0 \
	? inpdata.get( idim0+i0, idim1+i1 ) : mUdf(float);
	mGetAll;
#undef mGet
    }

    interp.set( v );
}


void VDA2DBitMapGenerator::drawVal( int ix, int iy, float val )
{
    mPrepVal();

    const float valratio = (val - scalerg_.start) / scalewidth_;
    const char bmval = (char)(VDA2DBitMapGenPars::cMinFill()
			      + valratio * cNrFillSteps - .5);
    bitmap_->set( ix, iy, bmval );
}

