/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : 23-3-1996 / July 2016
-*/


#include "wavelet.h"
#include "arrayndimpl.h"
#include "arrayndalgo.h"
#include "fourier.h"
#include "hilberttransform.h"
#include "seisinfo.h"
#include "valseriesinterpol.h"
#include "math2.h"


mDefineInstanceCreatedNotifierAccess(Wavelet)


Wavelet::Wavelet( const char* nm )
	: SharedObject(nm)
	, cidx_(0)
	, dpos_(SeisTrcInfo::defaultSampleInterval(true))
	, sz_(0)
	, samps_(0)
	, intpol_(0)
{
    mTriggerInstanceCreatedNotifier();
}


Wavelet::Wavelet( bool isricker, ValueType fpeak, ZType sr, ValueType scale )
	: SharedObject( BufferString(isricker ? "Ricker f=" : "Sinc f=",
					 fpeak, ")") )
	, dpos_(sr)
	, sz_(0)
	, samps_(0)
	, intpol_(0)
{
    if ( mIsUdf(dpos_) )
	dpos_ = SeisTrcInfo::defaultSampleInterval(true);
    if ( mIsUdf(scale) )
	scale = (ValueType)1;
    if ( mIsUdf(fpeak) || fpeak <= 0 )
	fpeak = (ValueType)25;
    cidx_ = (idx_type)( ( 1 + 1. / (fpeak*dpos_) ) );

    const size_type wvltlen = 1 + 2*cidx_;
    doReSize( wvltlen );
    ValueType pos = -cidx_ * dpos_;
    for ( idx_type idx=0; idx<wvltlen; idx++ )
    {
	double x = M_PI * fpeak * pos;
	double x2 = x * x;
	if ( idx == cidx_ )
	    samps_[idx] = scale;
	else if ( isricker )
	    samps_[idx] = (ValueType)(scale * Math::Exp(-x2) * (1-2*x2));
	else
	{
	    samps_[idx] = (ValueType)(scale * exp(-x2) * Math::Sinc(x));
	    if ( samps_[idx] < 0 )
		samps_[idx] = 0;
	}
	pos += dpos_;
    }
    mTriggerInstanceCreatedNotifier();
}


Wavelet::Wavelet( const Wavelet& oth )
	: SharedObject(oth)
	, sz_(0)
	, samps_(0)
	, intpol_(0)
{
    copyClassData( oth );
    mTriggerInstanceCreatedNotifier();
}


Wavelet::~Wavelet()
{
    sendDelNotif();
    delete [] samps_;
    delete intpol_;
}


mImplMonitorableAssignment( Wavelet, SharedObject )


void Wavelet::copyClassData( const Wavelet& oth )
{
    cidx_ = oth.cidx_;
    dpos_ = oth.dpos_;

    doReSize( oth.sz_ );
    if ( sz_ )
	OD::memCopy( samps_, oth.samps_, sz_*sizeof(ValueType) );

    delete intpol_;
    intpol_ = !oth.intpol_ ? 0
	    : new ValueSeriesInterpolator<ValueType>( *oth.intpol_ );
}


Monitorable::ChangeType Wavelet::compareClassData( const Wavelet& oth ) const
{
    if ( sz_ != oth.sz_ )
	return cEntireObjectChange();
    for ( int idx=0; idx<sz_; idx++ )
	if ( samps_[idx] != oth.samps_[idx] )
	    return cEntireObjectChange();

    mDeliverSingCondMonitorableCompare(
	    cidx_ == oth.cidx_ && dpos_ == oth.dpos_, cParChange() );
}


void Wavelet::doReSize( int newsz )
{
    if ( newsz < 1 )
	{ delete [] samps_; samps_ = 0; sz_ = 0; }
    else
    {
	ValueType* newsamps = 0;
	mTryAlloc( newsamps, ValueType [newsz] );
	if ( newsamps )
	{
	    delete [] samps_;
	    samps_ = newsamps;
	    sz_ = newsz;
	}
    }
}


