/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 28-2-1996
 * FUNCTION : Seismic trace functions
-*/

static const char* rcsID = "$Id: seistrc.cc,v 1.3 2000-03-03 15:42:40 bert Exp $";

#include "seistrc.h"
#include "susegy.h"
#include "simpnumer.h"
#include "timeser.h"
#include <math.h>
#include <float.h>

 
DefineEnumNames(SeisTrc,EvType,0,"Event type")
{
	"None",
	"Peak or trough",
	"Peak (Max)",
	"Trough (Min)",
	"Zero crossing",
	"Zero crossing - to +",
	"Zero crossing + to -",
	"Largest peak",
	"Largest trough",
	0
};


SeisTrc::SeisTrc( int ns, int byts )
	: data_(ns,byts)
	, imag(0)
{
}


SeisTrc::~SeisTrc()
{
    delete [] imag;
}


bool SeisTrc::copyData( const SeisTrc& trc )
{
    if ( classID() == trc.classID() )
	data_ = trc.data_;
    else
    {
	if ( trc.size() != size() && !reSize(trc.size()) )
	    return NO;
	for ( int idx=0; idx<size(); idx++ )
	    set( idx, trc[idx] );
    }

    return size() == trc.size();
}


void SeisTrc::stack( const SeisTrc& trc, int alongpick )
{
    float pick = trc.info().pick;
    if ( alongpick && (mIsUndefined(pick) || mIsUndefined(info().pick)) )
	alongpick = NO;
    float diff;
    if ( alongpick )
    {
	diff = info().pick - pick;
	if ( mIS_ZERO(diff) ) alongpick = NO;
    }

    int wght = info_.stack_count;
    info_.stack_count++;
    for ( int idx=0; idx<size(); idx++ )
    {
	float val = (*this)[idx] * wght;
	val += alongpick ? trc.getValue( info().sampleTime(idx) - diff )
	     : trc[idx];
	val /= (wght + 1.);
	set( idx, val );
    }
}


void SeisTrc::removeDC()
{
    int sz = size();
    if ( !sz ) return;

    float avg = 0;
    for ( int idx=0; idx<sz; idx++ )
	avg += (*this)[idx];
    avg /= sz;
    for ( int idx=0; idx<sz; idx++ )
	set( idx, (*this)[idx] - avg );
}


float SeisTrc::getValue( double t ) const
{
    static const float trcundef = 0;
    static const float snapdist = 1e-4;

    t -= info_.starttime;
    float pos = t / (info_.dt * 1e-6);
    interpolateSampled( *this, size(), pos, pos, false, trcundef, snapdist );

    return pos;
}


int SeisTrc::getIndex( double val ) const
{
    return info_.nearestSample( val );
}


double SeisTrc::getX( int idx ) const
{
    return info_.starttime + idx * 1e-6 * info_.dt;
}


SamplingData SeisTrc::samplingData() const
{
    return SamplingData( info_.starttime, info_.dt * 1e-6 );
}


SampleGate SeisTrc::sampleGate( const TimeGate& tg, int check ) const
{
    SampleGate sg( info_.sampleGate(tg) );
    if ( !check ) return sg;

    int maxsz = size() - 1;
    if ( sg.start > maxsz ) sg.start = maxsz;
    if ( sg.stop > maxsz ) sg.stop = maxsz;

    return sg;
}


class SeisTrcIter : public XFunctionIter
{
public:
SeisTrcIter( SeisTrc* xf, int bw ) : XFunctionIter(xf,bw)
{
    reset();
}

void reset()
{
    idx_ = 0;
    if ( tr()->size() ) idx_ = bw_ ? tr()->size() : 1;
}

double x() const { return tr()->info().starttime
			+ (idx_-1)*1e-6*tr()->info().dt; }
float y() const  { return idx_ > 0 ? (*tr())[idx_-1] : mUndefValue; }
int next()
{
    if ( idx_ > 0 ) idx_ += bw_ ? -1 : 1;
    if ( idx_ > tr()->size() ) idx_ = 0;
    return idx_;
}

SeisTrc* tr() const { return (SeisTrc*)func_; }

};


XFunctionIter* SeisTrc::iter( int bw ) const
{
    return new SeisTrcIter( (SeisTrc*)this, bw );
}


bool SeisTrc::reSize( int n )
{
    data_.reSize( n );
    return data_.data ? YES : NO;
}


void SeisTrc::getPacketInfo( SeisPacketInfo& spi ) const
{
    spi.ns = size();
    spi.dt = info_.dt;
    spi.starttime = info_.starttime;
    spi.nr = info_.binid.inl;
}


bool SeisTrc::isNull() const
{
    for ( int idx=0; idx<size(); idx++ )
	if ( (*this)[idx] != 0 ) return NO;
    return YES;
}


