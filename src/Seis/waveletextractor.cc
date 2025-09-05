/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "waveletextractor.h"

#include "arrayndimpl.h"
#include "arrayndalgo.h"
#include "binidvalset.h"
#include "bufstringset.h"
#include "trckeyzsampling.h"
#include "fourier.h"
#include "genericnumer.h"
#include "seisioobjinfo.h"
#include "seisread.h"
#include "seisselectionimpl.h"
#include "seistrc.h"
#include "survinfo.h"
#include "wavelet.h"
#include "waveletattrib.h"

WaveletExtractor::WaveletExtractor( const IOObj& ioobj, int wvltsize )
    : Executor( "Extracting Wavelet" )
    , wvlt_(*new Wavelet)
    , iobj_(ioobj)
    , fft_( Fourier::CC::createDefault() )
    , lineidx_(-1)
    , paramval_( mUdf(float) )
    , wvltsize_(wvltsize)
    , phase_(0)
    , nrusedtrcs_(0)
    , nrdone_(0)
    , totalnr_(0)
    , msg_(tr("Extracting wavelet"))
{
    fft_->setInputInfo( Array1DInfoImpl(wvltsize_) );
    fft_->setDir( true );

    initWavelet( iobj_ );
}


WaveletExtractor::~WaveletExtractor()
{
    delete fft_;
    delete seisrdr_;
    delete &wvlt_;
}


void WaveletExtractor::initWavelet( const IOObj& ioobj )
{
    TrcKeyZSampling cs;
    PtrMan<SeisIOObjInfo> si = new SeisIOObjInfo( ioobj );
    si->getRanges( cs );
    wvlt_.reSize( wvltsize_ );
    wvlt_.setSampleRate( cs.zsamp_.step_ );
    wvlt_.setCenterSample( wvltsize_/2 );
    for ( int samp=0; samp<wvltsize_; samp++ )
	wvlt_.samples()[samp] = 0;
}


void WaveletExtractor::init3D()
{
    const Seis::GeomType gt = Seis::Vol;
    delete seisrdr_;
    seisrdr_ = new SeisTrcReader( iobj_, &gt );
    if ( !sd_->isAll() )
	seisrdr_->setSelData( sd_->clone() );

    seisrdr_->prepareWork();
    isbetweenhor_ = false;

    mDynamicCastGet(const Seis::RangeSelData*,rsd,sd_)
    mDynamicCastGet(const Seis::TableSelData*,tsd,sd_)
    if ( tsd )
	isbetweenhor_ = tsd->binidValueSet().hasDuplicateBinIDs();

    if ( rsd )
	totalnr_ = rsd->cubeSampling().hsamp_.totalNr();
    else if ( tsd && isbetweenhor_ )
	totalnr_ = tsd->binidValueSet().nrDuplicateBinIDs();
    else if ( tsd )
	totalnr_ = tsd->binidValueSet().totalSize();
}


void WaveletExtractor::init2D()
{
    StepInterval<int> range;
    for ( int idx=0; idx<sdset_.size(); idx++ )
    {
	range.set( sdset_[idx]->crlRange(), 1 );
	totalnr_ += range.nrSteps() + 1;
    }

    getNextLine();
}


void WaveletExtractor::setSelData( const Seis::SelData& sd )
{
    sd_ = &sd;
    init3D();
}


void WaveletExtractor::setSelData( const ObjectSet<Seis::SelData>& sdset )
{
    sdset_ = sdset;
    init2D();
}


bool WaveletExtractor::getNextLine()
{
    lineidx_++;
    if ( lineidx_ >= sdset_.size() )
	return false;

    const Seis::SelData* sd = sdset_[lineidx_];
    const Seis::GeomType gt = Seis::Line;
    const Pos::GeomID gid = sd->geomID();
    delete seisrdr_;
    seisrdr_ = new SeisTrcReader( iobj_, gid, &gt );
    if ( !sd->isAll() )
	seisrdr_->setSelData( sd->clone() );

    seisrdr_->prepareWork();

    return true;
}


uiString WaveletExtractor::uiNrDoneText() const
{ return tr("Traces Processed"); }


uiString WaveletExtractor::uiMessage() const
{ return msg_; }


int WaveletExtractor::nextStep()
{
    SeisTrc trc;
    const int res = seisrdr_->get( trc.info() );

    if ( res == -1 )
    {
	msg_ = tr("Error reading input data");
	return ErrorOccurred();
    }

    if ( res == 0 )
    {
	if ( seisrdr_->is2D() && getNextLine() )
	    return MoreToDo();

	return finish(nrusedtrcs_) ? Finished() : ErrorOccurred();
    }

    if ( res == 2 )
	return MoreToDo();

    if ( res == 1 )
    {
	seisrdr_->get( trc );
	if ( trc.isNull() )
	    return MoreToDo();

	if ( trc.zRange().width(false) <  wvlt_.samplePositions().width(false) )
	    return MoreToDo();

	int startsample, signalsz;
	if ( !getSignalInfo(trc,startsample,signalsz) )
	    return MoreToDo();

	if ( processTrace(trc,startsample,signalsz) )
	    nrusedtrcs_++;
    }

    nrdone_++;

    return MoreToDo();
}