Wavelet::size_type Wavelet::size() const
{
    mLock4Read();
    return sz_;
}


#define mValidIdx(idx) ((idx) > -1 && (idx) < sz_)
#define mZStart (-cidx_ * dpos_)


bool Wavelet::validIdx( idx_type idx ) const
{
    mLock4Read();
    return mValidIdx( idx );
}


Wavelet::ValueType Wavelet::get( idx_type idx ) const
{
    mLock4Read();
    return mValidIdx( idx ) ? samps_[idx] : mUdf(ValueType);;
}


Wavelet::ValueType Wavelet::getValue( ZType z ) const
{
    mLock4Read();
    const ZType pos = (z - mZStart) / dpos_;
    const WaveletValueSeries wvs( *this );
    return interpolator().value( wvs, pos );
}


void Wavelet::set( idx_type idx, ValueType val )
{
    mLock4Read();
    if ( !mValidIdx(idx) || samps_[idx] == val )
	return;
    if ( !mLock2Write() )
    {
	if ( !mValidIdx(idx) || samps_[idx] == val )
	    return;
    }

    samps_[idx] = val;
    mSendChgNotif( cSampleChange(), idx );
}


Wavelet::ValueType* Wavelet::getSamples() const
{
    mLock4Read();
    ValueType* ret = 0;
    mTryAlloc( ret, ValueType[ sz_ ] );
    if ( ret )
	OD::memCopy( ret, samps_, sz_*sizeof(ValueType) );
    return ret;
}


void Wavelet::getSamples( float* ret ) const
{
    if ( !ret )
	return;

    mLock4Read();
    OD::memCopy( ret, samps_, sz_*sizeof(ValueType) );
}


void Wavelet::getSamples( TypeSet<float>& ret ) const
{
    mLock4Read();
    if ( ret.setSize(sz_) )
	OD::memCopy( ret.arr(), samps_, sz_*sizeof(ValueType) );
    else
	ret.setEmpty();
}


void Wavelet::setSamples( const TypeSet<float>& samps )
{
    setSamples( samps.arr(), samps.size() );
}


void Wavelet::setSamples( const ValueType* vals, size_type newsz )
{
    mLock4Read();
    if ( sz_ < 1 && (!vals || newsz<1) )
	return;

    mLock2Write();

    if ( newsz < 1 || !vals )
	doReSize( 0 );
    else
    {
	doReSize( newsz );
	if ( sz_ == newsz )
	    OD::memCopy( samps_, vals, sz_*sizeof(ValueType) );
    }

    mSendEntireObjChgNotif();
}


void Wavelet::reSize( size_type newsz, ValueType val )
{
    mLock4Read();
    if ( sz_ < 1 && newsz<1 )
	return;

    mLock2Write();

    doReSize( newsz );
    if ( sz_ == newsz )
    {
	for ( int idx=0; idx<sz_; idx++ )
	    samps_[idx] = val;
    }

    mSendEntireObjChgNotif();
}


static Threads::Atomic<bool> interpolinited_( false );
static ValueSeriesInterpolator<Wavelet::ValueType>* definterpol_ = 0;


const ValueSeriesInterpolator<Wavelet::ValueType>& Wavelet::interpolator() const
{
    mLock4Read();
    ValueSeriesInterpolator<Wavelet::ValueType>* ret = intpol_;
    if ( !ret )
    {
	if ( interpolinited_.setIfValueIs(false,true,0) )
	{
	    definterpol_ = new ValueSeriesInterpolator<Wavelet::ValueType>();
	    definterpol_->snapdist_ = 1e-5f;
	    definterpol_->smooth_ = true;
	    definterpol_->extrapol_ = false;
	    definterpol_->udfval_ = 0;
	}
	ret = definterpol_;
    }

    if ( ret->maxidx_ != sz_-1 )
	ret->maxidx_ = sz_ - 1;
    return *ret;
}


void Wavelet::setInterpolator( ValueSeriesInterpolator<ValueType>* newinterpol )
{
    mLock4Read();
    if ( newinterpol == intpol_ )
	return;

    mLock2Write();
    delete intpol_;
    intpol_ = newinterpol;
    mSendChgNotif( cParChange(), 0 );
}


