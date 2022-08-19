/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wavelet.h"
#include "waveletio.h"
#include "arrayndimpl.h"
#include "arrayndalgo.h"
#include "ctxtioobj.h"
#include "fourier.h"
#include "hilberttransform.h"
#include "ascstream.h"
#include "ioobj.h"
#include "ioman.h"
#include "keystrs.h"

#include "ptrman.h"
#include "seisinfo.h"
#include "separstr.h"
#include "statruncalc.h"
#include "streamconn.h"
#include "survinfo.h"
#include "tabledef.h"
#include "valseriesinterpol.h"

#include <math.h>

defineTranslatorGroup(Wavelet,"Wavelet");
defineTranslator(dgb,Wavelet,mDGBKey);
mDefSimpleTranslatorSelector(Wavelet);

uiString WaveletTranslatorGroup::sTypeName(int num)
{ return uiStrings::sWavelet(num); }

static const char* sKeyScaled = "Scaled";
#define mDefaultSnapdist (1e-4f);

Wavelet::Wavelet( const char* nm )
    : NamedCallBacker(nm)
    , dpos_(SeisTrcInfo::defaultSampleInterval(true))
    , cidx_(0)
{
}


Wavelet::Wavelet( bool isricker, float fpeak, float sr, float scale )
    : dpos_(sr)
{
    if ( mIsUdf(dpos_) )
	dpos_ = SeisTrcInfo::defaultSampleInterval(true);
    if ( mIsUdf(scale) )
	scale = 1;
    if ( mIsUdf(fpeak) || fpeak <= 0 )
	fpeak = 25;
    cidx_ = (int)( ( 1 + 1. / (fpeak*dpos_) ) );

    BufferString nm( isricker ? "Ricker " : "Sinc " );
    nm += " (Central freq="; nm += fpeak; nm += ")";
    setName( nm );

    int lw = 1 + 2*cidx_;
    reSize( lw );
    float pos = -cidx_ * dpos_;
    for ( int idx=0; idx<lw; idx++ )
    {
	double x = M_PI * fpeak * pos;
	double x2 = x * x;
	if ( idx == cidx_ )
	    samps_[idx] = scale;
	else if ( isricker )
	    samps_[idx] = (float) (scale * exp(-x2) * (1-2*x2));
	else
	{
	    samps_[idx] = (float) (scale * exp(-x2) * sin(x)/x);
	    if ( samps_[idx] < 0 ) samps_[idx] = 0;
	}
	pos += dpos_;
    }
}


Wavelet::Wavelet( const Wavelet& wv )
{
    *this = wv;
}


Wavelet& Wavelet::operator =( const Wavelet& oth )
{
    if ( &oth == this )
	return *this;

    setName( oth.name() );
    cidx_ = oth.cidx_;
    dpos_ = oth.dpos_;
    reSize( oth.size() );
    if ( sz_ )
	OD::memCopy( samps_, oth.samps_, sz_*sizeof(float) );

    delete intpol_;
    intpol_ = oth.intpol_
	    ? new ValueSeriesInterpolator<float>( *oth.intpol_ ) : nullptr;

    return *this;
}


Wavelet::~Wavelet()
{
    delete [] samps_;
    delete intpol_;
}


IOObj* Wavelet::getIOObj( const char* waveletnm )
{
    if ( !waveletnm || !*waveletnm )
	return 0;

    IOObjContext ctxt( mIOObjContext(Wavelet) );
    IOM().to( ctxt.getSelKey() );
    return IOM().getLocal( waveletnm, mTranslGroupName(Wavelet) );
}


Wavelet* Wavelet::get( const IOObj* ioobj )
{
    if ( !ioobj ) return 0;
    PtrMan<WaveletTranslator> tr =
		(WaveletTranslator*)ioobj->createTranslator();
    if ( !tr ) return 0;
    Wavelet* newwv = 0;

    Conn* connptr = ioobj->getConn( Conn::Read );
    if ( !connptr || connptr->isBad() )
	ErrMsg( "Cannot open Wavelet file" );
    else
    {
	newwv = new Wavelet;
	if ( tr->read( newwv, *connptr ) )
	    newwv->setName( ioobj->name() );
	else
	{
	    ErrMsg( "Problem reading Wavelet from file (format error?)" );
	    delete newwv;
	    newwv = 0;
	}
    }

    delete connptr;
    return newwv;
}


bool Wavelet::put( const IOObj* ioobj ) const
{
    if ( !ioobj ) return false;
    PtrMan<WaveletTranslator> trans =
        (WaveletTranslator*)ioobj->createTranslator();
    if ( !trans ) return false;
    bool retval = false;

    Conn* connptr = ioobj->getConn( Conn::Write );
    if ( connptr && !connptr->isBad() )
    {
	if ( trans->write( this, *connptr ) )
	    retval = true;
	else
	    ErrMsg( "Cannot write Wavelet" );
    }
    else
	ErrMsg( "Cannot open Wavelet file for write" );

    delete connptr;
    return retval;
}


