/*+
________________________________________________________________________
            
 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nageswara
 Date:          April 2009
 RCS:           $Id: waveletextractor.cc,v 1.1 2009-08-07 12:34:55 cvsnageswara Exp $ 
 ________________________________________________________________________
                   
-*/   

#include "waveletextract.h"

#include "arrayndimpl.h"
#include "arrayndutils.h"
#include "binidvalset.h"
#include "cubesampling.h"
#include "fft.h"
#include "genericnumer.h"
#include "hilberttransform.h"
#include "seisselection.h"
#include "seisselectionimpl.h"
#include "seistrc.h"
#include "survinfo.h"
#include "seisread.h"


WaveletExtract::WaveletExtract( const IOObj* ioobj,const Seis::SelData* sd,
				const int wvltsize )
    : Executor( "Extracting Wavelet" )
    , wvltsize_(wvltsize)
    , phase_(0)
    , nrgoodtrcs_(0)
    , seisrdr_(0) 
    , stackedwvlt_(new Array1DImpl<float>(wvltsize))
    , sd_(sd) 
    , fft_( new FFT() )
    , nrdone_(0)
    , start_(0)
    , stop_(0)
    , msg_("Calculating wavelet")
{
    seisrdr_ = new SeisTrcReader( ioobj );
    seisrdr_->setSelData( sd_->clone() );
    seisrdr_->prepareWork();

    mDynamicCastGet(const Seis::TableSelData*,tsd,sd_)
    isdouble_ = isBetween2Hors( *tsd );

    for ( int idx=0; idx<wvltsize_; idx++ )
	stackedwvlt_->set( idx, 0 );

    fft_->setInputInfo( Array1DInfoImpl(wvltsize_) );
    fft_->setDir( true );
    fft_->init();
}


const float* WaveletExtract::getWavelet() const 
{
    return stackedwvlt_->arr();
}


WaveletExtract::~WaveletExtract()
{
    delete fft_;
    delete seisrdr_;
    delete stackedwvlt_;
}


bool WaveletExtract::isBetween2Hors( const Seis::TableSelData& sd )
{
    mDynamicCastGet(const Seis::RangeSelData*,rsd,&sd)
    if ( rsd ) return false;

    mDynamicCastGet(const Seis::TableSelData*,tsd,&sd)
    if ( !tsd ) return false;

    const BinIDValueSet& bivs = tsd->binidValueSet();
    int dupbids =  bivs.nrDuplicateBinIDs();

    return dupbids > 0 ? true : false;
}


od_int64 WaveletExtract::totalNr() const
{
    mDynamicCastGet(const Seis::RangeSelData*,rsd,sd_)
    if ( rsd ) return rsd->cubeSampling().hrg.totalNr();

    mDynamicCastGet(const Seis::TableSelData*,tsd,sd_)

    int totalnr = 0;
    return totalnr = isdouble_ ? tsd->binidValueSet().nrDuplicateBinIDs()
			       : tsd->binidValueSet().totalSize();
}


const char* WaveletExtract::nrDoneText() const
{
    return "Traces Processed";
}


const char* WaveletExtract::message() const
{
    return msg_.buf();
}


int WaveletExtract::nextStep()
{
    const Seis::TableSelData* tsd = 0;
    if ( sd_ )
	mDynamicCast(const Seis::TableSelData*,tsd,sd_)

    SeisTrc trc;
    int info = seisrdr_->get( trc.info() );

    if ( info == -1 )
	return ErrorOccurred();

    if ( info == 0 )
    {
	if ( finish(nrgoodtrcs_) )
	    return Finished();
	else
	{
	    msg_ = "Problem while reading data.Please change selection";
	    return ErrorOccurred();
	}
    }

    if ( info ==1 )
    {
	seisrdr_->get( trc );
	int signalsz = trc.size();

	if ( trc.isNull() )
	    return MoreToDo();

	start_ = 0;
	stop_ = trc.size() - 1;

	if ( tsd )
	{
	    if ( !setTraces( trc, tsd ) )
		return MoreToDo();

	    signalsz = 1 + stop_ - start_;

	    if ( signalsz < wvltsize_ )
		return MoreToDo();
	}

	if ( doStatistics( signalsz, trc ) )
	    nrgoodtrcs_++;
    }

    if ( info == 2 )
	return MoreToDo();

    nrdone_++;

    return MoreToDo();
}


