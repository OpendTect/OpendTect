/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		June 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "hilberttransform.h"

#include "genericnumer.h"
#include "arraynd.h"
#include "arrayndimpl.h"
#include "arrayndinfo.h"
#include "odmemory.h"
#include "uistrings.h"
#include "valseries.h"


HilbertTransform::HilbertTransform()
    : info_(0)
    , forward_(true)
    , nrsamples_(0)
    , halflen_(30)
    , hilbwindow_(0)
    , startidx_(0)
    , convstartidx_(0)
{}


HilbertTransform::~HilbertTransform()
{
    delete info_;
    delete [] hilbwindow_;
}


bool HilbertTransform::setInputInfo( const ArrayNDInfo& info )
{
    if ( info_ ) delete info_;
    info_ = info.clone();

    startidx_ = 0;
    nrsamples_ = info.getSize( 0 );

    return true;
}


bool HilbertTransform::isPossible( int sz ) const
{
    const int windowsz = halflen_ * 2 + 1;
    return sz >= windowsz;
}


void HilbertTransform::setCalcRange( int startidx, int convstartidx )
{
    startidx_ = startidx;
    convstartidx_ = convstartidx;
}


bool HilbertTransform::init()
{
    errmsg_ = uiString::emptyString();
    hilbwindow_ = makeHilbWindow( halflen_ );
    return true;
}


float* HilbertTransform::makeHilbWindow( int hlen )
{
    float* h = new float[hlen*2+1];
    OD::memZero( h, (hlen*2+1)*sizeof(float) );
    const double hlend = mCast(double,hlen);
    for ( int i=1; i<=hlen; i+=2 )
    {
	const double pii = M_PI * mCast(double,i);
	const double taper = ( 0.54 + 0.46 * cos( pii / hlend ) );
	const float win = mCast( float, 2. * taper / pii );
	h[hlen-i] = -win;
	h[hlen+i] = win;
    }
    return h;
}


class Masker
{
public:

Masker( const ValueSeries<float>& data, int sz )
    : avg_(0)
    , data_(data)
    , size_(sz)
{}


float operator[]( int pos ) const
{
    float val = mUdf(float);
    const int nrsamples = size_;
    if ( pos < 0 )
	val = data_.value(0);
    else if ( pos >= nrsamples )
	val = data_.value( nrsamples-1 );
    else
	val = data_.value( pos );

    bool isudf = mIsUdf(val);

    if ( isudf )
    {
	const bool goup = pos < nrsamples/2;
	int tmppos = goup ? (pos<0 ? 1 : pos+1)
			  : (pos >= nrsamples ? nrsamples-2 : pos-1);

	while ( isudf && tmppos>0 && tmppos < nrsamples )
	{
	    val = data_.value( tmppos );
	    isudf = mIsUdf( val );
	    goup ? tmppos++ : tmppos--;
	}
    }

    return isudf ? mUdf(float) : val - avg_;
}

    int				size_;
    float			avg_;
    const ValueSeries<float>&	data_;
};


bool HilbertTransform::transform( const ValueSeries<float>& input, int szin,
				  ValueSeries<float>& output, int szout ) const
{
    Masker masker( input, szin );
    float sum = 0;

    if ( startidx_<0 )
	return false;

    mAllocLargeVarLenArr( float, maskerarr, szin );
    if ( !maskerarr )
	return false;

    int nrsampforavg = 0;
    for ( int idx=0; idx<szin; idx++ )
    {
	const float val = masker[idx + startidx_];
	if ( !mIsUdf(val) )
	{
	    sum += val;
	    nrsampforavg++;
	}

	maskerarr[idx] = masker[idx];
    }

    masker.avg_ = sum / nrsampforavg;

    float* outarr = output.arr();
    const int windowsz = halflen_ * 2 + 1;
    if ( nrsampforavg != szin )		//means there are undefined values
	GenericConvolve( windowsz, -halflen_, hilbwindow_,
			 szin, 0, maskerarr.ptr(),
			 szout, convstartidx_, outarr );
    else
	GenericConvolveNoUdf( windowsz, -halflen_, hilbwindow_, szin, 0,
			      maskerarr.ptr(), szout, convstartidx_, outarr );

    return true;
}


bool HilbertTransform::transform( const ArrayND<float>& in,
				  ArrayND<float>& out ) const
{
    const int insize = in.info().getSize(0);
    const int outsize = out.info().getSize(0);
    const ValueSeries<float>* inptr = in.getStorage();
    ValueSeries<float>* outptr = out.getStorage();
    if ( !inptr || !outptr ) return false;

    return transform( *inptr, insize, *outptr, outsize );
}


bool HilbertTransform::transform( const ArrayND<float_complex>& in,
				  ArrayND<float_complex>& out ) const
{
    pErrMsg( "Not implemented" );
    return false;
}


bool HilbertTransform::transform( const ArrayND<float>& real,
				  ArrayND<float_complex>& img ) const
{
    const int insize = real.info().getSize(0);
    Array1DImpl<float> imgout( insize );
    const bool trans = transform( real, imgout );
    if ( !trans ) return false;

    mDynamicCastGet(const Array1D<float>*,realarr,&real);
    mDynamicCastGet(Array1D<float_complex>*,imgarr,&img);
    if ( !realarr || !imgarr ) return false;

    for ( int idx=0; idx<insize; idx++ )
	imgarr->set( idx, float_complex(realarr->get(idx),imgout.get(idx)) );

    return true;
}
