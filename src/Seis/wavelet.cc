/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : 23-3-1996
 * FUNCTION : Wavelet
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "wavelet.h"
#include "wvltfact.h"
#include "arrayndimpl.h"
#include "arrayndutils.h"
#include "fourier.h"
#include "hilberttransform.h"
#include "ascstream.h"
#include "errh.h"
#include "ioobj.h"
#include "ioman.h"
#include "keystrs.h"
#include "ptrman.h"
#include "seisinfo.h"
#include "seistrc.h"
#include "separstr.h"
#include "statruncalc.h"
#include "streamconn.h"
#include "survinfo.h"
#include "tabledef.h"

#include <math.h>

static const char* sKeyScaled = "Scaled";


Wavelet::Wavelet( const char* nm )
	: NamedObject(nm)
	, cidx_(0)
	, dpos_(SeisTrcInfo::defaultSampleInterval(true))
	, sz_(0)
	, samps_(0)
{
}


Wavelet::Wavelet( bool isricker, float fpeak, float sr, float scale )
    	: dpos_(sr)
    	, sz_(0)
	, samps_(0)
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
	: sz_(0)
	, samps_(0)
{
    *this = wv;
}


Wavelet& Wavelet::operator =( const Wavelet& wv )
{
    cidx_ = wv.cidx_;
    dpos_ = wv.dpos_;
    reSize( wv.size() );
    if ( sz_ ) memcpy( samps_, wv.samps_, sz_*sizeof(float) );
    return *this;
}


Wavelet::~Wavelet()
{
    delete [] samps_;
}


Wavelet* Wavelet::get( const IOObj* ioobj )
{
    if ( !ioobj ) return 0;
    PtrMan<WaveletTranslator> tr = (WaveletTranslator*)ioobj->createTranslator();
    if ( !tr ) return 0;
    Wavelet* newwv = 0;

    Conn* connptr = ioobj->getConn( Conn::Read );
    if ( !connptr || connptr->bad() )
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
    PtrMan<WaveletTranslator> tr =
        (WaveletTranslator*)ioobj->createTranslator();
    if ( !tr ) return false;
    bool retval = false;

    Conn* connptr = ioobj->getConn( Conn::Write );
    if ( connptr && !connptr->bad() )
    {
	if ( tr->write( this, *connptr ) )
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
    const int outsz = mNINT32( samplePositions().width() / newsr ) + 1;

    WaveletFFTData inp(sz_,dpos_), out(outsz,newsr);

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
	const bool isabovenf = abs(freq) > inp.nyqfreq_;
	const float realval = isabovenf ? 0.f : spectrumreal.getValue( freq );
	const float imagval = isabovenf ? 0.f : spectrumimag.getValue( freq );
	const float_complex val( realval, imagval );
	out.cfreqwvlt_.set( idx, val );
    }

    if ( !out.doFFT(false) )
	return false;

    reSize( outsz );
    const float starttwtinp = samplePositions().start;
    dpos_ = newsr;
    cidx_ = mNINT32( -starttwtinp / dpos_ );
    const int revfirstidx = out.halfsz_ - cidx_;
    const float normfact = ((float)out.sz_) / inp.sz_;
    for ( int idx=0; idx<sz_; idx++ )
	samps_[idx] = normfact * out.ctwtwvlt_.get( idx + revfirstidx ).real();

    return true;
}


bool Wavelet::reSampleTime( float newsr )
{
    if ( newsr < 1e-6 )
	return false;
    float fnewsz = (sz_-1) * dpos_ / newsr + 1 - 1e-5;
    const int newsz = mNINT32( ceil(fnewsz) );
    float* newsamps = new float [newsz];
    if ( !newsamps )
	return false;

    StepInterval<float> twtrg = samplePositions();
    twtrg.step = dpos_;
    SeisTrc wvlttrc( sz_ );
    wvlttrc.info().sampling.start = twtrg.start;
    wvlttrc.info().sampling.step = twtrg.step;
    for ( int idx=0; idx<sz_; idx++ )
	wvlttrc.set( idx, samps_[idx], 0 );

    sz_ = newsz;
    twtrg.step = newsr;
    delete [] samps_; samps_ = newsamps;
    for ( int idx=0; idx<sz_; idx++ )
	samps_[idx] = wvlttrc.getValue( twtrg.atIndex(idx), 0 );
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
    transform( 0, 1.f/mMAX( fabs(getExtrValue(true)),
		  fabs(getExtrValue(false))) );
}


float Wavelet::getExtrValue( bool ismax ) const
{
    Stats::RunCalc<float> rc( Stats::CalcSetup().require(Stats::Max) );
    rc.addValues( sz_, samps_ );
    return ismax ? rc.max() : rc.min();
}


static void markWaveletScaled( const MultiID& id, const char* val )
{
    IOObj* ioobj = IOM().get( id );
    if ( !ioobj ) return;
    ioobj->pars().set( sKeyScaled, val );
    IOM().commitChanges( *ioobj );
    delete ioobj;
}


void Wavelet::markScaled( const MultiID& id )
{
    markWaveletScaled( id, "External" );
}