StepInterval<Wavelet::ZType> Wavelet::samplePositions() const
{
    mLock4Read();
    return gtSamplePositions();
}


StepInterval<Wavelet::ZType> Wavelet::gtSamplePositions() const
{
    return StepInterval<ZType>( mZStart, (sz_-cidx_-1)*dpos_, dpos_ );
}


Wavelet::idx_type Wavelet::nearestSample( ZType z ) const
{
    mLock4Read();
    const float fidx = mIsUdf(z) ? 0.f : (z - mZStart) / dpos_;
    return mRounded(idx_type,fidx);
}


bool Wavelet::hasSymmetricalSamples() const
{
    mLock4Read();
    return sz_ == cidx_ * 2 + 1;
}


class WaveletFFTData
{
public:

    typedef Wavelet::size_type	size_type;
    typedef Wavelet::ValueType	ValueType;
    typedef Wavelet::ZType	ZType;
    typedef float_complex	ComplexType;

WaveletFFTData( size_type sz, ZType sr )
    : fft_(*Fourier::CC::createDefault())
    , sz_(getPower2Size(sz))
    , halfsz_(sz_/2)
    , sr_(sr)
    , ctwtwvlt_(sz_)
    , cfreqwvlt_(sz_)
    , nyqfreq_(1.f / (2.f * sr_))
    , freqstep_(2 * nyqfreq_ / sz_)
{
    const ComplexType cnullval = ComplexType( 0, 0 );
    ctwtwvlt_.setAll( cnullval );
    cfreqwvlt_.setAll( cnullval );
}

~WaveletFFTData()
{
    delete &fft_;
}

int getPower2Size( size_type inpsz )
{
    size_type outsz = 1;
    while ( outsz < inpsz )
	outsz <<=1;
    return outsz;
}


bool doFFT( bool isfwd )
{
    fft_.setInputInfo( Array1DInfoImpl(sz_) );
    fft_.setDir( isfwd );
    fft_.setNormalization( !isfwd );
    fft_.setInput(   (isfwd ? ctwtwvlt_ : cfreqwvlt_).getData() );
    fft_.setOutput( (!isfwd ? ctwtwvlt_ : cfreqwvlt_).getData() );
    return fft_.run( isfwd );
}

			// Beware ... the order *is* important!
    Fourier::CC&	fft_;
    const size_type	sz_;
    const size_type	halfsz_;
    Array1DImpl<ComplexType> ctwtwvlt_;
    Array1DImpl<ComplexType> cfreqwvlt_;
    const ZType		sr_;
    const ValueType	nyqfreq_;
    const ValueType	freqstep_;

};


