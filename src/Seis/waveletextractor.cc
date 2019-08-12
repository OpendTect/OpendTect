/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nageswara
 Date:          April 2009
 ________________________________________________________________________

-*/

#include "waveletextractor.h"

#include "arrayndimpl.h"
#include "arrayndalgo.h"
#include "binnedvalueset.h"
#include "ioobj.h"
#include "trckeyzsampling.h"
#include "fourier.h"
#include "genericnumer.h"
#include "seisioobjinfo.h"
#include "seisprovider.h"
#include "seistableseldata.h"
#include "seistrc.h"
#include "survinfo.h"
#include "wavelet.h"
#include "waveletattrib.h"

WaveletExtractor::WaveletExtractor( const IOObj& ioobj, int wvltsize )
    : Executor( "Extracting Wavelet" )
    , iobj_(ioobj)
    , sd_(0)
    , wvltsize_(wvltsize)
    , phase_(0)
    , nrusedtrcs_(0)
    , nrdone_(0)
    , fft_( Fourier::CC::createDefault() )
    , relzwin_(0.f,0.f)
    , totalnr_(0)
    , msg_(tr("Extracting wavelet"))
    , wvlt_(new Wavelet)
    , lineidx_(-1)
    , paramval_( mUdf(float) )
{
    fft_->setInputInfo( Array1DInfoImpl(wvltsize_) );
    fft_->setDir( true );

    initWavelet( ioobj );
    uiRetVal uirv;
    seisprov_ = Seis::Provider::create( ioobj, &uirv );
    if ( !seisprov_ )
	msg_ = uirv;
}


WaveletExtractor::~WaveletExtractor()
{
    delete fft_;
    delete seisprov_;
    delete sd_;
}


void WaveletExtractor::initWavelet( const IOObj& ioobj )
{
    TrcKeyZSampling cs;
    PtrMan<SeisIOObjInfo> si = new SeisIOObjInfo( ioobj );
    si->getRanges( cs );
    wvlt_->setSampleRate( cs.zsamp_.step );
    wvlt_->setCenterSample( wvltsize_/2 );
    wvlt_->reSize( wvltsize_, 0.f );
}


void WaveletExtractor::setSelData( const Seis::SelData& sd )
{
    sd_ = sd.clone();
    totalnr_ = sd_->expectedNrTraces();
    seisprov_->setSelData( *sd_ );
    isbetweenhor_ = false;
    const auto* tsd = sd_->asTable();
    if ( !tsd )
	return;
    isbetweenhor_ = tsd->binidValueSet().hasDuplicatePositions();
    if ( isbetweenhor_ )
	totalnr_ = tsd->binidValueSet().nrDuplicatePositions();
}


uiString WaveletExtractor::nrDoneText() const
{ return tr("Traces Processed"); }


uiString WaveletExtractor::message() const
{ return msg_; }


int WaveletExtractor::nextStep()
{
    if ( !seisprov_ ) return ErrorOccurred();

    SeisTrc trc;
    const uiRetVal uirv = seisprov_->getNext( trc );
    if ( !uirv.isOK() )
    {
	if ( !isFinished(uirv) )
	    { msg_ = uirv; return ErrorOccurred(); }

	return finish(nrusedtrcs_) ? Finished() : ErrorOccurred();
    }

    if ( trc.isNull() )
	return MoreToDo();

    int startsample, signalsz;
    if ( !getSignalInfo(trc,startsample,signalsz) )
	return MoreToDo();

    if ( processTrace(trc,startsample,signalsz) )
	nrusedtrcs_++;

    nrdone_++;
    return MoreToDo();
}


bool WaveletExtractor::getSignalInfo( const SeisTrc& trc, int& startsample,
				      int& signalsz ) const
{
    mDynamicCastGet(const Seis::TableSelData*,tsd,sd_);
    if ( !tsd )
	{ startsample = 0; signalsz = trc.size(); return true; }

    if ( trc.zRange().width(false) <  wvlt_->samplePositions().width(false) )
	return false;

    const BinnedValueSet& bvis = tsd->binidValueSet();
    BinID bid = trc.info().binID();
    float z1(mUdf(float)), z2(mUdf(float));
    BinID duplicatebid;
    BinnedValueSet::SPos pos = bvis.find( bid );
    bvis.get( pos, bid, z1 );

    if ( !isbetweenhor_ )
	z2 = z1;
    else
    {
	bvis.next( pos );
	bvis.get( pos, duplicatebid, z2  );
	if ( duplicatebid != bid || mIsUdf(z2) )
	    return false;
    }

    if ( z2 < z1 )
	std::swap( z1, z2 );

    if ( !trc.dataPresent(z1 + relzwin_.start)
      || !trc.dataPresent(z2 + relzwin_.stop) )
	return false;

    startsample = trc.nearestSample( z1 + relzwin_.start );
    const int stopsample = trc.nearestSample( z2 + relzwin_.stop );
    signalsz = stopsample - startsample + 1;

    return signalsz >= wvltsize_;
}


