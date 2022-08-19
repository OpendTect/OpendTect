/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

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

Masker( const float* data, int sz )
    : dataptr_(data)
    , size_(sz)
    , valsinp_(0)
{}


void setValSeries( const ValueSeries<float>& valsinp )
{
    valsinp_ = &valsinp;
}


float operator[]( int pos ) const
{
    float val = mUdf(float);
    const int nrsamples = size_;
    if ( pos < 0 )
	val = dataptr_ ? dataptr_[0] : valsinp_->value( 0 );
    else if ( pos >= nrsamples )
	val = dataptr_ ? dataptr_[nrsamples-1] : valsinp_->value( nrsamples-1 );
    else
	val = dataptr_ ? dataptr_[pos] : valsinp_->value( pos );

    bool isudf = mIsUdf(val);

    if ( isudf )
    {
	const bool goup = pos < nrsamples/2;
	int tmppos = goup ? (pos<0 ? 1 : pos+1)
			  : (pos >= nrsamples ? nrsamples-2 : pos-1);

	while ( isudf && tmppos>0 && tmppos < nrsamples )
	{
	    val = dataptr_ ? dataptr_[tmppos] : valsinp_->value( tmppos );
	    isudf = mIsUdf( val );
	    goup ? tmppos++ : tmppos--;
	}
    }

    return isudf ? mUdf(float) : val;
}

    int				size_;
    const float*		dataptr_;
    const ValueSeries<float>*	valsinp_;
};


bool HilbertTransform::transform( const float* input, int szin,
				  float* output, int szout ) const
{
    return transform( input, szin, output, szout, 0 );
}


bool HilbertTransform::transform( const float* input, int szin,
				  float* output, int szout,
				  const ValueSeries<float>* valsinp ) const
{
    if ( !input && !valsinp )
    {
	pErrMsg( "Must have either a float pointer or a ValueSeries" );
	return false;
    }

#ifdef __debug__
    bool alludfvals = true;
    for ( int idx=0; idx<szin; idx++ )
    {
	const float val = input ? input[idx] : valsinp->value( idx );
	if ( !mIsUdf(val) )
	{
	    alludfvals = false;
	    break;
	}
    }

    if ( alludfvals )
	{ pErrMsg("All input values are undefined"); return false; }
#endif

    Masker masker( input, szin );
    if ( !input && valsinp )
	masker.setValSeries( *valsinp );

    if ( startidx_<0 )
	return false;

    mAllocLargeVarLenArr( float, maskerarr, szin );
    if ( !maskerarr )
	return false;

    bool nulltrace = true;
    float sum = 0;
    int nrsampforavg = 0;
    for ( int idx=0; idx<szin; idx++ )
    {
	const float val = masker[idx + startidx_];
	if ( !mIsUdf(val) )
	{
	    if ( nulltrace && val!=0.f )
		nulltrace = false;

	    sum += val;
	    nrsampforavg++;
	}

	maskerarr[idx] = masker[idx];
    }

    if ( !nrsampforavg )
    {
	for ( int idx=0; idx<szout; idx++ )
	    output[idx] = mUdf(float);
	return true;
    }

    if ( nulltrace )
    {
	OD::sysMemZero( output, szout*sizeof(float) );
	return true;
    }

    const float avg = sum / nrsampforavg;
    for ( int idx=0; idx<szin; idx++ )
	maskerarr[idx] -= avg;

    const int windowsz = halflen_ * 2 + 1;
    if ( nrsampforavg != szin )		//means there are undefined values
	GenericConvolve( windowsz, -halflen_, hilbwindow_,
			 szin, 0, maskerarr.ptr(),
			 szout, convstartidx_, output );
    else
	GenericConvolveNoUdf( windowsz, -halflen_, hilbwindow_, szin, 0,
			      maskerarr.ptr(), szout, convstartidx_, output );

    return true;
}


bool HilbertTransform::transform( const ValueSeries<float>& input, int szin,
				  ValueSeries<float>& output, int szout ) const
{
    if ( !output.arr() )
	return false;

    return transform( input.arr(), szin, output.arr(), szout, &input );
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