bool WaveletExtractor::getSignalInfo( const SeisTrc& trc, int& startsample,
				      int& signalsz ) const
{
    mDynamicCastGet(const Seis::TableSelData*,tsd,sd_);
    if ( !tsd )
	return getSignalInfoBetweenZ( trc, startsample, signalsz );

    const BinIDValueSet& bvis = tsd->binidValueSet();
    Interval<float> extz = tsd->extraZ();
    BinID bid = trc.info().binID();
    float z1(mUdf(float)), z2(mUdf(float));
    BinID duplicatebid;
    BinIDValueSet::SPos pos = bvis.find( bid );
    bvis.get( pos, bid, z1 );
    if ( isbetweenhor_ )
    {
	bvis.next( pos );
	bvis.get( pos, duplicatebid, z2  );
	if ( duplicatebid != bid || mIsUdf(z2) )
	    return false;
    }
    else
	z2 = z1;

    if ( z2 < z1 )
	Swap( z1, z2 );

    if( !trc.dataPresent(z1+extz.start_) || !trc.dataPresent(z2+extz.stop_) )
	return false;

    startsample = trc.nearestSample( z1 + extz.start_ );
    const int stopsample = trc.nearestSample( z2 + extz.stop_ );
    signalsz = stopsample - startsample + 1;

    return signalsz >= wvltsize_;
}


bool WaveletExtractor::getSignalInfoBetweenZ( const SeisTrc& trc,
					int& startsample, int& signalsz ) const
{
    mDynamicCastGet(const Seis::RangeSelData*,rsd,sd_);
    if ( !rsd )
	return getSignalInfoFull( trc, startsample, signalsz );

    const Interval<float> zrg = rsd->zRange();
    if ( zrg.isUdf() )
	return getSignalInfoFull( trc, startsample, signalsz );

    float z1 = rsd->zRange().start_;
    float z2 = rsd->zRange().stop_;
    if ( z2 < z1 )
	Swap( z1, z2 );

    if( !trc.dataPresent(z1) || !trc.dataPresent(z2) )
	return false;

    startsample = trc.nearestSample( z1 );
    const int stopsample = trc.nearestSample( z2 );
    signalsz = stopsample - startsample + 1;

    return signalsz >= wvltsize_;
}


bool WaveletExtractor::getSignalInfoFull( const SeisTrc& trc,
					int& startsample, int& signalsz ) const
{
    startsample = 0;
    signalsz = trc.size();
    return true;
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

    const uiRetVal uirv = processTrace( signal, *fft_,
					wvltsize_, paramval_, wvlt_ );
    return uirv.isOK();
}


uiRetVal WaveletExtractor::processTrace( Array1D<float>& signal,
					 Fourier::CC& fft, int wvltsize,
					 float taperval, Wavelet& wvlt )
{
    uiRetVal ret;
    if ( !wvltsize )
	ret.add( tr("Please provide a valid wavelet") );

    if ( signal.isEmpty() )
	ret.add( tr("Invalid trace data") );

    if ( mIsUdf(taperval) )
	ret.add( tr("Invalid taper parameter value") );

    const int signalsz = signal.size();
    ArrayNDWindow window( signal.info(), true, "CosTaper", taperval );
    window.apply( &signal );

    Array1DImpl<float> acarr( signalsz );
    float* acarrptr = acarr.arr();
    genericCrossCorrelation( signalsz, 0, signal.arr(),
			     signalsz, 0, signal.arr(),
			     signalsz, -signalsz/2, acarrptr );

    Array1DImpl<float> temp( wvltsize );
    const int startidx = (signalsz/2) - ((wvltsize-1)/2);
    for ( int idx=0; idx<wvltsize; idx++ )
	temp.set( idx, acarr.get( startidx+idx ) );

    removeBias<float,float>( temp );
    normalization( temp, wvltsize );

    Array1DImpl<float_complex> freqdomsignal( wvltsize );
    Array1DImpl<float_complex> timedomsignal( wvltsize );
    for ( int idx=0; idx<wvltsize; idx++ )
	timedomsignal.set( idx, temp.arr()[idx] );

    fft.setInput( timedomsignal.getData() );
    fft.setOutput( freqdomsignal.getData() );
    fft.run( true );

    for ( int idx=0; idx<wvltsize; idx++ )
    {
	const float val = std::abs( freqdomsignal.arr()[idx] );
	wvlt.samples()[idx] += val;
    }

    return ret;
}