bool WaveletExtractor::processTrace( const SeisTrc& trc, int startsample,
				     int signalsz )
{
    Array1DImpl<float> signal( signalsz );
    bool foundundef = false;
    int count = 0;
    for ( int sidx=0; sidx<signalsz; sidx++ )
    {
	const int trcsidx = startsample + sidx;
	const float val = trc.get( trcsidx, 0 );
	if ( !mIsUdf(val) )
	{
	    signal.set( sidx, val );
	    if ( val == 0 )
		count++;
	}
	else
	{
	    foundundef = true;
	    break;
	}
    }

    if ( foundundef )
	return false;

    if ( count == signalsz || count == signalsz-1 )
	return false;

    ArrayNDWindow window( signal.info(), true, "CosTaper", paramval_ );
    window.apply( &signal );

    Array1DImpl<float> acarr( signalsz );
    float* acarrptr = acarr.arr();
    genericCrossCorrelation( signalsz, 0, signal.arr(),
			     signalsz, 0, signal.arr(),
			     signalsz,  -signalsz/2, acarrptr );

    Array1DImpl<float> temp( wvltsize_ );
    const int startidx = (signalsz/2) - ((wvltsize_-1)/2);
    for ( int idx=0; idx<wvltsize_; idx++ )
	temp.set( idx, acarr.get( startidx+idx ) );

    ArrayMath::removeBias<float,double,double>( temp, true, true );
    doNormalisation( temp );

    Array1DImpl<float_complex> freqdomsignal( wvltsize_ );
    Array1DImpl<float_complex> timedomsignal( wvltsize_ );
    for ( int idx=0; idx<wvltsize_; idx++ )
	timedomsignal.set( idx, temp.arr()[idx] );

    fft_->setInput( timedomsignal.getData() );
    fft_->setOutput( freqdomsignal.getData() );
    fft_->run( true );

    TypeSet<float> samps; wvlt_->getSamples( samps );
    for ( int idx=0; idx<wvltsize_; idx++ )
    {
	const float val = std::abs( freqdomsignal.arr()[idx] );
	samps[idx] += val;
    }

    wvlt_->setSamples( samps );
    return true;
}


void WaveletExtractor::doNormalisation( Array1DImpl<float>& normal )
{
    float maxval = fabs( normal.arr()[0] );
    for ( int idx=1; idx<wvltsize_; idx++ )
    {
	float val = fabs( normal.arr()[idx] );
	if( val > maxval )
	    maxval = val;
    }

    if( mIsZero(maxval, 1e-6f) )
	return;

    for( int idx=0; idx<wvltsize_; idx++ )
	normal.arr()[idx] = (normal.arr()[idx])/(maxval);
}


bool WaveletExtractor::finish( int nrusedtrcs )
{
    if ( nrusedtrcs == 0 )
	{ msg_ = tr("No valid traces read"); return false; }

    TypeSet<float> samps; wvlt_->getSamples( samps );
    samps[0] = 0;
    for ( int i=1; i<wvltsize_; i++ )
	samps[i] = Math::Sqrt( samps[i] / nrusedtrcs );

    float* samparr = samps.arr();
    if ( !doWaveletIFFT(samparr)
      || !rotateWavelet(samparr)
      || !taperWavelet(samparr) )
	{ msg_ = tr("Failed to generate wavelet"); return false; }

    wvlt_->setSamples( samps );
    return true;
}


void WaveletExtractor::setTaperParamVal( float paramval )
{ paramval_ = paramval; }


void WaveletExtractor::setPhase( int phase )
{ phase_ = phase; }


bool WaveletExtractor::doWaveletIFFT( float* samps )
{
    fft_->setDir( false );

    Array1DImpl<float_complex> signal( wvltsize_ ), transfsig( wvltsize_ );
    for ( int idx=0; idx<wvltsize_; idx++ )
	signal.set( idx, samps[idx] );

    fft_->setInput( signal.getData() );
    fft_->setOutput( transfsig.getData() );
    fft_->run( true );

    for ( int idx=0; idx<wvltsize_; idx++ )
    {
	if ( idx>=wvltsize_/2 )
	    samps[idx] = transfsig.get( idx - wvltsize_/2 ).real();
	else
	    samps[idx] = transfsig.get( wvltsize_/2 - idx ).real();
    }

    return true;
}


bool WaveletExtractor::rotateWavelet( float* samps )
{
    Array1DImpl<float> rotatewvlt( wvltsize_ );
    for ( int idx=0; idx<wvltsize_; idx++ )
	rotatewvlt.set( idx, samps[idx] );

    WaveletAttrib wvltattr( *wvlt_ );
    wvltattr.getHilbert( rotatewvlt );

    double angle = phase_ * M_PI/180;
    for ( int idx=0; idx<wvltsize_; idx++ )
    {
	const float realval = samps[idx];
	const float imagval = -rotatewvlt.arr()[idx];
	samps[idx] = (float) (realval*cos(angle)-imagval*sin(angle));
    }

    return true;
}


bool WaveletExtractor::taperWavelet( float* samps )
{
    Array1DImpl<float> taperwvlt( wvltsize_ );
    for ( int idx=0; idx<wvltsize_; idx++ )
	taperwvlt.set( idx, samps[idx] );

    ArrayNDWindow window( taperwvlt.info(), true, "CosTaper", paramval_ );
    window.apply( &taperwvlt );
    WaveletAttrib::muteZeroFrequency( taperwvlt );
    for ( int samp=0; samp<wvltsize_; samp++ )
	samps[samp] = taperwvlt.arr()[samp];

    return true;
}


RefMan<Wavelet> WaveletExtractor::getWavelet() const
{
    return wvlt_;
}
