/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Apr 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: welltiegeocalculator.cc,v 1.38 2009-10-07 10:30:00 cvsbruno Exp $";


#include "welltiegeocalculator.h"

#include "arrayndutils.h"
#include "fft.h"
#include "fftfilter.h"
#include "hilberttransform.h"
#include "genericnumer.h"
#include "survinfo.h"
#include "wavelet.h"
#include "welllog.h"
#include "welllogset.h"
#include "welltrack.h"
#include "welltiedata.h"
#include "welltiesetup.h"
#include "welld2tmodel.h"

#include <complex>

namespace WellTie
{

GeoCalculator::GeoCalculator( const WellTie::DataHolder& dh )
		: params_(*dh.params())
		, setup_(dh.setup())	
		, wd_(*dh.wd())
		, denfactor_(0)	   
		, velfactor_(0)	   
{
    denfactor_ = dh.getUnits().denFactor();
    velfactor_ = dh.getUnits().velFactor();
}    

//each sample is converted to a time using the travel time log
//the correspondance between samples and depth values provides a first
//TWT approx.
Well::D2TModel* GeoCalculator::getModelFromVelLog( const char* vellog,
							  bool doclean )
{
    const Well::Log* log = wd_.logs().getLog( vellog );
    if ( !log ) return 0;
    TypeSet<float> vals, dpt, time;

    for ( int idx=0; idx<log->size(); idx++ )
    {
	vals += log->valArr()[idx];
	dpt  += log->dah( idx );
    }

    if ( doclean )
    {
	interpolateLogData( dpt, log->dahStep(true), true );
	interpolateLogData( vals, log->dahStep(true), false );
	removeSpikes( vals );
    }

    TWT2Vel( vals, dpt, time, false );
    mAllocVarLenArr( int, zidxs, vals.size() );
    for ( int idx=0; idx<time.size(); idx++ )
	zidxs[idx] += idx;
    sort_coupled( time.arr(), mVarLenArr(zidxs), vals.size() );

    Well::D2TModel* d2tnew = new Well::D2TModel;
    d2tnew->add ( wd_.track().dah(0) - wd_.track().value(0), 0 ); //set KB depth
    for ( int idx=1; idx<time.size(); idx++ )
	d2tnew->add( dpt[idx], time[idx] );

    return d2tnew;
}


//Small TWT/Interval Velocity converter
#define mFactor 10e6
void GeoCalculator::TWT2Vel( const TypeSet<float>& timevel,
			     const TypeSet<float>& dpt,	
			     TypeSet<float>& outp, bool t2vel  )
{
    outp += 0;
    if ( t2vel )
    {
	for ( int idx=1; idx<timevel.size(); idx++ )
	    outp +=  ( timevel[idx]-timevel[idx-1] )
		    /( (dpt[idx]-dpt[idx-1])/velfactor_*2 );
	outp[0] = outp[1];
    }
    else 
    {
	const bool issonic = setup_.issonic_;
	for ( int idx=1; idx<timevel.size(); idx++ )
	{
	    const float velval = issonic ? timevel[idx] : 1/timevel[idx];
	    outp += 2*( dpt[idx] - dpt[idx-1] )*velval/velfactor_;
	}
	for ( int idx=1;  idx<timevel.size(); idx++ )
	    outp[idx] += outp[idx-1];
    }
}


void GeoCalculator::stretch( WellTie::GeoCalculator::StretchData& sd ) const
{
    sd.stretchfac_ = (sd.pick2_-sd.start_)/(float)(sd.pick1_-sd.start_);
    sd.isstretch_ = true;
    stretch( sd, sd.stretchfac_ ); 

    sd.isstretch_ = false;
    sd.squeezefac_ = (sd.stop_-sd.pick2_ )/(float)(sd.stop_-sd.pick1_);
    stretch( sd, sd.squeezefac_ );
}


void GeoCalculator::stretch( const WellTie::GeoCalculator::StretchData& sd, 
			     float factor ) const
{
    int start = sd.isstretch_ ? sd.start_ : sd.pick2_; 
    int stop = sd.isstretch_ ? sd.pick2_ : sd.stop_; 
    const int datasz = sd.inp_->info().getSize(0);
    for ( int idx=start; idx<stop; idx++ )
    {
	float v = sd.isstretch_ ? sd.start_ : sd.stop_;
	const float curval = Interpolate::linearReg1D( v, (float)idx, factor );
	const int curidx = (int) curval;
	if ( curidx >= datasz-1 || curidx < 0 ) continue;
	const float newval = Interpolate::linearReg1D( sd.inp_->get(curidx),
						       sd.inp_->get(curidx+1),
						       curval-curidx);
	sd.outp_->setValue( idx , newval );
    }
}

//only for ascending arrays
const int GeoCalculator::getIdx( const Array1DImpl<float>& inp, 
					float pos ) const
{
    int idx = 0;
    while ( inp.get(idx)<pos )
    {
	if( idx == inp.info().getSize(0)-1 )
	    break;
	idx++;
    }
    return idx;
}


//Small Data Interpolator, specially designed for log (dah) data
//assumes no possible negative values in dens or vel log
//will replace them by interpolated values if so. 
void GeoCalculator::interpolateLogData( TypeSet<float>& data,
					       float dahstep, bool isdah )
{
    int startidx = getFirstDefIdx( data );
    int lastidx = getLastDefIdx( data );

    for ( int idx=startidx; idx<lastidx; idx++)
    {
	//no negative values in dens or vel log assumed
	if ( idx && !isdah && ( mIsUdf(data[idx]) || data[idx]<0 ) )
	    data[idx] = data[idx-1];
	if ( idx && isdah && (mIsUdf(data[idx]) || data[idx]<data[idx-1]
	     || data[idx] >= data[lastidx])  )
	    data[idx] = data[idx-1] + dahstep;
    }
    for ( int idx=0; idx<startidx; idx++ )
	data[idx] = data[startidx];
    for ( int idx=lastidx; idx<data.size(); idx++ )
	data[idx] = data[lastidx];
}


void GeoCalculator::removeSpikes( TypeSet<float>& logdata )
{
    const int winsize = 6;
    if ( logdata.size() < 2*winsize ) 
	return;
    float prevval = logdata[0];
    for ( int idx = winsize/2; idx<logdata.size()-winsize; idx = idx+winsize  ) 
    {
	float avg = 0;
	for ( int winidx = idx-winsize/2; winidx<idx+winsize/2; winidx++ )
	    avg += logdata[winidx]/winsize; 
	for ( int winidx = idx-winsize/2; winidx<idx+winsize/2; winidx++ )
	{
	    if ( logdata[winidx] > 5*avg )
		logdata[winidx] = prevval;
	    prevval = logdata[winidx];
	}
    }
}


int GeoCalculator::getFirstDefIdx( const TypeSet<float>& logdata )
{
    int idx = 0;
    while ( mIsUdf(logdata[idx]) )
	idx++;
    return idx;
}


int GeoCalculator::getLastDefIdx( const TypeSet<float>& logdata )
{
    int idx = logdata.size()-1;
    while ( mIsUdf( logdata[idx] ) )
	idx--;
    return idx;
}


bool GeoCalculator::isValidLogData( const TypeSet<float>& logdata )
{
    if ( !logdata.size() || getFirstDefIdx(logdata) >=logdata.size() )
	return false;
    return true;
}



//low pass filter, almost similar as this of the freqfilter attribute
//TODO put in algo
#define mDoTransform(tf,isstraight,inp,outp,sz) \
{   \
    tf.setInputInfo(Array1DInfoImpl(sz));\
    tf.setDir(isstraight);\
    tf.init();\
    tf.transform(*inp,*outp);\
}
void GeoCalculator::lowPassFilter( Array1DImpl<float>& vals, float cutf )
{
    FFTFilter filter;
    Array1DImpl<float> orgvals ( vals );
    ArrayNDWindow window( Array1DInfoImpl(100), false, "CosTaper", .05 );
    filter.setFreqBorderWindow( window.getValues(), 100 );
    const int filtersz = vals.info().getSize(0);
    float df = FFT::getDf( params_.dpms_.timeintvs_[0].step, filtersz );

    filter.FFTFreqFilter( df, cutf, true, orgvals, vals );
}


//resample data. If higher step, linear interpolation
//else nearest sample
void GeoCalculator::resampleData( const Array1DImpl<float>& invals,
				 Array1DImpl<float>& outvals, float step )
{
    int datasz = invals.info().getSize(0);
    if ( step >1  )
    {
	for ( int idx=0; idx<datasz-1; idx++ )
	{
	    float stepval = ( invals.get(idx+1) - invals.get(idx) ) / step;
	    for ( int stepidx=0; stepidx<step; stepidx++ )
		outvals.setValue( idx, invals.get(idx) + stepidx*stepval ); 
	}
	outvals.setValue( datasz-1, invals.get(datasz-1) );
    }
    else 
    {
	for ( int idx=0; idx<datasz*step; idx+=(int)(1/step) ) 
	    outvals.setValue( idx, invals.get(idx) );
    }
}


void GeoCalculator::computeAI( const Array1DImpl<float>& velvals,
			       const Array1DImpl<float>& denvals,
			       Array1DImpl<float>& aivals )
{
    const int datasz = velvals.info().getSize(0);
    const bool issonic = setup_.issonic_;
    float prevval = 0;
    for ( int idx=0; idx<datasz; idx++ )
    {
	float velval = issonic ? velvals.get(idx) : 1/velvals.get(idx);
	float denval = denvals.get( idx );
	float aival = denval/velval*mFactor*denfactor_*velfactor_;
	if ( mIsUdf(denval) || mIsUdf(velval) || mIsUdf(aival) )
	    aival = prevval;
	aivals.setValue( idx, aival );
	prevval = aival;
    }
}


//Compute reflectivity values at display sample step (Survey step)
void GeoCalculator::computeReflectivity(const Array1DImpl<float>& aivals,
				   Array1DImpl<float>& reflvals,int shiftstep )
{
    float prevval=0, ai1, ai2, rval = 0; 
    const int sz = reflvals.info().getSize(0);
    if ( sz<2 ) return;

    for ( int idx=0; idx<sz-1; idx++ )
    {
	ai2 = aivals.get( shiftstep*(idx+1));
	ai1 = aivals.get( shiftstep*(idx)  );	   

	if ( mIsZero( ai1+ai2, 1e-8 ) )
	    rval = prevval;
	else if ( !mIsUdf( ai1 ) || !mIsUdf( ai2 ) )
	    rval =  ( ai2 - ai1 ) / ( ai2 + ai1 );     
	
	reflvals.setValue( idx, rval ); 
	prevval = rval;
    }
    reflvals.setValue( 0, reflvals.get(1) ); 
    reflvals.setValue( sz-1, reflvals.get(sz-2) ); 
}


void GeoCalculator::convolveWavelet( const Array1DImpl<float>& wvltvals,
				     const Array1DImpl<float>& reflvals,
				     Array1DImpl<float>& synvals, int widx )
{
    int reflsz = reflvals.info().getSize(0);
    int wvltsz = wvltvals.info().getSize(0);
    float* outp = new float[reflsz];

    GenericConvolve( wvltsz, -widx, wvltvals.getData(),
		     reflsz, 0  	 , reflvals.getData(),
		     reflsz, 0  	 , outp );

    memcpy( synvals.getData(), outp, reflsz*sizeof(float));
    delete outp;
}


#define mNoise 0.05
void GeoCalculator::deconvolve( const Array1DImpl<float>& tinputvals,
				       const Array1DImpl<float>& tfiltervals,
				       Array1DImpl<float>& deconvals, 
				       int wvltsz )
{
    const int filtersz = tfiltervals.info().getSize(0);
    if ( !filtersz || filtersz<wvltsz ) return;

    ArrayNDWindow window( Array1DInfoImpl(filtersz), false, "CosTaper", 0.1 );
    Array1DImpl<float>* inputvals = new Array1DImpl<float>( filtersz );
    Array1DImpl<float>* filtervals = new Array1DImpl<float>( filtersz );

    memcpy(inputvals->getData(),tinputvals.getData(),filtersz*sizeof(float));
    memcpy(filtervals->getData(),tfiltervals.getData(),filtersz*sizeof(float));

    window.apply( inputvals );		removeBias( inputvals );
    window.apply( filtervals );		removeBias( filtervals );

    HilbertTransform hil;
    hil.setCalcRange(0, filtersz, 0);
    Array1DImpl<float_complex>* cinputvals = 
				new Array1DImpl<float_complex>( filtersz );
    mDoTransform( hil, true, inputvals, cinputvals, filtersz );
    delete inputvals;
    Array1DImpl<float_complex>* cfiltervals = 
				new Array1DImpl<float_complex>( filtersz );
    hil.setCalcRange(0, filtersz, 0);
    mDoTransform( hil, true, filtervals, cfiltervals, filtersz );
    delete filtervals;
   
    FFT fft(false);
    Array1DImpl<float_complex>* cfreqinputvals = 
				new Array1DImpl<float_complex>( filtersz );
    mDoTransform( fft, true, cinputvals, cfreqinputvals, filtersz );
    delete cinputvals;
    Array1DImpl<float_complex>* cfreqfiltervals = 
				new Array1DImpl<float_complex>( filtersz );
    mDoTransform( fft, true, cfiltervals, cfreqfiltervals, filtersz );

    Spectrogram spec;
    Array1DImpl<float_complex>* cspecfiltervals = 
				new Array1DImpl<float_complex>( filtersz );
    mDoTransform( spec, true, cfiltervals, cspecfiltervals, filtersz );
    delete cfiltervals;

    float_complex wholespec = 0;
    float_complex noise = mNoise/filtersz;
    for ( int idx=0; idx<filtersz; idx++ )
	wholespec += cspecfiltervals->get( idx );  
    float_complex cnoiseshift = noise*wholespec;
   
    Array1DImpl<float_complex>* cdeconvvals = 
				new  Array1DImpl<float_complex>( filtersz ); 
    for ( int idx=0; idx<filtersz; idx++ )
    {
	float_complex inputval = cfreqinputvals->get(idx);
	float_complex filterval = cfreqfiltervals->get(idx);

	double rfilterval = filterval.real();
	double ifilterval = filterval.imag();
	float_complex conjfilterval = float_complex( rfilterval ,-ifilterval ); 

	float_complex num = inputval * conjfilterval;
	float_complex denom = filterval * conjfilterval + cnoiseshift;
	float_complex res = num / denom;

	cdeconvvals->setValue( idx, res );
    }
    delete cfreqinputvals; delete  cfreqfiltervals;

    float avg = 0;
    for ( int idx=0; idx<filtersz; idx++ )
	avg += abs( cdeconvvals->get( idx ) )/filtersz;
    for ( int idx=0; idx<filtersz; idx++ )
    {
	if ( abs( cdeconvvals->get( idx ) ) < avg/4 )
	    cdeconvvals->set( idx, 0 );
    }

    Array1DImpl<float_complex>* ctimedeconvvals = 
				new Array1DImpl<float_complex>( filtersz );
    mDoTransform( fft, false, cdeconvvals, ctimedeconvvals, filtersz );
    delete cdeconvvals;

    int mid = (int)(filtersz)/2;
    for ( int idx=0; idx<=mid; idx++ )
	deconvals.set( idx, ctimedeconvvals->get( mid-idx ).real() );
    for ( int idx=mid+1; idx<filtersz; idx++ )
	deconvals.set( idx, ctimedeconvvals->get( filtersz-idx+mid ).real() );
    delete ctimedeconvvals;
}


void GeoCalculator::crosscorr( const Array1DImpl<float>& seisvals, 
				     const Array1DImpl<float>& synthvals,
       				     Array1DImpl<float>& outpvals	)
{
    const int datasz = seisvals.info().getSize(0);
    float* outp = new float[datasz];
    genericCrossCorrelation( datasz, 0, seisvals,
			     datasz, 0, synthvals,
			     datasz, -datasz/2, outp);
    memcpy( outpvals.getData(), outp, datasz*sizeof(float));
    delete outp;
}

}; //namespace WellTie