void Wavelet::reSize( int newsz )
{
    if ( newsz < 1 ) { delete [] samps_; samps_ = 0; sz_ = 0; return; }

    float* newsamps = new float [newsz];
    if ( !newsamps ) return;

    delete [] samps_;
    samps_ = newsamps;
    sz_ = newsz;
}


const ValueSeriesInterpolator<float>& Wavelet::interpolator() const
{
    mDefineStaticLocalObject(
	   PtrMan<ValueSeriesInterpolator<float> >, defintpol, = 0);
    if ( !defintpol )
    {
	ValueSeriesInterpolator<float>* newdefintpol = new
					ValueSeriesInterpolator<float>();
	newdefintpol->snapdist_ = mDefaultSnapdist;
	newdefintpol->smooth_ = true;
	newdefintpol->extrapol_ = false;
	newdefintpol->udfval_ = 0;
	defintpol.setIfNull(newdefintpol,true);
    }
    ValueSeriesInterpolator<float>& ret
	= const_cast<ValueSeriesInterpolator<float>&>(
					intpol_ ? *intpol_ : *defintpol );
    ret.maxidx_ = size() - 1;
    return ret;
}


void Wavelet::setInterpolator( ValueSeriesInterpolator<float>* intpol )
{
    delete intpol_; intpol_ = intpol;
}


class WaveletFFTData
{
public:

WaveletFFTData( int sz, float sr )
    : fft_(*Fourier::CC::createDefault())
    , sz_(getPower2Size(sz))
    , halfsz_(sz_/2)
    , sr_(sr)
    , ctwtwvlt_(sz_)
    , cfreqwvlt_(sz_)
    , nyqfreq_(1.f / (2.f * sr_))
    , freqstep_(2 * nyqfreq_ / sz_)
{
    const float_complex cnullval = float_complex( 0, 0 );
    ctwtwvlt_.setAll( cnullval );
    cfreqwvlt_.setAll( cnullval );
}

~WaveletFFTData()
{
    delete &fft_;
}

int getPower2Size( int inpsz )
{
    int outsz = 1;
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
    const int		sz_;
    const int		halfsz_;
    Array1DImpl<float_complex> ctwtwvlt_;
    Array1DImpl<float_complex> cfreqwvlt_;
    const float		sr_;
    const float		nyqfreq_;
    const float		freqstep_;

};


bool Wavelet::reSample( float newsr )
{
    const Interval<float> twtrg = samplePositions();
    const float maxlag = -1 * twtrg.start > twtrg.stop
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
	float freq = idx * inp.freqstep_;
	if ( idx > inp.halfsz_ )
	    freq -= 2 * inp.nyqfreq_;
	spectrumreal.add( freq, inp.cfreqwvlt_.get(idx).real() );
	spectrumimag.add( freq, inp.cfreqwvlt_.get(idx).imag() );
    }
    spectrumreal.add( -inp.nyqfreq_, spectrumreal.getValue(inp.nyqfreq_) );
    spectrumimag.add( -inp.nyqfreq_, spectrumimag.getValue(inp.nyqfreq_) );

    for ( int idx=0; idx<out.sz_; idx++ )
    {
	float freq = idx * out.freqstep_;
	if ( idx > out.halfsz_ )
	    freq -= 2 * out.nyqfreq_;
	const bool isabovenf = fabs(freq) > inp.nyqfreq_;
	const float realval = isabovenf ? 0.f : spectrumreal.getValue( freq );
	const float imagval = isabovenf ? 0.f : spectrumimag.getValue( freq );
	const float_complex val( realval, imagval );
	out.cfreqwvlt_.set( idx, val );
    }

    if ( !out.doFFT(false) )
	return false;

    reSize( mNINT32( twtrg.width() / newsr ) + 1 );
    dpos_ = newsr;
    cidx_ = mNINT32( -twtrg.start / newsr );
    const int revfirstidx = out.halfsz_ - cidx_;
    const float normfact = ((float)out.sz_) / inp.sz_;
    for ( int idx=0; idx<sz_; idx++ )
	samps_[idx] = normfact * out.ctwtwvlt_.get( idx + revfirstidx ).real();

    return true;
}


bool Wavelet::reSampleTime( float newsr )
{
    if ( newsr < 1e-6f )
	return false;

    float fnewsz = (sz_-1) * dpos_ / newsr + 1.f - 1e-5f;
    const int newsz = mNINT32( ceil(fnewsz) );
    float* newsamps = new float [newsz];
    if ( !newsamps )
	return false;

    StepInterval<float> twtrg = samplePositions();
    twtrg.step = newsr;
    for ( int idx=0; idx<newsz; idx++ )
	newsamps[idx] = getValue( twtrg.atIndex(idx) );

    sz_ = newsz;
    delete [] samps_;
    samps_ = newsamps;
    cidx_ = twtrg.getIndex( 0.f );
    dpos_ = newsr;
    return true;
}


