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
    , iobj_(ioobj)
    , sd_(0)
    , wvltsize_(wvltsize)
    , phase_(0)
    , nrusedtrcs_(0)
    , nrdone_(0)
    , fft_( Fourier::CC::createDefault() )
    , totalnr_(0)
    , msg_(tr("Extracting wavelet"))
    , wvlt_(*new Wavelet)
    , lineidx_(-1)
    , paramval_( mUdf(float) )
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
    wvlt_.setSampleRate( cs.zsamp_.step );
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
	range = sdset_[idx]->crlRange();
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
    {
	startsample = 0;
	signalsz = trc.size();
	return true;
    }

    if ( trc.zRange().width(false) <  wvlt_.samplePositions().width(false) )
	return false;

    const BinIDValueSet& bvis = tsd->binidValueSet();
    Interval<float> extz = tsd->extraZ();
    BinID bid = trc.info().binID();
    float z1(mUdf(float)), z2(mUdf(float));
    BinID duplicatebid;
    BinIDValueSet::SPos pos = bvis.find( bid );
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

    if ( z2 < z1 ) Swap( z1, z2 );

    if( !trc.dataPresent(z1 + extz.start) || !trc.dataPresent(z2 + extz.stop) )
	return false;

    startsample = trc.nearestSample( z1 + extz.start );
    const int stopsample = trc.nearestSample( z2 + extz.stop );
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

    removeBias<float,float>( temp );
    normalization( temp );

    Array1DImpl<float_complex> freqdomsignal( wvltsize_ );
    Array1DImpl<float_complex> timedomsignal( wvltsize_ );
    for ( int idx=0; idx<wvltsize_; idx++ )
	timedomsignal.set( idx, temp.arr()[idx] );

    fft_->setInput( timedomsignal.getData() );
    fft_->setOutput( freqdomsignal.getData() );
    fft_->run( true );

    for ( int idx=0; idx<wvltsize_; idx++ )
    {
	const float val = std::abs( freqdomsignal.arr()[idx] );
	wvlt_.samples()[idx] += val;
    }

    return true;
}


void WaveletExtractor::normalization( Array1DImpl<float>& normal )
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

    float * stackedarr = wvlt_.samples();
    stackedarr[0] = 0;
    for ( int i=1; i<wvltsize_; i++ )
	stackedarr[i] = Math::Sqrt( stackedarr[i] / nrusedtrcs );

    if ( !doWaveletIFFT() || !rotateWavelet() || !taperWavelet() )
    { msg_ = tr("Failed to generate wavelet"); return false; }

   return true;
}


void WaveletExtractor::setTaperParamVal( float paramval )
{ paramval_ = paramval; }


void WaveletExtractor::setPhase( int phase )
{ phase_ = phase; }


bool WaveletExtractor::doWaveletIFFT()
{
    fft_->setDir( false );

    Array1DImpl<float_complex> signal( wvltsize_ ), transfsig( wvltsize_ );
    for ( int idx=0; idx<wvltsize_; idx++ )
	signal.set( idx, wvlt_.samples()[idx] );

    fft_->setInput( signal.getData() );
    fft_->setOutput( transfsig.getData() );
    fft_->run( true );

    for ( int idx=0; idx<wvltsize_; idx++ )
    {
	if ( idx>=wvltsize_/2 )
	    wvlt_.samples()[idx] = transfsig.get( idx - wvltsize_/2 ).real();
	else
	    wvlt_.samples()[idx] = transfsig.get( wvltsize_/2 - idx ).real();
    }

    return true;
}


bool WaveletExtractor::rotateWavelet()
{
    Array1DImpl<float> rotatewvlt( wvltsize_ );
    for ( int idx=0; idx<wvltsize_; idx++ )
	rotatewvlt.set( idx, wvlt_.samples()[idx] );

    WaveletAttrib wvltattr( wvlt_ );
    wvltattr.getHilbert( rotatewvlt );

    double angle = phase_ * M_PI/180;
    for ( int idx=0; idx<wvltsize_; idx++ )
    {
	const float realval = wvlt_.samples()[idx];
	const float imagval = -rotatewvlt.arr()[idx];
	wvlt_.samples()[idx] = (float) (realval*cos(angle)-imagval*sin(angle));
    }

    return true;
}


bool WaveletExtractor::taperWavelet()
{
    Array1DImpl<float> taperwvlt( wvltsize_ );
    for ( int idx=0; idx<wvltsize_; idx++ )
	taperwvlt.set( idx, wvlt_.samples()[idx] );

    ArrayNDWindow window( taperwvlt.info(), true, "CosTaper", paramval_ );
    window.apply( &taperwvlt );
    WaveletAttrib::muteZeroFrequency( taperwvlt );
    for ( int samp=0; samp<wvltsize_; samp++ )
	wvlt_.samples()[samp] = taperwvlt.arr()[samp];

    return true;
}


const Wavelet& WaveletExtractor::getWavelet() const
{ return wvlt_; }