bool Wavelet::reSample( ZType newsr )
{
    if ( newsr < 1e-6f )
	return false;

    mLock4Read();
    if ( sz_ < 1 )
	return true;

    const Interval<ZType> twtrg = gtSamplePositions();
    const ZType maxlag = -1 * twtrg.start > twtrg.stop
		       ? -1 * twtrg.start : twtrg.stop;
    const int inpsz = mNINT32( 2.f * maxlag / dpos_ ) + 1;
    const int outsz = mNINT32( 2.f * maxlag / newsr ) + 1;
    WaveletFFTData inp(inpsz,dpos_), out(outsz,newsr);

    const int fwdfirstidx = inp.halfsz_ - cidx_;
    for ( int idx=0; idx<sz_; idx++ )
    {
	const float_complex val( samps_[idx], 0 );
	inp.ctwtwvlt_.set( idx + fwdfirstidx, val );
    }
    if ( !inp.doFFT(true) )
	return false;

    // Interpolate Frequency Wavelet
    PointBasedMathFunction spectrumreal( PointBasedMathFunction::Poly,
					 PointBasedMathFunction::None );
    PointBasedMathFunction spectrumimag( PointBasedMathFunction::Poly,
					 PointBasedMathFunction::None );
    for ( int idx=0; idx<inp.sz_; idx++ )
    {
	ValueType freq = idx * inp.freqstep_;
	if ( idx > inp.halfsz_ )
	    freq -= 2 * inp.nyqfreq_;
	spectrumreal.add( freq, inp.cfreqwvlt_.get(idx).real() );
	spectrumimag.add( freq, inp.cfreqwvlt_.get(idx).imag() );
    }
    spectrumreal.add( -inp.nyqfreq_, spectrumreal.getValue(inp.nyqfreq_) );
    spectrumimag.add( -inp.nyqfreq_, spectrumimag.getValue(inp.nyqfreq_) );

    for ( int idx=0; idx<out.sz_; idx++ )
    {
	ValueType freq = idx * out.freqstep_;
	if ( idx > out.halfsz_ )
	    freq -= 2 * out.nyqfreq_;
	const bool isabovenf = fabs(freq) > inp.nyqfreq_;
	const ValueType realval = isabovenf ? 0.f
					    : spectrumreal.getValue( freq );
	const ValueType imagval = isabovenf ? 0.f
					    : spectrumimag.getValue( freq );
	const float_complex val( realval, imagval );
	out.cfreqwvlt_.set( idx, val );
    }

    if ( !out.doFFT(false) )
	return false;

    doReSize( mNINT32( twtrg.width() / newsr ) + 1 );
    dpos_ = newsr;
    cidx_ = mNINT32( -twtrg.start / newsr );
    const int revfirstidx = out.halfsz_ - cidx_;
    const ValueType normfact = ((ValueType)out.sz_) / inp.sz_;

    const int sz = sz_;
    if ( !mLock2Write() )
    {
	doReSize( sz );
	if ( sz_ < 1 )
	    { mSendEntireObjChgNotif(); return true; }
    }

    for ( int idx=0; idx<sz_; idx++ )
	samps_[idx] = normfact * out.ctwtwvlt_.get( idx + revfirstidx ).real();

    mSendEntireObjChgNotif();
    return true;
}


bool Wavelet::reSampleTime( ZType newsr )
{
    if ( newsr < 1e-6f )
	return false;

    mLock4Read();
    ZType fnewsz = (sz_-1) * dpos_ / newsr + 1.f - 1e-5f;
    const int newsz = mNINT32( ceil(fnewsz) );
    ValueType* newsamps = 0;
    mTryAlloc( newsamps, ValueType[ newsz ] );
    if ( !newsamps )
	return false;

    StepInterval<ZType> twtrg = gtSamplePositions();
    twtrg.step = newsr;
    for ( int idx=0; idx<newsz; idx++ )
	newsamps[idx] = getValue( twtrg.atIndex(idx) );

    mLock2Write();
    sz_ = newsz;
    delete [] samps_;
    samps_ = newsamps;
    cidx_ = twtrg.getIndex( 0.f );
    dpos_ = newsr;

    mSendEntireObjChgNotif();
    return true;
}


void Wavelet::ensureSymmetricalSamples()
{
    if ( hasSymmetricalSamples() )
	return;

    mLock4Read();
    StepInterval<ZType> sampposns( gtSamplePositions() );
    const ZType halftwtwvltsz = mMAX( -sampposns.start, sampposns.stop );
    const int newcidx = mNINT32( halftwtwvltsz / dpos_ );
    const int outsz = 2 * newcidx + 1;

    ValueType* newsamps = new ValueType [outsz];
    if ( !newsamps )
	return;

    for ( int idx=0; idx<outsz; idx++ )
	newsamps[idx] = 0.f;

    StepInterval<ZType> newsamplepos( -halftwtwvltsz, halftwtwvltsz, dpos_ );
    for ( int idx=0; idx<sz_; idx++ )
    {
	const ZType twt = sampposns.atIndex(idx);
	const int idy = newsamplepos.getIndex(twt);
	newsamps[idy] = samps_[idx];
    }

    mLock2Write();
    doReSize( outsz );
    samps_ = newsamps;
    cidx_ = newcidx;

    mSendEntireObjChgNotif();
}