void Wavelet::ensureSymmetricalSamples()
{
    if ( hasSymmetricalSamples() )
	return;

    const float halftwtwvltsz = mMAX( -samplePositions().start,
				       samplePositions().stop );
    const int newcidx = mNINT32( halftwtwvltsz / dpos_ );
    const int outsz = 2 * newcidx + 1;

    float* newsamps = new float [outsz];
    if ( !newsamps )
	return;

    for ( int idx=0; idx<outsz; idx++ )
	newsamps[idx] = 0.f;

    StepInterval<float> newsamplepos( -halftwtwvltsz, halftwtwvltsz, dpos_ );
    for ( int idx=0; idx<sz_; idx++ )
    {
	const float twt = samplePositions().atIndex(idx);
	const int idy = newsamplepos.getIndex(twt);
	newsamps[idy] = samps_[idx];
    }

    reSize( outsz );
    samps_ = newsamps;
    cidx_ = newcidx;
}


void Wavelet::transform( float constant, float factor )
{
    for ( int idx=0; idx<sz_; idx++ )
	samps_[idx] = constant + samps_[idx] * factor;
}


void Wavelet::normalize()
{
    const float maxval = mMAX( fabs(getExtrValue(true)),
				fabs(getExtrValue(false)) );
    if ( mIsZero(maxval,mDefEpsF) )
	return;

    transform( 0, 1.f/maxval );
}


bool Wavelet::trimPaddedZeros()
{
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

    delete samps_;
    samps_ = newsamps;
    sz_ = newsz;
    cidx_ -= newrg.start;
    return true;
}


float Wavelet::getExtrValue( bool ismax ) const
{
    Interval<float> vals;
    Wavelet::getExtrValues( vals );
    return ismax ? vals.stop : vals.start;
}


void Wavelet::getExtrValues(Interval<float>& vals) const
{
    vals.set( mUdf(float), -mUdf(float) );
    for ( int idx=0; idx<sz_; idx++ )
	vals.include( samps_[idx], false );
}


