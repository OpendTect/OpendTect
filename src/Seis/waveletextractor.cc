/*+
________________________________________________________________________
            
 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nageswara
 Date:          April 2009
 RCS:           $Id: waveletextractor.cc,v 1.3 2009-11-19 10:21:20 cvsnageswara Exp $ 
 ________________________________________________________________________
                   
-*/   

#include "waveletextract.h"

#include "arrayndimpl.h"
#include "arrayndutils.h"
#include "binidvalset.h"
#include "bufstringset.h"
#include "cubesampling.h"
#include "fft.h"
#include "genericnumer.h"
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
    , fft_( new FFT() )
    , totalnr_(0)
    , msg_("Extracting wavelet")
    , wvlt_(*new Wavelet)
    , lineidx_(-1)
{
    initFFT();
    initWavelet();
    seisrdr_ = new SeisTrcReader( &ioobj );
}


WaveletExtractor::~WaveletExtractor()
{
    delete fft_;
    delete seisrdr_;
    delete &wvlt_;
}


void WaveletExtractor::initWavelet()
{
    wvlt_.reSize( wvltsize_ );
    for ( int samp=0; samp<wvltsize_; samp++ )
	wvlt_.samples()[samp] = 0;
}


void WaveletExtractor::init3D()
{
    seisrdr_->setSelData( sd_->clone() );
    seisrdr_->prepareWork();
    isdouble_ = false;

    mDynamicCastGet(const Seis::RangeSelData*,rsd,sd_)
    mDynamicCastGet(const Seis::TableSelData*,tsd,sd_)
    if ( tsd )
	isdouble_ = tsd->binidValueSet().hasDuplicateBinIDs();
    if ( rsd )
	totalnr_ = rsd->cubeSampling().hrg.totalNr();
    else if ( tsd && isdouble_ )
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


void WaveletExtractor::setSelData( ObjectSet<Seis::SelData>& sdset )
{
    sdset_ = sdset;
    init2D();
}


void WaveletExtractor::initFFT()
{
    fft_->setInputInfo( Array1DInfoImpl(wvltsize_) );
    fft_->setDir( true );
    fft_->init();
}


bool WaveletExtractor::getNextLine()
{
    if ( seisrdr_ )
	delete seisrdr_;
    seisrdr_ = new SeisTrcReader( &iobj_ );
    if ( !seisrdr_ )
	return false;
    lineidx_++;
    if ( lineidx_ >= sdset_.size() )
	return false;

    seisrdr_->setSelData( sdset_[lineidx_]->clone() );
    seisrdr_->prepareWork();
    return true;
}


const char* WaveletExtractor::nrDoneText() const
{ return "Traces Processed"; }


const char* WaveletExtractor::message() const
{ return msg_.buf(); }


int WaveletExtractor::nextStep()
{
    SeisTrc trc;
    const int trcinfo = seisrdr_->get( trc.info() );

    if ( trcinfo == -1 )
	return ErrorOccurred();

    if ( trcinfo == 0 )
    {
	if ( seisrdr_->is2D() && getNextLine() )
	    return MoreToDo();
	if ( finish(nrusedtrcs_) )
	    return Finished();
	else
	{
	    msg_ = "Problem while reading data. Please change selection";
	    return ErrorOccurred();
	}
    }

    if ( trcinfo == 2 )
	return MoreToDo();

    if ( trcinfo ==1 )
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

    const BinIDValueSet& bvis = tsd->binidValueSet();
    Interval<float> extz = tsd->extraZ();
    BinID bid = trc.info().binid;
    float z1(mUdf(float)), z2(mUdf(float));
    BinID duplicatebid;
    BinIDValueSet::Pos pos = bvis.findFirst( bid );
    bvis.get( pos, bid, z1 );

    if ( !isdouble_ )
      	z2 = z1;
    else
    {
	bvis.next( pos );
	bvis.get( pos, duplicatebid, z2 );
	if ( duplicatebid != bid || mIsUdf(z2) )
	    return false;
    }

    if ( z2 < z1 ) { float tmp; mSWAP( z1, z2, tmp ); }
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

    if ( count >= signalsz || count == 1 )
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

    removeBias( &temp );
    normalisation( temp );

    Array1DImpl<float_complex> timedomsignal( wvltsize_ );
    for ( int idx=0; idx<wvltsize_; idx++ )
	timedomsignal.set( idx, temp.arr()[idx] );

    Array1DImpl<float_complex> freqdomsignal( wvltsize_ );
    fft_->transform( timedomsignal, freqdomsignal );

    for ( int idx=0; idx<wvltsize_; idx++ )
    {
	const float val = std::abs( freqdomsignal.arr()[idx] );
	wvlt_.samples()[idx] += val;
    }

    return true;
}


void WaveletExtractor::normalisation( Array1DImpl<float>& normal )
{
    float maxval = fabs( normal.arr()[0] );
    for ( int idx=1; idx<wvltsize_; idx++ )
    {
	float val = fabs( normal.arr()[idx] );
	if( val > maxval )
	    maxval = val;
    }

    for( int idx=0; idx<wvltsize_; idx++ )
	normal.arr()[idx] = (normal.arr()[idx])/(maxval);
}


bool WaveletExtractor::finish( int nrusedtrcs )
{
    if ( nrusedtrcs == 0 )
	return false;

    float * stackedarr = wvlt_.samples();
    stackedarr[0] = 0;
    for ( int i=1; i<wvltsize_; i++ )
	stackedarr[i] = sqrt( stackedarr[i] / nrusedtrcs );

    if ( !doWaveletIFFT() )
	return false;
    if ( !rotateWavelet() )
	return false;
    if ( !taperWavelet() )
	    return false;

   return true;
}


void WaveletExtractor::setCosTaperParamVal( float paramval )
{
    float val = 1-(2*paramval/( (wvltsize_-1)*SI().zStep()*SI().zFactor()) );
    paramval_ = val == 1 ? 1.0 - 1e-6 : val;
}


void WaveletExtractor::setPhase( int phase )
{
    phase_ = phase;
}


bool WaveletExtractor::doWaveletIFFT()
{
    fft_->setDir( false );
    fft_->init();

    Array1DImpl<float_complex> signal( wvltsize_ ), transfsig( wvltsize_ );

    for ( int idx=0; idx<wvltsize_; idx++ )
	signal.set( idx, wvlt_.samples()[idx] );

    fft_->transform( signal, transfsig );

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

    float angle = (float)phase_ * M_PI/180;
    for ( int idx=0; idx<wvltsize_; idx++ )
    {
	const float realval = wvlt_.samples()[idx];
	const float imagval = -rotatewvlt.arr()[idx];
	wvlt_.samples()[idx] = realval*cos( angle ) - imagval*sin( angle );
    }

    return true;
}


bool WaveletExtractor::taperWavelet()
{ 

    WaveletAttrib wvltattr( wvlt_ );

    Array1DImpl<float> taperwvlt( wvltsize_ );
    for ( int idx=0; idx<wvltsize_; idx++ )
	taperwvlt.set( idx, wvlt_.samples()[idx] );

    ArrayNDWindow window( taperwvlt.info(), true, "CosTaper", paramval_ );
    window.apply( &taperwvlt );
    wvltattr.muteZeroFrequency( taperwvlt );
    for ( int samp=0; samp<wvltsize_; samp++ )
    	wvlt_.samples()[samp] = taperwvlt.arr()[samp];

    return true;
}


Wavelet WaveletExtractor::getWavelet()
{
    return wvlt_;
}
