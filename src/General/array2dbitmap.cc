/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2006
-*/

static const char* rcsID = "$Id: array2dbitmap.cc,v 1.11 2006-10-04 17:17:03 cvsbert Exp $";

#include "array2dbitmapimpl.h"
#include "arraynd.h"
#include "sorting.h"
#include "interpol2d.h"
#include "statrand.h"
#include "envvars.h"
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
#define mXPMStartLn '"'
#define mXPMEndLn "\",\n"


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
	Stats::RandGen::init();
	for ( int itry=0; itry<maxtries; itry++ )
	{
	    const int totidx = Stats::RandGen::getIndex( totnrsamps );
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
    if ( !dim0pos_ || mIsEqual( dim0pos_[0], dim0pos_[szdim0_-1], 1e-8 ) )
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
    self.pixperdim0_ = (nrxpix_-1) / dim0rg_.width();
    self.pixperdim1_ = (nrypix_-1) / dim1rg_.width();
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
    if ( mIsZero(scalewidth_,1e-8) )
    {
	scalewidth_ = 1e-8;
	scalerg_.start -= scalewidth_ / 2;
	scalerg_.stop += scalewidth_ / 2;
    }

    doFill();
}


static inline int gtPrettyBMVal( char c )
{
    static const float rgmax = 1000;
    float v = (c - VDA2DBitmapGenPars::cMinFill) * (rgmax + 1)
	    / (VDA2DBitmapGenPars::cMaxFill-VDA2DBitmapGenPars::cMinFill) - .5;
    int ret = mNINT(v);
    return ret < 0 ? 0 : (ret > rgmax+.5 ? (int)rgmax : ret);
}