void SeisTrc::gettr( SUsegy& trc ) const
{
    info_.gettr( trc );
    trc.ns = size();

    if ( isSUCompat() )
	memcpy( trc.data, data_.data, data_.size() * data_.bytesPerSample() );
    else
    {
	for ( int idx=0; idx<size(); idx++ )
	    trc.data[idx] = (*this)[idx];
    }
}


void SeisTrc::puttr( SUsegy& trc )
{
    info_.puttr( trc );
    data_.reSize( trc.ns );

    if ( isSUCompat() )
	memcpy( data_.data, trc.data, trc.ns * data_.bytesPerSample() );
    else
    {
	for ( int idx=0; idx<trc.ns; idx++ )
	    set( idx, trc.data[idx] );
    }
}


SeisTrc::Event SeisTrc::find( SeisTrc::EvType evtype,
					TimeGate tg, int occ ) const
{
    Event ev;
    if ( mIS_ZERO(tg.width()) ) return ev;

    int sz = size(); float sr = info_.dt * 1e-6;
    SampleGate sg;
    tg.shift( -info_.starttime );
    if ( tg.start < 0 ) tg.start = 0; if ( tg.stop < 0 ) tg.stop = 0;
    float endtm = info_.sampleTime(sz-1);
    if ( tg.start > endtm ) tg.start = endtm;
    if ( tg.stop > endtm ) tg.stop = endtm;

    int inc;
    if ( tg.start < tg.stop )
    {
	inc = 1;
	sg.start = (int)floor(tg.start/sr);
	sg.stop = (int)ceil(tg.stop/sr);
    }
    else
    {
	inc = -1;
	sg.start = (int)ceil(tg.start/sr);
	sg.stop = (int)floor(tg.stop/sr);
	if ( evtype == ZCNegPos ) evtype = ZCPosNeg;
	else if ( evtype == ZCPosNeg ) evtype = ZCNegPos;
    }

    float prev = (*this)[sg.start];
    int upward;
    if ( sg.start-inc >= 0 && sg.start-inc < sz )
	upward = prev > (*this)[sg.start-inc];
    else
	// avoid extreme at start of gate
        upward = evtype == Max ? prev > (*this)[sg.start+inc]
			       : prev < (*this)[sg.start+inc];

    if ( evtype == GateMax || evtype == GateMin ) occ = 2;
    int iextr = sg.start;
    float extr = prev;

    for ( int idx=sg.start+inc; idx!=sg.stop+inc; idx+=inc )
    {
	float cur = (*this)[idx];

	switch ( evtype )
	{
	case Extr:	if ( ( prev <= cur && !upward )
			  || ( prev >= cur && upward  ) )
			    occ--;
	break;
	case Min:	if ( prev <= cur && !upward ) occ--;
	break;
	case Max:	if ( prev >= cur && upward ) occ--;
	break;
	case GateMin:	if ( prev <= cur && !upward && cur < extr )
			    { extr = cur; iextr = idx; }
	break;
	case GateMax:	if ( prev >= cur && upward && cur > extr )
			    { extr = cur; iextr = idx; }
	break;

	case ZC:	if ( ( cur >= 0 && prev < 0 )
			  || ( cur <= 0 && prev > 0 ) )
			    occ--;
	break;
	case ZCNegPos:	if ( cur >= 0 && prev < 0 ) occ--;
	break;
	case ZCPosNeg:	if ( cur <= 0 && prev > 0 ) occ--;
	break;
	}

	if ( occ < 1 )
	{
	    switch ( evtype )
	    {
	    case Extr: case Min: case Max: case GateMax: case GateMin:
		getPreciseExtreme( ev, idx-inc, inc, prev, cur );
	    break;
	    case ZC: case ZCNegPos: case ZCPosNeg:
		// Linear (who cares?)
		float reldist = cur / ( cur - prev );
		float pos = inc < 0 ? idx + reldist : idx - reldist;
		ev.pos = info_.starttime + pos * sr;
		ev.val = 0;
	    break;
	    }
	    return ev;
	}

	upward = prev < cur;
	prev = cur;
    }

    if ( evtype == GateMin || evtype == GateMax )
    {
	if ( iextr != sg.start && iextr != sg.stop )
	{
	    getPreciseExtreme( ev, iextr, inc,
				(*this)[iextr], (*this)[iextr+inc]);
	}
    }

    return ev;
}


