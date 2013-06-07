/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 28-2-1996
 * FUNCTION : Seismic trace functions
-*/

static const char* rcsID = "$Id$";

#include "seistrcprop.h"
#include "seistrc.h"
#include "simpnumer.h"
#include "timeser.h"
#include "ptrman.h"
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


#define mSetStackTrcVal() mtrc().set( idx, val / (wght + 1.), icomp )

void SeisTrcPropChg::stack( const SeisTrc& t2, bool alongpick, float wght )
{
    mChkSize();

    if ( !alongpick )
    {
	if ( t2.size() == sz && t2.info().sampling == trc.info().sampling )
	{
	    mStartCompLoop
	    for ( int idx=0; idx<sz; idx++ )
	    {
		const float val = trc.get( idx, icomp ) * wght
			        + t2.get( idx, icomp );
		mSetStackTrcVal();
	    }
	    mEndCompLoop
	}
	else
	{
	    mStartCompLoop
	    for ( int idx=0; idx<sz; idx++ )
	    {
		const float z = trc.samplePos( idx );
		const float val = trc.get( idx, icomp ) * wght
			        + t2.getValue( z, icomp );
		mSetStackTrcVal();
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
	    const float val = trc.get( idx, icomp ) * wght
			    + t2.getValue( z - diff, icomp );
	    mSetStackTrcVal();
	}
	mEndCompLoop
    }
}


void SeisTrcPropChg::removeDC()
{
    mChkSize();

mStartCompLoop
    float avg = 0;
    for ( int idx=0; idx<sz; idx++ )
	avg += trc.get(idx,icomp);
    avg /= sz;
    for ( int idx=0; idx<sz; idx++ )
	mtrc().set( idx, trc.get(idx,icomp) - avg, icomp );
mEndCompLoop
}


void SeisTrcPropChg::scale( float fac, float shft )
{
    mChkSize();

mStartCompLoop

    for ( int idx=0; idx<sz; idx++ )
	mtrc().set( idx, shft + fac * trc.get( idx, icomp ), icomp );

mEndCompLoop
}


void SeisTrcPropChg::normalize( bool aroundzero )
{
    mChkSize();

mStartCompLoop

    float val = trc.get( 0, icomp );
    Interval<float> rg( val, val );
    if ( aroundzero )
	rg.start = rg.stop = 0;

    for ( int idx=1; idx<sz; idx++ )
    {
        val = trc.get( idx, icomp );
	if ( aroundzero )
	{
	    if ( val > 0 )
		{ if ( val > rg.stop ) rg.stop = val; }
	    else
		{ if ( val < rg.start ) rg.start = val; }
	}
	else
	{
	    if ( val < rg.start ) rg.start = val;
	    if ( val > rg.stop ) rg.stop = val;
	}
    }
    const float diff = rg.stop - rg.start;
    if ( mIsZero(diff,mDefEps) )
    {
	for ( int idx=0; idx<sz; idx++ )
	    mtrc().set( idx, 0, icomp );
    }
    else
    {
	if ( aroundzero )
	{
	    float maxval = -rg.start;
	    if ( rg.stop > maxval ) maxval = rg.stop;
	    const float sc = 1. / maxval;
	    for ( int idx=0; idx<sz; idx++ )
		mtrc().set( idx, trc.get(idx,icomp) * sc, icomp );
	}
	else
	{
	    float a = 2 / diff;
	    float b = 1 - rg.stop * a;
	    for ( int idx=0; idx<sz; idx++ )
		mtrc().set( idx, trc.get( idx, icomp ) * a + b, icomp );
	}
    }

mEndCompLoop
}


void SeisTrcPropChg::corrNormalize()
{
    mChkSize();

mStartCompLoop

    double val = trc.get( 0, icomp );
    double autocorr = val * val;
    for ( int idx=1; idx<sz; idx++ )
    {
        val = trc.get( idx, icomp );
	autocorr += val*val;
    }
    double sqrtacorr = Math::Sqrt( autocorr );

    if ( sqrtacorr < 1e-30 )
    {
	for ( int idx=0; idx<sz; idx++ )
	    mtrc().set( idx, 0, icomp );
    }
    else
    {
	for ( int idx=0; idx<sz; idx++ )
	    mtrc().set( idx, trc.get( idx, icomp ) / sqrtacorr, icomp );
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
	int idx = trc.nearestSample( pos );
	float x = ((pos - mpos) / taperlen) * M_PI;
	float taper = 0.5 * ( 1 - cos(x) );
	mtrc().set( idx, trc.get( idx, icomp ) * taper, icomp );
	pos += trc.info().sampling.step;
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
	int idx = trc.nearestSample( pos );
	float x = ((mpos - pos) / taperlen) * M_PI;
	float taper = 0.5 * ( 1 - cos(x) );
	mtrc().set( idx, trc.get( idx, icomp ) * taper, icomp );
	pos += trc.info().sampling.step;
    }

mEndCompLoop
}


#undef mChkSize
#define mChkSize() const int sz = trc.size(); if ( sz < 1 ) return 0


float SeisTrcPropCalc::getFreq( int isamp ) const
{
    mChkSize();

    float mypos = trc.samplePos( isamp );
    float endpos = trc.samplePos( sz - 1 );
    if ( mypos < trc.startPos() || mypos > endpos )
	return mUdf(float);

    // Find nearest crests
    Interval<float> tg( mypos-2*trc.info().sampling.step, trc.startPos() );
    mFlValSerEv ev1 = find( VSEvent::Extr, tg, 1 );
    tg.start = mypos+2*trc.info().sampling.step; tg.stop = endpos;
    mFlValSerEv ev2 = find( VSEvent::Extr, tg, 1 );
    if ( mIsUdf(ev1.pos) || mIsUdf(ev2.pos) )
	return mUdf(float);

    // If my sample is an extreme, the events are exactly at a wavelength
    float myval = trc.get( isamp, curcomp );
    float val0 = trc.get( isamp-1, curcomp );
    float val1 = trc.get( isamp+1, curcomp );
    if ( (myval > val0 && myval > val1) || (myval < val0 && myval < val1) )
    {
	float wvpos = ev2.pos - ev1.pos;
	return wvpos > 1e-6 ? 1. / wvpos : mUdf(float);
    }

    // Now find next events ...
    tg.start = mypos-2*trc.info().sampling.step; tg.stop = trc.startPos();
    mFlValSerEv ev0 = find( VSEvent::Extr, tg, 2 );
    tg.start = mypos+2*trc.info().sampling.step; tg.stop = endpos;
    mFlValSerEv ev3 = find( VSEvent::Extr, tg, 2 );

    // Guess where they would have been when not found
    float dpos = ev2.pos - ev1.pos;
    if ( mIsUdf(ev0.pos) )
	ev0.pos = ev1.pos - dpos;
    if ( mIsUdf(ev3.pos) )
	ev3.pos = ev2.pos + dpos;

    // ... and create a weigthed wavelength
    float d1 = (mypos - ev1.pos) / dpos;
    float d2 = (ev2.pos - mypos) / dpos;
    float wvpos = dpos + d1*(ev3.pos-ev2.pos) + d2*(ev1.pos-ev0.pos);
    return wvpos > 1e-6 ? 1. / wvpos : mUdf(float);
}


float SeisTrcPropCalc::getPhase( int isamp ) const
{
    mChkSize();

    const int quadsz = 1 + 2 * mHalfHilbertLength;
    ArrPtrMan<float> trcdata = new float[ quadsz ];
    int start = isamp - mHalfHilbertLength;
    int stop = isamp + mHalfHilbertLength;
    for ( int idx=start; idx<=stop; idx++ )
    {
        int trcidx = idx < 0 ? 0 : (idx>=sz ? sz-1 : idx);
        trcdata[idx-start] = trc.get( trcidx, curcomp );
    }
    ArrPtrMan<float> quadtrc = new float[ quadsz ];
    Hilbert( quadsz, trcdata, quadtrc );

    float q = quadtrc[mHalfHilbertLength];
    float d = trcdata[mHalfHilbertLength];
    return d*d + q*q ? atan2( q, d ) : 0;
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


double SeisTrcPropCalc::corr( const SeisTrc& t2, const SampleGate& sgin,
				bool alpick ) const
{
    mChkSize();

    mDefSGAndPicks;
    double acorr1 = 0, acorr2 = 0, ccorr = 0;
    for ( int idx=sg.start; idx<=sg.stop; idx++ )
    {
	mSetVals;
	acorr1 += val1 * val1;
	acorr2 += val2 * val2;
	ccorr += val1 * val2;
    }
 
    return ccorr / Math::Sqrt( acorr1 * acorr2 );
}


double SeisTrcPropCalc::dist( const SeisTrc& t2, const SampleGate& sgin,
				bool alpick ) const
{
    mChkSize();

    mDefSGAndPicks;
    double sqdist = 0, sq1 = 0, sq2 = 0;
    for ( int idx=sg.start; idx<=sg.stop; idx++ )
    {
	mSetVals;
	sq1 += val1 * val1;
	sq2 += val2 * val2;
	sqdist += (val1-val2) * (val1-val2);
    }
    if ( sq1 + sq2 < 1e-10 ) return 0;
    return 1 - (Math::Sqrt(sqdist) / (Math::Sqrt(sq1) + Math::Sqrt(sq2)));
}