void Wavelet::markScaled( const MultiID& id, const MultiID& orgid,
			  const MultiID& horid, const MultiID& seisid,
			  const char* lvlnm )
{
    FileMultiString fms( orgid.buf() );
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
    if ( val.isEmpty() ) return false;
    FileMultiString fms( val );
    const int fmssz = fms.size();
    if ( fmssz < 3 )
	{ orgid = "0"; return true; }

    orgid = fms[0]; horid = fms[1]; seisid = fms[2]; lvlnm = fms[3];
    return true;
}


int WaveletTranslatorGroup::selector( const char* key )
{
    int retval = defaultSelector( theInst().userName(), key );
    if ( retval ) return retval;

    if ( defaultSelector("Wavelet directory",key)
      || defaultSelector("Seismic directory",key) ) return 1;
    return 0;
}

mDefSimpleTranslatorioContext(Wavelet,Seis)


static const char* sLength	= "Length";
static const char* sIndex	= "Index First Sample";
static const char* sSampRate	= "Sample Rate";


bool dgbWaveletTranslator::read( Wavelet* wv, Conn& conn )
{
    if ( !conn.forRead() || !conn.isStream() )	return false;

    ascistream astream( ((StreamConn&)conn).iStream() );
    if ( !astream.isOfFileType(mTranslGroupName(Wavelet)) )
	return false;

    const float scfac = 1000;
    int cidx = 0; float sr = scfac * SI().zStep();
    while ( !atEndOfSection( astream.next() ) )
    {
        if ( astream.hasKeyword( sLength ) )
	    wv->reSize( astream.getIValue() );
        else if ( astream.hasKeyword( sIndex ) )
	    cidx = -1 * astream.getIValue();
        else if ( astream.hasKeyword(sKey::Name()) )
	    wv->setName( astream.value() );
        else if ( astream.hasKeyword( sSampRate ) )
	    sr = astream.getFValue();
    }
    wv->setSampleRate( sr / scfac );
    wv->setCenterSample( cidx );

    for ( int idx=0; idx<wv->size(); idx++ )
	astream.stream() >> wv->samples()[idx];

    return astream.stream().good();
}


bool dgbWaveletTranslator::write( const Wavelet* wv, Conn& conn )
{
    if ( !conn.forWrite() || !conn.isStream() )	return false;

    ascostream astream( ((StreamConn&)conn).oStream() );
    const BufferString head( mTranslGroupName(Wavelet), " file" );
    if ( !astream.putHeader( head ) ) return false;

    if ( *(const char*)wv->name() ) astream.put( sKey::Name(), wv->name() );
    astream.put( sLength, wv->size() );
    astream.put( sIndex, -wv->centerSample() );
    astream.put( sSampRate, wv->sampleRate() * SI().zDomain().userFactor() );
    astream.newParagraph();
    for ( int idx=0; idx<wv->size(); idx++ )
	astream.stream() << wv->samples()[idx] << '\n';
    astream.newParagraph();

    return astream.stream().good();
}


Table::FormatDesc* WaveletAscIO::getDesc()
{
    Table::FormatDesc* fd = new Table::FormatDesc( "Wavelet" );
    fd->headerinfos_ += new Table::TargetInfo( "Sample interval",
	    		FloatInpSpec(SI().zRange(true).step), Table::Required,
			PropertyRef::surveyZType() );
    fd->headerinfos_ += new Table::TargetInfo( "Center sample",
						IntInpSpec(), Table::Optional );
    fd->bodyinfos_ += new Table::TargetInfo( "Data samples", FloatInpSpec(),
					     Table::Required );
    return fd;
}


#define mErrRet(s) { if ( s ) errmsg_ = s; return 0; }

Wavelet* WaveletAscIO::get( std::istream& strm ) const
{
    if ( !getHdrVals(strm) )
	return 0;

    float sr = getfValue( 0 );
    if ( sr == 0 || mIsUdf(sr) )
	sr = SI().zStep();
    else if ( sr < 0 )
	sr = -sr;

    int centersmp = -1 * getIntValue( 1 );
    if ( !mIsUdf(centersmp) && centersmp < 0 )
	centersmp = -centersmp + 1; // Users start at 1

    TypeSet<float> samps;
    while ( true )
    {
	int ret = getNextBodyVals( strm );
	if ( ret < 0 ) mErrRet(0)
	if ( ret == 0 ) break;

	float val = getfValue( 0 );
	if ( !mIsUdf(val) ) samps += val;
    }

    if ( samps.isEmpty() )
	mErrRet( "No valid data samples found" )
    if ( mIsUdf(centersmp) || centersmp > samps.size() )
	centersmp = samps.size() / 2;

    Wavelet* ret = new Wavelet( "" );
    ret->reSize( samps.size() );
    ret->setCenterSample( centersmp  );
    ret->setSampleRate( sr );
    memcpy( ret->samples(), samps.arr(), ret->size() * sizeof(float) );
    return ret;
}


bool WaveletAscIO::put( std::ostream& ) const
{
    errmsg_ = "TODO: WaveletAscIO::put not implemented";
    return false;
}