bool WaveletExtract::setTraces( const SeisTrc& trc,
		                const Seis::TableSelData* tsd )
{
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

    start_ = trc.nearestSample( z1 < z2 ? z1 + extz.start
					       : z2 + extz.start );
    stop_ = trc.nearestSample( z1 < z2 ? z2 + extz.stop
					      : z1 + extz.stop );

    return true;
}


bool WaveletExtract::doStatistics( int signalsz, const SeisTrc& trc )
{
    Array1DImpl<float> signal( signalsz );
    ArrayNDWindow window( signal.info(), true, ArrayNDWindow::CosTaper5 );

    bool foundundef = false;
    int count = 0;
    for ( int sidx=start_; sidx<=stop_; sidx++ )
    {
	const float val = trc.get( sidx, 0 );
	if ( !mIsUdf(val) )
	{
	    signal.set( sidx-start_, val );

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

    window.apply( &signal );

    Array1DImpl<float> acarr( signalsz );
    float* acarrptr = acarr.arr();
    genericCrossCorrelation( signalsz, 0, signal.arr(),
			     signalsz, 0, signal.arr(),
			     signalsz,  -signalsz/2, acarrptr );

    Array1DImpl<float> temp( wvltsize_ );
    int startidx = (signalsz/2) - ((wvltsize_-1)/2);
    int endidx = (signalsz/2) + ((wvltsize_-1)/2);
    for ( int idx=0; idx<wvltsize_; idx++ )
	temp.set( idx, acarr.get( startidx+idx ) );

    removeBias( &temp );
    normalisation( temp );

    Array1DImpl<float_complex> timedomsignal( wvltsize_ );
    Array1DImpl<float_complex> freqdomsignal( wvltsize_ );
    for ( int idx=0; idx<wvltsize_; idx++ )
	timedomsignal.set( idx, temp.arr()[idx] );

    fft_->transform( timedomsignal, freqdomsignal );

    float * stackedarr = stackedwvlt_->arr();
    for ( int idx=0; idx<wvltsize_; idx++ )
    {
	const float val = std::abs( freqdomsignal.arr()[idx] );
	stackedarr[idx] += val;
    }

    return true;
}


void WaveletExtract::normalisation( Array1DImpl<float>& normal )
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


bool WaveletExtract::finish( int nrgoodtrcs )
{
    if ( nrgoodtrcs == 0 )
	return false;

    float * stackedarr = stackedwvlt_->arr();
    stackedarr[0] = 0;
    for ( int idx=1; idx<wvltsize_; idx++ )
	stackedarr[idx] = sqrt( stackedarr[idx] / nrgoodtrcs );

    Array1DImpl<float> ifftwavelet( wvltsize_ );

    if ( !doIFFT( stackedarr, ifftwavelet.arr()) ) 
	   return false; 
    if ( !calcWvltPhase( ifftwavelet.arr(), stackedarr ) ) 
	    return false;

   return true;
}


bool WaveletExtract::doIFFT( const float* in, float* out )
{
    fft_->setDir( false );
    fft_->init();

    Array1DImpl<float_complex> complexsig( wvltsize_ );
    Array1DImpl<float_complex> ifftsig( wvltsize_ );

    for ( int idx=0; idx<wvltsize_; idx++ )
	complexsig.set( idx, in[idx] );

    fft_->transform( complexsig, ifftsig );

    for ( int idx=0; idx<wvltsize_; idx++ )
    {
	if ( idx>=wvltsize_/2 )
	    out[idx] = ifftsig.get( idx - wvltsize_/2 ).real();
	else
	    out[idx] = ifftsig.get( wvltsize_/2 - idx ).real();
    }

    return true;
}


void WaveletExtract::setOutputPhase( int phase )
{
    phase_ = phase;
}


bool WaveletExtract::calcWvltPhase( const float* vals, float* valswithphase )
{
    Array1DImpl<float> wvltzerophase( wvltsize_ );
    Array1DImpl<float_complex> wvltincalcphase( wvltsize_ );
    HilbertTransform ht;
    ht.init();
    ht.setCalcRange( 0, wvltsize_, 0 );

    for ( int idx=0; idx<wvltsize_; idx++ )
	wvltzerophase.set( idx, vals[idx] );
    bool isht = ht.transform( wvltzerophase, wvltincalcphase );
    if ( !isht )
	return false;

    float angle = (float)phase_ * M_PI/180;
    for ( int idx=0; idx<wvltsize_; idx++ )
    {
	const float realval = wvltzerophase.arr()[idx];
	const float imagval = -wvltincalcphase.arr()[idx].imag();
	valswithphase[idx] = realval*cos( angle ) - imagval*sin( angle );
    }

    return true;
}