void SeisTrc::getPreciseExtreme( Event& ev, int idx, int inc,
				 float y2, float y3 ) const
{
    float sr = info_.dt * 1e-6;
    float y1 = idx-inc < 0 && idx-inc >= size() ? y2 : (*this)[idx-inc];
    ev.pos = info_.starttime + idx*sr;
    ev.val = y2;

    float a = ( y3 + y1 - 2 * y2 ) / 2;
    float b = ( y3 - y1 ) / 2;
    if ( !mIS_ZERO(a) )
    {
	float pos = - b / ( 2 * a );
	ev.pos += inc * pos * sr;
	ev.val += ( a * pos + b ) * pos;
    }
}


Interval<float> SeisTrc::getRange() const
{
    Interval<float> rg( 0, 0 );
    int sz = size();
    if ( sz == 0 ) return rg;
    rg.start = rg.stop = (*this)[0];

    float val;
    for ( int idx=1; idx<sz; idx++ )
    {
	val = (*this)[idx];
	if ( val > rg.stop ) rg.stop = val;
	if ( val < rg.start ) rg.start = val;
    }

    return rg;
}


void SeisTrc::normalize()
{
    int sz = size();
    if ( !sz ) return;

    float val = (*this)[0];
    Interval<float> rg( val, val );
    for ( int idx=1; idx<sz; idx++ )
    {
        val = (*this)[idx];
	if ( val < rg.start ) rg.start = val;
	if ( val > rg.stop ) rg.stop = val;
    }
    float diff = rg.stop - rg.start;
    if ( mIS_ZERO(diff) )
    {
	for ( int idx=0; idx<sz; idx++ )
	    set( idx, 0 );
    }
    else
    {
	float a = 2 / diff;
	float b = 1 - rg.stop * a;
	for ( int idx=0; idx<sz; idx++ )
	    set( idx, (*this)[idx] * a + b );
    }
}


void SeisTrc::corrNormalize()
{
    int sz = size();
    if ( !sz ) return;

    double val = (*this)[0];
    double autocorr = val * val;
    for ( int idx=1; idx<sz; idx++ )
    {
        val = (*this)[idx];
	autocorr += val*val;
    }
    double sqrtacorr = sqrt( autocorr );

    if ( sqrtacorr < 1e-30 )
    {
	for ( int idx=0; idx<sz; idx++ )
	    set( idx, 0 );
    }
    else
    {
	for ( int idx=0; idx<sz; idx++ )
	    set( idx, (*this)[idx] / sqrtacorr );
    }
}


float SeisTrc::getFreq( int isamp ) const
{
    float mytm = info_.sampleTime( isamp );
    float endtm = info_.sampleTime( size() - 1 );
    if ( mytm < info_.starttime || mytm > endtm )
	return mUndefValue;

    // Find nearest crests
    float delt = info_.dt * 1.e-6;
    TimeGate tg( mytm-2*delt, info_.starttime );
    Event ev1 = SeisTrc::find( Extr, tg, 1 );
    tg.start = mytm+2*delt; tg.stop = endtm;
    Event ev2 = SeisTrc::find( Extr, tg, 1 );
    if ( mIsUndefined(ev1.pos) || mIsUndefined(ev2.pos) )
	return mUndefValue;

    // If my sample is an extreme, the events are exactly at a wavelength
    float myval = (*this)[isamp];
    float val0 = (*this)[isamp-1];
    float val1 = (*this)[isamp+1];
    if ( (myval > val0 && myval > val1) || (myval < val0 && myval < val1) )
    {
	float wvtm = ev2.pos - ev1.pos;
	return wvtm > 1e-6 ? 1 / wvtm : mUndefValue;
    }

    // Now find next events ...
    tg.start = mytm-2*delt; tg.stop = info_.starttime;
    Event ev0 = SeisTrc::find( Extr, tg, 2 );
    tg.start = mytm+2*delt; tg.stop = endtm;
    Event ev3 = SeisTrc::find( Extr, tg, 2 );

    // Guess where they would have been when not found
    delt = ev2.pos - ev1.pos; // yes, delt now means something different!
    if ( mIsUndefined(ev0.pos) )
	ev0.pos = ev1.pos - delt;
    if ( mIsUndefined(ev3.pos) )
	ev3.pos = ev2.pos + delt;

    // ... and create a weigthed wavelength
    float d1 = (mytm - ev1.pos) / delt;
    float d2 = (ev2.pos - mytm) / delt;
    float wvtm = delt + d1*(ev3.pos-ev2.pos) + d2*(ev1.pos-ev0.pos);
    return wvtm > 1e-6 ? 1 / wvtm : mUndefValue;
}


float SeisTrc::getPhase( int isamp ) const
{
    if ( !imag || !imagwin.includes(isamp) )
	calcImag( SampleGate(isamp,isamp) );
    if ( !imag ) return mUndefValue;

    float q = imag[isamp - imagwin.start];
    float d = (*this)[isamp];
    return d*d + q*q ? atan2( q, d ) : 0;
}