void WaveletExtractor::normalization( Array1DImpl<float>& normal )
{
    normalization( normal, wvltsize_ );
}


void WaveletExtractor::normalization( Array1D<float>& normal, int wvltsize )
{
    float maxval = fabs( normal.arr()[0] );
    for ( int idx=1; idx<wvltsize; idx++ )
    {
	float val = fabs( normal.arr()[idx] );
	if( val > maxval )
	    maxval = val;
    }

    if( mIsZero(maxval, 1e-6f) )
	return;

    for( int idx=0; idx<wvltsize; idx++ )
	normal.arr()[idx] = (normal.arr()[idx])/(maxval);
}


bool WaveletExtractor::finish( int nrusedtrcs )
{
    if ( nrusedtrcs == 0 )
    {
	msg_ = tr("No valid traces read");
	return false;
    }

    float* stackedarr = wvlt_.samples();
    stackedarr[0] = 0;
    for ( int i=1; i<wvltsize_; i++ )
	stackedarr[i] = Math::Sqrt( stackedarr[i] / nrusedtrcs );

    if ( !finalize(*fft_,wvlt_,wvltsize_,phase_,paramval_) )
    {
	msg_ = tr("Failed to generate wavelet");
	return false;
    }

    return true;
}


bool WaveletExtractor::finalize( Fourier::CC& fft, Wavelet& wvlt, int wvltsize,
				 double phase, float paramval )
{
    double angle = phase * M_PI/180;
    return doWaveletIFFT( fft, wvlt, wvltsize )
	&& rotateWavelet( wvlt,wvltsize,angle )
	&& taperWavelet( wvlt, wvltsize, paramval );
}


void WaveletExtractor::setTaperParamVal( float paramval )
{ paramval_ = paramval; }


void WaveletExtractor::setPhase( int phase )
{ phase_ = phase; }


bool WaveletExtractor::doWaveletIFFT()
{
    return doWaveletIFFT( *fft_, wvlt_, wvltsize_ );
}


bool WaveletExtractor::doWaveletIFFT( Fourier::CC& fft,
				      Wavelet& wvlt, int wvltsize )
{
    if ( !wvltsize || wvltsize>wvlt.size() )
	return false;

    fft.setDir( false );

    Array1DImpl<float_complex> signal( wvltsize ), transfsig( wvltsize );
    for ( int idx=0; idx<wvltsize; idx++ )
	signal.set( idx, wvlt.samples()[idx] );

    fft.setInput( signal.getData() );
    fft.setOutput( transfsig.getData() );
    fft.run( true );

    for ( int idx=0; idx<wvltsize; idx++ )
    {
	if ( idx>=wvltsize/2 )
	    wvlt.samples()[idx] = transfsig.get( idx - wvltsize/2 ).real();
	else
	    wvlt.samples()[idx] = transfsig.get( wvltsize/2 - idx ).real();
    }

    return true;
}


bool WaveletExtractor::rotateWavelet()
{
    double angle = phase_ * M_PI/180;
    return rotateWavelet( wvlt_, wvltsize_, angle );
}


bool WaveletExtractor::rotateWavelet( Wavelet& wvlt,
				      int wvltsize, double phase )
{
    if ( !wvltsize || wvltsize>wvlt.size() || mIsUdf(phase) )
	return false;

    Array1DImpl<float> rotatewvlt( wvltsize );
    for ( int idx=0; idx<wvltsize; idx++ )
	rotatewvlt.set( idx, wvlt.samples()[idx] );

    WaveletAttrib wvltattr( wvlt );
    wvltattr.getHilbert( rotatewvlt );

    for ( int idx=0; idx<wvltsize; idx++ )
    {
	const float realval = wvlt.samples()[idx];
	const float imagval = -rotatewvlt.arr()[idx];
	wvlt.samples()[idx] = (float) (realval*cos(phase)-imagval*sin(phase));
    }

    return true;
}


bool WaveletExtractor::taperWavelet()
{
    return taperWavelet( wvlt_, wvltsize_, paramval_ );
}


bool WaveletExtractor::taperWavelet( Wavelet& wvlt,
				     int wvltsize, float paramval )
{
    if ( !wvltsize || wvltsize>wvlt.size() || mIsUdf(paramval) )
	return false;

    Array1DImpl<float> taperwvlt( wvltsize );
    for ( int idx=0; idx<wvltsize; idx++ )
	taperwvlt.set( idx, wvlt.samples()[idx] );

    ArrayNDWindow window( taperwvlt.info(), true, "CosTaper", paramval );
    window.apply( &taperwvlt );
    WaveletAttrib::muteZeroFrequency( taperwvlt );
    for ( int samp=0; samp<wvltsize; samp++ )
	wvlt.samples()[samp] = taperwvlt.arr()[samp];

    return true;
}


const Wavelet& WaveletExtractor::getWavelet() const
{ return wvlt_; }