bool A2DBitmapGenerator::dump( std::ostream& strm ) const
{
    const int nrxpix = setup_.nrXPix(); const int nrypix = setup_.nrYPix();
    if ( !bitmap_ || nrxpix == 0 || nrypix == 0 )
	return false;

    if ( !GetEnvVarYN("OD_DUMP_A2DBITMAP_AS_NUMBERS" ) && dumpXPM(strm) )
	return true;

    for ( int iy=0; iy<nrypix; iy++ )
    {
	strm << gtPrettyBMVal( bitmap_->get(0,iy) );
	for ( int ix=1; ix<nrxpix; ix++ )
	    strm << '\t' << gtPrettyBMVal( bitmap_->get(ix,iy) );
	strm << std::endl;
    }
    return true;
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

#define mApplyValClipping(val) \
    if ( val < scalerg_.start ) val = scalerg_.start; \
    else if ( val > scalerg_.stop ) val = scalerg_.stop

#define mPrepVal() \
    if ( mIsUdf(val) ) return; \
    mApplyValClipping(val)


void WVAA2DBitmapGenerator::drawVal( int idim0, int iy, float val,
				     float prevval )
{
    mPrepVal();

    const float middim0pos = dim0pos_[idim0];

    const float midval = (wvapars().medismid_ ? data_.midVal() : 0);
    const float valratio = (val - midval) / scalewidth_;
    const float valdim0pos = middim0pos + valratio * stripwidth_;

    const bool isleft = val< midval;
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
	mApplyValClipping( prevval );
	const float prevratio = (prevval - midval) / scalewidth_;
	const float prevvaldim0pos = middim0pos + prevratio * stripwidth_;
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


bool WVAA2DBitmapGenerator::dumpXPM( std::ostream& strm ) const
{
    const int nrxpix = setup_.nrXPix(); const int nrypix = setup_.nrYPix();

    strm << "/* XPM */\nstatic char*wva[]={\n";
    strm << '"' << nrxpix << ' ' << nrypix << ' ' << "5 1"
		<< mXPMEndLn;
    strm << "\"l c #0000ff" << mXPMEndLn;
    strm << "\"r c #ff0000" << mXPMEndLn;
    strm << "\"0 c #00ff00" << mXPMEndLn;
    strm << "\"w c #000000" << mXPMEndLn;
    strm << "\"e c #ffffff" << mXPMEndLn;

    for ( int iy=0; iy<nrypix; iy++ )
    {
	strm << mXPMStartLn;
	for ( int ix=0; ix<nrxpix; ix++ )
	{
	    char c = bitmap_->get( ix, iy );
	    switch ( c )
	    {
	    case WVAA2DBitmapGenPars::cWiggFill:	strm << 'w';	break;
	    case WVAA2DBitmapGenPars::cZeroLineFill:	strm << '0';	break;
	    case WVAA2DBitmapGenPars::cLeftFill:	strm << 'l';	break;
	    case WVAA2DBitmapGenPars::cRightFill:	strm << 'r';	break;
	    default:					strm << 'e';	break;
	    }
	}
	strm << mXPMEndLn;
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
    const float avgdim0dist = setup_.avgDist( 0 );
    strippixs_ = avgdim0dist * setup_.getPixPerDim(0);

    for ( int idim0=0; idim0<szdim0_; idim0++ )
    {
	float pos2chk = dim0pos_[idim0];
	if ( pos2chk < dim0rg_.start ) pos2chk += avgdim0dist*.5;
	if ( pos2chk > dim0rg_.stop ) pos2chk -= avgdim0dist*.5;

	if ( setup_.isInside(0,pos2chk) )
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

    drawPixLines( idim0, pixs2do );
}


#define mV00Val \
    inpdata.get( idim0, idim1 )
#define mV10Val \
    idim0 < szdim0_-1 ? inpdata.get( idim0+1, idim1 ) : v[0]
#define mV01Val \
    idim1 < szdim1_-1 ? inpdata.get( idim0, idim1+1 ) : v[0]
#define mV11Val \
    idim0 < szdim0_-1 ? (idim1 < szdim1_-1 \
		      ? inpdata.get( idim0+1, idim1+1 ) : v[2]) : v[1]

void VDA2DBitmapGenerator::drawPixLines( int stripdim0,
					 const Interval<int>& xpixs2do )
{
    const float dim0eps = setup_.dimEps( 0 );
    const float dim1eps = setup_.dimEps( 1 );
    int previdim1 = mUdf(int);

    PtrMan< Interpolate::Applier2D<float> > interp = 0;
    if ( vdpars().lininterp_ )
	interp = new Interpolate::LinearReg2DWithUdf<float>;
    else
    {
	const float pixperval0 = setup_.nrXPix() / ((float)szdim0_);
	const float pixperval1 = setup_.nrYPix() / ((float)szdim1_);
	const float xstretch = pixperval0 / pixperval1;
	interp = new Interpolate::PolyReg2DWithUdf<float>( xstretch );
    }
    const Array2D<float>& inpdata = data_.data();

    for ( int ix=xpixs2do.start; ix<=xpixs2do.stop; ix++ )
    {
	const float dim0pos = dim0rg_.start + ix * dim0perpix_;
	int idim0 = stripdim0;
	if ( dim0pos_[stripdim0] > dim0pos && stripdim0 > 0 )
	    idim0--;
	const float v0dim0pos = dim0pos_[idim0];
	const float v1dim0pos = idim0<szdim0_-1 ? dim0pos_[idim0+1] : dim0pos+1;
	const float dim0offs = (dim0pos-v0dim0pos) / (v1dim0pos-v0dim0pos);

	for ( int iy=0; iy<setup_.nrYPix(); iy++ )
	{
	    const float dim1pos = dim1rg_.start + iy * dim1perpix_;
	    if ( !setup_.isInside(1,dim1pos) )
		continue;

	    const int idim1 = (int)floor( dim1pos + dim1eps );
	    const float dim1offs = dim1pos - idim1;

	    if ( !pars_.nointerpol_ && idim1 != previdim1 )
		fillInterpPars( *interp, idim0, idim1 );

	    float val;
	    if ( !pars_.nointerpol_ )
		val = interp->apply( dim0offs, dim1offs );
	    else
	    {
		float v[4]; val = v[0] = mV00Val;
		if ( dim0offs > 0.5 || dim1offs > 0.5 )
		{
		    v[1] = mV01Val; v[2] = mV10Val;
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
}


void VDA2DBitmapGenerator::fillInterpPars(
	Interpolate::Applier2D<float>& interp, int idim0, int idim1 )
{
    float v[12];
    const Array2D<float>& inpdata = data_.data();

    v[0] = mV00Val;
    v[1] = mV01Val;
    v[2] = mV10Val;
    v[3] = mV11Val;

    if ( !vdpars().lininterp_ )
    {
	v[4] = idim0 > 0
			 ? inpdata.get(idim0-1,idim1) : v[0];
	v[5] = idim0 > 0 ? (idim1 < szdim1_-1
			 ? inpdata.get(idim0-1,idim1+1) : v[4]) : v[1];
	v[6] = idim1 > 0
			 ? inpdata.get(idim0,idim1-1) : v[0];
	v[7]  = idim1 < szdim1_-2
			 ? inpdata.get(idim0,idim1+2) : v[1];
	v[8] = idim0 < szdim0_-1 ? (idim1 > 0
			 ? inpdata.get(idim0+1,idim1-1) : v[4]) : v[2];
	v[9]  = idim0 < szdim0_-1 ? (idim1 < szdim1_-2
			 ? inpdata.get(idim0+1,idim1+2) : v[3]) : v[7];
	v[10]  = idim0 < szdim0_-2
			 ? inpdata.get(idim0+2,idim1) : v[2];
	v[11]  = idim0 < szdim0_-2 ? (idim1 < szdim1_-1
			 ? inpdata.get(idim0+2,idim1+1) : v[2]) : v[10];
    }

    interp.set( v );
}


void VDA2DBitmapGenerator::drawVal( int ix, int iy, float val )
{
    mPrepVal();

    const float valratio = (val - scalerg_.start) / scalewidth_;
    const char bmval = (char)(VDA2DBitmapGenPars::cMinFill
	    		      + valratio * cNrFillSteps - .5);
    bitmap_->set( ix, iy, bmval );
}


static void getColValHex( int idx, char* ptr )
{
    int major = idx / 15;
    int minor = idx % 16;
    ptr[0] = major < 10 ? '0' + major : 'a' + major - 10;
    ptr[1] = minor < 10 ? '0' + minor : 'a' + minor - 10;
}


bool VDA2DBitmapGenerator::dumpXPM( std::ostream& strm ) const
{
    const int nrxpix = setup_.nrXPix(); const int nrypix = setup_.nrYPix();
    const float fac = ((float)51) / (cNrFillSteps - 1);
    char prevc = -1; int nrcols = 0;
    for ( int idx=0; idx<cNrFillSteps; idx++ )
    {
	char c = (char)(fac * idx + .5);
	if ( c != prevc ) nrcols++;
	prevc = c;
    }

    strm << "/* XPM */\nstatic char*vd[]={\n";
    strm << '"' << nrxpix << ' ' << nrypix << ' ' << nrcols+1 << " 1"
		<< mXPMEndLn;
    strm << '"' << ". c #00ff00" << mXPMEndLn;

    char buf[7];
    for ( int idx=0; idx<cNrFillSteps; idx++ )
    {
	char c = (char)(fac * idx + .5);
	if ( c == prevc ) continue;
	prevc = c;

	if ( c < 26 )	c += 'a';
	else		c += 'A' - 26;

	getColValHex( idx, buf );
	getColValHex( idx, buf+2 );
	getColValHex( idx, buf+4 );
	buf[6] = '\0';

	strm << '"' << c << " c #" << buf << mXPMEndLn;
    }

    for ( int iy=0; iy<nrypix; iy++ )
    {
	strm << mXPMStartLn;
	for ( int ix=0; ix<nrxpix; ix++ )
	{
	    int c = ((int)bitmap_->get(ix,iy)) - VDA2DBitmapGenPars::cMinFill;
	    if ( c < 0 || c >= cNrFillSteps )
		strm << '.';
	    else
	    {
		char out = (char)(fac * c + .5);
		if ( out < 26 )	out += 'a';
		else		out += 'A' - 26;
		strm << out;
	    }
	}
	strm << mXPMEndLn;
    }

    return true;
}