int Wavelet::getPos( float val, bool closetocenteronly ) const
{
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


static void markWaveletScaled( const MultiID& id, const char* val )
{
    PtrMan<IOObj> ioobj = IOM().get( id );
    if ( !ioobj ) return;
    ioobj->pars().set( sKeyScaled, val );
    IOM().commitChanges( *ioobj );
}


void Wavelet::markScaled( const MultiID& id )
{
    markWaveletScaled( id, "External" );
}


void Wavelet::markScaled( const MultiID& id, const MultiID& orgid,
			  const MultiID& horid, const MultiID& seisid,
			  const char* lvlnm )
{
    FileMultiString fms( orgid.toString() );
    fms += horid; fms += seisid; fms += lvlnm;
    markWaveletScaled( id, fms );
}


static BufferString waveletScaleStr( const MultiID& id )
{
    BufferString ret;
    IOObj* ioobj = IOM().get( id );
    if ( !ioobj ) return ret;
    ret = ioobj->pars().find( sKeyScaled );
    delete ioobj;
    return ret;
}


bool Wavelet::isScaled( const MultiID& id )
{
    return !waveletScaleStr(id).isEmpty();
}


bool Wavelet::isScaled( const MultiID& id, MultiID& orgid, MultiID& horid,
			MultiID& seisid, BufferString& lvlnm )
{
    BufferString val( waveletScaleStr(id) );
    if ( val.isEmpty() )
	return false;

    FileMultiString fms( val );
    const int fmssz = fms.size();
    if ( fmssz < 3 )
    {
	orgid.setUdf();
	return true;
    }

    orgid.fromString( fms[0] );
    horid.fromString( fms[1] );
    seisid.fromString( fms[2] );
    lvlnm = fms[3];
    return true;
}


int Wavelet::nearestSample( float z ) const
{
    float s = mIsUdf(z) ? 0.f : ( z - samplePositions().start ) / dpos_;
    return mNINT32(s);
}


float Wavelet::getValue( float z ) const
{
    const float pos = ( z - samplePositions().start ) / dpos_;
    return interpolator().value( WaveletValueSeries(*this), pos );
}


float WaveletValueSeries::value( od_int64 idx ) const
{ return wv_.get((int) idx ); }


void WaveletValueSeries::setValue( od_int64 idx,float v )
{ wv_.set((int) idx,v); }


float* WaveletValueSeries::arr()
{ return wv_.samples(); }


const float* WaveletValueSeries::arr() const
{ return const_cast<WaveletValueSeries*>( this )->arr(); }


mDefSimpleTranslatorioContext(Wavelet,Seis)


static const char* sLength	= "Length";
static const char* sIndex	= "Index First Sample";
static const char* sSampRate	= "Sample Rate";


bool dgbWaveletTranslator::read( Wavelet* wv, Conn& conn )
{
    if ( !wv || !conn.forRead() || !conn.isStream() )	return false;

    ascistream astream( ((StreamConn&)conn).iStream() );
    if ( !astream.isOfFileType(mTranslGroupName(Wavelet)) )
	return false;

    int cidx = 0; float sr = SI().zStep();
    while ( !atEndOfSection( astream.next() ) )
    {
        if ( astream.hasKeyword( sLength ) )
	    wv->reSize( astream.getIValue() );
        else if ( astream.hasKeyword( sIndex ) )
	    cidx = -1 * astream.getIValue();
        else if ( astream.hasKeyword(sKey::Name()) )
	    wv->setName( astream.value() );
        else if ( astream.hasKeyword( sSampRate ) )
	    sr = astream.getFValue() / SI().showZ2UserFactor();
    }
    wv->setSampleRate( sr );
    wv->setCenterSample( cidx );

    for ( int idx=0; idx<wv->size(); idx++ )
	astream.stream() >> wv->samples()[idx];

    wv->trimPaddedZeros();
    return astream.isOK();
}


bool dgbWaveletTranslator::write( const Wavelet* inwv, Conn& conn )
{
    if ( !inwv || !conn.forWrite() || !conn.isStream() )	return false;

    Wavelet wv( *inwv );
    wv.trimPaddedZeros();

    ascostream astream( ((StreamConn&)conn).oStream() );
    const BufferString head( mTranslGroupName(Wavelet), " file" );
    if ( !astream.putHeader( head ) ) return false;

    if ( *(const char*)wv.name() ) astream.put( sKey::Name(), wv.name() );
    astream.put( sLength, wv.size() );
    astream.put( sIndex, -wv.centerSample() );
    astream.put( IOPar::compKey(sSampRate,sKey::Unit()),
		 SI().zDomain().unitStr() );
    astream.put( sSampRate, wv.sampleRate() * SI().zDomain().userFactor() );
    astream.newParagraph();
    for ( int idx=0; idx<wv.size(); idx++ )
	astream.stream() << wv.samples()[idx] << od_newline;
    astream.newParagraph();

    return astream.isOK();
}


Table::FormatDesc* WaveletAscIO::getDesc()
{
    Table::FormatDesc* fd = new Table::FormatDesc( "Wavelet" );
    fd->headerinfos_ += new Table::TargetInfo( "Sample interval",
			FloatInpSpec(SI().zRange(true).step), Table::Required,
			Mnemonic::surveyZType() );
    fd->headerinfos_ += new Table::TargetInfo( "Center sample",
						IntInpSpec(), Table::Optional );
    fd->bodyinfos_ += new Table::TargetInfo( "Data samples", FloatInpSpec(),
					     Table::Required );
    return fd;
}


#define mErrRet(s) { if ( !s.isEmpty() ) errmsg_ = s; return 0; }

Wavelet* WaveletAscIO::get( od_istream& strm ) const
{
    if ( !getHdrVals(strm) )
	return 0;

    float sr = getFValue( 0 );
    if ( sr == 0 || mIsUdf(sr) )
	sr = SI().zStep();
    else if ( sr < 0 )
	sr = -sr;

    int centersmp = getIntValue( 1 );
    if ( !mIsUdf(centersmp) )
    {
	if ( centersmp <= 0 )
	    centersmp = -centersmp;
	else
	    centersmp--;
    }

    TypeSet<float> samps;
    while ( true )
    {
	int ret = getNextBodyVals( strm );
	if ( ret < 0 ) mErrRet(uiString::emptyString())
	if ( ret == 0 ) break;

	float val = getFValue( 0 );
	if ( !mIsUdf(val) ) samps += val;
    }

    if ( samps.isEmpty() )
	mErrRet( tr("No valid data samples found") )
    if ( mIsUdf(centersmp) || centersmp > samps.size() )
	centersmp = samps.size() / 2;

    Wavelet* ret = new Wavelet( "" );
    ret->reSize( samps.size() );
    ret->setCenterSample( centersmp  );
    ret->setSampleRate( sr );
    OD::memCopy( ret->samples(), samps.arr(), ret->size() * sizeof(float) );
    ret->trimPaddedZeros();
    return ret;
}


bool WaveletAscIO::put( od_ostream& ) const
{
    errmsg_ = tr("TODO: WaveletAscIO::put not implemented");
    return false;
}