const float* SeisTrc::calcImag( const SampleGate& sg ) const
{
    int sz = sg.width()+1;
    if ( imagwin.width() != sg.width() || (!imagwin.start && !imagwin.stop) )
    {
	delete [] ((SeisTrc*)this)->imag;
	((SeisTrc*)this)->imag = new float [ sz ];
	if ( !imag ) return 0;
    }
    ((SeisTrc*)this)->imagwin = sg;
    ((SeisTrc*)this)->imagwin.sort();

    int quadsz = imagwin.width() + 1 + 2 * mHalfHilbertLength;
    float trcdata[ quadsz ];
    float quadtrc[ quadsz ];
    int start = imagwin.start - mHalfHilbertLength;
    int stop = imagwin.stop + mHalfHilbertLength;
    for ( int idx=start; idx<=stop; idx++ )
    {
	int trcidx = idx;
	if ( trcidx < 0 )	trcidx = 0;
	if ( trcidx >= size() )	trcidx = size() - 1;
	trcdata[idx-start] = (*this)[trcidx];
    }
    Hilbert( quadsz, trcdata, quadtrc );

    for ( int idx=0; idx<sz; idx++ )
    {
	int qidx = idx + mHalfHilbertLength;
	imag[idx] = quadtrc[qidx];
    }

    return imag;
}


void SeisTrc::mute( float mt, float taperlen )
{
    if ( mt < info().starttime ) return;
    int endidx = size()-1;
    if ( mt > info().sampleTime(endidx) )
	mt = info().sampleTime(endidx);
    if ( taperlen < 0 || mIsUndefined(taperlen) )
	taperlen = 0;
    if ( !mIsUndefined(info().mute_time) && info().mute_time >= mt )
	return;

    info().mute_time = mt;
    info().taper_length = taperlen;

    float t = info().starttime;
    while ( t < mt + mEPSILON )
    {
	int idx = info().nearestSample( t );
	set( idx, 0 );
	t += info().dt * 1.e-6;
    }

    if ( mIS_ZERO(taperlen) ) return;

    float tt = mt + taperlen;
    if ( tt > info().sampleTime(endidx) )
	tt = info().sampleTime(endidx);
    while ( t < tt + mEPSILON )
    {
	int idx = info().nearestSample( t );
	float x = ((t - mt) / taperlen) * M_PI;
	float taper = 0.5 * ( 1 - cos(x) );
	set( idx, (*this)[idx] * taper );
	t += info().dt * 1.e-6;
    }
}


double corr( const SeisTrc& t1, const SeisTrc& t2, const SampleGate& sgin,
	     bool alpick )
{
    double acorr1 = 0, acorr2 = 0, ccorr = 0;
    float val1, val2;
    SampleGate sg( sgin ); sg.sort();
    if ( !alpick && (sg.start<0 || sg.stop>=t1.size() || sg.stop>=t2.size()) )
	return mUndefValue;

    float p1 = t1.info().pick;
    float p2 = t2.info().pick;
    if ( alpick && (mIsUndefined(p1) || mIsUndefined(p2)) )
	return mUndefValue;

    for ( int idx=sg.start; idx<=sg.stop; idx++ )
    {
	val1 = alpick ? t1.getValue(p1+idx*t1.info().dt*1e-6) : t1[idx];
	val2 = alpick ? t2.getValue(p2+idx*t2.info().dt*1e-6) : t2[idx];
	acorr1 += val1 * val1;
	acorr2 += val2 * val2;
	ccorr += val1 * val2;
    }
 
    return ccorr / sqrt( acorr1 * acorr2 );
}


double dist( const SeisTrc& t1, const SeisTrc& t2, const SampleGate& sgin,
	     bool alpick )
{
    float val1, val2;
    double sqdist = 0, sq1 = 0, sq2 = 0;
    SampleGate sg( sgin ); sg.sort();
    if ( !alpick && (sg.start<0 || sg.stop>=t1.size() || sg.stop>=t2.size()) )
	return mUndefValue;

    float p1 = t1.info().pick;
    float p2 = t2.info().pick;
    if ( alpick && (mIsUndefined(p1) || mIsUndefined(p2)) )
	return mUndefValue;

    for ( int idx=sg.start; idx<=sg.stop; idx++ )
    {
	val1 = alpick ? t1.getValue(p1+idx*t1.info().dt*1e-6) : t1[idx];
	val2 = alpick ? t2.getValue(p2+idx*t2.info().dt*1e-6) : t2[idx];
	sq1 += val1 * val1;
	sq2 += val2 * val2;
	sqdist += (val1-val2) * (val1-val2);
    }
    if ( sq1 + sq2 < 1e-10 ) return 0;
    return 1 - (sqrt(sqdist) / (sqrt(sq1) + sqrt(sq2)));
}
