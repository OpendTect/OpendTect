/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 28-2-1996
 * FUNCTION : Seismic trace functions
-*/


#include "seistrcprop.h"

#include "arrayndimpl.h"
#include "hilberttransform.h"
#include "ptrman.h"
#include "seistrc.h"
#include "simpnumer.h"
#include "timeser.h"
#include "scaler.h"
#include <math.h>
#include <float.h>


ValueSeriesEvent<float,float> SeisTrcPropCalc::find( VSEvent::Type typ,
				Interval<float> tg , int occ ) const
{
    SeisTrcValueSeries stvs( trc, curcomp < 0 ? 0 : curcomp );
    ValueSeriesEvFinder<float,float> evf( stvs, trc.size() - 1,
					  trc.info().sampling );
    return evf.find( typ, tg, occ );
}


#define mChkSize() const int sz = trc.size(); if ( sz < 1 ) return

#define mStartCompLoop \
    for ( int icomp = (curcomp<0 ? 0 : curcomp); \
	  icomp < (curcomp<0 ? trc.data().nrComponents() : curcomp+1); \
	  icomp++ ) \
    {
#define mEndCompLoop }


#define mSetStackTrcVal( a, b )\
{\
    float val = a; \
    const float val2 = b; \
    if ( !mIsUdf(val2) ) \
	val = mIsUdf(val) ? 2.f * val2 : val * wght + val2; \
    else \
	val *= 2.f; \
\
    mtrc().set( idx, val / (wght + 1.f), icomp ); \
}

void SeisTrcPropChg::stack( const SeisTrc& t2, bool alongpick, float wght )
{
    mChkSize();

    if ( !alongpick )
    {
	if ( t2.size() == sz && t2.info().sampling == trc.info().sampling )
	{
	    mStartCompLoop
	    for ( int idx=0; idx<sz; idx++ )
		mSetStackTrcVal( trc.get( idx, icomp ), t2.get( idx, icomp ) );

	    mEndCompLoop
	}
	else
	{
	    mStartCompLoop
	    for ( int idx=0; idx<sz; idx++ )
	    {
		const float z = trc.samplePos( idx );
		mSetStackTrcVal( trc.get( idx, icomp ),
				 t2.getValue( z, icomp ) );
	    }
	    mEndCompLoop
	}
    }
    else
    {
	const float pick1 = trc.info().pick;
	const float pick2 = t2.info().pick;
	const float diff = mIsUdf(pick1) || mIsUdf(pick2) ? 0 : pick1 - pick2;

	mStartCompLoop
	for ( int idx=0; idx<sz; idx++ )
	{
	    const float z = trc.samplePos( idx );
	    mSetStackTrcVal( trc.get( idx, icomp ),
			     t2.getValue( z - diff, icomp ) );
	}
	mEndCompLoop
    }
}


void SeisTrcPropChg::removeAVG()
{
    mChkSize();

mStartCompLoop
    float avg = 0;
    int count = 0;
    for ( int idx=0; idx<sz; idx++ )
    {
	const float val =  trc.get(idx,icomp);
	if ( mIsUdf(val) )
	    continue;

	avg += val;
	count++;
    }

    if ( count )
	avg /= mCast(float,count);

    for ( int idx=0; idx<sz; idx++ )
    {
	float val = trc.get( idx, icomp );
	if ( !mIsUdf(val) )
	    val -= avg;

	mtrc().set( idx, val, icomp );
    }

mEndCompLoop
}


void SeisTrcPropChg::scale( float fac, float shft )
{
    return scale( LinScaler(fac,shft) );
}


void SeisTrcPropChg::scale( const Scaler& sclr )
{
    mChkSize();

mStartCompLoop

    for ( int idx=0; idx<sz; idx++ )
    {
	const float val =  trc.get( idx, icomp );
	if ( mIsUdf(val) )
	    continue;

	mtrc().set( idx, (float)sclr.scale(val), icomp );
    }

mEndCompLoop
}


void SeisTrcPropChg::normalize( bool aroundzero )
{
    mChkSize();

mStartCompLoop

    Interval<float> rg( Interval<float>::udf() );
    for ( int idx=0; idx<sz; idx++ )
    {
	const float val = trc.get( idx, icomp );
	if ( mIsUdf(val) )
	    continue;

	if ( rg.isUdf() )
	{
	    rg.start = aroundzero ? 0.f : val;
	    rg.stop = rg.start;
	    continue;
	}

	rg.include( val );
    }

    if ( rg.isUdf() )
	continue;

    const float diff = rg.width();
    if ( mIsZero(diff,mDefEpsF) )
    {
	mtrc().zero( icomp );
    }
    else
    {
	const float a = aroundzero ? 1.f / mMAX(-rg.start,rg.stop) : 2.f / diff;
	const float b = aroundzero ? 0.f : 1.f - rg.stop * a;
	for ( int idx=0; idx<sz; idx++ )
	{
	    const float val = trc.get( idx, icomp );
	    if ( mIsUdf(val) )
		continue;

	    mtrc().set( idx, a * val + b, icomp );
	}
    }

mEndCompLoop
}


void SeisTrcPropChg::corrNormalize()
{
    mChkSize();

mStartCompLoop

    double autocorr = 0.f;
    bool hasvaliddata = false;
    for ( int idx=0; idx<sz; idx++ )
    {
	const double val = trc.get( idx, icomp );
	if ( mIsUdf(val) )
	    continue;

	autocorr += val*val;
	hasvaliddata = true;
    }

    if ( !hasvaliddata )
	continue;

    if ( mIsZero(autocorr,mDefEps) )
    {
	mtrc().zero( icomp );
	continue;
    }

    const double sqrtacorr = Math::Sqrt( autocorr );
    for ( int idx=0; idx<sz; idx++ )
    {
	const double val = mCast(double,trc.get( idx, icomp ) );
	if ( mIsUdf(val) )
	    continue;

	mtrc().set( idx, mCast(float,val/sqrtacorr), icomp );
    }

mEndCompLoop
}


void SeisTrcPropChg::topMute( float mpos, float taperlen )
{
    mChkSize();
    if ( mpos < trc.startPos() ) return;

mStartCompLoop

    const int endidx = sz - 1;
    if ( mpos > trc.samplePos(endidx) )
	mpos = trc.samplePos(endidx);
    if ( taperlen < 0 || mIsUdf(taperlen) )
	taperlen = 0;

    float pos = trc.startPos();
    while ( pos < mpos + mDefEps )
    {
	int idx = trc.nearestSample( pos );
	mtrc().set( idx, 0, icomp );
	pos += trc.info().sampling.step;
    }

    if ( mIsZero(taperlen,mDefEps) ) return;

    float pp = mpos + taperlen;
    if ( pp > trc.samplePos(endidx) )
	pp = trc.samplePos(endidx);
    while ( pos < pp + mDefEps )
    {
	const int idx = trc.nearestSample( pos );
	const double x = ((pos - mpos) / taperlen) * M_PI;
	const double taper = 0.5 * ( 1. - cos(x) );
	pos += trc.info().sampling.step;
	const float val = trc.get( idx, icomp );
	if ( mIsUdf(val) )
	    continue;

	mtrc().set( idx, val * mCast(float,taper), icomp );
    }

mEndCompLoop
}


void SeisTrcPropChg::tailMute( float mpos, float taperlen )
{
    mChkSize();

mStartCompLoop

    const int endidx = sz - 1;
    if ( mpos > trc.samplePos(endidx) ) return;
    if ( mpos < trc.startPos() )
	mpos = trc.startPos();

    if ( taperlen < 0 || mIsUdf(taperlen) )
	taperlen = 0;

    float pos = mpos;
    while ( pos <= trc.samplePos(endidx)  )
    {
	int idx = trc.nearestSample( pos );
	mtrc().set( idx, 0, icomp );
	pos += trc.info().sampling.step;
    }

    if ( mIsZero(taperlen,mDefEps) ) return;

    float pp = mpos - taperlen;
    if ( pp < trc.startPos() )
	pp = trc.startPos();
    pos = pp;
    while ( pos <= mpos )
    {
	const int idx = trc.nearestSample( pos );
	const double x = ((mpos - pos) / taperlen) * M_PI;
	const double taper = 0.5 * ( 1. - cos(x) );
	pos += trc.info().sampling.step;
	const float val = trc.get( idx, icomp );
	if ( mIsUdf(val) )
	    continue;

	mtrc().set( idx, val * mCast(float,taper), icomp );
    }

mEndCompLoop
}


#undef mChkSize
#define mChkSize() const int sz = trc.size(); if ( sz < 1 ) return 0

float SeisTrcPropCalc::getFreq( float z ) const
{
    mChkSize();

    const float step = trc.info().sampling.step;
    const float prevph = getPhase( z - step );
    const float nextph = getPhase( z + step );
    if ( mIsUdf(prevph) || mIsUdf(nextph) )
	return mUdf(float);

    float dphase = nextph - prevph;
    if ( dphase < 0.f )
	dphase += M_2PIf;

    return dphase / ( M_2PIf * 2.f * step );
}


float SeisTrcPropCalc::getFreq( int isamp ) const
{
    const float zpos = trc.samplePos( isamp );
    return getFreq( zpos );
}


float SeisTrcPropCalc::getPhase( float z, bool indegrees ) const
{
    mChkSize();

    const int halfsz = mNINT32( mHalfHilbertLength );
    const int quadsz = 2 * halfsz + 1;
    Array1DImpl<float> trcdata( quadsz );
    const float dz = trc.info().sampling.step;
    z -= dz * mCast( float, halfsz );
    for ( int idx=0; idx<quadsz; idx++ )
    {
	trcdata.set( idx, trc.getValue( z, curcomp ) );
	z += dz;
    }

    HilbertTransform hilbert;
    if ( !hilbert.init() )
	return mUdf(float);

    hilbert.setHalfLen( halfsz );
    Array1DImpl<float> hilberttrace( quadsz );
    if ( !hilbert.transform(trcdata,quadsz,hilberttrace,quadsz) )
	return mUdf(float);

    const float phase = Math::Atan2( hilberttrace.get(halfsz),
				     trcdata.get(halfsz) );

    return indegrees ? Math::toDegrees( phase ) : phase;
}


float SeisTrcPropCalc::getPhase( int isamp, bool indegrees ) const
{
    const float zpos = trc.samplePos( isamp );
    return getPhase( zpos, indegrees );
}



#define mDefSGAndPicks \
    SampleGate sg( sgin ); sg.sort(); \
    if ( !alpick ) \
    { \
	if ( sg.start < 0 ) sg.start = 0; \
	if ( sg.stop >= sz ) sg.stop = sz - 1; \
	if ( sg.stop >= t2.size() ) sg.stop = t2.size() - 1; \
    } \
 \
    float p1 = trc.info().pick; \
    float p2 = t2.info().pick; \
    if ( alpick && (mIsUdf(p1) || mIsUdf(p2)) ) \
	return mUdf(double); \
    float val1, val2

#define mSetVals \
	val1 = alpick ? trc.getValue(p1+idx*trc.info().sampling.step,curcomp) \
		      : trc.get( idx, curcomp ); \
	val2 = alpick ? t2.getValue(p2+idx*t2.info().sampling.step,curcomp) \
		      : t2.getValue( trc.samplePos(idx), curcomp )

#define mCheckRetUdf(val1,val2) \
	if ( mIsUdf(val1) || mIsUdf(val2) ) continue;


double SeisTrcPropCalc::corr( const SeisTrc& t2, const SampleGate& sgin,
				bool alpick ) const
{
    mChkSize();

    mDefSGAndPicks;
    double acorr1 = 0, acorr2 = 0, ccorr = 0;
    int count = 0;
    for ( int idx=sg.start; idx<=sg.stop; idx++ )
    {
	mSetVals;
	mCheckRetUdf( val1, val2 )
	acorr1 += val1 * val1;
	acorr2 += val2 * val2;
	ccorr += val1 * val2;
	count++;
    }

    if ( !count || mIsZero(acorr1+acorr2,mDefEpsF) )
	return mUdf(double);

    return ccorr / Math::Sqrt( acorr1 * acorr2 );
}


double SeisTrcPropCalc::dist( const SeisTrc& t2, const SampleGate& sgin,
			      bool alpick ) const
{
    mChkSize();

    mDefSGAndPicks;
    double sqdist = 0, sq1 = 0, sq2 = 0;
    int count = 0;
    for ( int idx=sg.start; idx<=sg.stop; idx++ )
    {
	mSetVals;
	mCheckRetUdf( val1, val2 )
	sq1 += val1 * val1;
	sq2 += val2 * val2;
	sqdist += (val1-val2) * (val1-val2);
    }

    if ( !count || mIsZero(sq1+sq2,mDefEpsF) )
	return mUdf(double);

    return 1.f - (Math::Sqrt(sqdist) / (Math::Sqrt(sq1) + Math::Sqrt(sq2)));
}