void Wavelet::transform( float constant, float factor )
{
    mLock4Write();
    for ( int idx=0; idx<sz_; idx++ )
	samps_[idx] = constant + samps_[idx] * factor;
    mSendEntireObjChgNotif();
}


void Wavelet::normalize()
{
    const Interval<float> rg( getExtrValue(true), getExtrValue(false) );
    if ( rg.start == 0 && rg.stop == 0 )
	return;
    transform( 0, 1.f / mMAX( fabs(rg.start),fabs(rg.stop)) );
}


bool Wavelet::trimPaddedZeros()
{
    mLock4Read();
    if ( sz_ < 4 )
	return false;

    Interval<int> nonzerorg( 0, sz_-1 );
    while ( samps_[nonzerorg.start] == 0 && nonzerorg.start < nonzerorg.stop )
	nonzerorg.start++;
    while ( samps_[nonzerorg.stop] == 0 && nonzerorg.stop > nonzerorg.start )
	nonzerorg.stop--;
    if ( nonzerorg.start < 2 && nonzerorg.stop > sz_-3 )
	return false;

    Interval<int> newrg( nonzerorg.start-1, nonzerorg.stop+1 );
    if ( newrg.start < 0 ) newrg.start = 0;
    if ( newrg.stop > sz_-1 ) newrg.stop = sz_-1;
    const int newsz = newrg.width() + 1;
    float* newsamps = new float [newsz];
    if ( !newsamps )
	return false;

    for ( int idx=0; idx<newsz; idx++ )
	newsamps[idx] = samps_[newrg.start+idx];

    mLock2Write();
    delete samps_;
    samps_ = newsamps;
    sz_ = newsz;
    cidx_ -= newrg.start;

    mSendEntireObjChgNotif();
    return true;
}


float Wavelet::getExtrValue( bool ismax ) const
{
    Interval<float> vals;
    getExtrValues( vals );
    return ismax ? vals.stop : vals.start;
}


void Wavelet::getExtrValues( Interval<float>& vals ) const
{
    vals.set( mUdf(float), -mUdf(float) );
    mLock4Read();
    for ( int idx=0; idx<sz_; idx++ )
	vals.include( samps_[idx], false );
}


int Wavelet::getPos( float val, bool closetocenteronly ) const
{
    mLock4Read();
    const int width = mCast( int, mCast(float, sz_ ) / 10.f );
    int start = closetocenteronly ? cidx_ - width : 0;
    if ( start < 0 ) start = 0;
    int stop = closetocenteronly ? cidx_ + width : sz_;
    if ( stop > sz_ ) stop = sz_;
    bool startnotreached = true;
    bool stopnotreached = true;
    for ( int idx=0; startnotreached || stopnotreached; idx++ )
    {
	if ( (cidx_+idx) > stop)
	    stopnotreached = false;
	else if ( mIsEqual(samps_[cidx_+idx],val,mDefEpsF) )
	    return cidx_+idx;

	if ( (cidx_-idx) <= start)
	    startnotreached = false;
	else if ( mIsEqual(samps_[cidx_-idx],val,mDefEpsF) )
	    return cidx_-idx;
    }

    return mUdf(int);
}


WaveletValueSeries::WaveletValueSeries( const Wavelet& wv )
    : wv_(const_cast<Wavelet*>(&wv))
    , ml_(wv)
{
}


float WaveletValueSeries::value( od_int64 idx ) const
{
    return wv_->get( (Wavelet::idx_type)idx );
}


void WaveletValueSeries::setValue( od_int64 idx, float v )
{
    ml_.unlockNow();
    wv_->set( (Wavelet::idx_type)idx, v );
    ml_.reLock();
}


float* WaveletValueSeries::arr()
{
    return wv_->samps_;
}


const float* WaveletValueSeries::arr() const
{
    return wv_->samps_;
}


WaveletFunction::WaveletFunction( const Wavelet& wv )
    : wv_(&wv)
    , ml_(wv)
{
}
